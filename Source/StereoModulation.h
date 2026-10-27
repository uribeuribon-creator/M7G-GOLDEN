#pragma once
#include "RackParameters.h"
#include <vector>
class StereoModulation {
public:
  void prepare(double sr) {
    rate = sr;
    for (auto &d : mod)
      d.assign(size_t(sr * .08) + 8, 0);
    for (auto &d : spread)
      d.assign(size_t(sr * .025) + 8, 0);
    write = spreadWrite = 0;
    phase = 0;
    low.fill(0);
  }
  void setControls(const rack::Values &values) {
    p = values;
    pole = std::exp(-2 * juce::MathConstants<float>::pi * p[rack::StereoLow] /
                    float(rate));
  }
  void process(float *frame, int channels) {
    float dry[2]{frame[0], channels == 2 ? frame[1] : frame[0]};
    int n = int(mod[0].size());
    float rateHz = p[rack::ModRate];
    phase += rateHz / float(rate);
    phase -= std::floor(phase);
    for (int c = 0; c < channels; ++c) {
      size_t k = size_t(c);
      int type = int(p[rack::ModType]);
      float base = type == 1 ? .0025f : (type == 2 ? .008f : .014f);
      float depth = (type == 1 ? .0018f : .006f) * p[rack::ModDepth] * .01f;
      float lfo = std::sin(juce::MathConstants<float>::twoPi *
                           (phase + (type == 2 ? 0.0f : float(c) * .23f)));
      float delay = float(rate) * (base + depth * lfo);
      float at = float(write) - delay;
      while (at < 0)
        at += float(n);
      int i = int(at) % n;
      float f = at - std::floor(at);
      float tap = mod[k][size_t(i)] * (1 - f) + mod[k][size_t((i + 1) % n)] * f;
      mod[k][size_t(write)] = dry[c] + (type == 1 ? .2f * tap : 0);
      float mix = p[rack::ModOn] > .5f ? p[rack::ModMix] * .01f : 0;
      frame[c] = dry[c] + mix * (tap - dry[c]);
    }
    write = (write + 1) % n;
    if (channels == 2) {
      int sn = int(spread[0].size());
      float mid = (frame[0] + frame[1]) * .5f;
      spread[0][size_t(spreadWrite)] = mid;
      float delayed =
          spread[0][size_t((spreadWrite + sn - int(rate * .011)) % sn)];
      low[0] = pole * low[0] + (1 - pole) * mid;
      float width = p[rack::StereoOn] > .5f ? p[rack::Width] * .01f : 1;
      float side = (frame[0] - frame[1]) * .5f * width;
      if (p[rack::StereoOn] > .5f)
        side += (delayed - mid) * p[rack::Spread] * .0035f;
      // Keep low-frequency side content mono and preserve a stable centre.
      low[1] = pole * low[1] + (1 - pole) * side;
      if (p[rack::StereoOn] > .5f)
        side -= low[1];
      float pan = p[rack::StereoOn] > .5f ? p[rack::Pan] * .01f : 0;
      float originalL = frame[0], originalR = frame[1];
      frame[0] = (mid + side) * std::sqrt(1 - juce::jmax(0.0f, pan));
      frame[1] = (mid - side) * std::sqrt(1 + juce::jmin(0.0f, pan));
      frame[0] =
          originalL + p[rack::StereoParallel] * .01f * (frame[0] - originalL);
      frame[1] =
          originalR + p[rack::StereoParallel] * .01f * (frame[1] - originalR);
      spreadWrite = (spreadWrite + 1) % sn;
    }
  }
  float lfoPhase() const { return phase; }

private:
  rack::Values p = rack::defaults();
  std::array<std::vector<float>, 2> mod, spread;
  std::array<float, 2> low{};
  double rate = 48000;
  int write = 0, spreadWrite = 0;
  float phase = 0, pole = 0;
};
