#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace scratch {
class WordDetector {
public:
  struct Event {
    bool start = false, end = false;
    int64_t first = 0, last = 0;
  };
  void prepare(double sr) {
    rate = sr;
    hop = std::max(8, int(sr * .0025));
    reset();
  }
  void reset() {
    clock = first = last = -1;
    sum = 0;
    fill = 0;
    open = false;
    learning = false;
    learnCount = 0;
    learnSum = 0;
  }
  void controls(float sensitivity, float gap, float floor) {
    threshold = std::max(std::pow(10.f, (-26.f - sensitivity * .34f) / 10.f),
                         std::pow(10.f, floor / 10.f) * 6.3f);
    gapSamples = int(rate * gap * .001);
  }
  void learn() {
    learning = true;
    learnCount = 0;
    learnSum = 0;
  }
  Event process(float l, float r, int64_t now) {
    clock = now;
    double p = (double(l) * l + double(r) * r) * .5;
    sum += p;
    if (learning) {
      learnSum += p;
      if (++learnCount >= int(rate)) {
        learnedDb =
            std::clamp(float(10 * std::log10(learnSum / learnCount + 1e-12)),
                       -85.f, -18.f);
        learning = false;
        learned = true;
      }
    }
    if (learning || learned) {
      sum = 0;
      fill = 0;
      open = false;
      first = last = -1;
      return {};
    }
    Event e;
    if (++fill < hop)
      return e;
    float power = float(sum / fill);
    sum = 0;
    fill = 0;
    if (power > threshold) {
      if (!open) {
        first = std::max<int64_t>(0, now - hop + 1 - int(rate * .008));
        open = true;
        e.start = true;
        e.first = first;
      }
      last = now;
    } else if (open && power > threshold * .35f)
      last = now;
    if (open && now - last >= gapSamples) {
      open = false;
      e.end = true;
      e.first = first;
      e.last = last;
    }
    return e;
  }
  bool active() const { return open; }
  int64_t firstSample() const { return first; }
  int64_t lastSample() const { return last; }
  bool learning = false, learned = false;
  float learnedDb = -70;

private:
  double rate = 48000, sum = 0, learnSum = 0;
  int hop = 120, fill = 0, gapSamples = 4560, learnCount = 0;
  int64_t first = -1, last = -1, clock = -1;
  bool open = false;
  float threshold = .00001f;
};
} // namespace scratch
