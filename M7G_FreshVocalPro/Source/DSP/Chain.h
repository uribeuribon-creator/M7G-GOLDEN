#pragma once
#include "CharacterSat.h"
#include "DeEsser.h"
#include "FreshEQ.h"
#include "FreshSpace.h"
#include "MultiBandComp.h"
namespace fresh {
struct Meters {
  std::atomic<float> in{0}, out{0}, compression{0}, deEss{0}, duck{0},
      limiting{0};
};
class Chain {
public:
  void prepare(double, int, int);
  void reset();
  void process(juce::AudioBuffer<float> &, const Values &, double,
               bool hostBypass = false);
  void process(juce::dsp::AudioBlock<float>, const Values &, double,
               bool hostBypass = false);
  int latency() const { return totalLatency; }
  Meters meters;

private:
  DeEsser deEsser;
  MultiBandComp compressor;
  FreshEQ eq;
  CharacterSat saturation;
  FreshSpace space;
  double rate = 48000;
  int maximum = 512, channels = 2, totalLatency = 0;
  SampleDelay limiterLine, globalLine;
  std::array<SampleDelay, 4> bypassLines;
  std::array<juce::SmoothedValue<float>, 4> enabled;
  juce::SmoothedValue<float> inputGain, outputGain, globalWet;
  juce::AudioBuffer<float> dry, globalDry;
  float limiterGain = 1;
  int limiterHold = 0, limiterLookahead = 48;
  std::array<bool, 4> wasActive{{true, true, true, true}};
  bool wasGlobalActive = true;
};
} // namespace fresh
