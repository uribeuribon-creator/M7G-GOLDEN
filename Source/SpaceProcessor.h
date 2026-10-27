#pragma once
#include "RackParameters.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
// Eight-line orthogonal feedback network and a separately ducked ping-pong
// delay.
class SpaceProcessor {
public:
  void prepare(double sr) {
    rate = sr;
    for (auto &line : lines)
      line.assign(size_t(sr * .25) + 8, 0);
    for (auto &line : echo)
      line.assign(size_t(sr * 4.2) + 8, 0);
    for (auto &line : pre)
      line.assign(size_t(sr * .25) + 8, 0);
    write = echoWrite = preWrite = 0;
    damped.fill(0);
    echoLP.fill(0);
    env = 0;
    revDuckDB = delDuckDB = 0;
    lastSize = -1;
    verbSend.reset(sr, .05);
    delaySend.reset(sr, .05);
    verbSend.setCurrentAndTargetValue(0);
    delaySend.setCurrentAndTargetValue(0);
    setControls(rack::defaults(), 120);
  }
  void setControls(const rack::Values &values, double tempo) {
    p = values;
    bpm = juce::jlimit(30.0, 300.0, tempo);
    verbSend.setTargetValue(p[rack::ReverbOn] > .5f ? p[rack::ReverbMix] * .01f
                                                    : 0);
    delaySend.setTargetValue(p[rack::DelayOn] > .5f ? p[rack::DelayMix] * .01f
                                                    : 0);
    constexpr float times[]{.0233f, .0297f, .0379f, .0437f,
                            .0531f, .0617f, .0713f, .0839f};
    float scale = .75f + p[rack::ReverbSize] * .014f;
    for (size_t k = 0; k < 8; ++k) {
      length[k] = juce::jlimit(2, int(lines[k].size()) - 1,
                               int(rate * times[k] * scale));
      feedback[k] = std::pow(10.0f, -3 * float(length[k]) /
                                        float(rate * p[rack::ReverbDecay]));
    }
    constexpr float beats[]{.25f, .5f, .75f, 1, 1.5f, 2};
    delaySamples =
        juce::jlimit(2, int(echo[0].size()) - 2,
                     int(rate * 60 / bpm *
                         beats[juce::jlimit(0, 5, int(p[rack::DelayNote]))]));
    predelay = juce::jlimit(0, int(pre[0].size()) - 2,
                            int(rate * p[rack::ReverbPredelay] * .001));
    lp = std::exp(-2 * juce::MathConstants<float>::pi * p[rack::DelayTone] /
                  float(rate));
    damp = std::exp(-2 * juce::MathConstants<float>::pi *
                    (6200 - p[rack::ReverbSize] * 25) / float(rate));
    envAttack = std::exp(-1 / float(rate * .005));
    envRelease = std::exp(-1 / float(rate * .18));
  }
  void process(float *x, int channels) {
    float power = 0;
    for (int c = 0; c < channels; ++c)
      power += x[c] * x[c];
    power /= float(channels);
    float a = power > env ? envAttack : envRelease;
    env = a * env + (1 - a) * power;
    float db = juce::Decibels::gainToDecibels(std::sqrt(env + 1e-20f), -120.0f);
    float activity = juce::jlimit(0.0f, 1.0f, (db + 48) / 30);
    revDuckDB = activity * p[rack::ReverbDuck] * .24f;
    delDuckDB = activity * p[rack::DelayDuck] * .24f;
    float vg = verbSend.getNextValue() *
               juce::Decibels::decibelsToGain(-revDuckDB),
          dg = delaySend.getNextValue() *
               juce::Decibels::decibelsToGain(-delDuckDB);
    float dry[2]{x[0], channels == 2 ? x[1] : x[0]};
    int ps = int(pre[0].size());
    for (int c = 0; c < 2; ++c)
      pre[size_t(c)][size_t(preWrite)] = dry[c];
    float excitation = .5f * (pre[0][size_t((preWrite + ps - predelay) % ps)] +
                              pre[1][size_t((preWrite + ps - predelay) % ps)]);
    float taps[8]{}, sum = 0;
    for (size_t k = 0; k < 8; ++k) {
      int idx =
          (write + int(lines[k].size()) - length[k]) % int(lines[k].size());
      float v = lines[k][size_t(idx)];
      damped[k] = damp * damped[k] + (1 - damp) * v;
      taps[k] = damped[k];
      sum += taps[k];
    }
    float vl = 0, vr = 0;
    for (size_t k = 0; k < 8; ++k) {
      float mixed = taps[k] - .25f * sum;
      float sign = (k & 1) ? -1.0f : 1.0f;
      lines[k][size_t(write)] =
          std::tanh(excitation * .22f * sign + feedback[k] * mixed);
      vl += taps[k] * ((k & 2) ? -1.0f : 1.0f);
      vr += taps[k] * ((k & 4) ? -1.0f : 1.0f);
    }
    int es = int(echo[0].size()), left = (echoWrite + es - delaySamples) % es,
        right =
            (echoWrite + es - juce::jmin(es - 2, int(delaySamples * 1.5))) % es;
    float dl = echo[0][size_t(left)], dr = echo[1][size_t(right)];
    float fb = p[rack::DelayFeedback] * .01f;
    for (size_t c = 0; c < 2; ++c) {
      float tap = c == 0 ? dr : dl;
      echoLP[c] = lp * echoLP[c] + (1 - lp) * tap;
      echo[c][size_t(echoWrite)] = dry[c] + fb * echoLP[c];
    }
    float width = p[rack::StereoOn] > .5f ? p[rack::Width] * .01f : 1.0f;
    float mid = (vl + vr) * .5f, side = (vl - vr) * .5f * width;
    if (channels == 1)
      x[0] += vg * mid * .3f + dg * (dl + dr) * .5f;
    else {
      x[0] += vg * (mid + side) * .3f + dg * dl;
      x[1] += vg * (mid - side) * .3f + dg * dr;
    }
    write = (write + 1) % int(lines[0].size());
    echoWrite = (echoWrite + 1) % es;
    preWrite = (preWrite + 1) % ps;
  }
  float reverbDucking() const { return revDuckDB; }
  float delayDucking() const { return delDuckDB; }
  float tailSeconds() const {
    return p[rack::ReverbDecay] + float(60 / bpm * 2) * 5;
  }

private:
  rack::Values p = rack::defaults();
  std::array<std::vector<float>, 8> lines;
  std::array<std::vector<float>, 2> echo, pre;
  std::array<int, 8> length{};
  std::array<float, 8> feedback{}, damped{};
  std::array<float, 2> echoLP{};
  juce::SmoothedValue<float> verbSend, delaySend;
  double rate = 48000, bpm = 120;
  int write = 0, echoWrite = 0, preWrite = 0, delaySamples = 12000,
      predelay = 1200;
  float env = 0, envAttack = 0, envRelease = 0, lp = 0, damp = 0, lastSize = -1,
        revDuckDB = 0, delDuckDB = 0;
};
