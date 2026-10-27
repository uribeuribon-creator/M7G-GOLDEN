#include "../Source/PluginProcessor.h"
#include <functional>
#include <iostream>
#include <set>
using namespace rack;
namespace {
void neutral(M7GProcessor &p) {
  for (Param id :
       {AutoGain, GateOn, DeessOn, WarmOn, EQOn, AirOn, CompOn, LimitOn, TuneOn,
        NoiseOn, ReverbOn, DelayOn, StereoOn, ModOn, Bypass, Solo})
    p.setValue(id, 0);
  p.setValue(Mix, 100);
  p.setValue(InputTrim, 0);
  p.setValue(OutputTrim, 0);
}
std::vector<float> render(const std::function<void(M7GProcessor &)> &configure,
                          float frequency = 220, float amplitude = .15f,
                          int samples = 48000, int channel = 0) {
  auto p = std::make_unique<M7GProcessor>();
  neutral(*p);
  configure(*p);
  p->prepareToPlay(48000, 256);
  juce::MidiBuffer midi;
  std::vector<float> out;
  out.reserve(size_t(samples));
  for (int start = 0; start < samples; start += 256) {
    int count = juce::jmin(256, samples - start);
    juce::AudioBuffer<float> b(2, count);
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < count; ++i)
        b.setSample(c, i,
                    frequency > 0
                        ? amplitude * std::sin(float(
                                          juce::MathConstants<double>::twoPi *
                                          frequency * (start + i) / 48000))
                        : (start + i == 0 ? amplitude : 0));
    p->processBlock(b, midi);
    for (int i = 0; i < count; ++i) {
      float x = b.getSample(channel, i);
      if (!std::isfinite(x))
        throw std::runtime_error("Nonfinite rack output");
      out.push_back(x);
    }
  }
  return out;
}
double energy(const std::vector<float> &x, int begin = 24000) {
  double e = 0;
  for (size_t i = size_t(juce::jmin(begin, int(x.size()))); i < x.size(); ++i)
    e += x[i] * x[i];
  return e / juce::jmax(1, int(x.size()) - begin);
}
double diff(const std::vector<float> &a, const std::vector<float> &b) {
  double e = 0;
  for (size_t i = 12000; i < a.size(); ++i)
    e += std::pow(a[i] - b[i], 2);
  return std::sqrt(e / double(a.size() - 12000));
}
} // namespace
int main() {
  try {
    auto p = std::make_unique<M7GProcessor>();
    juce::MidiBuffer midi;
    std::set<juce::String> names;
    std::set<std::array<float, Count>> unique;
    for (int preset = 0; preset < 50; ++preset) {
      p->setCurrentProgram(preset);
      names.insert(p->getProgramName(preset));
      unique.insert(presetValues(preset));
      p->prepareToPlay(48000, 256);
      double e = 0;
      for (int k = 0; k < 100; ++k) {
        juce::AudioBuffer<float> b(2, 256);
        for (int c = 0; c < 2; ++c)
          for (int i = 0; i < 256; ++i)
            b.setSample(c, i,
                        .18f *
                            std::sin(float(juce::MathConstants<double>::twoPi *
                                           223 * (k * 256 + i) / 48000)));
        p->processBlock(b, midi);
        for (int c = 0; c < 2; ++c)
          for (int i = 0; i < 256; ++i) {
            float x = b.getSample(c, i);
            if (!std::isfinite(x) || std::abs(x) > .892f)
              return 1;
            e += x * x;
          }
      }
      if (e < .01)
        return 2;
    }
    if (names.size() != 50 || unique.size() != 50)
      return 3;
    std::cout << "Presets: 50 distinct names/settings, all rendered with "
                 "finite non-silent output\n";
    auto base = render([](auto &) {});
    auto warm = render(
        [](auto &x) {
          x.setValue(WarmOn, 1);
          x.setValue(WarmDrive, 100);
          x.setValue(WarmBias, 90);
          x.setValue(WarmTone, 3);
        },
        220, .6f);
    auto loudBase = render([](auto &) {}, 220, .6f);
    if (diff(warm, loudBase) < .01)
      return 4;
    auto quiet = render([](auto &) {}, 1000, .0001f);
    auto gated = render(
        [](auto &x) {
          x.setValue(GateOn, 1);
          x.setValue(GateThreshold, -40);
          x.setValue(GateRange, 30);
        },
        1000, .0001f);
    if (energy(gated) > energy(quiet) * .1)
      return 5;
    auto hiss = render([](auto &) {}, 8000, .1f);
    auto deess = render(
        [](auto &x) {
          x.setValue(DeessOn, 1);
          x.setValue(DeessAmount, 95);
          x.setValue(DeessThreshold, -45);
        },
        8000, .1f);
    if (energy(deess) > energy(hiss) * .75)
      return 6;
    auto eqBase = render([](auto &) {}, 3200);
    auto eq = render(
        [](auto &x) {
          x.setValue(EQOn, 1);
          x.setValue(EQGain, 8);
          x.setValue(EQFreq, 3200);
        },
        3200);
    if (energy(eq) < energy(eqBase) * 1.4)
      return 7;
    auto airBase = render([](auto &) {}, 11000, .03f);
    auto airy = render(
        [](auto &x) {
          x.setValue(AirOn, 1);
          x.setValue(AirAmount, 100);
          x.setValue(AirGain, 6);
          x.setValue(AirFilter, 20000);
        },
        11000, .03f);
    if (energy(airy) < energy(airBase) * 2)
      return 8;
    auto comp = render(
        [](auto &x) {
          x.setValue(CompOn, 1);
          x.setValue(CompReduction, 90);
          x.setValue(CompRatio, 8);
          x.setValue(CompGain, 0);
        },
        220, .6f);
    if (energy(comp) > energy(loudBase) * .5)
      return 9;
    auto limited = render([](auto &x) { x.setValue(LimitOn, 1); }, 220, 1.7f);
    for (float x : limited)
      if (std::abs(x) > .892f)
        return 10;
    auto verb = render(
        [](auto &x) {
          x.setValue(ReverbOn, 1);
          x.setValue(ReverbMix, 40);
          x.setValue(ReverbDuck, 0);
          x.setValue(ReverbDecay, 2.5f);
          x.setValue(ReverbPredelay, 0);
        },
        0, .8f);
    if (energy(verb, 12000) < 1e-10)
      return 11;
    auto echo = render(
        [](auto &x) {
          x.setValue(DelayOn, 1);
          x.setValue(DelayMix, 40);
          x.setValue(DelayDuck, 0);
          x.setValue(DelayFeedback, 40);
        },
        0, .8f);
    if (energy(echo, 12000) < 1e-8)
      return 12;
    auto spreadLeft = render(
        [](auto &x) {
          x.setValue(StereoOn, 1);
          x.setValue(Spread, 80);
          x.setValue(Width, 125);
        },
        220, .15f, 48000, 0);
    auto spreadRight = render(
        [](auto &x) {
          x.setValue(StereoOn, 1);
          x.setValue(Spread, 80);
          x.setValue(Width, 125);
        },
        220, .15f, 48000, 1);
    double monoError = 0;
    for (size_t i = 12000; i < base.size(); ++i)
      monoError +=
          std::pow((spreadLeft[i] + spreadRight[i]) * .5f - base[i], 2);
    if (diff(spreadLeft, spreadRight) < .001 || monoError > 1e-5)
      return 13;
    auto mod = render([](auto &x) {
      x.setValue(ModOn, 1);
      x.setValue(ModMix, 75);
      x.setValue(ModDepth, 70);
      x.setValue(ModRate, 1.3f);
    });
    if (diff(base, mod) < .005)
      return 14;
    auto pitch = render(
        [](auto &x) {
          x.setValue(TuneOn, 1);
          x.setValue(TuneAmount, 100);
          x.setValue(TuneSpeed, 5);
          x.setValue(PitchScale, 1);
        },
        223, .15f, 96000);
    double best = 0, bestHz = 0;
    for (double f = 218; f < 225; f += .1) {
      double re = 0, im = 0;
      for (size_t i = 48000; i < pitch.size(); ++i) {
        double a = juce::MathConstants<double>::twoPi * f * double(i) / 48000;
        re += pitch[i] * std::cos(a);
        im += pitch[i] * std::sin(a);
      }
      double power = re * re + im * im;
      if (power > best) {
        best = power;
        bestHz = f;
      }
    }
    std::cout << "Pitch: 223 Hz -> " << bestHz << " Hz\n";
    if (bestHz > 222.2 || bestHz < 218.5)
      return 15;
    // Ducking is measured independently of dry audio and feedback state.
    auto wetEnergy = [](bool reverb, float duck) {
      SpaceProcessor s;
      s.prepare(48000);
      auto v = defaults();
      v[ReverbOn] = reverb ? 1.0f : 0.0f;
      v[DelayOn] = reverb ? 0.0f : 1.0f;
      v[ReverbMix] = v[DelayMix] = 30;
      v[ReverbDuck] = v[DelayDuck] = duck;
      s.setControls(v, 120);
      double e = 0;
      for (int i = 0; i < 96000; ++i) {
        float dry = .3f * std::sin(float(juce::MathConstants<double>::twoPi *
                                         220 * i / 48000));
        float x[2]{dry, dry};
        s.process(x, 2);
        if (i > 72000)
          e += std::pow(x[0] - dry, 2);
      }
      return e;
    };
    for (bool verbMode : {true, false}) {
      double normal = wetEnergy(verbMode, 0), ducked = wetEnergy(verbMode, 85);
      double db = 10 * std::log10(ducked / normal);
      std::cout << (verbMode ? "Reverb" : "Delay") << " ducking: " << db
                << " dB\n";
      if (db > -8)
        return 16;
    }
    SpaceProcessor tailSpace;
    tailSpace.prepare(48000);
    auto tailSettings = defaults();
    tailSettings[ReverbDuck] = tailSettings[DelayDuck] = 100;
    tailSettings[ReverbDecay] = 3;
    tailSettings[DelayFeedback] = 55;
    tailSpace.setControls(tailSettings, 120);
    for (int i = 0; i < 48000; ++i) {
      float x =
          .3f *
          std::sin(float(juce::MathConstants<double>::twoPi * 220 * i / 48000));
      float f[2]{x, x};
      tailSpace.process(f, 2);
    }
    if (tailSpace.reverbDucking() < 18)
      return 21;
    double returningTail = 0;
    for (int i = 0; i < 96000; ++i) {
      float f[2]{};
      tailSpace.process(f, 2);
      if (i > 92000)
        returningTail += f[0] * f[0];
    }
    if (tailSpace.reverbDucking() > .1f || tailSpace.delayDucking() > .1f ||
        returningTail < 1e-10)
      return 22;
    std::cout << "Ducking release: wet tails return after the voice stops\n";
    // Analysis runs on actual audio, then applies settings on the control
    // thread.
    for (int module = 0; module <= 12; ++module) {
      p->setCurrentProgram(0);
      p->prepareToPlay(48000, 256);
      p->setValue(OutputTrim, -3);
      const Param probes[]{
          GateThreshold, DeessAmount,  WarmDrive,   EQGain,        AirAmount,
          CompReduction, LimitCeiling, ReverbDecay, DelayFeedback, Width,
          ModMix,        TuneAmount,   InputTrim};
      Param probe = probes[module];
      p->setValue(probe, specs()[size_t(probe)].min);
      float previous = p->value(probe);
      p->beginAnalysis(module);
      for (int block = 0; block < 565; ++block) {
        juce::AudioBuffer<float> b(2, 256);
        for (int c = 0; c < 2; ++c)
          for (int i = 0; i < 256; ++i)
            b.setSample(c, i,
                        .1f *
                            std::sin(float(juce::MathConstants<double>::twoPi *
                                           220 * (block * 256 + i) / 48000)));
        p->processBlock(b, midi);
      }
      if (p->analysisState.load() != 2 || !p->applyAnalysis() ||
          p->analysisState.load() != 0 || p->value(OutputTrim) != -3 ||
          std::abs(p->value(probe) - previous) < .01f)
        return 17;
    }
    p->setCurrentProgram(3);
    p->setValue(WarmDrive, 82);
    p->selectCompare(1);
    p->setValue(WarmDrive, 19);
    p->selectCompare(0);
    if (std::abs(p->value(WarmDrive) - 82) > .1)
      return 18;
    juce::MemoryBlock state;
    p->getStateInformation(state);
    auto restored = std::make_unique<M7GProcessor>();
    restored->setStateInformation(state.getData(), int(state.getSize()));
    restored->selectCompare(1);
    if (std::abs(restored->value(WarmDrive) - 19) > .1)
      return 19;
    std::cout << "PASS: 12 modules, 50 presets, reverb/delay ducking, 13 "
                 "analysis paths, A/B state persistence\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 20;
  }
}
