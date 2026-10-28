#pragma once
#include "AudioMemory.h"
#include "DigitalStretch.h"
#include "Parameters.h"
#include "WordDetector.h"
namespace scratch {
class VinylEngine {
public:
  static constexpr int Block = DigitalStretch::Block,
                       Latency = DigitalStretch::Padding + Block;
  static int latencyFor(double sr) {
    return DigitalStretch::paddingFor(sr) + Block;
  }
  int latencySamples() const { return digital.latencyPadding() + Block; }
  void prepare(double sr) {
    rate = sr;
    ring.prepare(sr);
    for (auto &s : snapshots)
      s.prepare(sr);
    digital.prepare(sr);
    vinylDelay.prepare(digital.latencyPadding());
    dryDelay.prepare(latencySamples());
    for (auto &b : echo)
      b.assign(size_t(std::ceil(sr * 2.1)), 0);
    reset();
  }
  void reset() {
    ring.reset();
    detector.reset();
    dryDelay.reset();
    vinylDelay.reset();
    digital.reset();
    activeSnapshot = 0;
    haveWord = false;
    playing = false;
    automatic = false;
    position = .5;
    velocity = 0;
    tick = 0;
    fill = 0;
    age = 0;
    manualAge = 0;
    releaseAge = 0;
    echoWrite = 0;
    echoLow.fill(0);
    colourLow.fill(0);
    dc.fill(0);
    lastInput.fill(0);
    heldSamples.fill(0);
    sampleHold = 0;
    noiseState = .0f;
    crackleEnv = 0;
    captured = 0;
    words = 0;
    serial = 0;
    lastStart = lastEnd = -1;
    onsetCaptured = false;
    frameBegin = frameMovement = 0;
    frameSnapshot = -1;
    oldSnapshot = -1;
    crossfade = 0;
    lastChop = -1;
    manualTTL = 0;
    lastTarget = .5f;
    fade = 0;
    mode = 1;
    masterFader = 1;
    bypass = 0;
    stopGain = 1;
    stopTarget = 1;
    echoPeak = 0;
    forceFade = 0;
    for (auto &b : rendered)
      b.fill(0);
    for (auto &b : meta)
      b.fill(0);
    for (auto &b : echo)
      std::fill(b.begin(), b.end(), 0);
    metaPosition = 0;
    frameActive.fill(0);
    frameGate.fill(0);
    frameSpeech.fill(0);
    noiseSeed = 0x71c30a5du;
    renderedActive.fill(0);
    renderedGate.fill(0);
    renderedSpeech.fill(0);
    gateState = currentGate = currentActivity = outputRate = 0;
  }
  void controls(const Values &values, double tempo = 120, bool preset = false) {
    p = values;
    if (preset) {
      lastTarget = p[Position];
      manualTTL = 0;
    }
    outputGain = std::pow(10.f, p[Output] / 20.f);
    bpm = std::clamp(tempo, 30., 300.);
    if (p[NoiseFloor] != lastNoiseFloor) {
      learnedFloor = p[NoiseFloor];
      lastNoiseFloor = p[NoiseFloor];
    }
    detector.controls(p[Sensitivity], p[Gap], learnedFloor);
    if (std::abs(p[Position] - lastTarget) > .00001f) {
      manualTTL = int(rate * .10);
      lastTarget = p[Position];
    }
  }
  void requestCapture() { captureNow = true; }
  void requestPlay() { playNow = true; }
  void requestStop() {
    playing = false;
    automatic = false;
    stopTarget = 0;
  }
  void requestCue(int i) { cueNow = std::clamp(i, 0, 7); }
  void requestLearn() { detector.learn(); }
  std::array<float, 2> process(float l, float r) {
    int64_t now = ring.push(l, r);
    auto event = detector.process(l, r, now);
    if (event.start) {
      lastStart = event.first;
      onsetCaptured = false;
      ++words;
    }
    // Start with the NEW word's own first 80 ms, never a recycled old word.
    // The full fragment replaces this preview after its detected end/pause.
    if (detector.active() && !onsetCaptured &&
        now - detector.firstSample() >= int64_t(rate * .080)) {
      onsetCaptured = true;
      if (p[AutoScratch] > .5f && p[Touch] < .5f && manualTTL <= 0 &&
          capture(detector.firstSample(), now))
        startAuto();
    }
    if (event.end) {
      lastStart = event.first;
      lastEnd = event.last;
      if (p[AutoScratch] > .5f && p[Touch] < .5f && manualTTL <= 0 &&
          capture(event.first, event.last)) {
        startAuto();
      }
    }
    if (detector.learned) {
      learnedFloor = detector.learnedDb;
      detector.learned = false;
      learnReady = true;
      detector.controls(p[Sensitivity], p[Gap], learnedFloor);
    }
    if (captureNow) {
      captureNow = false;
      int64_t end = detector.active() ? now : lastEnd;
      int64_t first = detector.active() ? detector.firstSample() : lastStart;
      if (end < 0) {
        end = now;
        first = std::max<int64_t>(0, end - int64_t(rate * .8));
      }
      capture(first, end);
    }
    if (playNow) {
      playNow = false;
      if (!haveWord) {
        int64_t end = lastEnd >= 0 ? lastEnd : now;
        int64_t first = lastStart >= 0
                            ? lastStart
                            : std::max<int64_t>(0, end - int64_t(rate * .8));
        capture(first, end);
      }
      if (haveWord)
        startAuto();
    }
    if (cueNow >= 0 && haveWord) {
      jump(p[size_t(FirstCue + cueNow)]);
      playing = true;
      automatic = false;
      manualAge = 0;
      releaseAge = 0;
      manualTTL = int(rate * .08);
      stopTarget = 1;
      cueNow = -1;
    }
    if (p[Touch] > .5f && haveWord) {
      if (automatic) {
        automatic = false;
        manualAge = 0;
      }
      playing = true;
      releaseAge = 0;
      stopTarget = 1;
    }
    if (manualTTL > 0) {
      --manualTTL;
      if (haveWord) {
        playing = true;
        automatic = false;
        releaseAge = 0;
        stopTarget = 1;
      }
    }
    auto dry = dryDelay.process(l, r);
    std::array<float, 2> wet{rendered[0][size_t(fill)],
                             rendered[1][size_t(fill)]};
    float replace = renderedActive[size_t(fill)],
          autoGate = renderedGate[size_t(fill)],
          speech = renderedSpeech[size_t(fill)];
    float raw[2]{};
    double before = position;
    if (playing && haveWord) {
      advance();
      float motion =
          float((position - before) * snapshots[size_t(activeSnapshot)].size());
      if (std::abs(motion) > 12)
        motion = lastRate;
      lastRate = motion;
      float moving = std::clamp(std::abs(motion) / .12f, 0.f, 1.f);
      float envelope = std::min(1.f, float(age / (rate * .006))) * moving;
      if (!automatic && p[Touch] < .5f && manualTTL <= 0) {
        releaseAge += 1 / rate;
        if (p[Hold] < .5f)
          envelope *= std::clamp(
              1.f - float((releaseAge - p[ReleaseTime] * .001) / .05), 0.f,
              1.f);
        if (releaseAge > p[ReleaseTime] * .001 + .07 && p[Hold] < .5f)
          playing = false;
      }
      if (automatic &&
          age / rate > p[Duration] * .001 + p[ReleaseTime] * .001) {
        if (p[Hold] < .5f)
          playing = false;
        else if (age / rate > 15)
          playing = false;
      }
      for (int c = 0; c < 2; ++c) {
        raw[c] = snapshots[size_t(activeSnapshot)].read(
            position * snapshots[size_t(activeSnapshot)].size(), c,
            std::max(1.f, std::abs(motion)), true);
        if (oldSnapshot >= 0 && crossfade > 0) {
          float old = snapshots[size_t(oldSnapshot)].read(
              oldPosition, c, std::max(1.f, std::abs(oldRate)), true);
          float a = float(crossfade) / std::max(1, int(rate * .006));
          raw[c] = raw[c] * (1 - a) + old * a;
        }
      }
      if (crossfade > 0) {
        oldPosition += oldRate;
        --crossfade;
      }
      for (int c = 0; c < 2; ++c)
        vinylFrame[size_t(c)][size_t(fill)] = raw[c] * envelope;
      frameGate[size_t(fill)] = p[AutoFader] > .5f ? gestureGate() : 1;
      frameActive[size_t(fill)] = envelope;
      if (frameSnapshot != activeSnapshot) {
        frameBegin = position * snapshots[size_t(activeSnapshot)].size();
        frameMovement = 0;
        frameSnapshot = activeSnapshot;
      }
      frameMovement += motion;
      ++age;
    } else {
      for (int c = 0; c < 2; ++c)
        vinylFrame[size_t(c)][size_t(fill)] = 0;
      frameGate[size_t(fill)] = 0;
      frameActive[size_t(fill)] = 0;
      frameMovement = 0;
      frameSnapshot = -1;
    }
    frameSpeech[size_t(fill)] = detector.active() ? 1.f : 0.f;
    float target = p[Cut] > .5f                ? 0.f
                   : p[CrossfaderCurve] > 1.5f ? 1 - p[Crossfader]
                   : p[CrossfaderCurve] < .5f
                       ? (p[Crossfader] > .5f ? 1.f : 0.f)
                       : std::pow(std::clamp(p[Crossfader], 0.f, 1.f), 1.4f);
    masterFader +=
        std::min(1.f, float(1 / (rate * .001))) * (target - masterFader);
    stopGain +=
        std::min(1.f, float(1 / (rate * .012))) * (stopTarget - stopGain);
    float byTarget = p[Bypass] > .5f ? 1.f : 0.f;
    bypass += std::min(1.f, float(1 / (rate * .008))) * (byTarget - bypass);
    float strength = p[Intensity] * .01f;
    float wetMix = p[Mix] * .01f * (1 - bypass);
    float duck = p[Intelligent] > .5f ? 1 - .65f * speech : 1;
    float replaced = replace * (p[Intelligent] > .5f ? 1 - .85f * speech : 1) *
                     std::min(1.f, strength) * stopGain;
    std::array<float, 2> out{};
    for (int c = 0; c < 2; ++c) {
      float effect = wet[size_t(c)] * masterFader * stopGain * duck * strength;
      out[size_t(c)] =
          dry[size_t(c)] * (1 - wetMix * replaced) + effect * wetMix;
      out[size_t(c)] *= outputGain;
      if (wetMix > 1e-6f &&
          (replace > 0 || std::abs(wet[0]) + std::abs(wet[1]) > 1e-7f) &&
          std::abs(out[size_t(c)]) > .94f)
        out[size_t(c)] = std::copysign(
            .94f + .06f * std::tanh((std::abs(out[size_t(c)]) - .94f) / .06f),
            out[size_t(c)]);
      if (!std::isfinite(out[size_t(c)]))
        out[size_t(c)] = 0;
    }
    if (p[Bypass] > .5f && bypass > .9999f)
      out = dry;
    currentGate = autoGate * masterFader * stopGain;
    currentActivity = replace;
    outputRate = playing ? lastRate : 0;
    if (++fill == Block) {
      renderBlock();
      fill = 0;
      frameSnapshot = -1;
      frameMovement = 0;
    }
    return out;
  }
  float wordPeak(int index) const {
    return haveWord ? snapshots[size_t(activeSnapshot)].peakAt(index, 512) : 0;
  }
  int wordSize() const {
    return haveWord ? snapshots[size_t(activeSnapshot)].size() : 0;
  }
  int64_t time() const { return ring.now(); }
  double position = .5;
  float velocity = 0, currentGate = 0, currentActivity = 0, outputRate = 0,
        learnedFloor = -70;
  bool learnReady = false, playing = false, haveWord = false;
  int captured = 0, words = 0, serial = 0;
  DigitalStretch digital;
  WordDetector detector;

private:
  bool capture(int64_t first, int64_t last) {
    if (first < 0 || last < first)
      return false;
    first = std::max(first, last - int64_t(rate * 5) + 1);
    int next = 1 - activeSnapshot;
    if (!snapshots[size_t(next)].capture(ring, first, last))
      return false;
    oldSnapshot = haveWord ? activeSnapshot : -1;
    oldPosition = position * snapshots[size_t(activeSnapshot)].size();
    oldRate = lastRate;
    activeSnapshot = next;
    haveWord = true;
    crossfade = oldSnapshot >= 0 ? int(rate * .006) : 0;
    captured = wordSize();
    ++serial;
    position = std::clamp(double(p[FirstCue]), .01, .99);
    velocity = 0;
    return true;
  }
  void startAuto() {
    playing = true;
    automatic = true;
    age = 0;
    releaseAge = 0;
    lastChop = -1;
    stopTarget = 1;
    float start = p[Reverse] > .5f ? 1 - p[FirstCue] : p[FirstCue];
    jump(std::clamp(start, .03f, .95f));
    velocity = 0;
  }
  void jump(float normalized) {
    oldSnapshot = haveWord ? activeSnapshot : -1;
    oldPosition = position * wordSize();
    oldRate = lastRate;
    crossfade = int(rate * .006);
    position = std::clamp(double(normalized), .003, .997);
    velocity = 0;
  }
  double targetPosition() {
    double t = age / rate;
    int pattern = std::clamp(int(p[Pattern]), 0, 9);
    int variant = int(p[Variant]);
    double sourceSeconds = std::max(.02, wordSize() / rate);
    double speed = p[Speed], depth = p[Depth] * .01,
           cycle = (2.7 + variant * .045) * speed * t;
    double f = cycle - std::floor(cycle), triangle = 1 - std::abs(2 * f - 1);
    double q = .1 + depth * .8 * triangle;
    if (pattern == 0)
      q = .07 + depth * .88 * (.5 - .5 * std::cos(6.28318530718 * cycle));
    if (pattern == 1)
      q = .08 + depth * .82 * (f < .6 ? f / .6 : 1 - (f - .6) / .4);
    if (pattern == 2)
      q = .08 +
          depth * .82 *
              (.5 - .5 * std::cos(6.28318530718 *
                                  (cycle + .08 * std::sin(cycle * 12.56637))));
    if (pattern == 3) {
      double cuts = (p[TransformerRate] * bpm / 120) * t;
      int step = int(cuts);
      float at = p[size_t(FirstCue + (step == 0 ? 0 : (step + variant) % 8))];
      if (step != lastChop) {
        lastChop = step;
        jump(at);
      }
      q = std::fmod(at + (cuts - step) * (.15 + .02 * (variant % 5)), .98);
    }
    if (pattern == 4)
      q = .08 + depth * .84 * triangle;
    if (pattern == 5) {
      double tear = std::pow(triangle, .55 + .15 * (variant % 4));
      q = .1 + depth * .82 *
                   (.78 * tear + .22 * std::floor(tear * (2 + variant % 3)) /
                                     (2 + variant % 3));
    }
    if (pattern == 6) {
      double slow = .34 + .016 * (variant % 7);
      if (p[VinylMode] < .5f)
        slow = std::max(slow, .30 / std::max(.25, double(p[Speed])));
      q = std::fmod(.06 + t * slow * speed / sourceSeconds, .94);
    }
    if (pattern == 7)
      q = .08 + depth * .82 * (.5 - .5 * std::cos(6.28318530718 * cycle));
    if (pattern == 8) {
      double warp = .5 + .5 * std::sin(6.28318530718 * cycle +
                                       1.4 * std::sin(cycle * 2.3));
      q = .06 + depth * .9 * warp;
    }
    if (pattern == 9)
      q = .08 + depth * .84 * triangle;
    if (p[Reverse] > .5f)
      q = 1 - q;
    return std::clamp(q, .003, .997);
  }
  void advance() {
    if (--tick <= 0) {
      tick = std::max(1, int(rate * .001));
      double target = automatic ? targetPosition() : p[Position];
      bool touched = p[Touch] > .5f || manualTTL > 0;
      float adaptation =
          p[Intelligent] > .5f
              ? std::clamp(float(.65 * rate / std::max(1, wordSize())), .3f,
                           6.f)
              : 1.f;
      float friction = p[Friction] * (.45f + p[Tension] * .013f) * adaptation;
      float inertia = std::clamp(p[Inertia], .85f, .98f);
      if (automatic && p[Brake] > .5f) {
        double u = std::clamp(age / (rate * p[Duration] * .001), 0., 1.);
        float wanted = float(
            (p[Reverse] > .5f ? -1 : 1) * (.5 + 3.5 * p[PitchRange] * .01) *
            std::pow(1 - u, 2) * rate / std::max(1, wordSize()) * .001);
        velocity = velocity * inertia + wanted * (1 - inertia);
      } else if (automatic && p[ReleaseAction] > .5f) {
        float wanted = float(
            (p[Reverse] > .5f ? -1 : 1) * p[MotorTorque] *
            (1 -
             std::exp(-age / (rate * std::max(40.f, p[ReleaseTime]) * .001))) *
            p[Speed] * rate / std::max(1, wordSize()) * .001);
        velocity = velocity * inertia + wanted * (1 - inertia);
      } else if (automatic || touched) {
        velocity = float((target - position) * friction + velocity * inertia);
      } else {
        float wanted = float((p[Rpm] > .5f ? 1.35 : 1.) * p[Speed] * rate /
                             std::max(1, wordSize()) * .001);
        float torque = p[MotorTorque];
        velocity = velocity * inertia + wanted * torque * (1 - inertia);
      }
      float cap = float((.5 + 3.5 * p[PitchRange] * .01) * rate /
                        std::max(1, wordSize()) * .001);
      velocity = std::clamp(velocity, -cap, cap);
      step = velocity / std::max(1, int(rate * .001));
    }
    position += step;
    if (position >= .999 || position <= .001) {
      position = std::clamp(position, .001, .999);
      velocity = 0;
      step = 0;
    }
  }
  float gestureGate() const {
    if (!automatic && p[AutoFader] < .5f)
      return 1;
    double t = age / rate;
    double rateHz = p[TransformerRate] * bpm / 120;
    int pattern = int(p[Pattern]), variant = int(p[Variant]);
    double cycle = (2.7 + variant * .045) * p[Speed] * t,
           f = cycle - std::floor(cycle);
    float g = 1;
    if (pattern == 0)
      g = (f > .05 && f < .45) || (f > .55 && f < .95) ? 1.f : 0.f;
    else if (pattern == 1) {
      double phase = t * rateHz;
      phase -= std::floor(phase);
      g = phase < (.32 + .025 * (variant % 8)) ? 1.f : 0.f;
    } else if (pattern == 2) {
      int clicks = 1 + variant % 4;
      double phase = f * 2;
      phase -= std::floor(phase);
      double pphase = phase * clicks;
      pphase -= std::floor(pphase);
      g = pphase < .14 ? 0.f : 1.f;
    } else if (pattern == 3) {
      double phase = t * rateHz;
      phase -= std::floor(phase);
      g = phase < .82 ? 1.f : 0.f;
    } else if (pattern == 5) {
      double phase = f * (2 + variant % 3);
      phase -= std::floor(phase);
      g = phase < .09 ? 0.f : 1.f;
    }
    if (p[CrossfaderCurve] > .5f && p[CrossfaderCurve] < 1.5f)
      g = g ? std::min(1.f, float(std::min(f, 1 - f) * 22)) : 0;
    return g;
  }
  float random() {
    noiseSeed ^= noiseSeed << 13;
    noiseSeed ^= noiseSeed >> 17;
    noiseSeed ^= noiseSeed << 5;
    return float(double(noiseSeed) / 4294967296.0) * 2 - 1;
  }
  void renderBlock() {
    std::array<std::array<float, Block>, 2> digitalFrame{};
    const WordSnapshot *source =
        haveWord ? &snapshots[size_t(activeSnapshot)] : nullptr;
    digital.block(source, frameBegin, frameMovement, frameSnapshot >= 0,
                  digitalFrame);
    float color =
        std::exp(-6.2831853f * (18000.f - p[LoFi] * 165.f) / float(rate));
    float echoPole = std::exp(-6.2831853f * 6500 / float(rate));
    float targetMode = p[VinylMode] > .5f ? 1.f : 0.f;
    float drive = 1 + p[Drive] * .065f;
    float outGain = 1.18f;
    int delay =
        std::clamp(int(rate * p[EchoDelay] * .001), 1, int(echo[0].size()) - 2);
    int hold = 1 + int(p[LoFi] * .09f);
    float quant = std::pow(2.f, 16 - p[LoFi] * .11f);
    for (int i = 0; i < Block; ++i) {
      auto vinyl = vinylDelay.process(vinylFrame[0][size_t(i)],
                                      vinylFrame[1][size_t(i)]);
      float act = meta[0][size_t(metaPosition)],
            gate = meta[1][size_t(metaPosition)],
            speech = meta[2][size_t(metaPosition)];
      meta[0][size_t(metaPosition)] = frameActive[size_t(i)];
      meta[1][size_t(metaPosition)] = frameGate[size_t(i)];
      meta[2][size_t(metaPosition)] = frameSpeech[size_t(i)];
      metaPosition = (metaPosition + 1) % digital.latencyPadding();
      mode += std::min(1.f, float(1 / (rate * .015))) * (targetMode - mode);
      gateState +=
          std::min(1.f, float(1 / (rate * .0005))) * (gate - gateState);
      float n = random();
      noiseState = .92f * noiseState + .08f * n;
      if (act > .001f &&
          std::abs(n) > 1.f - (.5f + p[Crackle] * .18f) / float(rate)) {
        crackleEnv = n * p[Crackle] * .0006f;
      }
      crackleEnv *= std::exp(-1.f / float(rate * .00045));
      float texture = (noiseState * p[VinylNoise] * .0008f + crackleEnv) * act;
      for (int c = 0; c < 2; ++c) {
        float x = (vinyl[size_t(c)] * mode +
                   digitalFrame[size_t(c)][size_t(i)] * (1 - mode)) *
                  gateState;
        x += texture * gateState;
        colourLow[size_t(c)] = color * colourLow[size_t(c)] + (1 - color) * x;
        x = colourLow[size_t(c)];
        if (sampleHold % hold == 0)
          heldSamples[size_t(c)] = std::round(x * quant) / quant;
        x = x * (1 - p[LoFi] * .01f) + heldSamples[size_t(c)] * p[LoFi] * .01f;
        x = std::tanh(x * drive) / std::sqrt(drive) * outGain;
        float hp = x - lastInput[size_t(c)] + .995f * dc[size_t(c)];
        lastInput[size_t(c)] = x;
        dc[size_t(c)] = hp;
        x = hp;
        int read =
            (echoWrite - delay + int(echo[0].size())) % int(echo[0].size());
        float delayed = echo[size_t(c)][size_t(read)];
        echoLow[size_t(c)] =
            echoPole * echoLow[size_t(c)] + (1 - echoPole) * delayed;
        echo[size_t(c)][size_t(echoWrite)] =
            x + std::tanh(echoLow[size_t(c)]) *
                    std::clamp(p[EchoFeedback], 0.f, .8f);
        float output = x + delayed * p[EchoMix] * .01f;
        rendered[size_t(c)][size_t(i)] = output;
        echoPeak = std::max(std::abs(output), echoPeak * .9999f);
      }
      renderedActive[size_t(i)] = act;
      renderedGate[size_t(i)] = gateState;
      renderedSpeech[size_t(i)] = speech;
      echoWrite = (echoWrite + 1) % int(echo[0].size());
      ++sampleHold;
    }
  }
  Values p = defaults();
  double rate = 48000, bpm = 120, age = 0, manualAge = 0, releaseAge = 0,
         step = 0, oldPosition = 0, frameBegin = 0, frameMovement = 0;
  float outputGain = 1, lastRate = 0, oldRate = 0, fade = 0, mode = 1,
        masterFader = 1, bypass = 0, stopGain = 1, stopTarget = 1, echoPeak = 0,
        forceFade = 0, noiseState = 0, crackleEnv = 0, gateState = 0,
        lastTarget = .5f, lastNoiseFloor = -999;
  int tick = 0, fill = 0, activeSnapshot = 0, oldSnapshot = -1, crossfade = 0,
      lastChop = -1, manualTTL = 0, cueNow = -1, frameSnapshot = -1,
      metaPosition = 0, echoWrite = 0, sampleHold = 0;
  bool automatic = false, captureNow = false, playNow = false,
       onsetCaptured = false;
  int64_t lastStart = -1, lastEnd = -1;
  uint32_t noiseSeed = 0x71c30a5d;
  AudioRing ring;
  std::array<WordSnapshot, 2> snapshots;
  Delay dryDelay, vinylDelay;
  std::array<std::array<float, Block>, 2> vinylFrame{}, rendered{};
  std::array<float, Block> frameActive{}, frameGate{}, frameSpeech{},
      renderedActive{}, renderedGate{}, renderedSpeech{};
  std::array<std::array<float, 16384>, 3> meta{};
  std::array<std::vector<float>, 2> echo;
  std::array<float, 2> echoLow{}, colourLow{}, dc{}, lastInput{}, heldSamples{};
};
} // namespace scratch
