#pragma once
#include "Parameters.h"
namespace fresh {
class CharacterSat {
public:
  void prepare(double, int, int);
  void reset();
  void process(juce::dsp::AudioBlock<float>, const Values &);
  int latency() const { return delay; }
  static float transfer(float, float, int, int, int);

private:
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
  Biquad dc;
  int delay = 0;
  juce::SmoothedValue<float> amount;
};
} // namespace fresh
