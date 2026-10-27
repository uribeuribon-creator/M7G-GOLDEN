#pragma once
#include "RackParameters.h"
#include <juce_dsp/juce_dsp.h>
class VocalFinisher {
public:
  void prepare(double sr, int) {
    rate = sr;
    fullEnv = essEnv = bodyEnv = harshEnv = essReduction = 0;
    gateGain = 1;
    lastFreq = -1;
    for (auto *bank : {&hp, &ess, &body, &harsh})
      for (auto &f : *bank)
        f.reset();
    setControls(rack::defaults());
  }
  void setControls(const rack::Values &v) {
    p = v;
    attack = std::exp(-1 / float(rate * v[rack::GateAttack] * .001));
    release = std::exp(-1 / float(rate * v[rack::GateRelease] * .001));
    if (lastFreq != v[rack::DeessFreq]) {
      lastFreq = v[rack::DeessFreq];
      auto a = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(rate, 60,
                                                                      .707f);
      auto b = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(
          rate, juce::jmin(double(lastFreq), rate * .4));
      auto c = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(rate, 420,
                                                                      1.4f);
      auto d = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(
          rate, juce::jmin(2900.0, rate * .3), 2.0f);
      for (size_t k = 0; k < 2; ++k) {
        *hp[k].coefficients = a;
        *ess[k].coefficients = b;
        *body[k].coefficients = c;
        *harsh[k].coefficients = d;
      }
    }
  }
  void cleanFrame(float *frame, int channels) {
    float sum = 0, ss = 0, bb = 0, hh = 0;
    float highs[2]{}, bodies[2]{}, edges[2]{};
    for (int c = 0; c < channels; ++c) {
      auto k = static_cast<size_t>(c);
      float filtered = hp[k].processSample(frame[c]);
      if (p[rack::GateOn] > .5f)
        frame[c] = filtered;
      highs[c] = ess[k].processSample(frame[c]);
      bodies[c] = body[k].processSample(frame[c]);
      edges[c] = harsh[k].processSample(frame[c]);
      sum += frame[c] * frame[c];
      ss += highs[c] * highs[c];
      bb += bodies[c] * bodies[c];
      hh += edges[c] * edges[c];
    }
    auto env = [&](float &e, float x) {
      x /= float(channels);
      float a = std::exp(-1 / float(rate * (x > e ? .002 : .09)));
      e = a * e + (1 - a) * x;
    };
    env(fullEnv, sum);
    env(essEnv, ss);
    env(bodyEnv, bb);
    env(harshEnv, hh);
    const float level =
        juce::Decibels::gainToDecibels(std::sqrt(fullEnv + 1e-20f), -120.0f);
    float below = juce::jmax(0.0f, p[rack::GateThreshold] - level);
    float target = juce::Decibels::decibelsToGain(
        -juce::jmin(p[rack::GateRange], below * 1.5f));
    float a = target > gateGain ? attack : release;
    gateGain = a * gateGain + (1 - a) * target;
    const float dominance = std::sqrt(essEnv / (fullEnv + 1e-15f));
    const float bandDB =
        juce::Decibels::gainToDecibels(std::sqrt(essEnv + 1e-20f), -120.0f);
    const float activation =
        juce::jlimit(0.0f, 1.0f, (bandDB - p[rack::DeessThreshold] + 12) / 12);
    const float cut = 12 * (p[rack::DeessAmount] * .01f) *
                      juce::jlimit(0.0f, 1.0f, (dominance - .22f) / .58f) *
                      activation;
    const float remove =
        p[rack::DeessOn] > .5f ? 1 - juce::Decibels::decibelsToGain(-cut) : 0;
    essReduction = p[rack::DeessOn] > .5f ? cut : 0;
    const float mud =
        p[rack::EQOn] > .5f
            ? juce::jlimit(0.0f, .32f,
                           (bodyEnv / (fullEnv + 1e-15f) - .28f) * .8f)
            : 0;
    const float edge =
        p[rack::EQOn] > .5f
            ? juce::jlimit(0.0f, .32f,
                           (harshEnv / (fullEnv + 1e-15f) - .22f) * .7f)
            : 0;
    for (int c = 0; c < channels; ++c) {
      float removed = remove * highs[c];
      float x = frame[c] - removed - mud * bodies[c] - edge * edges[c];
      if (p[rack::GateOn] > .5f)
        x *= gateGain;
      frame[c] = p[rack::DeessListen] > .5f ? removed : x;
    }
  }
  float getDeEssReduction() const { return essReduction; }
  float getGateReduction() const {
    return -juce::Decibels::gainToDecibels(juce::jmax(.0001f, gateGain));
  }

private:
  using Filter = juce::dsp::IIR::Filter<float>;
  std::array<Filter, 2> hp, ess, body, harsh;
  rack::Values p = rack::defaults();
  double rate = 48000;
  float fullEnv = 0, essEnv = 0, bodyEnv = 0, harshEnv = 0, gateGain = 1,
        essReduction = 0, attack = 0, release = 0, lastFreq = -1;
};
