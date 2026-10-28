#pragma once
#include "Parameters.h"
namespace fresh {
class FreshEQ {
public:
  void prepare(double);
  void reset();
  void process(juce::dsp::AudioBlock<float>, const Values &);

private:
  double rate = 48000;
  std::array<Biquad, 8> filters;
  std::array<juce::SmoothedValue<float>, 8> gain;
  int untilUpdate = 0;
};
} // namespace fresh
