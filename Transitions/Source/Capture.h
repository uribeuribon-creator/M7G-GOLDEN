#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
namespace transition {
class Capture {
public:
  void prepare(double rate) {
    size = int(std::ceil(rate * 2)) + 8;
    for (auto &b : data)
      b.assign(size_t(size), 0);
    reset();
  }
  void reset() { latest = -1; }
  int64_t push(float l, float r) {
    ++latest;
    auto at = size_t(latest % size);
    data[0][at] = l;
    data[1][at] = r;
    return latest;
  }
  float read(double at, int ch) const {
    auto index = int64_t(std::floor(at));
    if (index < 0 || index > latest || index < latest - size + 1)
      return 0;
    float f = float(at - double(index));
    auto next = std::min(index + 1, latest);
    return data[size_t(ch)][size_t(index % size)] * (1 - f) +
           data[size_t(ch)][size_t(next % size)] * f;
  }
  void copy(int64_t start, int count, int ch, float *out) const {
    int64_t oldest = std::max<int64_t>(0, latest - size + 1);
    int prefix = int(std::clamp<int64_t>(oldest - start, 0, count));
    std::fill_n(out, prefix, 0);
    start += prefix;
    out += prefix;
    count -= prefix;
    int valid = int(std::clamp<int64_t>(latest - start + 1, 0, count));
    if (valid > 0) {
      int at = int(start % size), first = std::min(valid, size - at);
      std::memcpy(out, data[size_t(ch)].data() + at,
                  size_t(first) * sizeof(float));
      if (valid > first)
        std::memcpy(out + first, data[size_t(ch)].data(),
                    size_t(valid - first) * sizeof(float));
    }
    std::fill_n(out + valid, count - valid, 0);
  }
  int64_t now() const { return latest; }

private:
  std::array<std::vector<float>, 2> data;
  int size = 1;
  int64_t latest = -1;
};
class Snapshot {
public:
  void prepare(double rate) {
    for (auto &b : data)
      b.assign(size_t(std::ceil(rate * 1.55)) + 8, 0);
    length = 0;
  }
  void capture(const Capture &source, int64_t start, int count) {
    length = std::clamp(count, 1, int(data[0].size()));
    double energy = 0;
    for (int c = 0; c < 2; ++c) {
      source.copy(start, length, c, data[size_t(c)].data());
      for (int i = 0; i < length; ++i) {
        float x = data[size_t(c)][size_t(i)];
        energy += x * x;
      }
    }
    level = float(std::sqrt(energy / (2 * length)));
  }
  float read(double p, int c) const {
    if (length < 2)
      return 0;
    p = std::fmod(p, double(length));
    if (p < 0)
      p += length;
    auto linear = [&](double at) {
      int a = int(at);
      int b = std::min(a + 1, length - 1);
      float f = float(at - a);
      return data[size_t(c)][size_t(a)] * (1 - f) +
             data[size_t(c)][size_t(b)] * f;
    };
    float value = linear(p);
    int fade = std::min(length / 4, 256);
    if (p > length - fade && fade > 1) {
      float f = float((p - (length - fade)) / fade);
      value = value * (1 - f) + linear(p - (length - fade)) * f;
    }
    return value;
  }
  int size() const { return length; }
  float rms() const { return level; }

private:
  std::array<std::vector<float>, 2> data;
  int length = 0;
  float level = 0;
};
class DelayMemory {
public:
  void prepare(int size) {
    for (auto &b : data)
      b.assign(size_t(size + 4), 0);
    reset();
  }
  void reset() {
    write = 0;
    count = 0;
  }
  float read(float distance, int ch) const {
    distance = std::clamp(distance, 1.0f, float(data[0].size() - 2));
    if (count < int64_t(std::ceil(distance)) + 1)
      return 0;
    double p = double(write) - distance;
    while (p < 0)
      p += data[0].size();
    int a = int(p), b = (a + 1) % int(data[0].size());
    float f = float(p - a);
    return data[size_t(ch)][size_t(a)] * (1 - f) +
           data[size_t(ch)][size_t(b)] * f;
  }
  void push(float l, float r) {
    data[0][size_t(write)] = l;
    data[1][size_t(write)] = r;
    write = (write + 1) % int(data[0].size());
    ++count;
  }

private:
  std::array<std::vector<float>, 2> data;
  int write = 0;
  int64_t count = 0;
};
} // namespace transition
