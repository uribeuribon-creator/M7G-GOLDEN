#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <juce_core/juce_core.h>

// Key inference is tentative. Ambiguous melodies remain chromatic.
class MusicalTuner {
public:
  void reset() {
    hist.fill(0);
    recent.fill(0);
    history = 0;
    lastTarget = -1;
    scale = -1;
    candidate = -1;
    candidateFrames = 0;
    updates = 0;
  }
  float target(float hz, float confidence, float elapsed, int key = 0,
               int mode = 0) {
    if (hz < 35 || hz > 1400 || confidence < .8f)
      return 0;
    recent[static_cast<size_t>(history++ % 3)] = hz;
    float f = hz;
    if (history >= 3) {
      auto sorted = recent;
      std::sort(sorted.begin(), sorted.end());
      f = sorted[1];
    }
    const float midi = 69 + 12 * std::log2(f / 440);
    const int note = int(std::round(midi)), pc = (note % 12 + 12) % 12;
    const float decay = std::exp(-elapsed / 25);
    for (auto &x : hist)
      x *= decay;
    hist[static_cast<size_t>(pc)] += elapsed;
    ++updates;
    float total = 0;
    for (auto x : hist)
      total += x;
    int distinct = 0;
    for (auto x : hist)
      if (x > total * .035f)
        ++distinct;
    if (distinct >= 6 && updates > 100) {
      float best = -1, second = -1;
      int root = 0;
      for (int key = 0; key < 12; ++key) {
        float fit = 0;
        for (int n = 0; n < 12; ++n)
          if (inScale(n, key))
            fit += hist[static_cast<size_t>(n)];
        if (fit > best) {
          second = best;
          best = fit;
          root = key;
        } else
          second = std::max(second, fit);
      }
      if (best > total * .94f && best - second > total * .055f) {
        if (candidate == root)
          ++candidateFrames;
        else {
          candidate = root;
          candidateFrames = 1;
        }
        if (candidateFrames > 20)
          scale = root;
      } else {
        candidateFrames = 0;
        if (best < total * .8f)
          scale = -1;
      }
    }
    int targetNote = note;
    auto allowed = [&](int n) {
      if (mode == 1)
        return true;
      if (key > 0 && mode >= 2) {
        static constexpr bool minor[]{true,  false, true, true,  false, true,
                                      false, true,  true, false, true,  false};
        int pc = (n - (key - 1) + 1200) % 12;
        return mode == 3 ? minor[pc] : inScale((n % 12 + 12) % 12, key - 1);
      }
      return scale < 0 || inScale((n % 12 + 12) % 12, scale);
    };
    if (mode != 1 && (scale >= 0 || (key > 0 && mode >= 2))) {
      float distance = 100;
      for (int n = note - 1; n <= note + 1; ++n)
        if (allowed(n) && std::abs(float(n) - midi) < distance) {
          distance = std::abs(float(n) - midi);
          targetNote = n;
        }
    }
    // Hysteresis reduces note toggling at vibrato boundaries.
    if (lastTarget >= 0 && allowed(lastTarget) &&
        std::abs(midi - float(lastTarget)) < .60f)
      targetNote = lastTarget;
    lastTarget = targetNote;
    return 440 * std::pow(2.0f, float(targetNote - 69) / 12);
  }
  bool hasScale() const { return scale >= 0; }

private:
  static bool inScale(int note, int root) {
    constexpr bool major[]{true,  false, true,  false, true,  true,
                           false, true,  false, true,  false, true};
    return major[(note - root + 12) % 12];
  }
  std::array<float, 12> hist{};
  std::array<float, 3> recent{};
  int history = 0, lastTarget = -1, scale = -1, candidate = -1,
      candidateFrames = 0, updates = 0;
};
