#pragma once
#include "V8Neural.h"
#include <juce_dsp/juce_dsp.h>
class V8PitchDetector {
public:
  float confidence = 0, neuralConfidence = 0;
  float detect(const float *data, int n, float sr) {
    confidence = neuralConfidence = 0;
    n = juce::jmin(n, 2048);
    double mean = 0;
    for (int i = 0; i < n; ++i)
      mean += data[i];
    mean /= n;
    prefix[0] = 0;
    in.fill({});
    for (int i = 0; i < n; ++i) {
      float v = data[i] - float(mean);
      in[size_t(i)] = {v, 0};
      prefix[size_t(i + 1)] = prefix[size_t(i)] + v * v;
    }
    if (prefix[size_t(n)] / n < .000005)
      return 0;
    fft.perform(in.data(), out.data(), false);
    for (auto &v : out)
      v = {std::norm(v), 0};
    fft.perform(out.data(), in.data(), true);
    double sum = 0;
    acf[0] = cmnd[0] = 1;
    int hi = juce::jmin(n / 2, int(sr / 35)),
        lo = juce::jmax(2, int(sr / 1400));
    for (int t = 1; t <= n / 2; ++t) {
      double ex = prefix[size_t(n - t)],
             ey = prefix[size_t(n)] - prefix[size_t(t)],
             r = in[size_t(t)].real();
      acf[size_t(t)] = float(r / std::sqrt(ex * ey + 1e-20));
      double d = juce::jmax(0.0, ex + ey - 2 * r);
      sum += d;
      cmnd[size_t(t)] = float(d * t / (sum + 1e-20));
    }
    int candidate = 0;
    for (int t = lo; t < hi; ++t)
      if (cmnd[size_t(t)] < .16f && cmnd[size_t(t)] <= cmnd[size_t(t - 1)] &&
          cmnd[size_t(t)] < cmnd[size_t(t + 1)]) {
        candidate = t;
        break;
      }
    if (!candidate) {
      float best = .78f;
      for (int t = lo + 1; t < hi; ++t)
        if (acf[size_t(t)] > best && acf[size_t(t)] > acf[size_t(t - 1)] &&
            acf[size_t(t)] > acf[size_t(t + 1)]) {
          candidate = t;
          best = acf[size_t(t)];
        }
    }
    if (!candidate)
      return 0;
    float best = -1;
    int chosen = candidate;
    for (float mult : {1.0f, .5f, 2.0f}) {
      int t = juce::roundToInt(candidate * mult);
      if (t <= lo || t >= hi)
        continue;
      for (int k = juce::jmax(lo + 1, t - 2); k <= juce::jmin(hi - 1, t + 2);
           ++k)
        if (cmnd[size_t(k)] < cmnd[size_t(t)])
          t = k;
      std::array<std::array<float, 32>, 2> features{};
      for (int i = 0; i < 32; ++i) {
        float at =
            juce::jlimit(1.0f, float(n / 2), float(t) * (.5f + 2.0f * i / 31));
        int a = int(at), b = juce::jmin(n / 2, a + 1);
        float f = at - a;
        features[0][size_t(i)] = acf[size_t(a)] * (1 - f) + acf[size_t(b)] * f;
        features[1][size_t(i)] =
            1 -
            juce::jmin(2.0f, cmnd[size_t(a)] * (1 - f) + cmnd[size_t(b)] * f);
      }
      float nn = v8::pitchCandidate(features);
      float score =
          .75f * nn + .2f * acf[size_t(t)] + .05f * (1 - cmnd[size_t(t)]);
      if (score > best) {
        best = score;
        chosen = t;
        neuralConfidence = nn;
      }
    }
    float a = cmnd[size_t(chosen - 1)], b = cmnd[size_t(chosen)],
          c = cmnd[size_t(chosen + 1)], den = a - 2 * b + c;
    float fraction = std::abs(den) > 1e-9f
                         ? juce::jlimit(-.5f, .5f, .5f * (a - c) / den)
                         : 0;
    confidence =
        juce::jlimit(0.0f, 1.0f, .7f * acf[size_t(chosen)] + .3f * (1 - b));
    return confidence > .78f ? sr / (chosen + fraction) : 0;
  }

private:
  juce::dsp::FFT fft{12};
  std::array<std::complex<float>, 4096> in{}, out{};
  std::array<double, 2049> prefix{};
  std::array<float, 1025> acf{}, cmnd{};
};
