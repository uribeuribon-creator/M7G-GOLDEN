#pragma once
#include "Capture.h"
#include "Formant.h"
#include "PresetBank.h"
#include "Space.h"
namespace transition {
class EffectVoice {
public:
  void prepare(double sr) {
    rate = sr;
    snapshot.prepare(sr);
    echo.prepare(int(sr * 2));
    micro.prepare(int(sr * .06));
    space.prepare(sr);
    formant.prepare();
    for (int i = 0; i < 1024; ++i)
      hann[size_t(i)] =
          .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i / 1024);
    active = false;
  }
  void arm(const Recipe &recipe, int64_t first, int64_t coreLast, int64_t final,
           int64_t source, int length, int64_t ready, double bpm) {
    spec = recipe;
    start = first;
    coreEnd = std::max(first + 64, coreLast);
    end = std::max(coreEnd, final);
    sourceStart = source;
    sourceLength = length;
    readyTime = ready;
    captured = false;
    active = true;
    fadeOutSamples = float(rate * .045);
    position = 0;
    frames = 0;
    phase = 0;
    lastRandomStep = -1;
    randomRatio = 1;
    essEnvelope = fullEnvelope = breathPower = 0;
    essLow.fill(0);
    last.fill(0);
    low.fill(0);
    high.fill(0);
    allpass.fill({});
    delayFeedback.fill(0);
    energy = 0;
    echo.reset();
    micro.reset();
    space.reset();
    if (spec.family == Family::Formant)
      formant.reset();
    beatSamples = float(rate * 60 / std::clamp(bpm, 30.0, 300.0));
    delaySamples =
        spec.family == Family::Slap
            ? float(rate * .115)
            : std::clamp(beatSamples * spec.beats, 1.0f, float(rate * 1.95));
    grainLength = std::max(32.0f, float(rate) * spec.grain);
    space.configure(spec.decay, spec.highCut * .65f);
    for (int i = 0; i < 32; ++i)
      reverseGain[size_t(i)] = std::exp(-4 * float(i + 1) / 32);
    noise = 0x9165u + uint32_t(first);
  }
  void captureIfReady(const Capture &capture, int64_t lastVoice) {
    if (!active || captured || capture.now() < readyTime)
      return;
    int count = sourceLength;
    if (spec.bank == Bank::Intro && lastVoice >= sourceStart)
      count = std::min(count, int(std::max<int64_t>(int64_t(rate * .065),
                                                    lastVoice - sourceStart +
                                                        int64_t(rate * .008))));
    snapshot.capture(capture, sourceStart, count);
    captured = true;
    if (snapshot.rms() < 1e-7f)
      active = false;
  }
  void finishBefore(int64_t time, int64_t now) {
    if (time <= start) {
      active = false;
      return;
    }
    auto next = std::min(end, time);
    if (next < end) {
      fadeOutSamples =
          std::min(fadeOutSamples, float(std::max<int64_t>(1, next - now)));
      end = next;
    }
  }
  bool holding() const { return spec.hold && spec.bank != Bank::Intro; }
  bool isActive() const { return active; }
  int64_t starts() const { return start; }
  int64_t ends() const { return end; }
  Bank bank() const { return spec.bank; }
  float progress(int64_t now) const {
    return std::clamp(float(now - start) /
                          float(std::max<int64_t>(1, coreEnd - start)),
                      0.0f, 1.0f);
  }
  void stop() { active = false; }
  float level() const { return energy; }
  float render(const Capture &capture, int64_t now, const float *clean,
               float *wet, float &replace, double gridOrigin, float amount) {
    wet[0] = wet[1] = 0;
    replace = 0;
    if (!active || !captured || now < start)
      return 0;
    if (now >= end) {
      active = false;
      return 0;
    }
    float elapsed = float(now - start),
          q = std::clamp(elapsed / float(coreEnd - start), 0.0f, 1.0f);
    float rampIn = std::min(1.0f, elapsed / float(rate * .012));
    float rampOut = std::min(1.0f, float(end - now) / fadeOutSamples);
    float envelope = rampIn * rampOut;
    if (now > coreEnd && !holding())
      envelope *= std::exp(-3 * float(now - coreEnd) /
                           float(std::max<int64_t>(1, end - coreEnd)));
    float ratio = spec.speed0 + (spec.speed1 - spec.speed0) * q;
    float x[2]{};
    using F = Family;
    bool additive = spec.family == F::Echo || spec.family == F::PingPong ||
                    spec.family == F::Slap || spec.family == F::Hall ||
                    spec.family == F::Room || spec.family == F::Plate ||
                    spec.family == F::Shimmer || spec.family == F::ReverseVerb;
    replace = spec.bank == Bank::Outro && !additive ? envelope : 0;
    auto read = [&](double p, int c) { return snapshot.read(p, c); };
    auto grains = [&](float pitch, float stretch, int heads = 4) {
      pitch = 1 + amount * (pitch - 1);
      stretch = 1 + amount * (stretch - 1);
      float out[2]{};
      float weights = 0;
      for (int h = 0; h < heads; ++h) {
        double age = std::fmod(double(frames) + double(h) * grainLength / heads,
                               double(grainLength));
        float window =
            hann[size_t(std::clamp(int(age / grainLength * 1024), 0, 1023))];
        double at = double(frames) * stretch + (pitch - stretch) * age +
                    (h % 2 ? .013 * rate : 0);
        if (spec.family == Family::Freeze)
          at += std::max(0.0, double(snapshot.size()) - grainLength * 1.2);
        for (int c = 0; c < 2; ++c)
          out[c] += read(at, c) * window;
        weights += window;
      }
      for (int c = 0; c < 2; ++c)
        x[c] = out[c] / std::max(.01f, weights);
    };
    for (int c = 0; c < 2; ++c)
      x[c] = read(double(frames), c);
    float musical = float(std::fmod(
        (double(now) - gridOrigin) / std::max(1.0f, beatSamples * spec.beats),
        1.0));
    if (musical < 0)
      musical += 1;
    float oscillator = std::sin(phase);
    switch (spec.family) {
    case F::Pitch: {
      if (spec.randomPitch) {
        int64_t step = int64_t(
            std::floor((double(now) - gridOrigin) / (beatSamples * .25f)));
        if (step != lastRandomStep) {
          lastRandomStep = step;
          uint32_t h = uint32_t(step) * 747796405u + 2891336453u;
          const int notes[]{-7, -5, 0, 3, 5, 7, 12};
          randomRatio = std::pow(2.0f, float(notes[h % 7]) / 12);
        }
        ratio = randomRatio;
      }
      grains(ratio, 1);
      break;
    }
    case F::Octave:
    case F::Fifth: {
      float plain[2]{x[0], x[1]};
      grains(ratio, 1);
      if (spec.stack) {
        float up[2]{x[0], x[1]};
        grains(.5f, 1);
        for (int c = 0; c < 2; ++c)
          x[c] = plain[c] * .4f + up[c] * .4f + x[c] * .3f;
      }
      break;
    }
    case F::Stretch:
      grains(1, std::max(.02f, ratio));
      break;
    case F::Granular:
      grains(.85f + .3f * q, .18f, 8);
      break;
    case F::Freeze:
      grains(1, .0f, 6);
      break;
    case F::Tape:
    case F::Vinyl:
    case F::HalfSpeed:
      for (int c = 0; c < 2; ++c)
        x[c] = read(position, c);
      position += 1 + amount * (ratio - 1);
      break;
    case F::Reverse:
      for (int c = 0; c < 2; ++c)
        x[c] = read(snapshot.size() - 1 + position, c);
      position -= std::abs(ratio);
      break;
    case F::Stutter:
    case F::Chops:
    case F::Repeat: {
      float period = beatSamples * spec.beats;
      if (spec.family == F::Repeat) {
        constexpr float notes[]{1, .5f, .25f, 1.0f / 6, .125f};
        float requested = float(coreEnd - start) / float(spec.repeats);
        float best = 1e30f;
        for (float note : notes) {
          float candidate = beatSamples * note;
          if (std::abs(candidate - requested) < best) {
            best = std::abs(candidate - requested);
            period = candidate;
          }
        }
      }
      if (spec.family == F::Chops)
        period = beatSamples / float(spec.repeats);
      if (spec.family == F::Stutter && spec.bank == Bank::Intro) {
        double switchAt =
            gridOrigin +
            std::ceil(
                (double(start) + .65 * double(coreEnd - start) - gridOrigin) /
                period) *
                period;
        if (double(now) >= switchAt)
          period *= .5f;
      }
      period = std::max(32.0f, period);
      float phaseInBeat =
          float(std::fmod((double(now) - gridOrigin) / period, 1.0));
      if (phaseInBeat < 0)
        phaseInBeat += 1;
      float span = std::min(period, float(snapshot.size()));
      float at = phaseInBeat * span;
      float edge =
          std::min(1.0f, std::min(phaseInBeat, 1 - phaseInBeat) * period / 96);
      float gate = spec.family == F::Chops && phaseInBeat > .58f ? 0.0f : edge;
      for (int c = 0; c < 2; ++c)
        x[c] = x[c] * (1 - amount) +
               amount * read(snapshot.size() - span + at, c) * gate;
      break;
    }
    case F::PitchLadder:
      grains(q < .333f ? 1.0f : q < .666f ? 1.259921f : 1.498307f, 1);
      break;
    case F::PitchRecover:
      grains(q < .55f ? 1.5f - q * 1.55f : .65f + (q - .55f) * .7778f, 1);
      break;
    case F::Riser:
      grains(.6f + 2.4f * q, .5f);
      break;
    case F::Spiral:
      grains(std::max(.15f, ratio), .7f);
      break;
    case F::Scratch: {
      float triangle =
          1 -
          4 * std::abs(float(std::fmod(double(frames) /
                                           std::max(32.0f, beatSamples * .5f),
                                       1.0)) -
                       .5f);
      position += triangle * 2.2;
      for (int c = 0; c < 2; ++c)
        x[c] = read(position, c);
      break;
    }
    case F::Shimmer: {
      float plain[2]{x[0], x[1]};
      grains(2, 1);
      for (int c = 0; c < 2; ++c)
        x[c] = plain[c] * .45f + x[c] * .65f;
      break;
    }
    case F::Formant:
      formant.process(x, 1 + amount * (spec.formant - 1));
      break;
    case F::Alien:
      grains(.7f + q * 1.1f, .3f, 8);
      for (int c = 0; c < 2; ++c)
        x[c] =
            std::tanh(x[c] * 3) *
            (.55f + .45f * std::sin(float(frames) / float(rate) * 628.3185f));
      break;
    default:
      break;
    }
    if (spec.chopped) {
      float g = musical < .55f ? 1.0f : 0.0f;
      for (auto &sample : x)
        sample *= 1 - amount + amount * g;
    }
    float cutoff = spec.highCut, loCut = spec.lowCut;
    if (spec.family == F::Filter)
      cutoff =
          spec.bank == Bank::Outro ? 16000 * (1 - q) + 400 : 250 + 15500 * q;
    if (spec.family == F::Riser)
      cutoff = 450 + 14000 * q;
    if (spec.family == F::Radio || spec.family == F::Telephone) {
      if (spec.bank == Bank::Intro) {
        loCut = 440 * (1 - q) + 55 * q;
        cutoff = 3200 + 13000 * q;
      }
    }
    if (spec.family == F::LoFi)
      cutoff = spec.bank == Bank::Intro ? 1100 + 14000 * q : 2600;
    if (spec.family == F::Air)
      cutoff = 18000;
    cutoff = std::exp(std::log(18000.0f) +
                      (std::log(std::max(80.0f, cutoff)) - std::log(18000.0f)) *
                          amount);
    loCut = 30 + amount * (loCut - 30);
    if ((frames & 31) == 0) {
      lowPole = std::exp(-6.28318530718f *
                         std::min(cutoff, float(rate) * .43f) / float(rate));
      highPole = std::exp(-6.28318530718f * loCut / float(rate));
      phaserCoefficient =
          (std::tan(3.14159265359f * (700 + 650 * oscillator) / float(rate)) -
           1) /
          (std::tan(3.14159265359f * (700 + 650 * oscillator) / float(rate)) +
           1);
    }
    noise = noise * 1664525u + 1013904223u;
    float random = float(noise >> 8) / 8388608.0f - 1;
    for (int c = 0; c < 2; ++c) {
      auto k = size_t(c);
      float y = x[c];
      if (spec.family == F::Whisper) {
        float hiss = random * snapshot.rms() * .7f;
        y = (y * .45f + hiss) *
            (spec.bank == Bank::Intro ? .35f + .65f * q : .7f);
      }
      if (spec.family == F::Breath) {
        breathPower = .998f * breathPower + .002f * y * y;
        float breath = random * (std::sqrt(std::max(0.0f, breathPower)) * 2.2f +
                                 snapshot.rms() * .05f);
        y = (y * .12f + breath) *
            (spec.bank == Bank::Intro ? .2f + .8f * q : .85f);
      }
      if (spec.family == F::Hiss)
        y += random * snapshot.rms() * .35f * (1 - q);
      if (spec.family == F::Crush || spec.family == F::LoFi) {
        int hold = spec.family == F::Crush ? 3 : 5;
        if (frames % hold == 0)
          held[k] = y;
        float bits =
            16 - amount * (16 - (spec.bank == Bank::Intro ? 5 + 11 * q : 5.5f));
        float steps = std::pow(2.0f, bits);
        y = std::round(held[k] * steps) / steps;
      }
      if (spec.family == F::Distortion || spec.family == F::Radio ||
          spec.family == F::Saturation || spec.family == F::Hiss) {
        float drive =
            1 + amount * (spec.drive *
                              (spec.family == F::Distortion ? 1 + 6 * q : 2) -
                          1);
        if (spec.family == F::Radio && spec.bank == Bank::Intro)
          drive = 1 + (drive - 1) * (1 - q);
        y = std::tanh(y * drive) / std::sqrt(drive);
      }
      if (spec.family == F::Robot) {
        float robot =
            std::sin(float(frames) * 6.28318530718f * 93 / float(rate));
        float robotMix = amount * (spec.bank == Bank::Intro ? 1 - q : 1);
        y *= 1 - robotMix + robotMix * robot;
      }
      if (spec.family == F::Tremolo)
        y *= .5f + .5f * oscillator;
      if (spec.family == F::Pan || spec.family == F::Width) {
        float pan = spec.family == F::Pan
                        ? amount * (spec.panMode == 1   ? -1 + 2 * q
                                    : spec.panMode == 2 ? -.85f
                                                        : oscillator)
                        : 0;
        y *= std::sqrt(std::max(0.0f, 1 + (c ? pan : -pan)));
      }
      if (spec.family == F::Phaser) {
        float original = y;
        for (int n = 0; n < 4; ++n) {
          float z = allpass[k][size_t(n)];
          float out = phaserCoefficient * y + z;
          allpass[k][size_t(n)] = y - phaserCoefficient * out;
          y = out;
        }
        y = .5f * (original + y);
      }
      high[k] = highPole * high[k] + (1 - highPole) * y;
      y -= high[k];
      low[k] = lowPole * low[k] + (1 - lowPole) * y;
      y = low[k];
      if (spec.family == F::Air)
        y += (y - last[k]) * .8f;
      last[k] = y;
      if (spec.deess) {
        essLow[k] += .5f * (y - essLow[k]);
        float highBand = y - essLow[k];
        essEnvelope = .995f * essEnvelope + .005f * highBand * highBand;
        fullEnvelope = .995f * fullEnvelope + .005f * y * y;
        float reduction = std::clamp(
            (essEnvelope / (fullEnvelope + 1e-12f) - .12f) * 2, 0.0f, .8f);
        y -= amount * reduction * highBand;
      }
      x[c] = y;
    }
    if (spec.family == F::Chorus || spec.family == F::Flanger ||
        spec.family == F::Warble || spec.family == F::Width) {
      float processed[2]{};
      for (int c = 0; c < 2; ++c) {
        float m = std::sin(phase + (c ? 1.7f : 0));
        float delay =
            float(rate) * (spec.family == F::Flanger ? .0015f : .012f) +
            m * float(rate) * .001f * spec.depth;
        processed[c] = micro.read(delay, c);
      }
      micro.push(x[0] + processed[0] * (spec.family == F::Flanger ? .65f : 0),
                 x[1] + processed[1] * (spec.family == F::Flanger ? .65f : 0));
      for (int c = 0; c < 2; ++c)
        x[c] = spec.family == F::Warble ? processed[c]
                                        : x[c] * .55f + processed[c] * .65f;
    }
    if (spec.family == F::ReverseVerb && spec.bank == Bank::Intro) {
      x[0] = x[1] = 0;
      float sum = 0;
      int taps = spec.reverseEcho
                     ? std::max(1, std::min(8, int(float(coreEnd - start) /
                                                   delaySamples)))
                     : 32;
      for (int tap = 0; tap < taps; ++tap) {
        double offset = spec.reverseEcho
                            ? double(tap + 1) * delaySamples
                            : double(coreEnd - start) * (tap + 1) / 32;
        if (offset > double(coreEnd - start))
          offset = double(coreEnd - start);
        double at = double(now) - 2 * rate + offset;
        float gain = spec.reverseEcho ? std::pow(spec.feedback, float(tap))
                                      : reverseGain[size_t(tap)];
        if (at >= sourceStart)
          for (int c = 0; c < 2; ++c)
            x[c] += capture.read(at, c) * gain;
        sum += gain;
      }
      for (int c = 0; c < 2; ++c)
        x[c] /= std::max(1.0f, sum * .45f);
    } else if (spec.family == F::ReverseVerb) {
      for (int c = 0; c < 2; ++c)
        x[c] = read(snapshot.size() - 1 - double(frames), c) * (q * q);
      space.process(x);
    }
    if (spec.family == F::Hall || spec.family == F::Room ||
        spec.family == F::Plate || spec.family == F::Shimmer) {
      if (spec.bank == Bank::Outro && spec.family != F::Shimmer) {
        x[0] = clean[0];
        x[1] = clean[1];
      } else if (!holding() && frames >= snapshot.size()) {
        x[0] = x[1] = 0;
      }
      space.process(x);
    }
    if (spec.family == F::Echo || spec.family == F::PingPong ||
        spec.family == F::Slap) {
      bool ping = spec.family == F::PingPong;
      float taps[2]{echo.read(delaySamples, 0), echo.read(delaySamples, 1)};
      float injection[2]{x[0], x[1]};
      if (spec.bank == Bank::Outro) {
        bool lastWord =
            now >= coreEnd - int64_t(std::min(float(snapshot.size()),
                                              float(rate * .42)));
        for (int c = 0; c < 2; ++c)
          injection[c] = lastWord && now < coreEnd ? clean[c] : 0;
      } else if (frames >= snapshot.size())
        injection[0] = injection[1] = 0;
      float fb = holding() ? .995f : std::min(.85f, spec.feedback);
      echo.push(std::tanh(injection[0] + taps[ping ? 1 : 0] * fb),
                std::tanh(injection[1] + taps[ping ? 0 : 1] * fb));
      for (int c = 0; c < 2; ++c)
        x[c] = injection[c] * .35f + taps[c];
    }
    phase += float(6.28318530718 * spec.modRate / rate);
    if (phase > 6.28318530718f)
      phase -= 6.28318530718f;
    ++frames;
    energy = .995f * energy + .005f * (x[0] * x[0] + x[1] * x[1]) * .5f;
    for (int c = 0; c < 2; ++c)
      wet[c] = std::clamp(x[c], -1.5f, 1.5f) * envelope;
    return envelope;
  }

private:
  Recipe spec;
  Snapshot snapshot;
  DelayMemory echo, micro;
  Space space;
  Formant formant;
  double rate = 48000, position = 0;
  int64_t start = 0, coreEnd = 0, end = 0, sourceStart = 0, readyTime = 0;
  int sourceLength = 0, frames = 0;
  bool active = false, captured = false;
  uint32_t noise = 1;
  int64_t lastRandomStep = -1;
  float randomRatio = 1, essEnvelope = 0, fullEnvelope = 0, breathPower = 0;
  std::array<float, 2> essLow{};
  float fadeOutSamples = 2160;
  float phase = 0, beatSamples = 24000, delaySamples = 12000,
        grainLength = 3000, lowPole = 0, highPole = 0, phaserCoefficient = 0,
        energy = 0;
  std::array<float, 1024> hann{};
  std::array<float, 32> reverseGain{};
  std::array<float, 2> last{}, low{}, high{}, held{}, delayFeedback{};
  std::array<std::array<float, 4>, 2> allpass{};
};
} // namespace transition
