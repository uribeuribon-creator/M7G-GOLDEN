#pragma once
#include "Capture.h"
#include <juce_core/juce_core.h>
namespace transition {
class Space {
public:
  void prepare(double sr) {
    rate = sr;
    for (int i = 0; i < 4; ++i) {
      delays[size_t(i)] = float(sr) * seconds[size_t(i)];
      line[size_t(i)].prepare(int(delays[size_t(i)] + sr * .002) + 8);
    }
    reset();
  }
  void reset() {
    for (auto &x : line)
      x.reset();
    for (auto &x : low)
      x.fill(0);
    phase = 0;
  }
  void configure(float decay, float cutoff) {
    for (int i = 0; i < 4; ++i)
      feedback[size_t(i)] =
          std::pow(.001f, seconds[size_t(i)] / std::max(.15f, decay));
    pole = std::exp(-6.28318530718f * std::min(cutoff, float(rate) * .4f) /
                    float(rate));
  }
  void process(float *x) {
    float tap[4][2]{}, sum[2]{};
    for (int i = 0; i < 4; ++i) {
      float mod = std::sin(phase * (1 + .13f * i) + i) * float(rate) * .00008f;
      for (int c = 0; c < 2; ++c) {
        float a = line[size_t(i)].read(delays[size_t(i)] + mod, c);
        low[size_t(i)][size_t(c)] =
            pole * low[size_t(i)][size_t(c)] + (1 - pole) * a;
        tap[i][c] = low[size_t(i)][size_t(c)];
        sum[c] += tap[i][c];
      }
    }
    for (int i = 0; i < 4; ++i)
      line[size_t(i)].push(x[i % 2] * .3f +
                               feedback[size_t(i)] * (.5f * sum[0] - tap[i][0]),
                           x[1 - i % 2] * .3f + feedback[size_t(i)] *
                                                    (.5f * sum[1] - tap[i][1]));
    x[0] = (tap[0][0] - tap[1][0] + tap[2][0] + tap[3][0]) * .5f;
    x[1] = (tap[0][1] + tap[1][1] - tap[2][1] + tap[3][1]) * .5f;
    phase += float(1 / rate) * .8f;
    if (phase > 1000)
      phase -= 1000;
  }

private:
  inline static constexpr std::array<float, 4> seconds{.0311f, .0473f, .0737f,
                                                       .1133f};
  double rate = 48000;
  float phase = 0, pole = 0;
  std::array<float, 4> delays{}, feedback{};
  std::array<DelayMemory, 4> line;
  std::array<std::array<float, 2>, 4> low{};
};
} // namespace transition
