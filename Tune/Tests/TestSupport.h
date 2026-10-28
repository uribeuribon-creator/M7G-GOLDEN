#pragma once
#include "../Source/PluginProcessor.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>
inline void requireTune(bool ok, const char *reason) {
  if (!ok)
    throw std::runtime_error(reason);
}
inline float vowel(double t, double f) {
  float x = 0;
  for (int h = 1; h <= 24; ++h) {
    double hz = h * f;
    double env = .1 + 1.1 * std::exp(-std::pow((hz - 800) / 300, 2)) +
                 .7 * std::exp(-std::pow((hz - 2400) / 450, 2));
    x += float(std::sin(6.28318530718 * h * f * t) * env / h);
  }
  return x * .22f;
}
struct Render {
  std::vector<float> left, right;
  uint64_t hash = 1469598103934665603ull;
};
inline Render renderTune(tune::Values p, int quality = 0, double rate = 48000,
                         double seconds = 2.3, double f = 224.7) {
  auto e = std::make_unique<tune::Engine>();
  e->prepare(rate, quality);
  e->set(p, true);
  Render r;
  int total = int(seconds * rate);
  r.left.reserve(size_t(total));
  r.right.reserve(size_t(total));
  for (int i = 0; i < total; ++i) {
    float env = std::min(1.f, float(i) / (float(rate) * .015f));
    float x = vowel(i / rate, f) * env;
    auto out = e->process(x, x, nullptr);
    requireTune(std::isfinite(out[0]) && std::abs(out[0]) <= 1.001f &&
                    std::isfinite(out[1]) && std::abs(out[1]) <= 1.001f,
                "Unbounded/nonfinite output");
    r.left.push_back(out[0]);
    r.right.push_back(out[1]);
    r.hash ^= uint32_t(int(std::round(out[0] * 1000000)));
    r.hash *= 1099511628211ull;
  }
  return r;
}
inline double amplitude(const std::vector<float> &x, double hz,
                        double rate = 48000, int length = 16000) {
  int start = std::max(0, int(x.size()) - length);
  double re = 0, im = 0;
  int n = int(x.size()) - start;
  for (int i = 0; i < n; ++i) {
    double window = .5 - .5 * std::cos(6.28318530718 * i / std::max(1, n - 1));
    double phase = 6.28318530718 * hz * i / rate;
    re += x[size_t(start + i)] * window * std::cos(phase);
    im += x[size_t(start + i)] * window * std::sin(phase);
  }
  return std::sqrt(re * re + im * im) / std::max(1, n);
}
inline double measuredPitch(const std::vector<float> &x, double expected,
                            double rate = 48000) {
  double best = 0, winner = 0;
  for (int i = -100; i <= 100; ++i) {
    double hz = expected * std::exp2(i * .25 / 1200.);
    double a = amplitude(x, hz, rate, 20000);
    if (a > best) {
      best = a;
      winner = hz;
    }
  }
  return winner;
}
struct TunePlayHead : juce::AudioPlayHead {
  int64_t sample = 0;
  bool playing = true;
  juce::Optional<PositionInfo> getPosition() const override {
    PositionInfo p;
    p.setTimeInSamples(sample);
    p.setIsPlaying(playing);
    return p;
  }
};
