#include "TestSupport.h"
#include <set>
void detection() {
  for (float hz : {65.f, 82.41f, 110.f, 196.f, 261.63f, 440.f, 880.f}) {
    tune::Analysis a;
    a.prepare(48000);
    for (int i = 0; i < 48000; ++i)
      a.process(vowel(i / 48000., hz), nullptr);
    float cents = 1200 * std::log2(std::max(.001f, a.hz) / hz);
    std::cout << "DETECT expected=" << hz << " observed=" << a.hz
              << " cents=" << cents << " confidence=" << a.confidence << "\n";
    requireTune(std::abs(cents) < 9, "Pitch detection cents/octave error");
  }
  std::cout
      << "PASS: independent synthetic voiced detection across 65–880 Hz\n";
}
void tuning() {
  for (int q = 0; q < 2; ++q) {
    auto p = tune::defaults();
    p[tune::AutoKey] = 0;
    p[tune::ScaleKind] = 0;
    p[tune::Correction] = 100;
    p[tune::Retune] = 0;
    p[tune::Human] = 0;
    p[tune::Robot] = 100;
    auto r = renderTune(p, q);
    double hz = measuredPitch(r.left, 220);
    double cents = 1200 * std::log2(hz / 220);
    std::cout << "TUNE quality=" << q << " actual=" << hz << " cents=" << cents
              << "\n";
    requireTune(std::abs(cents) < 12, "Actual tuned output misses target");
  }
  auto p = tune::defaults();
  p[tune::AutoKey] = 0;
  p[tune::Key] = 0;
  p[tune::ScaleKind] = 1;
  p[tune::Correction] = 100;
  p[tune::Retune] = 0;
  p[tune::Human] = 70;
  p[tune::HarmonyOn] = 1;
  p[tune::VoiceCount] = 1;
  p[tune::HarmonyMix] = 100;
  auto choir = renderTune(p);
  double harmony = tune::frequency(60);
  double peak = amplitude(choir.left, harmony);
  std::cout << "HARMONY C4 amplitude=" << peak
            << " nearby=" << amplitude(choir.left, harmony + 15) << "\n";
  requireTune(peak > .00003 && peak > amplitude(choir.left, harmony + 15) * 2,
              "Scale-correct third missing from actual output");
  auto e = std::make_unique<tune::Engine>();
  e->prepare(48000, 0);
  e->set(p, true);
  for (int i = 0; i < 48000; ++i)
    e->process(vowel(i / 48000., 224.7), vowel(i / 48000., 224.7), nullptr);
  requireTune(e->scale.allows(int(std::round(e->notes[1]))) &&
                  std::abs(e->notes[1] - 60) < .1f,
              "Harmony does not follow C major");
  auto raw = tune::defaults();
  raw[tune::Correction] = raw[tune::Robot] = 0;
  auto a = renderTune(raw);
  for (int i = tune::FirstFX; i < tune::Count; ++i)
    raw[size_t(i)] = i % 6 == 5 ? .9f : 99.f;
  auto b = renderTune(raw);
  requireTune(a.left == b.left,
              "Harmony-only FX modified the lead with choir disabled");
  raw[tune::DoublesOn] = 1;
  raw[tune::HarmonyMix] = 100;
  raw[tune::DoubleHuman] = 100;
  auto d = renderTune(raw);
  double diff = 0;
  for (size_t i = 0; i < d.left.size(); ++i)
    diff += std::pow(d.left[i] - d.right[i], 2);
  requireTune(diff > .001, "Two doubles have no independent stereo variation");
  std::cout << "PASS: actual tuned audio, diatonic third, independent doubles "
               "and generated-FX isolation\n";
}
std::pair<double, double> vibrato(bool robot) {
  auto e = std::make_unique<tune::Engine>();
  e->prepare(48000, 0);
  auto p = tune::defaults();
  p[tune::AutoKey] = 0;
  p[tune::ScaleKind] = 0;
  p[tune::Correction] = 100;
  p[tune::Robot] = robot ? 100.f : 0.f;
  p[tune::Human] = robot ? 0.f : 100.f;
  p[tune::Retune] = 0;
  e->set(p, true);
  double phase = 0, previous = 0, previousCross = -1;
  std::vector<double> cents;
  for (int i = 0; i < 48000 * 5; ++i) {
    double t = i / 48000.;
    double hz =
        220 * std::exp2((35 + 22 * std::sin(6.28318530718 * 5 * t)) / 1200.);
    phase += 6.28318530718 * hz / 48000.;
    float x = float(.18 * std::sin(phase));
    auto out = e->process(x, x, nullptr);
    double y = out[0];
    if (i > 96000 && y >= 0 && previous < 0) {
      double cross = i - y / (y - previous);
      if (previousCross >= 0) {
        double f = 48000 / (cross - previousCross);
        if (f > 180 && f < 270)
          cents.push_back(1200 * std::log2(f / 220));
      }
      previousCross = cross;
    }
    previous = y;
  }
  double mean = 0, var = 0;
  for (double c : cents)
    mean += c / std::max(size_t(1), cents.size());
  for (double c : cents)
    var += (c - mean) * (c - mean) / std::max(size_t(1), cents.size());
  return {mean, std::sqrt(var)};
}
double spectralCentre(const std::vector<float> &x) {
  juce::dsp::FFT fft(12);
  std::array<std::complex<float>, 4096> in{}, out{};
  int n = int(x.size());
  for (int i = 0; i < 4096; ++i)
    in[size_t(i)] = {x[size_t(n - 4096 + i)] *
                         float(.5 - .5 * std::cos(6.28318530718 * i / 4096)),
                     0};
  fft.perform(in.data(), out.data(), false);
  double sum = 0, moment = 0;
  for (int k = 1; k < 2048; ++k) {
    double f = k * 48000. / 4096;
    if (f < 400 || f > 4500)
      continue;
    double p = std::norm(out[size_t(k)]);
    sum += p;
    moment += p * f;
  }
  return moment / std::max(1e-12, sum);
}
void expressionAndFormants() {
  auto natural = vibrato(false), robot = vibrato(true);
  std::cout << "VIBRATO natural mean=" << natural.first
            << " std=" << natural.second << " robot mean=" << robot.first
            << " std=" << robot.second << " cents\n";
  requireTune(std::abs(natural.first) < 12 && natural.second > 6 &&
                  robot.second < natural.second * .85,
              "Natural vibrato not preserved or robot fails to tighten it");
  auto p = tune::defaults();
  p[tune::AutoKey] = 0;
  p[tune::ScaleKind] = 0;
  p[tune::Correction] = 100;
  p[tune::Robot] = 100;
  p[tune::Human] = 0;
  p[tune::Retune] = 0;
  auto neutral = renderTune(p);
  p[tune::Formant] = 4;
  auto bright = renderTune(p);
  double a = spectralCentre(neutral.left), b = spectralCentre(bright.left);
  std::cout << "FORMANT centroid neutral=" << a << " shifted=" << b << " Hz\n";
  requireTune(b > a * 1.06 &&
                  std::abs(1200 * std::log2(measuredPitch(bright.left, 220) /
                                            220)) < 12,
              "Formant control does not independently change timbre");
  auto spectral = std::make_unique<tune::Spectral<11>>();
  spectral->prepare(48000);
  auto cfg = tune::defaults();
  cfg[tune::Correction] = 100;
  cfg[tune::Robot] = 0;
  std::array<float, 7> ratios{2, 1, 1, 1, 1, 1, 1};
  spectral->controls(cfg, ratios, true, 110);
  std::vector<float> reference, shifted, chipmunk;
  for (int i = 0; i < 120000; ++i) {
    double t = i / 48000.;
    float x = vowel(t, 110);
    reference.push_back(x);
    chipmunk.push_back(vowel(t * 2, 110));
    shifted.push_back(spectral->process(x, x, false)[0]);
  }
  double originalCentre = spectralCentre(reference),
         shiftedCentre = spectralCentre(shifted),
         naiveCentre = spectralCentre(chipmunk);
  std::cout << "FORMANT octave source=" << originalCentre
            << " preserved=" << shiftedCentre
            << " plain-resample=" << naiveCentre << " Hz\n";
  requireTune(std::abs(shiftedCentre - originalCentre) <
                  std::abs(naiveCentre - originalCentre) * .65,
              "Octave shift fails spectral-envelope preservation comparison");
  std::cout << "PASS: measured natural/robot vibrato and independent formant "
               "timbre shift\n";
}
void stateAndRuntime() {
  auto p = std::make_unique<TuneProcessor>();
  requireTune(p->authorized.load(), "Shared licence not loaded");
  requireTune(
      p->neural->ready.load(),
      ("Bundled ONNX Runtime unavailable: " + p->neural->error).toRawUTF8());
  p->setCurrentProgram(68);
  p->setValue("correction", 82);
  p->copyCompare();
  p->selectCompare(1);
  p->setValue("correction", 31);
  p->selectCompare(0);
  requireTune(std::abs(p->value("correction") - 82) < .1f,
              "A/B restore failed");
  juce::MemoryBlock blob;
  p->getStateInformation(blob);
  auto copy = std::make_unique<TuneProcessor>();
  copy->setStateInformation(blob.getData(), int(blob.getSize()));
  requireTune(copy->getCurrentProgram() == 68 &&
                  std::abs(copy->value("correction") - 82) < .1f,
              "Session state failed");
  bool fav = p->isFavourite(68);
  p->toggleFavourite(68);
  auto remembered = std::make_unique<TuneProcessor>();
  requireTune(remembered->isFavourite(68) != fav,
              "Favourite persistence failed");
  p->toggleFavourite(68);
  p->setRateAndBufferSizeDetails(48000, 256);
  p->prepareToPlay(48000, 256);
  juce::MidiBuffer midi;
  for (int k = 0; k < 190; ++k) {
    juce::AudioBuffer<float> b(2, 256);
    for (int i = 0; i < 256; ++i) {
      float x = vowel((k * 256 + i) / 48000., 196);
      b.setSample(0, i, x);
      b.setSample(1, i, x);
    }
    p->processBlock(b, midi);
  }
  for (int i = 0; i < 50 && p->neural->runs.load() == 0; ++i)
    juce::Thread::sleep(5);
  requireTune(p->neural->runs.load() > 0,
              "ORT sessions loaded but no inference ran");
  std::cout << "ONNX successful inference batches=" << p->neural->runs.load()
            << " profile=" << p->profileText() << "\n";
  for (int i = 0; i < 4; ++i)
    requireTune(std::isfinite(p->neural->acoustic[size_t(i)].load()),
                "Invalid neural acoustic output");
  std::cout << "PASS: real ONNX Runtime inference, global licence, "
               "session/A-B/favourites\n";
}
void latencyAndSafety() {
  for (double rate : {44100., 48000., 96000.})
    for (int q = 0; q < 2; ++q) {
      auto p = std::make_unique<TuneProcessor>();
      p->setValue("quality", float(q));
      p->setValue("bypass", 1);
      p->setRateAndBufferSizeDetails(rate, 127);
      p->prepareToPlay(rate, 127);
      int delay = q == 0 ? 2048 : 512;
      requireTune(p->getLatencySamples() == delay, "Declared latency mismatch");
      juce::MidiBuffer m;
      double error = 0;
      for (int s = 0; s < delay + 800; s += 127) {
        juce::AudioBuffer<float> b(2, 127);
        b.clear();
        if (!s) {
          b.setSample(0, 0, .3f);
          b.setSample(1, 0, .3f);
        }
        p->processBlock(b, m);
        for (int i = 0; i < 127; ++i)
          error += std::abs(b.getSample(0, i) - (s + i == delay ? .3f : 0));
      }
      requireTune(error < 1e-6, "Bypass changes aligned clean audio");
      juce::AudioBuffer<float> empty(2, 0);
      p->processBlock(empty, m);
      juce::AudioBuffer<float> large(2, 65536);
      large.clear();
      p->processBlock(large, m);
      requireTune(large.getRMSLevel(0, 0, 65536) < 1e-8,
                  "Silent oversized buffer produced audio");
    }
  for (int q = 0; q < 2; ++q)
    for (double sr : {44100., 96000.})
      for (int block : {64, 1024}) {
        auto p = std::make_unique<TuneProcessor>();
        p->setValue("quality", float(q));
        p->setCurrentProgram(54);
        p->setRateAndBufferSizeDetails(sr, block);
        p->prepareToPlay(sr, block);
        juce::MidiBuffer m;
        for (int k = 0; k < 10; ++k) {
          juce::AudioBuffer<float> b(2, block);
          for (int i = 0; i < block; ++i) {
            float x = vowel((k * block + i) / sr, 224.7);
            b.setSample(0, i, x);
            b.setSample(1, i, x);
          }
          p->processBlock(b, m);
          for (int i = 0; i < block; ++i)
            requireTune(std::isfinite(b.getSample(0, i)), "Rate/block failure");
        }
      }
  std::cout << "PASS: exact bypass latency, three rates, two qualities, "
               "alternate/zero/oversized blocks\n";
}
void presets() {
  std::set<juce::String> names;
  std::set<uint64_t> hash;
  for (int i = 0; i < 100; ++i) {
    auto r = renderTune(tune::presets()[size_t(i)].p, 0, 48000, 1.7);
    names.insert(tune::presets()[size_t(i)].name);
    hash.insert(r.hash);
  }
  std::cout << "PRESETS unique names=" << names.size()
            << " distinct audio=" << hash.size() << "\n";
  requireTune(names.size() == 100 && hash.size() == 100,
              "Factory preset duplication");
  std::cout
      << "PASS: all 100 presets produce distinct finite audible renders\n";
}
void demo(const juce::File &dir) {
  dir.createDirectory();
  auto f = dir.getChildFile("M7G-Tune-V1-Demo.wav");
  f.deleteFile();
  auto stream = f.createOutputStream();
  juce::WavAudioFormat format;
  std::unique_ptr<juce::AudioFormatWriter> w(
      format.createWriterFor(stream.release(), 48000, 2, 16, {}, 0));
  requireTune(bool(w), "Demo writer unavailable");
  juce::String text = "Synthetic vowel, not a real singer. Same detuned source "
                      "through eight tuning/choir presets.\n";
  double t = 0;
  for (int preset : {0, 3, 25, 30, 50, 54, 75, 78}) {
    auto r = renderTune(tune::presets()[size_t(preset)].p, 0, 48000, 3);
    juce::AudioBuffer<float> b(2, int(r.left.size()));
    for (int i = 0; i < b.getNumSamples(); ++i) {
      b.setSample(0, i, r.left[size_t(i)] * .8f);
      b.setSample(1, i, r.right[size_t(i)] * .8f);
    }
    requireTune(w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples()),
                "Demo write failed");
    text += juce::String(t, 2) + "s | " + tune::presets()[size_t(preset)].name +
            "\n";
    t += 3;
  }
  w.reset();
  dir.getChildFile("M7G-Tune-V1-Demo.txt").replaceWithText(text);
  std::cout << "PASS: eight-preset synthetic demonstration\n";
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    if (argc == 3 && juce::String(argv[1]) == "--render") {
      demo(juce::File(juce::String::fromUTF8(argv[2])));
      return 0;
    }
    detection();
    tuning();
    expressionAndFormants();
    stateAndRuntime();
    latencyAndSafety();
    presets();
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
