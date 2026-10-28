#pragma once
#include "Parameters.h"
namespace fresh {
class MultiBandComp {
public:
  void prepare(double, int, int);
  void reset();
  void process(juce::dsp::AudioBlock<float>, float, float, int);
  int latency() const { return delay; }
  float reduction() const { return gr; }

private:
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
  juce::dsp::LinkwitzRileyFilter<float> splitLow, splitHigh, lowPhase;
  std::array<float, 3> envelope{}, gain{{1, 1, 1}};
  double rate = 96000;
  int delay = 0;
  float gr = 0;
  juce::SmoothedValue<float> mix, color;
};
} // namespace fresh
