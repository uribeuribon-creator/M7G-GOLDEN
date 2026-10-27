#pragma once
#include "RackParameters.h"
#include <vector>
class RackLimiter {
public:
  void prepare(double sr) {
    rate = sr;
    look = juce::jmax(1, int(sr * .001));
    for (auto &d : delay)
      d.assign(size_t(look + 1), 0);
    write = 0;
    gain = 1;
    hold = 0;
  }
  int latency() const { return look; }
  void setControls(const rack::Values &values) {
    p = values;
    ceiling = juce::Decibels::decibelsToGain(p[rack::LimitCeiling]);
    release = std::exp(-1 / float(rate * p[rack::LimitRelease] * .001));
  }
  void process(float *x, int channels) {
    float peak = 0;
    for (int c = 0; c < channels; ++c) {
      delay[size_t(c)][size_t(write)] = x[c];
      peak = juce::jmax(peak, std::abs(x[c]));
    }
    int read = (write + 1) % (look + 1);
    float delayedPeak = 0;
    for (int c = 0; c < channels; ++c)
      delayedPeak =
          juce::jmax(delayedPeak, std::abs(delay[size_t(c)][size_t(read)]));
    float detector = p[rack::LimitLookahead] > .5f
                         ? juce::jmax(peak, delayedPeak)
                         : delayedPeak;
    float target = detector > ceiling ? ceiling / detector : 1;
    if (target < gain) {
      float attack =
          p[rack::LimitFast] > .5f ? 0 : std::exp(-1 / float(rate * .0002));
      gain = attack * gain + (1 - attack) * target;
      hold = look;
    } else if (hold > 0)
      --hold;
    else
      gain = release * gain + (1 - release) * target;
    for (int c = 0; c < channels; ++c) {
      float d = delay[size_t(c)][size_t(read)];
      x[c] = p[rack::LimitOn] > .5f ? juce::jlimit(-ceiling, ceiling, d * gain)
                                    : d;
    }
    write = (write + 1) % (look + 1);
  }
  float reduction() const {
    return -juce::Decibels::gainToDecibels(juce::jmax(1e-6f, gain));
  }

private:
  rack::Values p = rack::defaults();
  std::array<std::vector<float>, 2> delay;
  double rate = 48000;
  int look = 48, write = 0, hold = 0;
  float gain = 1, ceiling = .891f, release = .99f;
};
