#pragma once
#include <algorithm>
#include <array>
#include <cmath>

class V11NoiseLearn {
public:
  void start(double sr) {
    rate = sr;
    samples = window = frames = voiced = 0;
    energy = 0;
    peak = 0;
    levels.fill(-100);
  }
  // Called on unprocessed input. The caller transfers completion through an
  // atomic release/acquire flag; no GUI or parameter operation runs here.
  bool push(float mono, bool tonal) {
    energy += double(mono) * mono;
    peak = std::max(peak, std::abs(mono));
    ++samples;
    if (tonal)
      ++voiced;
    if (++window >= int(rate * .02)) {
      levels[size_t(std::min(99, frames++))] =
          float(10 * std::log10(energy / window + 1e-20));
      window = 0;
      energy = 0;
    }
    return samples >= int(rate * 2);
  }
  float progress() const { return std::min(1.0f, float(samples / (rate * 2))); }
  float profile() {
    auto sorted = levels;
    std::sort(sorted.begin(), sorted.end());
    // 80th percentile tolerates a brief click while describing the noisier bed.
    float db = sorted[79];
    if (db < -90 || db > -28 || peak > .35f || voiced > int(samples * .25))
      return -100;
    return std::clamp(db, -90.0f, -28.0f);
  }

private:
  std::array<float, 100> levels{};
  int samples = 0, window = 0, frames = 0, voiced = 0;
  double rate = 48000, energy = 0;
  float peak = 0;
};
