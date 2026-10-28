#include "Support.h"
void threshold() {
  engineer::Analysis a;
  a.hopSeconds = .005;
  a.phrases = {{0, 48000}};
  a.notes = {{0, 200, 60.07f, false}};
  for (int i = 0; i < 200; ++i) {
    engineer::Frame f;
    f.time = i * .005;
    f.note = 60.07f;
    f.voiced = true;
    f.region = f.phrase = 0;
    a.frames.push_back(f);
  }
  auto natural = engineer::makePlan(a, 1, {});
  check(*std::max_element(natural.wet.begin(), natural.wet.end()) == 0,
        "Natural Drift modified a note inside 8 cents");
  for (auto &f : a.frames)
    f.note = 60.12f;
  auto fix = engineer::makePlan(a, 1, {});
  check(fix.delta[100] < -.05f && fix.wet[100] > .99f,
        "Natural Drift failed to correct outside 8 cents");
  std::cout << "PASS: strict 8-cent Natural Drift decision\n";
}
void rateAndSilence() {
  for (double sr : {44100., 96000.}) {
    auto b = vocal(sr, 1);
    engineer::Analyzer analyzer;
    auto a = analyzer.run(b, sr);
    auto plan = engineer::makePlan(a, 4, {});
    auto y = engineer::render(b, sr, a, plan);
    check(y.getNumSamples() == b.getNumSamples(),
          "Length changed at another sample rate");
    for (int i = 0; i < int(sr * .2); ++i)
      check(y.getSample(0, i) == b.getSample(0, i),
            "Initial silence/marker moved");
    std::cout << "RATE " << sr << " aligned_samples=" << y.getNumSamples()
              << "\n";
  }
  juce::AudioBuffer<float> silence(1, 48000);
  silence.clear();
  engineer::Analyzer analyzer;
  auto a = analyzer.run(silence, 48000);
  check(a.notes.empty() && a.phrases.empty(), "Silence classified as voice");
  auto p = engineer::makePlan(a, 0, {});
  auto y = engineer::render(silence, 48000, a, p);
  check(y.getMagnitude(0, y.getNumSamples()) == 0,
        "Silence created fake audio");
  bool cancel = true;
  check(analyzer.run(silence, 48000, [&] { return cancel; }).frames.empty(),
        "Analysis cancellation ignored");
}
std::shared_ptr<engineer::Project> projectTest(const juce::File &folder) {
  auto source = vocal();
  auto input = folder.getChildFile("Original-synthetic-vocal.wav");
  juce::StringPairArray meta;
  meta.set(juce::WavAudioFormat::bwavTimeReference, "221234");
  engineer::writeWav(input, source, 48000, meta);
  auto p = engineer::buildProject(
      input, {}, folder, [] { return false; },
      [](float, const juce::String &) {});
  check(p && p->original.notes.size() >= 5 && p->original.phrases.size() == 3,
        "Pitch/phrase detection failed");
  std::set<uint64_t> unique;
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  for (int v = 0; v < 5; ++v) {
    auto &y = p->versions[size_t(v)];
    check(y.getNumSamples() == source.getNumSamples() &&
              y.getNumChannels() == 2,
          "Export shape changed");
    for (int i = 0; i < source.getNumSamples(); ++i)
      for (int c = 0; c < 2; ++c) {
        check(std::isfinite(y.getSample(c, i)), "Nonfinite render");
        if (i < 9600 || i > 292800)
          check(y.getSample(c, i) == source.getSample(c, i),
                "Silence/impulse not preserved exactly");
      }
    unique.insert(fingerprint(y));
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(p->files[size_t(v)]));
    check(reader && reader->lengthInSamples == source.getNumSamples() &&
              reader->sampleRate == 48000 && reader->numChannels == 2 &&
              reader->usesFloatingPointData,
          "WAV not aligned float32");
    check(reader->metadataValues[juce::WavAudioFormat::bwavTimeReference] ==
              "221234",
          "BWF origin lost");
    std::cout << engineer::names[v] << " frames=" << y.getNumSamples()
              << " median_note="
              << engineer::median(pitches(p->measured[size_t(v)], .7, 1.2))
              << " hash=" << fingerprint(y) << "\n";
  }
  check(unique.size() == 5, "Versions are identical DSP copies");
  std::cout << "CENTERED verified_refinement=" << p->centredRefined << "\n";
  auto before = pitches(p->original, 5.05, 5.85),
       centred = pitches(p->measured[0], 5.05, 5.85),
       kept = pitches(p->measured[3], 5.05, 5.85);
  float v1 = engineer::median(centred), v4 = engineer::median(kept);
  std::cout << "PITCH V1_error_cents=" << (v1 - 57) * 100
            << " V4_centre_cents=" << (v4 - 57) * 100
            << " vibrato_before=" << spread(before) * 100
            << " after_V1=" << spread(centred) * 100
            << " after_V4=" << spread(kept) * 100 << "\n";
  check(centred.size() > 50 && std::abs(v1 - 57) < .055f &&
            spread(centred) < spread(before) * .40f,
        "Centered does not measurably centre pitch");
  check(kept.size() > 50 && std::abs(v4 - 57) < .10f &&
            spread(kept) > spread(before) * .60f &&
            spread(kept) < spread(before) * 1.40f,
        "Vibrato Keeper lost vibrato or failed to tune its centre");
  auto phase = [](const engineer::Analysis &data) {
    auto values = pitches(data, 5.05, 5.85);
    float mean = std::accumulate(values.begin(), values.end(), 0.f) /
                 std::max(1, int(values.size()));
    double cosine = 0, sine = 0;
    for (const auto &f : data.frames)
      if (f.voiced && f.time > 5.05 && f.time < 5.85) {
        double angle = 2 * juce::MathConstants<double>::pi * 6 * f.time;
        cosine += (f.note - mean) * std::cos(angle);
        sine += (f.note - mean) * std::sin(angle);
      }
    return std::atan2(sine, cosine);
  };
  double phaseDifference =
      std::remainder(phase(p->measured[2]) - phase(p->original),
                     2 * juce::MathConstants<double>::pi);
  double timingMs =
      phaseDifference / (2 * juce::MathConstants<double>::pi * 6) * 1000;
  std::cout << "VOICED ALIGNMENT phrase_vibrato_ms=" << timingMs << "\n";
  check(std::abs(timingMs) < 8,
        "Rubber Band moved voiced pitch modulation off the source timeline");
  double driftDifference = 0;
  for (int i = int(2.95 * 48000); i < int(3.3 * 48000); ++i)
    driftDifference +=
        std::abs(p->versions[1].getSample(0, i) - source.getSample(0, i));
  check(driftDifference < 1e-6,
        "Natural Drift changed an in-margin stable note");
  const auto &phrase = p->plans[2];
  float first = engineer::at(phrase.delta, 3.05 / p->original.hopSeconds),
        last = engineer::at(phrase.delta, 3.95 / p->original.hopSeconds);
  check(std::abs(first - last) < .003f,
        "Phrase Lead is correcting each note separately");
  int protectedFrames = 0;
  for (size_t i = 1; i + 1 < p->original.frames.size(); ++i)
    if (p->original.frames[i].attack && p->original.frames[i].voiced) {
      check(p->plans[4].wet[i] < .001f,
            "Transient Safe processed a protected attack");
      ++protectedFrames;
    }
  std::cout
      << "PASS: five distinct rendered versions, actual corrected pitch, "
         "retained vibrato, raw 8-cent margin, phrase shift, protected attacks="
      << protectedFrames
      << ", exact silence/length/stereo/BWF and float32 WAV\n";
  return p;
}
void processorTest() {
  auto p = std::make_unique<EngineerProcessor>();
  check(p->authorized, "Shared licence invalid");
  p->prepareToPlay(48000, 256);
  juce::MidiBuffer m;
  juce::AudioBuffer<float> b(2, 256);
  for (int c = 0; c < 2; ++c)
    for (int i = 0; i < 256; ++i)
      b.setSample(c, i, float(i) * .001f);
  auto ref = b;
  p->processBlock(b, m);
  for (int i = 0; i < 256; ++i)
    check(b.getSample(0, i) == ref.getSample(0, i),
          "Inactive plugin alters DAW audio");
  p->state.getParameter("volume")->setValueNotifyingHost(.3f);
  p->setMute(2, true);
  juce::MemoryBlock state;
  p->getStateInformation(state);
  auto restored = std::make_unique<EngineerProcessor>();
  restored->setStateInformation(state.getData(), int(state.getSize()));
  check(restored->isMuted(2) &&
            std::abs(restored->state.getParameter("volume")->getValue() - .3f) <
                .001f,
        "Session state failed");
  juce::AudioBuffer<float> zero(2, 0);
  p->processBlock(zero, m);
  juce::AudioBuffer<float> huge(2, 65536);
  huge.clear();
  p->processBlock(huge, m);
  check(p->getLatencySamples() == 0,
        "Offline preview should not delay DAW input");
  std::cout << "PASS: licence, unchanged live input, session, empty/large "
               "buffers, zero live-input latency\n";
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    auto folder =
        argc == 3 && juce::String(argv[1]) == "--render"
            ? juce::File(juce::String::fromUTF8(argv[2]))
            : juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("Engineer-test-" + juce::Uuid().toString());
    folder.createDirectory();
    threshold();
    projectTest(folder);
    rateAndSilence();
    processorTest();
    if (argc != 3)
      folder.deleteRecursively();
    std::cout << "ALL ENGINEER AUDIO TESTS PASSED\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
