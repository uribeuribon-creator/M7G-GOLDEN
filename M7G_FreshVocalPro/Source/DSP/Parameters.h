#pragma once
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
namespace fresh {
enum Param {
  input,
  output,
  comp,
  color,
  focus,
  dynStyle,
  eqLow,
  eqLowMid,
  eqHighMid,
  eqHigh,
  eq160,
  eq350,
  eq1500,
  eq6000,
  eqStyle,
  reverb,
  time,
  delay,
  spaceMode,
  verbType,
  division,
  character,
  charMode,
  harmonics,
  tape,
  width,
  dynOn,
  eqOn,
  spaceOn,
  charOn,
  order,
  bypass,
  count
};
struct Spec {
  const char *id;
  const char *name;
  float low, high, initial;
  int kind;
};
inline const std::array<Spec, count> specs{
    {{"input", "Input Gain dB", -100, 12, 0, 0},
     {"output", "Output Gain dB", -85, 6, 0, 0},
     {"comp", "Compression Amount", 0, 100, 18.9f, 0},
     {"color", "Dynamics Color", 0, 100, 50, 0},
     {"focus", "De-Ess Focus Hz", 2000, 12000, 5000, 0},
     {"dynStyle", "Dynamics Style", 0, 2, 1, 1},
     {"eqLow", "Low 80 Hz dB", -15, 15, -2, 0},
     {"eqLowMid", "Low Mid 700 Hz dB", -15, 15, -1, 0},
     {"eqHighMid", "High Mid 2500 Hz dB", -15, 15, 1, 0},
     {"eqHigh", "High 12000 Hz dB", -15, 15, 6, 0},
     {"eq160", "Expert 160 Hz dB", -15, 15, 0, 0},
     {"eq350", "Expert 350 Hz dB", -15, 15, 0, 0},
     {"eq1500", "Expert 1500 Hz dB", -15, 15, 0, 0},
     {"eq6000", "Expert 6000 Hz dB", -15, 15, 0, 0},
     {"eqStyle", "EQ Style", 0, 3, 3, 1},
     {"reverb", "Reverb Amount", 0, 100, 24.1f, 0},
     {"time", "Space Time", 0, 10, 4.6f, 0},
     {"delay", "Delay Amount", 0, 100, 6, 0},
     {"spaceMode", "Space Size", 0, 2, 1, 1},
     {"verbType", "Reverb Type", 0, 2, 2, 1},
     {"division", "Delay Tempo Division", 0, 2, 2, 1},
     {"character", "Character Amount", 0, 100, 0, 0},
     {"charMode", "Character Model", 0, 2, 0, 1},
     {"harmonics", "Harmonics", 0, 1, 0, 1},
     {"tape", "Tape Drive", 0, 2, 0, 1},
     {"width", "Stereo Width", 0, 1, 1, 1},
     {"dynOn", "Dynamics Enabled", 0, 1, 1, 2},
     {"eqOn", "EQ Enabled", 0, 1, 1, 2},
     {"spaceOn", "Space Enabled", 0, 1, 1, 2},
     {"charOn", "Character Enabled", 0, 1, 1, 2},
     {"order", "Module Order", 0, 23, 1, 1},
     {"bypass", "Global Bypass", 0, 1, 0, 2}}};
inline juce::StringArray choices(Param p) {
  switch (p) {
  case dynStyle:
    return {"Smooth", "Punchy", "Aggressive"};
  case eqStyle:
    return {"Warm", "Neutral", "Bright", "Airy"};
  case spaceMode:
    return {"Intimate", "Studio", "Ambient"};
  case verbType:
    return {"Plate", "Room", "Large"};
  case division:
    return {"1/16", "1/8", "1/4"};
  case charMode:
    return {"Warm", "Aggressive", "Saturated"};
  case harmonics:
    return {"LOW", "HIGH"};
  case tape:
    return {"SUBTLE", "Warm Tape", "Max Saturation"};
  case width:
    return {"NARROW", "WIDE"};
  default:
    break;
  }
  juce::StringArray a;
  for (int i = 0; i < 24; ++i)
    a.add(juce::String(i + 1));
  return a;
}
using Values = std::array<float, count>;
inline Values defaults() {
  Values v{};
  for (int i = 0; i < count; ++i)
    v[size_t(i)] = specs[size_t(i)].initial;
  return v;
}
inline Values sanitise(Values v) {
  for (int i = 0; i < count; ++i) {
    const auto &s = specs[size_t(i)];
    v[size_t(i)] = std::isfinite(v[size_t(i)])
                       ? juce::jlimit(s.low, s.high, v[size_t(i)])
                       : s.initial;
    if (s.kind)
      v[size_t(i)] = std::round(v[size_t(i)]);
  }
  return v;
}
inline const std::array<std::array<int, 4>, 24> &orders() {
  static const auto all = [] {
    std::array<std::array<int, 4>, 24> a{};
    std::array<int, 4> row{{0, 1, 2, 3}};
    int i = 0;
    do {
      a[size_t(i++)] = row;
    } while (std::next_permutation(row.begin(), row.end()));
    return a;
  }();
  return all;
}
struct Mailbox {
  std::array<std::atomic<float>, count> data;
  Mailbox() { store(defaults()); }
  void store(const Values &v) {
    for (int i = 0; i < count; ++i)
      data[size_t(i)].store(v[size_t(i)], std::memory_order_relaxed);
  }
  Values load() const {
    Values v{};
    for (int i = 0; i < count; ++i)
      v[size_t(i)] = data[size_t(i)].load(std::memory_order_relaxed);
    return v;
  }
};
inline float db(float x) {
  return juce::Decibels::gainToDecibels(std::max(1e-8f, x), -160.f);
}
// Coeficientes RBJ en pila: actualizar parametros nunca reserva memoria en
// audio.
class Biquad {
public:
  enum Type { lowpass, highpass, bandpass, peak, lowShelf, highShelf };
  void set(Type type, double sr, double frequency, double q, double gain = 0) {
    double f = juce::jlimit(10., sr * .45, frequency),
           w = 2 * juce::MathConstants<double>::pi * f / sr, c = std::cos(w),
           s = std::sin(w), a = s / (2 * q), A = std::pow(10., gain / 40),
           b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
    if (type == lowpass) {
      b0 = (1 - c) / 2;
      b1 = 1 - c;
      b2 = b0;
      a0 = 1 + a;
      a1 = -2 * c;
      a2 = 1 - a;
    }
    if (type == highpass) {
      b0 = (1 + c) / 2;
      b1 = -(1 + c);
      b2 = b0;
      a0 = 1 + a;
      a1 = -2 * c;
      a2 = 1 - a;
    }
    if (type == bandpass) {
      b0 = a;
      b1 = 0;
      b2 = -a;
      a0 = 1 + a;
      a1 = -2 * c;
      a2 = 1 - a;
    }
    if (type == peak) {
      b0 = 1 + a * A;
      b1 = -2 * c;
      b2 = 1 - a * A;
      a0 = 1 + a / A;
      a1 = -2 * c;
      a2 = 1 - a / A;
    }
    if (type == lowShelf || type == highShelf) {
      double alpha = s / std::sqrt(2.), r = 2 * std::sqrt(A) * alpha;
      if (type == lowShelf) {
        b0 = A * ((A + 1) - (A - 1) * c + r);
        b1 = 2 * A * ((A - 1) - (A + 1) * c);
        b2 = A * ((A + 1) - (A - 1) * c - r);
        a0 = (A + 1) + (A - 1) * c + r;
        a1 = -2 * ((A - 1) + (A + 1) * c);
        a2 = (A + 1) + (A - 1) * c - r;
      } else {
        b0 = A * ((A + 1) + (A - 1) * c + r);
        b1 = -2 * A * ((A - 1) + (A + 1) * c);
        b2 = A * ((A + 1) + (A - 1) * c - r);
        a0 = (A + 1) - (A - 1) * c + r;
        a1 = 2 * ((A - 1) - (A + 1) * c);
        a2 = (A + 1) - (A - 1) * c - r;
      }
    }
    coefficients = {{float(b0 / a0), float(b1 / a0), float(b2 / a0),
                     float(a1 / a0), float(a2 / a0)}};
  }
  float process(int channel, float x) {
    auto &z = state[size_t(channel)];
    auto &a = coefficients;
    float y = a[0] * x + z[0];
    z[0] = a[1] * x - a[3] * y + z[1];
    z[1] = a[2] * x - a[4] * y;
    if (!std::isfinite(y)) {
      z = {};
      return 0;
    }
    return y;
  }
  void reset() {
    for (auto &s : state)
      s = {};
  }

private:
  std::array<float, 5> coefficients{{1, 0, 0, 0, 0}};
  std::array<std::array<float, 2>, 2> state{};
};
class SampleDelay {
public:
  void prepare(int channels, int delay) {
    length = std::max(1, delay + 1);
    latency = delay;
    buffer.setSize(channels, length);
    reset();
  }
  void reset() {
    buffer.clear();
    head = 0;
  }
  void frame(float *x, int channels) {
    for (int c = 0; c < channels; ++c) {
      float y = latency == 0 ? x[c] : buffer.getSample(c, (head + 1) % length);
      buffer.setSample(c, head, x[c]);
      x[c] = y;
    }
    head = (head + 1) % length;
  }

private:
  juce::AudioBuffer<float> buffer;
  int length = 1, latency = 0, head = 0;
};
} // namespace fresh
