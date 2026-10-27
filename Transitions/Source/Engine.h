#pragma once
#include "EffectVoice.h"
#include "PhraseDetector.h"
namespace transition {
class Engine {
public:
  struct Result {
    float left = 0, right = 0, activity = 0;
    int colour = 0;
  };
  void prepare(double sr) {
    rate = sr;
    latency = int(std::round(sr * 2));
    capture.prepare(sr);
    detector.prepare(sr);
    for (auto &v : pool)
      v.prepare(sr);
    reset();
  }
  void reset() {
    capture.reset();
    detector.reset();
    for (auto &v : pool)
      v.stop();
    pendingBridge = -1;
    previousEnd = -1;
    activeCount = 0;
    quiet = true;
    lastMode = -1;
    peakLimiter = 1;
  }
  void setControls(int id, float duration, float sensitivity, double bpm,
                   double origin, bool enabled) {
    if (id != program) {
      for (auto &v : pool)
        v.finishBefore(capture.now() + int64_t(rate * .025), capture.now());
      program = id;
    }
    timeMs = durationMs(program / 50, duration);
    tempo = std::clamp(bpm, 30.0, 300.0);
    gridOrigin = origin;
    detector.setSensitivity(sensitivity);
    if (!enabled && effectsEnabled)
      stopFX();
    effectsEnabled = enabled;
  }
  void stopFX() {
    for (auto &v : pool)
      v.finishBefore(capture.now() + int64_t(rate * .025), capture.now());
    pendingBridge = -1;
  }
  Result process(float l, float r, float amount = 1) {
    int64_t now = capture.push(l, r);
    auto detected = detector.process(l, r, now);
    const auto &recipe = recipes()[size_t(program)];
    if (detected.start) {
      ++phrases;
      lastDetectedStart = detected.first;
      for (auto &v : pool)
        if (v.isActive() && (v.holding() || v.bank() == Bank::Bridge))
          v.finishBefore(detected.first + latency - int64_t(rate * .015), now);
      if (effectsEnabled && recipe.bank == Bank::Intro)
        scheduleIntro(detected.first);
      pendingBridge = -1;
    }
    if (detected.end) {
      previousEnd = detected.last;
      pendingFirst = detected.first;
      if (effectsEnabled && recipe.bank == Bank::Outro)
        scheduleTail(detected.first, detected.last, false);
      if (effectsEnabled && recipe.bank == Bank::Bridge)
        pendingBridge = detected.last;
    }
    if (effectsEnabled && pendingBridge >= 0 &&
        now - pendingBridge >= int64_t(rate * .3)) {
      scheduleTail(pendingFirst, pendingBridge, true);
      pendingBridge = -1;
    }
    float dry[2]{capture.read(double(now - latency), 0),
                 capture.read(double(now - latency), 1)};
    float wet[2]{}, sumEnvelope = 0, replace = 0, maxEnvelope = 0;
    int colour = 0;
    activeCount = 0;
    for (auto &v : pool)
      if (v.isActive()) {
        v.captureIfReady(capture, detector.lastVoice());
        float out[2]{}, removed = 0;
        float envelope = effectsEnabled ? v.render(capture, now, dry, out,
                                                   removed, gridOrigin, amount)
                                        : 0;
        if (!effectsEnabled && now >= v.ends())
          v.stop();
        if (envelope > maxEnvelope) {
          maxEnvelope = envelope;
          colour = int(v.bank()) + 1;
          currentProgress = v.progress(now);
        }
        wet[0] += out[0];
        wet[1] += out[1];
        replace = std::max(replace, removed);
        sumEnvelope += envelope;
        if (envelope > 0)
          ++activeCount;
      }
    float norm = sumEnvelope > 1.4f ? std::sqrt(1.4f / sumEnvelope) : 1;
    wet[0] *= norm;
    wet[1] *= norm;
    quiet = activeCount == 0;
    lastMode = colour - 1;
    lastActivity = maxEnvelope;
    lastDry[0] = dry[0];
    lastDry[1] = dry[1];
    lastWet[0] = wet[0];
    lastWet[1] = wet[1];
    lastReplace = replace;
    return {dry[0], dry[1], maxEnvelope, colour};
  }
  Result mix(float amount, float mix, float bypass) {
    float strength = std::clamp(amount * mix, 0.0f, 1.0f) * (1 - bypass);
    float duck = (std::abs(lastDry[0]) + std::abs(lastDry[1])) > .03f &&
                         program / 50 != 2
                     ? .6f
                     : 1.0f;
    float x[2]{};
    for (int c = 0; c < 2; ++c)
      x[c] = lastDry[c] * (1 - strength * lastReplace) +
             lastWet[c] * strength * duck;
    if (lastActivity > 0 && strength > 0) {
      float peak = std::max(std::abs(x[0]), std::abs(x[1]));
      float target = peak > .98f ? .98f / peak : 1;
      peakLimiter = target < peakLimiter
                        ? target
                        : peakLimiter + (1 - std::exp(-1 / float(rate * .06))) *
                                            (target - peakLimiter);
      for (auto &v : x)
        v *= peakLimiter;
    } else
      peakLimiter = 1;
    return {x[0], x[1], strength > 0 ? lastActivity : 0,
            strength > 0 ? lastMode + 1 : 0};
  }
  float progress() const { return currentProgress; }
  int latencySamples() const { return latency; }
  int voices() const { return activeCount; }
  bool idle() const { return quiet; }
  int dropped() const { return overflow; }
  int phraseCount() const { return phrases; }
  int64_t time() const { return capture.now(); }
  float gapMs() const { return detector.gapMs(capture.now()); }
  int64_t lastTrigger() const { return lastEventStart; }
  int mode() const { return lastMode; }
  double currentTempo() const { return tempo; }

private:
  EffectVoice *freeVoice() {
    for (auto &v : pool)
      if (!v.isActive())
        return &v;
    ++overflow;
    return nullptr;
  }
  void scheduleIntro(int64_t first) {
    auto *v = freeVoice();
    if (!v)
      return;
    int duration = int(rate * timeMs * .001);
    int64_t start = first + latency - duration;
    v->arm(recipes()[size_t(program)], start, first + latency, first + latency,
           first, int(rate * .42), first + int64_t(rate * .42), tempo);
    lastEventStart = start;
  }
  void scheduleTail(int64_t first, int64_t last, bool bridge) {
    auto *v = freeVoice();
    if (!v)
      return;
    const auto &r = recipes()[size_t(program)];
    int requested = int(rate * timeMs * .001);
    int length = int(std::clamp<int64_t>(last - first + 1, 32, requested));
    int64_t source = last - length + 1;
    int64_t start = bridge ? last + latency + int64_t(rate * .02)
                           : last + latency - length + 1;
    int64_t core = bridge ? start + requested : last + latency + 1;
    using F = Family;
    double extra = .08;
    if (r.family == F::Echo || r.family == F::PingPong || r.family == F::Slap)
      extra = std::min(4.0, 60 / tempo * 6);
    if (r.family == F::Hall || r.family == F::Plate || r.family == F::Room ||
        r.family == F::Shimmer || r.family == F::ReverseVerb)
      extra = std::min(7.0, double(r.decay) * 1.5);
    if (r.family == F::Granular || r.family == F::Alien)
      extra = timeMs * .0007;
    if (r.hold)
      extra = 30;
    v->arm(r, start, core, core + int64_t(rate * extra), source, length,
           capture.now(), tempo);
    v->captureIfReady(capture, detector.lastVoice());
    lastEventStart = start;
  }
  Capture capture;
  PhraseDetector detector;
  std::array<EffectVoice, 8> pool;
  double rate = 48000, tempo = 120, gridOrigin = 0;
  int latency = 96000, program = 0, activeCount = 0, overflow = 0, phrases = 0,
      lastMode = -1;
  bool quiet = true, effectsEnabled = true;
  float timeMs = 980, peakLimiter = 1, lastActivity = 0, lastReplace = 0,
        currentProgress = 0;
  float lastDry[2]{}, lastWet[2]{};
  int64_t pendingBridge = -1, previousEnd = -1, pendingFirst = 0,
          lastDetectedStart = 0, lastEventStart = -1;
};
} // namespace transition
