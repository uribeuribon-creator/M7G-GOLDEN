#include "Support.h"
#include <fstream>
using namespace fresh;
static juce::AudioBuffer<float> tone(double hz, float amplitude = .3f,
                                     double seconds = .8, int channels = 2,
                                     double rate = 48000) {
  juce::AudioBuffer<float> b(channels, int(seconds * rate));
  for (int c = 0; c < channels; ++c)
    for (int i = 0; i < b.getNumSamples(); ++i)
      b.setSample(c, i,
                  amplitude *
                      float(std::sin(2 * juce::MathConstants<double>::pi * hz *
                                     i / rate)));
  return b;
}
static float rms(const juce::AudioBuffer<float> &b) {
  int start = b.getNumSamples() / 2;
  return b.getRMSLevel(0, start, b.getNumSamples() - start);
}
static void wire() {
  for (double sr : {44100., 48000., 96000.}) {
    Chain chain;
    chain.prepare(sr, 2, 256);
    auto p = defaults();
    p[dynOn] = p[eqOn] = p[spaceOn] = p[charOn] = 0;
    juce::AudioBuffer<float> settle(2, int(sr * .12));
    settle.clear();
    chain.process(settle, p, 120);
    for (int i = 0; i < 24; ++i) {
      chain.reset();
      p[order] = float(i);
      juce::AudioBuffer<float> b(2, chain.latency() + 400);
      b.clear();
      b.setSample(0, 0, .5f);
      b.setSample(1, 0, .25f);
      chain.process(b, p, 120);
      int maximum = 0;
      for (int n = 0; n < b.getNumSamples(); ++n)
        if (std::abs(b.getSample(0, n)) > std::abs(b.getSample(0, maximum)))
          maximum = n;
      check(maximum == chain.latency(),
            "Module bypass or reorder changes reported latency");
      check(std::abs(b.getSample(0, maximum) - .5f) < 1e-5,
            "Module bypass changes dry amplitude");
    }
    std::cout << "LATENCY sr=" << sr << " samples=" << chain.latency()
              << " ms=" << chain.latency() * 1000 / sr
              << " (24 module orders)\n";
  }
}
class TempoHead final : public juce::AudioPlayHead {
public:
  juce::Optional<PositionInfo> getPosition() const override {
    PositionInfo p;
    p.setBpm(100);
    p.setIsPlaying(true);
    p.setTimeInSamples(0);
    return p;
  }
};
static void hostSync() {
  FreshProcessor p;
  TempoHead head;
  p.setPlayHead(&head);
  p.prepareToPlay(48000, 256);
  p.set(fresh::dynOn, 0);
  p.set(fresh::eqOn, 0);
  p.set(fresh::charOn, 0);
  p.set(fresh::reverb, 0);
  p.set(fresh::delay, 100);
  p.set(fresh::division, 2);
  juce::MidiBuffer midi;
  juce::AudioBuffer<float> silence(2, 8000);
  silence.clear();
  p.processBlock(silence, midi);
  juce::AudioBuffer<float> impulse(2, 40000);
  impulse.clear();
  impulse.setSample(0, 0, .5f);
  impulse.setSample(1, 0, .5f);
  p.processBlock(impulse, midi);
  int expected = 28800 + p.getLatencySamples();
  check(impulse.getMagnitude(0, expected, 50) > .03f,
        "Processor ignores tempo supplied by DAW playhead");
  p.releaseResources();
}
static void dynamics() {
  MultiBandComp comp;
  comp.prepare(48000, 2, 512);
  for (double hz : {80., 700., 5000., 10000.}) {
    comp.reset();
    auto b = tone(hz);
    float before = rms(b);
    juce::dsp::AudioBlock<float> full(b);
    for (size_t i = 0; i < full.getNumSamples(); i += 512)
      comp.process(
          full.getSubBlock(i, std::min(size_t(512), full.getNumSamples() - i)),
          0, 50, 1);
    float ratio = db(rms(b) / before);
    std::cout << "CROSSOVER hz=" << hz << " dB=" << ratio << "\n";
    check(std::abs(ratio) < .6f, "Three-band LR recombination is not flat");
  }
  comp.reset();
  auto loud = tone(700, .75f);
  float before = rms(loud);
  juce::dsp::AudioBlock<float> full(loud);
  for (size_t i = 0; i < full.getNumSamples(); i += 512)
    comp.process(
        full.getSubBlock(i, std::min(size_t(512), full.getNumSamples() - i)),
        100, 50, 2);
  check(rms(loud) < before * .75f && comp.reduction() > 3,
        "Multiband compression has no actual effect");
  DeEsser ess;
  ess.prepare(48000, 2);
  auto hiss = tone(8000, .8f);
  juce::dsp::AudioBlock<float> hb(hiss);
  ess.process(hb, 5000);
  std::cout << "DEESS dB=" << ess.reduction() << "\n";
  check(ess.reduction() > 1 && rms(hiss) < .5f,
        "Sibilance detector/reduction does not engage");
  ess.reset();
  auto body = tone(220, .3f);
  juce::dsp::AudioBlock<float> vb(body);
  ess.process(vb, 5000);
  check(ess.reduction() < .1f, "De-esser suppresses low vocal body");
}
static void equaliser() {
  FreshEQ eq;
  eq.prepare(48000);
  auto p = defaults();
  for (auto id :
       {eqLow, eqLowMid, eqHighMid, eqHigh, eq160, eq350, eq1500, eq6000})
    p[size_t(id)] = 0;
  auto flat = tone(1000);
  float level = rms(flat);
  juce::dsp::AudioBlock<float> f(flat);
  eq.process(f, p);
  check(std::abs(db(rms(flat) / level)) < .15f, "Neutral EQ is not flat");
  eq.reset();
  p[eqHigh] = 6;
  auto bright = tone(16000);
  level = rms(bright);
  juce::dsp::AudioBlock<float> b(bright);
  eq.process(b, p);
  check(db(rms(bright) / level) > 3, "High shelf gain has no audio effect");
  eq.reset();
  p[eqHigh] = 0;
  p[eq350] = -12;
  auto cut = tone(350);
  level = rms(cut);
  juce::dsp::AudioBlock<float> c(cut);
  eq.process(c, p);
  check(db(rms(cut) / level) < -9, "Expert EQ band does not process audio");
}
static double bin(const juce::AudioBuffer<float> &b, double hz) {
  double real = 0, imag = 0;
  int first = b.getNumSamples() / 2, n = b.getNumSamples() - first;
  for (int i = 0; i < n; ++i) {
    double phase =
        2 * juce::MathConstants<double>::pi * hz * (i + first) / 48000;
    real += b.getSample(0, i + first) * std::cos(phase);
    imag += b.getSample(0, i + first) * std::sin(phase);
  }
  return std::hypot(real, imag) * 2 / n;
}
static void characterTest() {
  CharacterSat sat;
  sat.prepare(48000, 2, 512);
  auto neutral = tone(20, .3f, 1);
  float base = rms(neutral);
  auto zero = defaults();
  zero[character] = 0;
  juce::dsp::AudioBlock<float> clean(neutral);
  for (size_t i = 0; i < clean.getNumSamples(); i += 512)
    sat.process(
        clean.getSubBlock(i, std::min(size_t(512), clean.getNumSamples() - i)),
        zero);
  check(std::abs(db(rms(neutral) / base)) < .3f,
        "Character at zero removes vocal low-frequency body");
  sat.reset();
  auto p = defaults();
  p[character] = 100;
  p[charMode] = 2;
  p[harmonics] = 1;
  p[tape] = 2;
  auto input = tone(10000, .8f, 1), processed = input, raw = input;
  for (int c = 0; c < raw.getNumChannels(); ++c)
    for (int i = 0; i < raw.getNumSamples(); ++i)
      raw.setSample(c, i,
                    CharacterSat::transfer(raw.getSample(c, i), 1, 2, 1, 2));
  juce::dsp::AudioBlock<float> b(processed);
  for (size_t i = 0; i < b.getNumSamples(); i += 512)
    sat.process(b.getSubBlock(i, std::min(size_t(512), b.getNumSamples() - i)),
                p);
  double reference = bin(raw, 18000), filtered = bin(processed, 18000);
  std::cout << "ALIAS 18k raw=" << reference << " 4x=" << filtered << "\n";
  check(filtered < reference * .6,
        "4x saturation oversampling does not reduce folded alias");
  check(fingerprint(input) != fingerprint(processed),
        "Character is cosmetic only");
  float warm = CharacterSat::transfer(.7f, .8f, 0, 0, 0),
        tube = CharacterSat::transfer(.7f, .8f, 1, 0, 1),
        hard = CharacterSat::transfer(.7f, .8f, 2, 1, 2);
  check(std::abs(warm - tube) > .001f && std::abs(warm - hard) > .001f,
        "Character modes do not differ");
}
static void spaceTest() {
  check(std::abs(FreshSpace::delaySeconds(0, 120) - .125) < 1e-9 &&
            std::abs(FreshSpace::delaySeconds(1, 120) - .25) < 1e-9 &&
            std::abs(FreshSpace::delaySeconds(2, 120) - .5) < 1e-9,
        "Delay beat divisions are wrong");
  for (double bpm : {100., 120.}) {
    FreshSpace fx;
    fx.prepare(48000, 2);
    auto p = defaults();
    p[reverb] = 0;
    p[delay] = 100;
    p[division] = 2;
    juce::AudioBuffer<float> b(2, 48000);
    b.clear();
    b.setSample(0, 0, .5f);
    b.setSample(1, 0, .5f);
    juce::dsp::AudioBlock<float> full(b);
    for (size_t i = 0; i < full.getNumSamples(); i += 256)
      fx.process(
          full.getSubBlock(i, std::min(size_t(256), full.getNumSamples() - i)),
          p, bpm);
    int expected = int(std::round(FreshSpace::delaySeconds(2, bpm) * 48000));
    float early = b.getMagnitude(0, 1, expected - 2),
          echo = b.getMagnitude(0, expected, 50);
    std::cout << "DELAY bpm=" << bpm << " sample=" << expected
              << " peak=" << echo << "\n";
    check(early < 1e-5 && echo > .03f,
          "Tempo delay arrival is incorrect or silent");
  }
  FreshSpace fx;
  fx.prepare(48000, 2);
  auto p = defaults();
  p[reverb] = 80;
  p[delay] = 0;
  auto signal = tone(440, .8f, .5);
  juce::dsp::AudioBlock<float> voice(signal);
  for (size_t i = 0; i < voice.getNumSamples(); i += 256)
    fx.process(
        voice.getSubBlock(i, std::min(size_t(256), voice.getNumSamples() - i)),
        p, 120);
  check(fx.duckReduction() > 3,
        "Space ducking does not respond to vocal input");
  juce::AudioBuffer<float> tail(2, 48000);
  tail.clear();
  juce::dsp::AudioBlock<float> t(tail);
  for (size_t i = 0; i < t.getNumSamples(); i += 256)
    fx.process(t.getSubBlock(i, std::min(size_t(256), t.getNumSamples() - i)),
               p, 120);
  check(tail.getRMSLevel(0, 12000, 24000) > .00001f,
        "Reverb has no actual tail");
}
static void presetsAndState() {
  check(PresetManager::factory().size() == 35,
        "Expected 25 factory presets plus 10 M7G presets");
  auto source = vocal(48000, 2);
  std::set<uint64_t> hashes;
  Chain c;
  c.prepare(48000, 2, 256);
  for (auto &preset : PresetManager::factory()) {
    c.reset();
    juce::AudioBuffer<float> b(2, 48000);
    for (int channel = 0; channel < 2; ++channel)
      b.copyFrom(channel, 0, source, channel, 12000, 48000);
    c.process(b, preset.values, 120);
    check(b.getMagnitude(0, b.getNumSamples()) <= .945f &&
              b.getMagnitude(1, b.getNumSamples()) <= .945f,
          "Preset output clips above limiter ceiling");
    for (int i = 0; i < b.getNumSamples(); ++i)
      check(std::isfinite(b.getSample(0, i)), "Preset produces NaN/Inf");
    hashes.insert(fingerprint(b));
    auto tree = PresetManager::toTree(preset.values, preset.name);
    Values value;
    juce::String name;
    check(PresetManager::fromTree(tree, value, name) &&
              value == preset.values && name == preset.name,
          "Preset XML does not retain full chain state");
  }
  check(hashes.size() >= 32, "Factory presets do not produce distinct audio");
  FreshProcessor p;
  p.prepareToPlay(48000, 256);
  p.choosePreset(22, false);
  p.set(fresh::input, 3);
  p.set(fresh::order, 17);
  juce::MemoryBlock state;
  p.getStateInformation(state);
  FreshProcessor restored;
  restored.prepareToPlay(48000, 256);
  restored.setStateInformation(state.getData(), int(state.getSize()));
  check(restored.settings() == p.settings(),
        "Plugin state restore loses parameters");
  p.set(comp, 61);
  auto a = p.settings();
  p.switchAB();
  p.set(comp, 17);
  auto b = p.settings();
  p.switchAB();
  check(p.settings() == a, "A state was not restored");
  p.switchAB();
  check(p.settings() == b, "B state was not restored");
  p.releaseResources();
  restored.releaseResources();
  std::cout << "PRESETS unique_audio=" << hashes.size()
            << "/35; A/B, state and full XML passed\n";
}
static void writeDemo(const juce::File &f, const juce::AudioBuffer<float> &b) {
  auto stream = f.createOutputStream();
  juce::WavAudioFormat wav;
  std::unique_ptr<juce::AudioFormatWriter> writer(
      stream ? wav.createWriterFor(stream.get(), 48000,
                                   unsigned(b.getNumChannels()), 24, {}, 0)
             : nullptr);
  check(bool(writer), "Demo writer failed");
  stream.release();
  check(writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples()),
        "Demo write failed");
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    wire();
    hostSync();
    dynamics();
    equaliser();
    characterTest();
    spaceTest();
    presetsAndState();
    if (argc == 3 && juce::String(argv[1]) == "--render") {
      juce::File root(juce::String::fromUTF8(argv[2]));
      root.createDirectory();
      auto source = vocal(), wet = source;
      Chain c;
      c.prepare(48000, 2, 256);
      c.process(wet, PresetManager::factory()[0].values, 120);
      writeDemo(root.getChildFile("Fresh-Vocal-Demo-Original.wav"), source);
      writeDemo(root.getChildFile("Fresh-Vocal-Demo-Aggressive.wav"), wet);
    }
    std::cout << "PASS: real de-essing, phase-aligned multiband, expert EQ, 4x "
                 "anti-alias saturation, tempo echoes, ducked reverb, fixed "
                 "measured latency, 35 presets, limiter, A/B and saved state\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
