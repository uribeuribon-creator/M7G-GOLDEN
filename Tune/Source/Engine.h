#pragma once
#include "Spectral.h"
namespace tune {
class Engine {
public:
  void prepare(double sr, int q) {
    rate = sr;
    quality = q;
    analysis.prepare(sr);
    studio.prepare(sr);
    monitor.prepare(sr);
    reset();
  }
  void reset() {
    analysis.reset();
    studio.reset();
    monitor.reset();
    centre = target = 0;
    pending = -1;
    held = correction = clock = 0;
    ratios.fill(1);
    notes.fill(0);
    voiced = false;
  }
  void setQuality(int q) {
    quality = q;
    reset();
  }
  int latency() const { return quality == 0 ? 2048 : 512; }
  void set(const Values &values, bool authorized) {
    p = values;
    if (!authorized) {
      p[Correction] = p[Robot] = p[Formant] = p[HarmonyOn] = p[DoublesOn] = 0;
    }
    updateScale();
    studio.controls(p, ratios, voiced, analysis.hz);
    monitor.controls(p, ratios, voiced, analysis.hz);
  }
  std::array<float, 2> process(float left, float right, NeuralWorker *neural) {
    clock += 1 / rate;
    if (analysis.process((left + right) * .5f, neural)) {
      updateScale();
      updatePitch();
      if (quality == 0)
        studio.controls(p, ratios, voiced, analysis.hz);
      else
        monitor.controls(p, ratios, voiced, analysis.hz);
      updated = true;
    } else
      updated = false;
    auto out = quality == 0 ? studio.process(left, right, p[Bypass] > .5f)
                            : monitor.process(left, right, p[Bypass] > .5f);
    wet = quality == 0 ? studio.wet : monitor.wet;
    return out;
  }
  void updateScale() {
    scale.root = int(p[Key]);
    scale.kind = int(p[ScaleKind]);
    if (p[AutoKey] > .5f) {
      scale.root = analysis.keyRoot;
      scale.kind =
          analysis.keyConfidence > .06f ? (analysis.keyMinor ? 2 : 1) : 0;
    }
  }
  void updatePitch() {
    float hz = analysis.hz;
    voiced = hz >= 50 && analysis.confidence >= .79f;
    if (!voiced) {
      ratios[0] = 1;
      correction = 0;
      return;
    }
    float input = midi(hz);
    int wanted = scale.nearest(input);
    float human = p[Human] * .01f, robot = p[Robot] * .01f;
    if (target == 0) {
      target = float(wanted);
      centre = input;
    }
    if (wanted != int(target) && std::abs(input - target) > .62f) {
      if (pending != wanted) {
        pending = wanted;
        held = 0;
      }
      held += float(128 / (rate / std::max(1, int(std::round(rate / 12000.)))));
      float dwell = .012f + human * .025f * (1 - robot);
      if (held >= dwell) {
        target = float(wanted);
        centre = input;
        held = 0;
      }
    } else {
      pending = -1;
      held = 0;
    }
    constexpr float curveTime[]{1.f, .7f, .2f, .65f, 1.3f};
    constexpr float centreTime[]{.16f, .21f, .055f, .12f, .22f};
    int model = std::clamp(int(p[Model]), 0, 4);
    float dt =
        float(128 / (rate / std::max(1, int(std::round(rate / 12000.)))));
    float alpha = 1 - std::exp(-dt / (.025f + centreTime[model] * human *
                                                  (1 + .2f * profileAir)));
    centre += alpha * (input - centre);
    float error = (target - centre) * 100;
    float invisible = std::abs(error) > 3 ? error * p[Correction] * .01f : 0;
    float artistic =
        (target * 100 - (input * 100 + invisible)) * robot * (1 - human * .75f);
    float desired = std::clamp(invisible + artistic, -300.f, 300.f);
    float tau = p[Retune] * .001f * curveTime[model] * (1 - .8f * robot) *
                (1 + .3f * profileRoughness);
    float response = tau < .0001f ? 1.f : 1 - std::exp(-dt / tau);
    correction += response * (desired - correction);
    ratios[0] = std::exp2(correction / 1200.f);
    notes[0] = input + correction * .01f;
    Scale harmony = scale;
    if (harmony.kind == 0)
      harmony.kind = 1;
    int lead = harmony.nearest(notes[0]);
    constexpr int degrees[5][4] = {{2, 4, 7, -7},
                                   {4, 7, 11, -7},
                                   {7, 14, 4, 2},
                                   {-7, -14, 2, 4},
                                   {2, 4, 7, -7}};
    int pattern = std::clamp(int(p[Interval]), 0, 5);
    for (int v = 0; v < 4; ++v) {
      float note =
          float(pattern < 5 ? harmony.degree(lead, degrees[pattern][v])
                            : harmony.nearest(float(std::clamp(
                                  float(lead + p[CustomInterval] + v * 3),
                                  float(lead - 24), float(lead + 24)))));
      float variation =
          (.8f + human * 3) *
              std::sin(float(clock) * (4.3f + v * .33f) + v * 1.7f) +
          human * (v % 2 ? 1.5f : -1.5f);
      notes[size_t(v + 1)] = note + variation * .01f;
      ratios[size_t(v + 1)] = frequency(notes[size_t(v + 1)]) / hz;
    }
    for (int v = 0; v < 2; ++v) {
      float cents = p[DoubleHuman] * .01f *
                    ((v ? 1.f : -1.f) * 6.f +
                     2.5f * std::sin(float(clock) * (3.9f + v * .6f) + v));
      notes[size_t(v + 5)] = notes[0] + cents * .01f;
      ratios[size_t(v + 5)] = ratios[0] * std::exp2(cents / 1200.f);
    }
  }
  float profileAir = 0, profileRoughness = 0;
  Analysis analysis;
  Scale scale;
  float target = 0, correction = 0;
  std::array<float, 7> ratios{1, 1, 1, 1, 1, 1, 1}, notes{};
  std::array<float, 6> wet{};
  bool updated = false, voiced = false;

private:
  Values p = defaults();
  double rate = 48000, clock = 0;
  int quality = 0, pending = -1;
  float centre = 0, held = 0;
  Spectral<11> studio;
  Spectral<9> monitor;
};
} // namespace tune
