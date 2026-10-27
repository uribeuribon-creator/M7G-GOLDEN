#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace transition {
class PhraseDetector {
public:
  struct Event {
    bool start = false, end = false;
    int64_t first = 0, last = 0;
  };
  void prepare(double sr) {
    rate = sr;
    hop = std::max(8, int(sr * .0025));
    reset();
    setSensitivity(65);
  }
  void reset() {
    energy = 0;
    fill = 0;
    open = false;
    first = last = -1;
  }
  void setSensitivity(float value) {
    threshold =
        std::pow(10.0f, (-26 - std::clamp(value, 0.0f, 100.0f) * .36f) / 10);
  }
  Event process(float l, float r, int64_t time) {
    Event result;
    energy += (double(l) * l + double(r) * r) * .5;
    if (++fill < hop)
      return result;
    float power = float(energy / fill);
    energy = 0;
    fill = 0;
    if (power > threshold) {
      if (!open) {
        open = true;
        first = std::max<int64_t>(0, time - hop + 1);
        result.start = true;
        result.first = first;
      }
      last = time;
    } else if (open && power > threshold * .25f)
      last = time;
    if (open && time - last >= int64_t(rate * .25)) {
      open = false;
      result.end = true;
      result.first = first;
      result.last = last;
    }
    return result;
  }
  bool active() const { return open; }
  int64_t lastVoice() const { return last; }
  float gapMs(int64_t now) const {
    return last < 0 ? 1000 : float((now - last) * 1000 / rate);
  }

private:
  double rate = 48000, energy = 0;
  int hop = 120, fill = 0;
  float threshold = .00001f;
  bool open = false;
  int64_t first = -1, last = -1;
};
} // namespace transition
