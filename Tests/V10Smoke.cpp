#include "../Source/PluginProcessor.h"
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace rack;
namespace {
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
void neutral(M7GProcessor &p) {
  for (size_t i = 0; i < Count; ++i)
    p.setValue(static_cast<Param>(i), specs()[i].def);
  for (Param i :
       {AutoGain,  GateOn,     DeessOn, WarmOn,     EQOn,        AirOn,
        CompOn,    FetOn,      LimitOn, TuneOn,     NoiseOn,     ReverbOn,
        DelayOn,   StereoOn,   ModOn,   DereverbOn, ResonanceOn, SaturationOn,
        HarmonyOn, SculptorOn, AlignOn, GodSpace})
    p.setValue(i, 0);
  p.setValue(Sino, 0);
  p.setValue(InputTrim, 0);
  p.setValue(OutputTrim, 0);
  p.setValue(Mix, 100);
}
double power(const std::vector<float> &x, int from = 0) {
  double e = 0;
  for (size_t i = size_t(from); i < x.size(); ++i)
    e += x[i] * x[i];
  return e / juce::jmax(1, int(x.size()) - from);
}
double spectral(const std::vector<float> &x, double f, int from = 48000) {
  double re = 0, im = 0;
  for (size_t i = size_t(from); i < x.size(); ++i) {
    double a = juce::MathConstants<double>::twoPi * f * i / 48000;
    re += x[i] * std::cos(a);
    im += x[i] * std::sin(a);
  }
  return (re * re + im * im) / std::pow(double(x.size() - size_t(from)), 2);
}
std::vector<float> spaceRender(int mask) {
  auto s = std::make_unique<V10TripleSpace>();
  s->prepare(48000, 1);
  auto v = defaults();
  v[ReverbMix] = 12;
  v[ReverbOn] = 1;
  v[GodSpace] = 0;
  v[ReverbDuck] = 0;
  v[ReverbDecay] = 1.5f;
  v[VerbAMix] = (mask & 1) ? 20.0f : 0;
  v[VerbBMix] = (mask & 2) ? 20.0f : 0;
  v[VerbCMix] = (mask & 4) ? 20.0f : 0;
  v[ReverbShimmer] = 25;
  v[DelayOn] = 0;
  s->setControls(v, 120);
  std::vector<float> out(48000 * 6);
  for (int i = 0; i < int(out.size()); ++i) {
    float dry = i == 0 ? .8f : 0;
    float x[2]{dry, dry};
    s->process(x, 2);
    out[size_t(i)] = x[0] - dry;
    require(std::isfinite(x[0]) && std::abs(x[0]) < 4,
            "Unstable triple reverb");
  }
  return out;
}
} // namespace
int main() {
  try {
    juce::ScopedJuceInitialiser_GUI init;
    juce::MidiBuffer midi;
    auto begin = std::chrono::steady_clock::now();
    auto p = std::make_unique<M7GProcessor>();
    std::set<juce::String> names;
    std::set<Values> values;
    for (int preset = 0; preset < presetCount; ++preset) {
      p->setCurrentProgram(preset);
      names.insert(p->getProgramName(preset));
      values.insert(presetValues(preset));
      p->setRateAndBufferSizeDetails(48000, 256);
      p->prepareToPlay(48000, 256);
      double e = 0;
      for (int block = 0; block < 110; ++block) {
        juce::AudioBuffer<float> b(2, 256);
        for (int i = 0; i < 256; ++i) {
          float phase = float(juce::MathConstants<double>::twoPi * 223 *
                              (block * 256 + i) / 48000);
          float x = .18f * (std::sin(phase) + .25f * std::sin(phase * 3));
          b.setSample(0, i, x);
          b.setSample(1, i, x);
        }
        p->processBlock(b, midi);
        for (int c = 0; c < 2; ++c)
          for (int i = 0; i < 256; ++i) {
            float x = b.getSample(c, i);
            require(std::isfinite(x) && std::abs(x) <= .892f,
                    "Preset exceeds ceiling or is non-finite");
            e += x * x;
          }
      }
      if (e < .001) {
        std::cerr << "Silent preset " << preset << " "
                  << p->getProgramName(preset) << " energy " << e << "\n";
        throw std::runtime_error("Silent preset");
      }
    }
    require(names.size() == 280 && values.size() == 280,
            "Preset names/settings are not distinct");
    std::cout << "PASS: 280 distinct presets rendered; 50 classic/future FX; "
                 "finite non-silent output and ceiling\n";
    for (int mode = 0; mode < 3; ++mode) {
      for (bool bypass : {false, true}) {
        neutral(*p);
        p->setValue(EngineMode, float(mode));
        p->setValue(Bypass, bypass ? 1.0f : 0);
        p->prepareToPlay(48000, 128);
        int latency = p->getLatencySamples();
        require(mode ? latency > 0 : latency == 0, "Invalid mode latency");
        double max = 0, error = 0;
        int peak = 0;
        for (int start = 0; start < 16000; start += 128) {
          juce::AudioBuffer<float> b(2, 128);
          b.clear();
          if (start == 0)
            b.setSample(0, 0, .2f);
          p->processBlock(b, midi);
          for (int i = 0; i < 128; ++i) {
            int t = start + i;
            float x = b.getSample(0, i);
            if (std::abs(x) > max) {
              max = std::abs(x);
              peak = t;
            }
            if (bypass)
              error += std::pow(x - (t == latency ? .2f : 0), 2);
          }
        }
        std::cout << "Mode " << mode << " bypass " << bypass << " declared "
                  << latency << " peak " << peak << " error " << error << "\n";
        require(std::abs(peak - latency) <= (mode == 1 ? 4 : 1),
                "Impulse delay differs from host latency");
        require(!bypass || error < 1e-9, "Bypass is not latency-aligned");
      }
    }
    std::cout << "PASS: Monitor zero latency, Studio/Linear delay and aligned "
                 "bypass\n";
    auto harmonyRender = [](float ratio, bool choir, float formant) {
      auto h = std::make_unique<V8Harmony>();
      h->prepare(48000);
      auto v = defaults();
      v[TuneOn] = choir ? 0.0f : 1.0f;
      v[HarmonyOn] = choir ? 1.0f : 0.0f;
      v[HarmonyMix] = 60;
      v[HarmonyPattern] = 1;
      v[HarmonyDetune] = 0;
      v[PitchFormant] = formant;
      h->setControls(v, ratio, 223, .99f);
      std::vector<float> out(144000);
      for (int i = 0; i < int(out.size()); ++i) {
        float x = .15f * std::sin(float(juce::MathConstants<double>::twoPi *
                                        (choir ? 220 : 223) * i / 48000));
        float frame[]{x, x};
        h->process(frame, 2);
        out[size_t(i)] = frame[0];
      }
      require(!choir || h->voices() == 8,
              "Harmony engine did not activate eight voices");
      return out;
    };
    auto chord = harmonyRender(1, true, 0);
    double fifth = spectral(chord, 220 * std::pow(2.0, 7.0 / 12)),
           octave = spectral(chord, 440);
    std::cout << "Harmony fifth power " << fifth << " octave power " << octave
              << "\n";
    require(fifth > 1e-7 && octave > 1e-7, "Harmony fifth/octave absent");
    auto tune = harmonyRender(220.0f / 223, false, 0);
    double best = 0, hz = 0;
    for (double f = 218; f < 225; f += .1) {
      double e = spectral(tune, f);
      if (e > best) {
        best = e;
        hz = f;
      }
    }
    std::cout << "Vocoder correction 223 -> " << hz << " Hz\n";
    require(std::abs(hz - 220) < .6, "Vocoder correction is inaccurate");
    std::cout << "PASS: eight harmony voices, fifths/octaves and phase-vocoder "
                 "correction\n";
    auto a = spaceRender(1), b = spaceRender(2), c = spaceRender(4),
         all = spaceRender(7);
    double err = 0;
    for (size_t i = 0; i < a.size(); ++i)
      err += std::pow(all[i] - a[i] - b[i] - c[i], 2);
    std::cout << "Triple verb sum error " << err << " late A "
              << power(a, 96000) << " B " << power(b, 96000) << " C "
              << power(c, 96000) << "\n";
    require(err < 1e-8, "Triple reverb layers do not sum independently");
    require(power(a, 96000) < power(b, 96000) * .1 && power(c, 96000) > 1e-13,
            "Independent decay tails absent");
    std::cout << "PASS: three simultaneous independent reverbs, "
                 "short/medium/long tails\n";
    auto delayEnergy = [](int style, float duck, bool tail) {
      auto d = std::make_unique<V10SmartDelay>();
      d->prepare(48000, 1);
      auto v = defaults();
      v[DelayOn] = 1;
      v[DelayStyle] = float(style);
      v[DelayMix] = 30;
      v[DelayDuck] = duck;
      v[DelayFeedback] = 45;
      v[DelayDrive] = 20;
      d->setControls(v, 120);
      double e = 0;
      for (int i = 0; i < 144000; ++i) {
        float x =
            (!tail || i < 48000)
                ? .3f * std::sin(float(juce::MathConstants<double>::twoPi *
                                       220 * i / 48000))
                : 0;
        float in[]{x, x}, out[2]{};
        d->wet(in, out, 2);
        if ((!tail && i > 96000) || (tail && i > 70000 && i < 110000))
          e += out[0] * out[0] + out[1] * out[1];
      }
      return e;
    };
    double unDucked = delayEnergy(3, 0, false),
           ducked = delayEnergy(3, 85, false),
           throwTail = delayEnergy(1, 80, true),
           throwDuring = delayEnergy(1, 80, false);
    std::cout << "Tape ducking " << 10 * std::log10(ducked / unDucked)
              << " dB; throw during " << throwDuring << " after " << throwTail
              << "\n";
    require(ducked < unDucked * .1 && throwTail > 1e-6 &&
                throwDuring < throwTail * .01,
            "Ducking or phrase throw failed");
    std::cout << "PASS: tape ducking and throw at phrase endings\n";
    auto align = std::make_unique<V10Align>();
    align->prepare(48000);
    auto settings = defaults();
    settings[AlignAmount] = 100;
    settings[AlignRange] = 60;
    settings[AlignSpeed] = 60;
    align->setControls(settings);
    double alignedError = 0, referencePower = 0;
    auto voice = [](double t) {
      if (t < 0)
        return 0.0f;
      double pulse = std::pow(
          .5 + .5 * std::sin(juce::MathConstants<double>::twoPi * 3.7 * t), 4);
      return float((.03 + .2 * pulse) *
                   std::sin(juce::MathConstants<double>::twoPi * 220 * t));
    };
    for (int i = 0; i < 192000; ++i) {
      double t = double(i) / 48000;
      float reference[]{voice(t), voice(t)},
          frame[]{voice(t - .025), voice(t - .025)};
      align->process(frame, reference, 2, true);
      if (i > 144000) {
        float expected = voice(t - .08);
        alignedError += std::pow(frame[0] - expected, 2);
        referencePower += expected * expected;
      }
    }
    std::cout << "Alignment offset " << align->offsetMs() << " ms confidence "
              << align->certainty() << " relative error "
              << alignedError / referencePower << "\n";
    require(std::abs(align->offsetMs() - 25) < 5.1 && align->certainty() > .6,
            "Reference alignment did not find delay");
    require(alignedError / referencePower < .15,
            "Reference alignment did not correct timing");
    std::cout << "PASS: reference-driven vocal alignment\n";
    // New individual-analysis routes run on real audio with unrelated output
    // preserved.
    const Param probes[]{DereverbAmount,  ResonanceAmount, SatMid,
                         HarmonyMix,      FetThreshold,    NoiseAmount,
                         PreampImpedance, CompParallel,    VerbAMix,
                         VerbBMix,        VerbCMix,        ThrowSensitivity,
                         SculptorAmount};
    for (int module = 12; module <= 24; ++module) {
      neutral(*p);
      p->setValue(probes[module - 12],
                  specs()[size_t(probes[module - 12])].min);
      p->setValue(OutputTrim, -3);
      p->prepareToPlay(48000, 256);
      p->beginAnalysis(module);
      for (int block = 0; block < 565; ++block) {
        juce::AudioBuffer<float> audio(2, 256);
        for (int i = 0; i < 256; ++i) {
          float x = .13f * std::sin(float(juce::MathConstants<double>::twoPi *
                                          220 * (block * 256 + i) / 48000));
          audio.setSample(0, i, x);
          audio.setSample(1, i, x);
        }
        p->processBlock(audio, midi);
      }
      require(p->analysisState.load() == 2 && p->applyAnalysis(),
              "Module analysis did not finish");
      require(p->value(probes[module - 12]) >
                  specs()[size_t(probes[module - 12])].min + .01f,
              "Module analysis did not change target");
      require(p->value(OutputTrim) == -3,
              "Individual analysis changed unrelated output");
    }
    std::cout << "PASS: new spectral, harmony, FET, Unison, parallel and "
                 "triple-space analysis routes\n";

    std::cout << "PASS: V10 integration checks in "
              << std::chrono::duration<double>(
                     std::chrono::steady_clock::now() - begin)
                     .count()
              << " seconds\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
