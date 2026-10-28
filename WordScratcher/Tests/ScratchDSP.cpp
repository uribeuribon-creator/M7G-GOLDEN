#include "TestSupport.h"
#include <map>
#include <set>
void memoryAndMotion() {
  for (float f : {0.f, .13f, .5f, .87f, 1.f}) {
    auto poly = [](float x) { return .001f * x * x * x - .004f * x + .01f; };
    float expected = poly(2 + f),
          observed = scratch::lagrange4(poly(1), poly(2), poly(3), poly(4), f);
    requireScratch(std::abs(observed - expected) < 1e-6f,
                   "Four-point Lagrange does not reproduce a cubic");
  }
  scratch::AudioRing ring;
  ring.prepare(1000);
  for (int i = 0; i < 6200; ++i)
    ring.push(float(i), float(-i));
  requireScratch(ring.capacity() == 5000 && ring.read(6199, 0) == 6199 &&
                     ring.read(1200, 1) == -1200 && ring.read(0, 0) == 0,
                 "Five-second capture ring wrap failed");
  auto p = scratch::defaults();
  p[scratch::AutoScratch] = 1;
  p[scratch::Intelligent] = 0;
  auto r = renderScratch(p);
  std::cout << "PHYSICS positive=" << r.positive << " reverse=" << r.negative
            << " audible=" << r.effectEnergy << " underflow=" << r.underflows
            << "\n";
  requireScratch(r.positive > 100 && r.negative > 100 && r.effectEnergy > 1,
                 "Platter failed to move/read actual audio both directions");
  std::cout << "PASS: exact Lagrange interpolation, five-second ring and "
               "forward/reverse physical audio\n";
}
void digitalPitch() {
  constexpr double sr = 48000, f = 196;
  auto ring = std::make_unique<scratch::AudioRing>();
  ring->prepare(sr);
  for (int i = 0; i < 144000; ++i) {
    float x = .18f * std::sin(float(6.28318530718 * f * i / sr));
    ring->push(x, x);
  }
  auto word = std::make_unique<scratch::WordSnapshot>();
  word->prepare(sr);
  requireScratch(word->capture(*ring, 0, 143999),
                 "Pitch fixture capture failed");
  for (double speed : {.5, 2., -.5, -2.}) {
    auto digital = std::make_unique<scratch::DigitalStretch>();
    digital->prepare(sr);
    std::vector<float> vinyl, held;
    double position = 71000;
    for (int k = 0; k < 900; ++k) {
      std::array<std::array<float, 128>, 2> out{};
      digital->block(word.get(), position, speed * 128, true, out);
      for (int i = 0; i < 128; ++i) {
        vinyl.push_back(
            word->read(position + speed * i, 0, float(std::abs(speed)), true));
        held.push_back(out[0][size_t(i)]);
      }
      position += speed * 128;
      position = std::fmod(position, word->size());
      if (position < 0)
        position += word->size();
    }
    double vinylHz = scratchPitch(vinyl, f * std::abs(speed)),
           digitalHz = scratchPitch(held, f);
    double vinylCents = 1200 * std::log2(vinylHz / (f * std::abs(speed))),
           digitalCents = 1200 * std::log2(digitalHz / f);
    std::cout << "PITCH speed=" << speed << " vinyl=" << vinylHz
              << " digital=" << digitalHz << " digital_cents=" << digitalCents
              << " underflow=" << digital->underflows
              << " queue=" << digital->queued() << "\n";
    requireScratch(
        std::abs(vinylCents) < 5 && std::abs(digitalCents) < 14 &&
            digital->underflows == 0 && digital->overflows == 0,
        "VINYL speed pitch / DIGITAL Rubber Band pitch retention failed");
  }
  std::cout << "PASS: real forward/reverse speed-following VINYL and Rubber "
               "Band pitch-preserving DIGITAL\n";
}
void incomingWords() {
  for (bool digital : {false, true}) {
    auto e = std::make_unique<scratch::VinylEngine>();
    e->prepare(48000);
    auto p = scratch::defaults();
    p[scratch::VinylMode] = digital ? 0.f : 1.f;
    p[scratch::VinylNoise] = p[scratch::Crackle] = 0;
    p[scratch::Intelligent] = 0;
    e->controls(p);
    int first = -1, second = -1, beforeSecond = 0;
    double withinWordDifference = 0;
    for (int i = 0; i < 110000; ++i) {
      double t = i / 48000.;
      if (i == 80000)
        beforeSecond = e->serial;
      float x = scratchVoice(t);
      auto y = e->process(x, x);
      if (first < 0 && e->serial > 0)
        first = i;
      if (i >= 81600 && second < 0 && e->serial > beforeSecond) {
        second = i;
        requireScratch(e->wordSize() < 6000,
                       "New onset recycled the previous complete word");
      }
      if (t > .32 && t < .65) {
        float dry = scratchVoice((i - scratch::VinylEngine::latencyFor(48000)) /
                                 48000.);
        withinWordDifference += std::pow(y[0] - dry, 2);
      }
    }
    std::cout << "ONSET mode=" << (digital ? "DIGITAL" : "VINYL")
              << " first_ms=" << first / 48. << " second_ms=" << second / 48.
              << " within_word_difference=" << withinWordDifference << "\n";
    requireScratch(first >= 0 && first < 12000 && second >= 81600 &&
                       second < 88000 && withinWordDifference > .05 &&
                       e->digital.underflows == 0,
                   "Fresh-word onset did not start a real audible scratch");
  }
  std::cout
      << "PASS: both modes scratch each incoming new word before its end\n";
}
void presets() {
  scratch::PresetManager manager;
  requireScratch(manager.presets.size() == 300, "Factory bank is not 300");
  std::set<juce::String> names;
  std::map<uint64_t, int> fingerprints;
  int quiet = 0;
  for (int i = 0; i < 300; ++i) {
    auto &p = manager.presets[size_t(i)];
    names.insert(p.name);
    auto r = renderScratch(p.values);
    if (r.effectEnergy < .02 || r.difference < 1) {
      std::cerr << "WEAK PRESET " << i << " " << p.name
                << " energy=" << r.effectEnergy
                << " difference=" << r.difference << "\n";
      ++quiet;
    }
    if (fingerprints.count(r.hash))
      std::cerr << "DUPLICATE AUDIO " << fingerprints[r.hash] << " and " << i
                << " " << p.name << "\n";
    fingerprints[r.hash] = i;
    requireScratch(r.captures >= 2, "Automatic word capture missed phrases");
    requireScratch(r.underflows == 0,
                   "Digital pipeline underflow in factory gesture");
  }
  std::cout << "FACTORY names=" << names.size()
            << " different_audio=" << fingerprints.size() << " weak=" << quiet
            << "\n";
  requireScratch(names.size() == 300 && fingerprints.size() == 300 &&
                     quiet == 0,
                 "Some factory effects duplicate or are not audibly effective");
  for (int id : {0, 15, 31, 65, 93, 125, 158, 195, 213, 243, 273}) {
    auto v = manager.presets[size_t(id)].values;
    v[scratch::VinylNoise] = v[scratch::Crackle] = 0;
    auto clean = renderScratch(v);
    requireScratch(clean.effectEnergy > .02 && clean.difference > 1,
                   "A gesture is only noise, not actual word processing");
  }
  std::cout << "PASS: all 300 exact factory names produce distinct finite "
               "substantial audio effects\n";
}
void faderAndLearning() {
  auto e = std::make_unique<scratch::VinylEngine>();
  e->prepare(48000);
  auto p = scratch::defaults();
  p[scratch::AutoScratch] = 0;
  p[scratch::Intelligent] = 0;
  p[scratch::AutoFader] = 0;
  e->controls(p);
  for (int i = 0; i < 48000; ++i) {
    float x = .15f * std::sin(float(6.28318530718 * 196 * i / 48000));
    e->process(x, x);
  }
  e->requestCapture();
  e->process(0, 0);
  requireScratch(e->haveWord, "Manual capture failed");
  for (int shape = 0; shape < 3; ++shape) {
    p[scratch::CrossfaderCurve] = float(shape);
    p[scratch::Crossfader] = shape == 2 ? 1.f : 0.f;
    p[scratch::Touch] = 1;
    p[scratch::EchoMix] = 100;
    p[scratch::EchoFeedback] = .75f;
    double muted = 0;
    for (int i = 0; i < 24000; ++i) {
      p[scratch::Position] = ((i / 2400) % 2) ? .85f : .15f;
      e->controls(p);
      auto out = e->process(0, 0);
      if (i > 12000)
        muted += out[0] * out[0];
    }
    requireScratch(muted < 1e-9,
                   "Closed crossfader did not mute scratch and echo");
  }
  p[scratch::CrossfaderCurve] = 0;
  p[scratch::Crossfader] = 1;
  p[scratch::Cut] = 0;
  double opened = 0;
  for (int i = 0; i < 36000; ++i) {
    p[scratch::Position] = ((i / 2400) % 2) ? .85f : .15f;
    e->controls(p);
    auto out = e->process(0, 0);
    if (i > 12000)
      opened += out[0] * out[0];
  }
  requireScratch(opened > .1,
                 "Open crossfader does not pass actual captured audio");
  auto learned = std::make_unique<scratch::VinylEngine>();
  learned->prepare(48000);
  learned->controls(scratch::defaults());
  learned->requestLearn();
  for (int i = 0; i < 49000; ++i) {
    float n = .012f * std::sin(float(i * 1.173));
    learned->process(n, n);
  }
  requireScratch(learned->learnReady && learned->learnedFloor > -50 &&
                     learned->learnedFloor < -30 && !learned->haveWord,
                 "Noise learn/capture rejection failed");
  auto shortWord = std::make_unique<scratch::VinylEngine>();
  shortWord->prepare(48000);
  auto shortParams = scratch::defaults();
  shortParams[scratch::VinylMode] = 0;
  shortParams[scratch::VinylNoise] = shortParams[scratch::Crackle] = 0;
  shortWord->controls(shortParams);
  double shortEnergy = 0;
  for (int i = 0; i < 72000; ++i) {
    float x = i >= 4800 && i < 8640
                  ? .16f * std::sin(float(6.28318530718 * 196 * i / 48000))
                  : 0;
    auto y = shortWord->process(x, x);
    if (i > 20000 && i < 48000)
      shortEnergy += y[0] * y[0];
  }
  std::cout << "SHORT DIGITAL word_energy=" << shortEnergy << "\n";
  requireScratch(shortWord->serial > 0 && shortEnergy > .05,
                 "Intelligent short-word DIGITAL gesture is ineffective");
  std::cout << "PASS: cut/smooth/hamster fader mute/open, echo mute and "
               "learned room-noise rejection\n";
}
void stateAndSafety() {
  auto p = std::make_unique<ScratchProcessor>();
  requireScratch(p->authorized.load(), "Shared M7G licence missing");
  p->setCurrentProgram(277);
  p->setValue("mix", 78);
  p->copyCompare();
  p->selectCompare(1);
  p->setValue("mix", 21);
  p->selectCompare(0);
  requireScratch(std::abs(p->value("mix") - 78) < .1f, "A/B recall failed");
  p->setValue("cue5", .73f);
  juce::MemoryBlock blob;
  p->getStateInformation(blob);
  auto copy = std::make_unique<ScratchProcessor>();
  copy->setStateInformation(blob.getData(), int(blob.getSize()));
  requireScratch(copy->getCurrentProgram() == 277 &&
                     std::abs(copy->value("cue5") - .73f) < .001f &&
                     std::abs(copy->value("mix") - 78) < .1f,
                 "XML program/cue recall failed");
  bool fav = p->isFavourite(277);
  p->toggleFavourite(277);
  auto remembered = std::make_unique<ScratchProcessor>();
  requireScratch(remembered->isFavourite(277) != fav,
                 "Favourites do not persist");
  p->toggleFavourite(277);
  for (double sr : {44100., 48000., 96000.})
    for (bool bypass : {false, true}) {
      auto proc = std::make_unique<ScratchProcessor>();
      proc->setValue(bypass ? "bypass" : "mix", bypass ? 1.f : 0.f);
      proc->setRateAndBufferSizeDetails(sr, 127);
      proc->prepareToPlay(sr, 127);
      requireScratch(proc->getLatencySamples() ==
                         scratch::VinylEngine::latencyFor(sr),
                     "Declared host latency wrong");
      double error = 0;
      juce::MidiBuffer m;
      for (int s = 0; s < scratch::VinylEngine::latencyFor(sr) + 600;
           s += 127) {
        juce::AudioBuffer<float> b(2, 127);
        b.clear();
        if (!s) {
          b.setSample(0, 0, .3f);
          b.setSample(1, 0, .3f);
        }
        proc->processBlock(b, m);
        for (int i = 0; i < 127; ++i)
          error += std::abs(
              b.getSample(0, i) -
              (s + i == scratch::VinylEngine::latencyFor(sr) ? .3f : 0));
      }
      requireScratch(error < 1e-6,
                     "Mix zero/bypass changes exact aligned dry audio");
      juce::AudioBuffer<float> zero(2, 0);
      proc->processBlock(zero, m);
      juce::AudioBuffer<float> large(2, 65536);
      large.clear();
      proc->processBlock(large, m);
      for (int i = 0; i < 65536; ++i)
        requireScratch(std::isfinite(large.getSample(0, i)),
                       "Oversized callback corrupted audio");
    }
  auto proc = std::make_unique<ScratchProcessor>();
  proc->setRateAndBufferSizeDetails(48000, 64);
  proc->prepareToPlay(48000, 64);
  ScratchPlayHead head;
  proc->setPlayHead(&head);
  juce::MidiBuffer midi;
  for (int k = 0; k < 130; ++k) {
    juce::AudioBuffer<float> b(2, 64);
    b.clear();
    if (k == 0)
      b.setSample(0, 0, .4f);
    head.sample = k * 64;
    proc->processBlock(b, midi);
  }
  head.sample = 1000000;
  juce::AudioBuffer<float> b(2, 64);
  b.clear();
  proc->processBlock(b, midi);
  requireScratch(b.getMagnitude(0, 64) < 1e-7, "Seek leaked obsolete audio");
  proc->setPlayHead(nullptr);
  std::cout << "PASS: global licence, session/cues/A-B/favourites, exact dry "
               "latency at three rates, seek and zero/oversized callbacks\n";
}
void renderDemo(const juce::File &dir) {
  dir.createDirectory();
  auto file = dir.getChildFile("Word-Scratcher-300-Demo.wav");
  file.deleteFile();
  auto stream = file.createOutputStream();
  juce::WavAudioFormat format;
  std::unique_ptr<juce::AudioFormatWriter> w(
      format.createWriterFor(stream.release(), 48000, 2, 16, {}, 0));
  requireScratch(bool(w), "Demo writer unavailable");
  scratch::PresetManager manager;
  juce::String manifest =
      "Synthetic vocal signal, not a real singer. Each 3.8s segment includes "
      "source phrase plus actual automatic scratch.\n";
  double t = 0;
  for (int id : {0, 1, 15, 31, 35, 55, 65, 93, 125, 158, 195, 213, 243, 273}) {
    auto r = renderScratch(manager.presets[size_t(id)].values);
    juce::AudioBuffer<float> b(2, int(r.audio.size()));
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < b.getNumSamples(); ++i)
        b.setSample(c, i, r.audio[size_t(i)] * .8f);
    requireScratch(w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples()),
                   "Demo write failed");
    manifest +=
        juce::String(t, 2) + "s | " + manager.presets[size_t(id)].name + "\n";
    t += 3.8;
  }
  w.reset();
  dir.getChildFile("Word-Scratcher-300-Demo.txt").replaceWithText(manifest);
  std::cout << "PASS: 14-preset actual scratch synthetic listening reel\n";
}
void renderSpeech(const juce::File &dir) {
  dir.createDirectory();
  juce::WavAudioFormat format;
  std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(
      new juce::FileInputStream(juce::File(SCRATCH_SPEECH_FIXTURE)), true));
  requireScratch(bool(reader), "Synthetic-word fixture unavailable");
  juce::AudioBuffer<float> source(int(reader->numChannels),
                                  int(reader->lengthInSamples));
  reader->read(&source, 0, source.getNumSamples(), 0, true, true);
  auto file = dir.getChildFile("Word-Scratcher-300-WordDemo.wav");
  file.deleteFile();
  auto stream = file.createOutputStream();
  std::unique_ptr<juce::AudioFormatWriter> writer(
      format.createWriterFor(stream.release(), 48000, 2, 16, {}, 0));
  requireScratch(bool(writer), "Word-demo writer failed");
  scratch::PresetManager manager;
  juce::String manifest =
      "Synthetic speech generated by eSpeak NG, not a human recording. "
      "Original phrase: Makkine. La palabra manda. Each segment is source plus "
      "actual automatic scratch.\n";
  double cursor = 0;
  for (int preset : {0, 15, 31, 65, 94, 125, 195, 273}) {
    auto e = std::make_unique<scratch::VinylEngine>();
    e->prepare(48000);
    e->controls(manager.presets[size_t(preset)].values);
    juce::AudioBuffer<float> audio(2, 220800);
    for (int i = 0; i < audio.getNumSamples(); ++i) {
      double at = (i / 48000. - .12) * reader->sampleRate;
      float x = 0;
      if (at >= 0 && at < source.getNumSamples() - 1) {
        int a = int(at), b = a + 1;
        float f = float(at - a);
        x = source.getSample(0, a) * (1 - f) + source.getSample(0, b) * f;
      }
      auto y = e->process(x, x);
      for (int c = 0; c < 2; ++c)
        audio.setSample(c, i, y[size_t(c)] * .8f);
    }
    requireScratch(e->serial > 0, "Synthetic words did not trigger capture");
    requireScratch(
        writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()),
        "Word demo write failed");
    manifest += juce::String(cursor, 2) + "s | " +
                manager.presets[size_t(preset)].name + "\n";
    cursor += 4.6;
  }
  writer.reset();
  dir.getChildFile("Word-Scratcher-300-WordDemo.txt").replaceWithText(manifest);
  juce::File(SCRATCH_SPEECH_FIXTURE)
      .copyFileTo(dir.getChildFile("Word-Scratcher-300-OriginalPhrase.wav"));
  std::cout << "PASS: intelligible synthetic-word source processed through "
               "eight actual scratch presets\n";
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    if (argc == 3 && juce::String(argv[1]) == "--render") {
      renderDemo(juce::File(juce::String::fromUTF8(argv[2])));
      return 0;
    }
    if (argc == 3 && juce::String(argv[1]) == "--speech") {
      renderSpeech(juce::File(juce::String::fromUTF8(argv[2])));
      return 0;
    }
    memoryAndMotion();
    digitalPitch();
    incomingWords();
    faderAndLearning();
    stateAndSafety();
    if (argc != 2 || juce::String(argv[1]) != "--core")
      presets();
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
