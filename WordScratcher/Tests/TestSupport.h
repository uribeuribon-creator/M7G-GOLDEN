#pragma once
#include "../Source/PluginProcessor.h"
#include <chrono>
#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>
#include <stdexcept>
#include <vector>
inline void requireScratch(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
inline float scratchVoice(double t) {
  double local = -1;
  if (t >= .12 && t < .87)
    local = t - .12;
  else if (t >= 1.70 && t < 2.12)
    local = t - 1.70;
  else
    return 0;
  double duration = t < 1 ? .75 : .42;
  float envelope = float(std::min(1., std::min(local, duration - local) / .01));
  double f = 177.3;
  float x = 0;
  for (int h = 1; h <= 22; ++h) {
    double hz = f * h, formant = .15 +
                                 .8 * std::exp(-std::pow((hz - 760) / 300, 2)) +
                                 .5 * std::exp(-std::pow((hz - 2400) / 550, 2));
    x += float(std::sin(6.28318530718 * f * h * t) * formant / h);
  }
  return envelope * x * .28f;
}
struct ScratchRender {
  std::vector<float> audio;
  double effectEnergy = 0, difference = 0;
  uint64_t hash = 1469598103934665603ull;
  int captures = 0, negative = 0, positive = 0, underflows = 0;
};
inline ScratchRender renderScratch(const scratch::Values &p, double sr = 48000,
                                   double seconds = 3.8) {
  auto e = std::make_unique<scratch::VinylEngine>();
  e->prepare(sr);
  e->controls(p);
  ScratchRender out;
  int n = int(sr * seconds);
  out.audio.reserve(size_t(n));
  for (int i = 0; i < n; ++i) {
    float x = scratchVoice(i / sr);
    auto y = e->process(x, x * .94f);
    requireScratch(std::isfinite(y[0]) && std::abs(y[0]) <= 1.001f,
                   "Nonfinite or excessive scratch output");
    float dry =
        i >= scratch::VinylEngine::latencyFor(sr)
            ? scratchVoice((i - scratch::VinylEngine::latencyFor(sr)) / sr)
            : 0;
    out.difference += std::pow(y[0] - dry, 2);
    if (i > int(sr * 1.05) && i < int(sr * 1.58))
      out.effectEnergy += y[0] * y[0];
    if (e->outputRate < -.15f)
      ++out.negative;
    if (e->outputRate > .15f)
      ++out.positive;
    out.audio.push_back(y[0]);
    out.hash ^= uint32_t(int(std::round(y[0] * 1000000)));
    out.hash *= 1099511628211ull;
  }
  out.captures = e->serial;
  out.underflows = e->digital.underflows;
  return out;
}
inline double toneAmplitude(const std::vector<float> &x, double hz,
                            double sr = 48000, int window = 16000) {
  int begin = std::max(0, int(x.size()) - window), n = int(x.size()) - begin;
  double a = 0, b = 0;
  for (int i = 0; i < n; ++i) {
    double w = .5 - .5 * std::cos(6.28318530718 * i / std::max(1, n - 1)),
           phase = 6.28318530718 * hz * i / sr;
    a += x[size_t(begin + i)] * w * std::cos(phase);
    b += x[size_t(begin + i)] * w * std::sin(phase);
  }
  return std::sqrt(a * a + b * b) / std::max(1, n);
}
inline double scratchPitch(const std::vector<float> &x, double expected) {
  double best = 0, winner = 0;
  for (int i = -80; i <= 80; ++i) {
    double hz = expected * std::exp2(i * .5 / 1200.);
    double a = toneAmplitude(x, hz);
    if (a > best) {
      best = a;
      winner = hz;
    }
  }
  return winner;
}
struct ScratchPlayHead : juce::AudioPlayHead {
  double bpm = 120;
  int64_t sample = 0;
  bool playing = true;
  juce::Optional<PositionInfo> getPosition() const override {
    PositionInfo p;
    p.setBpm(bpm);
    p.setTimeInSamples(sample);
    p.setIsPlaying(playing);
    return p;
  }
};
