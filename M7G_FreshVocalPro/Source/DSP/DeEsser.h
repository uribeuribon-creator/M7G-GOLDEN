#pragma once
#include "Parameters.h"
namespace fresh {
class DeEsser {
public:
  void prepare(double, int);
  void reset();
  void process(juce::dsp::AudioBlock<float>, float);
  int latency() const { return lookahead; }
  float reduction() const { return gr; }

private:
  double rate = 48000;
  int lookahead = 96, hold = 0;
  Biquad high, low, upper, band;
  SampleDelay line;
  float rmsHigh = 0, rmsLow = 0, peak = 0, gain = 1, gr = 0, lastFocus = -1;
};
} // namespace fresh
