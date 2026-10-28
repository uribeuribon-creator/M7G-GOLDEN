#pragma once
#include "Parameters.h"
namespace fresh {
class FreshSpace {
public:
  void prepare(double, int);
  void reset();
  void process(juce::dsp::AudioBlock<float>, const Values &, double);
  static double delaySeconds(int division, double bpm) {
    return 60. / juce::jlimit(30., 300., bpm) /
           (division == 0   ? 4
            : division == 1 ? 2
                            : 1);
  }
  float duckReduction() const { return duckDb; }

private:
  struct Ring {
    std::vector<float> data;
    int head = 0;
    float damp = 0;
    void prepare(int n) {
      data.assign(size_t(n), 0);
      head = 0;
      damp = 0;
    }
    void clear() {
      std::fill(data.begin(), data.end(), 0);
      head = 0;
      damp = 0;
    }
    float read(double delay) const {
      double at = head - juce::jlimit(1., double(data.size() - 2), delay);
      while (at < 0)
        at += data.size();
      int i = int(at);
      float f = float(at - i);
      return data[size_t(i)] * (1 - f) +
             data[size_t((i + 1) % int(data.size()))] * f;
    }
    void push(float x) {
      data[size_t(head)] = std::isfinite(x) ? x : 0;
      head = (head + 1) % int(data.size());
    }
  };
  double rate = 48000, phase = 0;
  int channels = 2;
  std::array<std::array<Ring, 8>, 2> combs;
  std::array<std::array<Ring, 4>, 2> diffusion;
  std::array<Ring, 2> pre, echo;
  Biquad verbLow, verbHigh, echoLow, echoHigh;
  juce::SmoothedValue<float> verbMix, echoMix, size, echoSamples;
  float envelope = 0, duck = 1, duckDb = 0;
};
} // namespace fresh
