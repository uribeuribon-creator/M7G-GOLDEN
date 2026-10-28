#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace scratch {
inline float lagrange4(float a, float b, float c, float d, float f) {
  return -a * f * (f - 1) * (f - 2) / 6 + b * (f + 1) * (f - 1) * (f - 2) / 2 -
         c * (f + 1) * f * (f - 2) / 2 + d * (f + 1) * f * (f - 1) / 6;
}
class AudioRing {
public:
  void prepare(double sr) {
    length = int(std::ceil(sr * 5)) + 8;
    for (auto &b : data)
      b.assign(size_t(length), 0);
    reset();
  }
  void reset() { clock = -1; }
  int64_t push(float l, float r) {
    ++clock;
    auto p = size_t(clock % length);
    data[0][p] = l;
    data[1][p] = r;
    return clock;
  }
  float read(int64_t at, int ch) const {
    if (at < 0 || at > clock || at < clock - length + 1)
      return 0;
    return data[size_t(ch)][size_t(at % length)];
  }
  int64_t now() const { return clock; }
  int capacity() const { return length - 8; }

private:
  std::array<std::vector<float>, 2> data;
  int length = 1;
  int64_t clock = -1;
};
class WordSnapshot {
public:
  void prepare(double sr) {
    rate = sr;
    for (auto &band : data)
      for (auto &b : band)
        b.assign(size_t(std::ceil(sr * 5)) + 8, 0);
    count = 0;
    level = 0;
  }
  bool capture(const AudioRing &ring, int64_t first, int64_t last) {
    count = int(std::clamp<int64_t>(last - first + 1, 0,
                                    int64_t(data[0][0].size() - 8)));
    if (count < std::max(64, int(rate * .035)))
      return false;
    double power = 0;
    std::array<std::array<std::array<float, 4>, 2>, 2> state{};
    float poles[2]{float(std::exp(-6.28318530718 * .22)),
                   float(std::exp(-6.28318530718 * .105))};
    for (int i = 0; i < count; ++i)
      for (int ch = 0; ch < 2; ++ch) {
        float x = ring.read(first + i, ch);
        data[0][size_t(ch)][size_t(i)] = x;
        power += x * x;
        for (int b = 0; b < 2; ++b) {
          float v = x;
          for (int s = 0; s < 4; ++s) {
            auto &h = state[size_t(b)][size_t(ch)][size_t(s)];
            h = poles[b] * h + (1 - poles[b]) * v;
            v = h;
          }
          data[size_t(b + 1)][size_t(ch)][size_t(i)] = v;
        }
      }
    level = float(std::sqrt(power / std::max(1, count * 2)));
    return level > .0012f;
  }
  float read(double p, int ch, float speed = 1, bool looping = true) const {
    if (count < 4)
      return 0;
    if (looping) {
      p = std::fmod(p, double(count));
      if (p < 0)
        p += count;
    } else if (p < 0 || p >= count)
      return 0;
    auto at = int(std::floor(p));
    float f = float(p - at);
    float band =
        std::clamp(float(std::log2(std::max(1.f, std::abs(speed)))), 0.f, 2.f);
    int b = int(band), next = std::min(2, b + 1);
    float blend = band - b;
    auto sample = [&](int which, int index) {
      if (looping) {
        index %= count;
        if (index < 0)
          index += count;
      } else
        index = std::clamp(index, 0, count - 1);
      return data[size_t(which)][size_t(ch)][size_t(index)];
    };
    auto interp = [&](int which) {
      return lagrange4(sample(which, at - 1), sample(which, at),
                       sample(which, at + 1), sample(which, at + 2), f);
    };
    float x = interp(b) * (1 - blend) + interp(next) * blend;
    float edge = float(std::min(p, double(count - 1) - p));
    float fade = float(std::min(count / 8, int(rate * .002)));
    if (fade > 1)
      x *= std::clamp(edge / fade, 0.f, 1.f);
    return x;
  }
  int size() const { return count; }
  float rms() const { return level; }
  float peakAt(int section, int total) const {
    if (!count)
      return 0;
    int a = section * count / total,
        b = std::max(a + 1, (section + 1) * count / total);
    float peak = 0;
    for (int i = a; i < std::min(count, b); ++i)
      peak = std::max(peak, std::max(std::abs(data[0][0][size_t(i)]),
                                     std::abs(data[0][1][size_t(i)])));
    return peak;
  }

private:
  std::array<std::array<std::vector<float>, 2>, 3> data;
  int count = 0;
  float level = 0;
  double rate = 48000;
};
class Delay {
public:
  void prepare(int n) {
    size = n + 1;
    for (auto &b : data)
      b.assign(size_t(size), 0);
    reset();
  }
  void reset() {
    index = 0;
    filled = 0;
  }
  std::array<float, 2> process(float l, float r) {
    std::array<float, 2> out{0, 0};
    int read = (index + 1) % size;
    if (filled >= size - 1)
      out = {data[0][size_t(read)], data[1][size_t(read)]};
    data[0][size_t(index)] = l;
    data[1][size_t(index)] = r;
    index = read;
    ++filled;
    return out;
  }

private:
  std::array<std::vector<float>, 2> data;
  int size = 1, index = 0;
  int64_t filled = 0;
};
} // namespace scratch
