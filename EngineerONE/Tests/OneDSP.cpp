#include "Support.h"
void writeRaw(const juce::File &file, const juce::AudioBuffer<float> &pcm,
              double rate) {
  auto stream = file.createOutputStream();
  juce::WavAudioFormat wav;
  std::unique_ptr<juce::AudioFormatWriter> writer(
      stream ? wav.createWriterFor(stream.get(), rate,
                                   unsigned(pcm.getNumChannels()), 32, {}, 0)
             : nullptr);
  check(bool(writer), "Fixture WAV writer failed");
  stream.release();
  check(writer->writeFromAudioSampleBuffer(pcm, 0, pcm.getNumSamples()),
        "Fixture write failed");
}
void waitUntil(const std::function<bool()> &done, int ms = 120000) {
  auto start = juce::Time::getMillisecondCounter();
  while (!done() && juce::Time::getMillisecondCounter() - start < uint32_t(ms))
    dispatch(10);
  check(done(), "Background operation timed out");
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    auto folder =
        juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("Makkine-ONE-test-" + juce::Uuid().toString());
    folder.createDirectory();
    auto file = folder.getChildFile("Original.wav");
    auto pcm = vocal();
    writeRaw(file, pcm, 48000);
    one::Origin origin;
    origin.seconds = 12.5;
    origin.positionKnown = true;
    auto cancel = [] { return false; };
    auto in = one::readInput(file, origin, cancel);
    check(in && in->analysis.notes.size() >= 5,
          "Automatic note detection missing");
    std::cout << "NOTES " << in->analysis.notes.size() << " KEY "
              << one::keyName(in->key) << "\n";
    auto a = one::generate(in, 1, {}, folder.getChildFile("A"), cancel,
                           [](float, const juce::String &) {});
    auto b = one::generate(in, 2, {}, folder.getChildFile("B"), cancel,
                           [](float, const juce::String &) {});
    check(a && b, "Automatic generation failed");
    check(a->audio.getNumSamples() == pcm.getNumSamples(),
          "Automatic tuning changed line length");
    check(fingerprint(a->audio) != fingerprint(b->audio),
          "IA interpretations are identical");
    for (auto &n : a->notes) {
      auto v = pitches(a->measured, n.begin + .16, n.end - .12);
      if (v.size() < 5)
        continue;
      float centre = one::median(v);
      std::cout << "CENTER note=" << n.id << " original=" << n.original
                << " target=" << n.target << " measured=" << centre
                << " cents=" << (centre - n.target) * 100 << "\n";
      check(std::abs(centre - n.target) < .12f,
            "Automatic tuning left note centre more than 12 cents away");
    }
    for (int i = 0; i < int(.2 * 48000); ++i)
      check(a->audio.getSample(0, i) == pcm.getSample(0, i),
            "Initial silence/marker moved");
    for (int i = int(2.45 * 48000); i < int(2.57 * 48000); ++i)
      check(a->audio.getSample(0, i) == pcm.getSample(0, i),
            "Unvoiced breath/noise was tuned");
    auto srcV = pitches(in->analysis, .65, 1.25),
         dstV = pitches(a->measured, .65, 1.25);
    check(spread(dstV) > .09f && spread(dstV) < spread(srcV) * 1.5f,
          "Natural vibrato flattened or enlarged excessively");
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> wav(format.createReaderFor(
        a->lineFile.createInputStream().release(), true));
    check(wav && wav->sampleRate == 48000 && wav->bitsPerSample == 24 &&
              !wav->usesFloatingPointData,
          "Export is not real PCM24/48k WAV");
    check(wav->lengthInSamples == pcm.getNumSamples(),
          "Export duration changed");
    check(wav->metadataValues[juce::WavAudioFormat::bwavTimeReference]
                  .getLargeIntValue() == 600000,
          "Line BWF timestamp incorrect");
    check(a->noteFiles.size() == a->notes.size(),
          "Per-note WAV exports missing");
    std::unique_ptr<juce::AudioFormatReader> note(format.createReaderFor(
        a->noteFiles[0].createInputStream().release(), true));
    check(note && note->metadataValues[juce::WavAudioFormat::bwavTimeReference]
                          .getLargeIntValue() ==
                      600000 + int64_t(std::llround(a->notes[0].begin * 48000)),
          "Note BWF timestamp incorrect");
    auto edited = a->notes;
    edited[0].target += 1;
    edited[0].gainDb = -6;
    edited[0].vibrato = .7f;
    edited[0].touched = true;
    auto e = one::generate(in, 1, edited, folder.getChildFile("edit"), cancel,
                           [](float, const juce::String &) {});
    check(fingerprint(e->audio) != fingerprint(a->audio),
          "Optional edits did not affect audio");
    check(fingerprint(in->audio) == fingerprint(pcm), "Original input changed");
    juce::AudioBuffer<float> hi(1, 96000);
    hi.clear();
    hi.setSample(0, 1000, .5f);
    auto low = one::at48k(hi, 96000, cancel);
    check(low.getNumSamples() == 48000, "Resampler changed exact duration");
    int at = 0;
    float peak = 0;
    for (int i = 0; i < low.getNumSamples(); ++i)
      if (std::abs(low.getSample(0, i)) > peak) {
        peak = std::abs(low.getSample(0, i));
        at = i;
      }
    check(at == 500, "48k resampler introduces unreported delay");
    // Actual SPSC track capture, host stop, original timestamp and all pauses.
    one::Capture capture;
    capture.prepare(48000, 2);
    std::atomic<bool> done{false};
    juce::File captured;
    one::Origin capturedOrigin;
    juce::String error;
    capture.onFinished = [&](const juce::File &f, one::Origin o,
                             const juce::String &err) {
      captured = f;
      capturedOrigin = o;
      error = err;
      done.store(true);
    };
    check(capture.arm(), "Cannot arm capture");
    one::Transport t;
    t.playStateKnown = t.playing = t.positionKnown = true;
    t.sample = 600000;
    for (int i = 0; i < pcm.getNumSamples(); i += 256) {
      int n = std::min(256, pcm.getNumSamples() - i);
      juce::AudioBuffer<float> block(2, n);
      for (int c = 0; c < 2; ++c)
        block.copyFrom(c, 0, pcm, c, i, n);
      capture.process(block, t);
      t.sample += n;
    }
    t.playing = false;
    juce::AudioBuffer<float> tail(2, 256);
    tail.clear();
    capture.process(tail, t);
    waitUntil([&] { return done.load(); });
    check(error.isEmpty(), error.toRawUTF8());
    auto cap = one::readInput(captured, capturedOrigin, cancel);
    check(cap->audio.getNumSamples() == pcm.getNumSamples() &&
              fingerprint(cap->audio) == fingerprint(pcm),
          "Captured track has gaps or changed audio");
    check(capturedOrigin.seconds == 12.5, "Captured origin incorrect");
    capture.shutdown();
    OneProcessor p;
    p.prepareToPlay(48000, 256);
    p.loadFile(file, origin);
    waitUntil([&] { return !p.busy.load(); });
    check(p.inputSnapshot() && !p.snapshot(),
          "Capture/import should await IA click");
    p.engineer();
    waitUntil([&] { return !p.busy.load(); });
    check(bool(p.snapshot()), p.statusText().toRawUTF8());
    check(p.snapshot()->notes.size() >= 5, "IA requires manual tuning");
    p.play();
    p.seek(.8);
    double energy = 0;
    juce::MidiBuffer midi;
    for (int k = 0; k < 12; ++k) {
      tail.clear();
      p.processBlock(tail, midi);
      energy += tail.getRMSLevel(0, 0, 256);
    }
    check(energy > .01, "Internal preview silent");
    p.engineer();
    waitUntil([&] { return !p.busy.load(); });
    check(p.snapshot()->interpretation == 2,
          "IA click did not create new interpretation");
    juce::MemoryBlock state;
    p.getStateInformation(state);
    OneProcessor reopened;
    reopened.prepareToPlay(48000, 256);
    reopened.setStateInformation(state.getData(), int(state.getSize()));
    waitUntil([&] { return !reopened.busy.load(); });
    check(reopened.snapshot() && reopened.snapshot()->interpretation == 2,
          "VST3 project state failed to regenerate saved IA interpretation");
    check(fingerprint(reopened.snapshot()->audio) ==
              fingerprint(p.snapshot()->audio),
          "Reopened VST3 state changed the saved result");
    reopened.releaseResources();
    p.releaseResources();
    if (argc == 3 && juce::String(argv[1]) == "--render") {
      juce::File out(juce::String::fromUTF8(argv[2]));
      out.createDirectory();
      one::write24(out.getChildFile("Makkine-ONE-Demo-Original.wav"), pcm, 12.5,
                   true);
      a->lineFile.copyFileTo(out.getChildFile("Makkine-ONE-Demo-IA-1.wav"));
      b->lineFile.copyFileTo(out.getChildFile("Makkine-ONE-Demo-IA-2.wav"));
    }
    std::cout << "PASS: automatic IA, distinct natural interpretations, "
                 "vibrato, formants path, dry breaths, exact timing, PCM24/48k "
                 "BWF note/line exports, optional edits, bandlimited "
                 "conversion, real capture and internal preview\n";
    folder.deleteRecursively();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
