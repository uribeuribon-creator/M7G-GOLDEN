#pragma once
#include "V8ModelWeights.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace v8 {
inline std::array<float, 8> autoMix(const std::array<float, 12> &x) {
  std::array<float, 16> h{};
  std::array<float, 8> y{};
  for (size_t j = 0; j < 16; ++j) {
    float s = v8models::mixB1[j];
    for (size_t i = 0; i < 12; ++i)
      s += x[i] * v8models::mixW1[i * 16 + j];
    h[j] = std::tanh(s);
  }
  for (size_t j = 0; j < 8; ++j) {
    float s = v8models::mixB2[j];
    for (size_t i = 0; i < 16; ++i)
      s += h[i] * v8models::mixW2[i * 8 + j];
    y[j] = std::clamp(s, 0.0f, 1.0f);
  }
  return y;
}
inline float pitchCandidate(const std::array<std::array<float, 32>, 2> &x) {
  std::array<float, 128> conv{};
  std::array<float, 12> h{};
  for (int pos = 0; pos < 32; ++pos)
    for (int out = 0; out < 4; ++out) {
      float s = v8models::pitchBias[size_t(out)];
      for (int c = 0; c < 2; ++c)
        for (int k = 0; k < 5; ++k) {
          int at = pos + k - 2;
          if (at >= 0 && at < 32)
            s += x[size_t(c)][size_t(at)] *
                 v8models::pitchKernels[size_t(out * 10 + c * 5 + k)];
        }
      conv[size_t(pos * 4 + out)] = std::tanh(s);
    }
  for (size_t j = 0; j < 12; ++j) {
    float s = v8models::pitchDenseBias[j];
    for (size_t i = 0; i < 128; ++i)
      s += conv[i] * v8models::pitchDense[i * 12 + j];
    h[j] = std::tanh(s);
  }
  float s = v8models::pitchOutBias[0];
  for (size_t j = 0; j < 12; ++j)
    s += h[j] * v8models::pitchOut[j];
  return 1 / (1 + std::exp(-std::clamp(s, -30.0f, 30.0f)));
}
} // namespace v8
