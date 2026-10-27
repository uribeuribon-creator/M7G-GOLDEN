#pragma once
#include "RackParameters.h"
#include <juce_audio_basics/juce_audio_basics.h>
class V10Align {
public:
  void prepare(double sr) {
    rate = sr;
    hopSize = juce::jmax(1, int(sr / 200));
    for (auto &b : audio)
      b.assign(size_t(sr * .25) + 8, 0);
    mainEnvelope.fill(0);
    referenceEnvelope.fill(0);
    write = hop = envWrite = frames = 0;
    mainPower = referencePower = 0;
    lastMain = lastReference = 0;
    offset = confidence = 0;
    tapA = tapB = float(sr * .08);
    fade = 1;
  }
  int latency() const { return int(rate * .08); }
  void setControls(const rack::Values &values) {
    p = values;
    fadeStep = 1 / float(rate * p[rack::AlignSpeed] * .001);
  }
  void process(float *frame, const float *reference, int channels,
               bool active) {
    float main = (frame[0] + frame[channels - 1]) * .5f,
          ref = (reference[0] + reference[channels - 1]) * .5f;
    mainPower += main * main;
    referencePower += ref * ref;
    for (int c = 0; c < channels; ++c)
      audio[size_t(c)][size_t(write)] = frame[c];
    if (++hop >= hopSize) {
      hop = 0;
      float m = juce::Decibels::gainToDecibels(
                std::sqrt(float(mainPower / hopSize)), -90.0f),
            r = juce::Decibels::gainToDecibels(
                std::sqrt(float(referencePower / hopSize)), -90.0f);
      mainEnvelope[size_t(envWrite)] =
          .25f * (m + 90) + juce::jmax(-20.0f, m - lastMain);
      referenceEnvelope[size_t(envWrite)] =
          .25f * (r + 90) + juce::jmax(-20.0f, r - lastReference);
      lastMain = m;
      lastReference = r;
      mainPower = referencePower = 0;
      envWrite = (envWrite + 1) % 256;
      frames = juce::jmin(256, frames + 1);
      if (active && frames > 120 && r > -65)
        analyse();
    }
    if (active) {
      float target = float(latency()) -
                     offset * float(rate) * .001f * p[rack::AlignAmount] * .01f;
      if (fade >= 1 && std::abs(target - tapB) > 2) {
        tapA = tapB;
        tapB = target;
        fade = 0;
      }
      fade = juce::jmin(1.0f, fade + fadeStep);
      float weight =
          .5f - .5f * std::cos(juce::MathConstants<float>::pi * fade);
      for (int c = 0; c < channels; ++c) {
        float dry = read(c, float(latency())),
              wet = read(c, tapA) * (1 - weight) + read(c, tapB) * weight;
        frame[c] = dry + p[rack::AlignParallel] * .01f * (wet - dry);
      }
    }
    write = (write + 1) % int(audio[0].size());
  }
  float offsetMs() const { return offset; }
  float certainty() const { return confidence; }

private:
  float read(int c, float delay) const {
    int n = int(audio[0].size());
    float at = write - delay;
    while (at < 0)
      at += n;
    int a = int(at) % n;
    float f = at - std::floor(at);
    return audio[size_t(c)][size_t(a)] * (1 - f) +
           audio[size_t(c)][size_t((a + 1) % n)] * f;
  }
  void analyse() {
    int range = juce::jlimit(1, 12, int(p[rack::AlignRange] / 5));
    float best = .55f;
    int selected = 0;
    for (int lag = -range; lag <= range; ++lag) {
      double a = 0, b = 0, aa = 0, bb = 0, ab = 0;
      for (int t = 16; t < 112; ++t) {
        float x = mainEnvelope[size_t((envWrite - t + 256) % 256)],
              y = referenceEnvelope[size_t((envWrite - t - lag + 512) % 256)];
        a += x;
        b += y;
        aa += x * x;
        bb += y * y;
        ab += x * y;
      }
      double cov = ab - a * b / 96,
             variance = (aa - a * a / 96) * (bb - b * b / 96);
      float corr = float(cov / std::sqrt(juce::jmax(1e-12, variance)));
      float penalty = .0005f * std::abs(float(lag) * 5 - offset);
      if (corr - penalty > best) {
        best = corr - penalty;
        selected = lag;
      }
    }
    confidence = best > .55f ? juce::jlimit(0.0f, 1.0f, best) : 0;
    if (confidence > .55f)
      offset = selected * float(hopSize) * 1000 / float(rate);
  }
  rack::Values p = rack::defaults();
  double rate = 48000, mainPower = 0, referencePower = 0;
  int hopSize = 240, write = 0, hop = 0, envWrite = 0, frames = 0;
  float lastMain = 0, lastReference = 0, offset = 0, confidence = 0,
        tapA = 3840, tapB = 3840, fade = 1, fadeStep = 1;
  std::array<std::vector<float>, 2> audio;
  std::array<float, 256> mainEnvelope{}, referenceEnvelope{};
};
