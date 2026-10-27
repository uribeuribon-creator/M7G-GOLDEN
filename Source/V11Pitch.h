#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// A small streaming note graph. Correct the pitch centre, retain its faster
// expressive component. The 15 cent dead band is applied to the centre, not
// to each vibrato cycle. All storage is bounded and allocated with the object.
class V11Pitch {
public:
  struct Note {
    float hz = 0, target = 0, cents = 0;
  };
  std::array<Note, 64> graph{};
  int position = 0;
  float correctionCents = 0;
  void reset() {
    centre = 0;
    previousTarget = 0;
    held = 0;
    correctionCents = 0;
    graph.fill({});
    position = 0;
  }
  float ratio(float hz, float target, float confidence, float seconds,
              float strength, float keepHuman, bool robot) {
    if (hz < 35 || target < 35 || confidence < .8f) {
      held = 0;
      correctionCents = 0;
      return 1;
    }
    float error = 1200 * std::log2(target / hz);
    bool changed = previousTarget < 35 ||
                   std::abs(1200 * std::log2(target / previousTarget)) > 70;
    if (changed) {
      centre = error;
      held = 0;
    }
    previousTarget = target;
    held += seconds;
    float human = std::clamp(keepHuman * .01f, 0.0f, 1.0f);
    float alpha = 1 - std::exp(-seconds / (.03f + human * .22f));
    centre += alpha * (error - centre);
    float e = robot ? error : centre + (error - centre) * (1 - human);
    if (std::abs(robot ? error : centre) <= 15 || (!robot && held < .055f))
      e = 0;
    correctionCents = std::clamp(e * strength * .01f, -180.0f, 180.0f);
    graph[size_t(position++ & 63)] = {hz, target, correctionCents};
    return std::pow(2.0f, correctionCents / 1200);
  }

private:
  float centre = 0, previousTarget = 0, held = 0;
};
