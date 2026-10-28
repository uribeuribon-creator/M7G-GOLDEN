#pragma once
#include "../../Transitions/Source/Space.h"
#include "Analysis.h"
#include "Parameters.h"
namespace tune {
struct VoiceFX {
  void prepare(double sr) {
    rate = sr;
    verb.prepare(sr);
    verb.configure(1.35f, 7200);
    reset();
  }
  void reset() {
    low = previous = 0;
    quietSamples = 0;
    spread.fill(0);
    spreadPos = 0;
    lastColour = -1;
    verb.reset();
  }
  std::array<float, 2> process(float x, const float *p) {
    if (std::abs(x) > 1e-8f)
      quietSamples = 0;
    else if (++quietSamples > int(rate * 4))
      return {0, 0};
    float cutoff = 1400.f + std::pow(p[1] * .01f, 1.5f) * 15000.f;
    if (lastColour != p[1]) {
      lastColour = p[1];
      pole = std::exp(-6.2831853f * std::min(cutoff, float(rate * .43)) /
                      float(rate));
    }
    float a = pole;
    low = a * low + (1 - a) * x;
    float high = x - low;
    x = low + high * (.18f + p[1] * .016f);
    float bright = x - previous;
    previous += .12f * (x - previous);
    x += std::tanh(bright * 3) * p[2] * .004f;
    float drive = 1 + p[4] * .09f;
    x = std::tanh(x * drive) / drive * (1 + p[4] * .012f);
    if (lastPan != p[5]) {
      lastPan = p[5];
      float angle = (std::clamp(p[5], -1.f, 1.f) + 1) * .785398163f;
      panL = std::cos(angle);
      panR = std::sin(angle);
    }
    spread[size_t(spreadPos)] = x;
    int d1 = std::min(2047, int(rate * .0077)),
        d2 = std::min(2047, int(rate * .0131));
    float side = (spread[size_t((spreadPos - d1 + 2048) & 2047)] -
                  spread[size_t((spreadPos - d2 + 2048) & 2047)]) *
                 p[0] * .0025f;
    spreadPos = (spreadPos + 1) & 2047;
    std::array<float, 2> out{(x + side) * panL, (x - side) * panR};
    if (p[3] > .001f) {
      float v[2]{out[0], out[1]};
      verb.process(v);
      out[0] += v[0] * p[3] * .012f;
      out[1] += v[1] * p[3] * .012f;
    }
    return out;
  }
  double rate = 48000;
  float low = 0, previous = 0;
  int quietSamples = 0, spreadPos = 0;
  float lastColour = -1, pole = 0, lastPan = 99, panL = .7071f, panR = .7071f;
  std::array<float, 2048> spread{};
  transition::Space verb;
};
template <int Order> class Spectral {
public:
  static constexpr int N = 1 << Order, H = N / 8, B = N / 2 + 1,
                       DelaySize = 8192;
  void prepare(double sr) {
    rate = sr;
    for (int i = 0; i < N; ++i)
      window[size_t(i)] =
          std::sqrt(.5f - .5f * std::cos(6.28318530718f * i / N));
    for (auto &f : fx)
      f.prepare(sr);
    blend.reset(sr, .018);
    level.reset(sr, .025);
    bypassLevel.reset(sr, .02);
    for (auto &s : voiceGains)
      s.reset(sr, .025);
    reset();
  }
  void reset() {
    position = hop = postPos = 0;
    clock = 0;
    for (auto &h : history)
      h.fill(0);
    for (auto &o : ola)
      o.fill(0);
    for (auto &d : post)
      d.fill(0);
    lastPhase.fill(0);
    seeded.fill(false);
    for (auto &p : phase)
      p.fill(0);
    for (auto &f : fx)
      f.reset();
    blend.setCurrentAndTargetValue(0);
    level.setCurrentAndTargetValue(0);
    bypassLevel.setCurrentAndTargetValue(0);
    for (auto &s : voiceGains)
      s.setCurrentAndTargetValue(0);
  }
  void controls(const Values &values, const std::array<float, 7> &shift,
                bool voiced, float f0 = 220) {
    fundamental = std::max(50.f, f0);
    p = values;
    ratios = shift;
    bool correction = (p[Correction] > .001f || p[Robot] > .001f ||
                       std::abs(p[Formant]) > .001f) &&
                      voiced;
    blend.setTargetValue(correction ? 1.f : 0.f);
    level.setTargetValue((p[HarmonyOn] > .5f || p[DoublesOn] > .5f)
                             ? p[HarmonyMix] * .01f
                             : 0.f);
    hasVoice = voiced;
    for (int v = 0; v < 6; ++v)
      voiceGains[size_t(v)].setTargetValue(
          (v < 4 ? (p[HarmonyOn] > .5f && v < int(p[VoiceCount]))
                 : p[DoublesOn] > .5f)
              ? 1.f
              : 0.f);
  }
  std::array<float, 2> process(float left, float right, bool bypass) {
    float dry[2]{history[0][size_t(position)], history[1][size_t(position)]};
    history[0][size_t(position)] = left;
    history[1][size_t(position)] = right;
    float lead = ola[0][size_t(position)];
    ola[0][size_t(position)] = 0;
    float active = blend.getNextValue(), gain = level.getNextValue();
    std::array<float, 2> out{dry[0] + active * (lead - (dry[0] + dry[1]) * .5f),
                             dry[1] +
                                 active * (lead - (dry[0] + dry[1]) * .5f)};
    wet.fill(0);
    for (int v = 0; v < 6; ++v) {
      float x = ola[size_t(v + 1)][size_t(position)];
      ola[size_t(v + 1)][size_t(position)] = 0;
      post[size_t(v)][size_t(postPos)] = x;
      float enableGain = voiceGains[size_t(v)].getNextValue();
      bool enabled = enableGain > 1e-6f;
      if (enabled && gain > .00001f) {
        float human = p[DoubleHuman] * .01f;
        double ms =
            v < 4 ? 10. + (v * 7 % 21) +
                        human * 1.3 * std::sin(clock * (.51 + v * .13) + v)
                  : 10. + human * (10. +
                                   7. * std::sin(clock * (.59 + v * .07) + v));
        float d = float(ms * rate * .001), at = postPos - d;
        while (at < 0)
          at += DelaySize;
        int a = int(at) & (DelaySize - 1), b = (a + 1) & (DelaySize - 1);
        float f = at - std::floor(at);
        float sample = post[size_t(v)][size_t(a)] * (1 - f) +
                       post[size_t(v)][size_t(b)] * f;
        auto result = fx[size_t(v)].process(sample, p.data() + FirstFX + v * 6);
        float norm = (v < 4 ? .5f : .6f) * enableGain;
        out[0] += result[0] * gain * norm;
        out[1] += result[1] * gain * norm;
        wet[size_t(v)] =
            (std::abs(result[0]) + std::abs(result[1])) * .5f * gain;
      }
    }
    postPos = (postPos + 1) & (DelaySize - 1);
    position = (position + 1) & (N - 1);
    clock += 1 / rate;
    if (++hop == H) {
      hop = 0;
      transform();
    }
    bypassLevel.setTargetValue(bypass ? 1.f : 0.f);
    float by = bypassLevel.getNextValue();
    if (by > .999999f || (active < 1e-6f && gain < 1e-6f))
      return {dry[0], dry[1]};
    for (int c = 0; c < 2; ++c)
      out[size_t(c)] = out[size_t(c)] * (1 - by) + dry[c] * by;
    for (float &x : out)
      if (!std::isfinite(x))
        x = 0;
      else if (std::abs(x) > .9f)
        x = std::copysign(.9f + .1f * std::tanh((std::abs(x) - .9f) / .1f), x);
    return out;
  }
  std::array<float, 6> wet{};

private:
  void transform() {
    if (!hasVoice && blend.getCurrentValue() < .00001f &&
        level.getCurrentValue() < .00001f) {
      seeded.fill(false);
      return;
    }
    double energy = 0;
    for (int i = 0; i < N; ++i) {
      float x = (history[0][size_t((position + i) & (N - 1))] +
                 history[1][size_t((position + i) & (N - 1))]) *
                .5f;
      energy += x * x;
      time[size_t(i)] = {x * window[size_t(i)], 0};
    }
    if (energy / N < 1e-10) {
      seeded.fill(false);
      return;
    }
    fft.perform(time.data(), bins.data(), false);
    for (int k = 0; k < N; ++k)
      temp[size_t(k)] = {std::log(std::max(1e-6f, std::abs(bins[size_t(k)]))),
                         0};
    fft.perform(temp.data(), cep.data(), true);
    int lifter =
        int(std::clamp(rate / (fundamental * 2.2), 12., double(N / 8)));
    for (int q = lifter + 1; q < N - lifter; ++q)
      cep[size_t(q)] = {};
    fft.perform(cep.data(), temp.data(), false);
    for (int k = 0; k < B; ++k) {
      envelope[size_t(k)] =
          std::exp(std::clamp(temp[size_t(k)].real(), -12.f, 12.f));
      float angle = std::arg(bins[size_t(k)]),
            expected = 6.28318530718f * k * H / N;
      omega[size_t(k)] = 6.28318530718f * k / N +
                         std::remainder(angle - lastPhase[size_t(k)] - expected,
                                        6.28318530718f) /
                             H;
      lastPhase[size_t(k)] = angle;
    }
    // Nearest spectral peak for identity phase locking.
    int last = 1;
    for (int k = 1; k < B - 1; ++k) {
      if (std::abs(bins[size_t(k)]) >= std::abs(bins[size_t(k - 1)]) &&
          std::abs(bins[size_t(k)]) > std::abs(bins[size_t(k + 1)]))
        last = k;
      nearest[size_t(k)] = last;
    }
    last = B - 2;
    for (int k = B - 2; k >= 1; --k) {
      if (std::abs(bins[size_t(k)]) >= std::abs(bins[size_t(k - 1)]) &&
          std::abs(bins[size_t(k)]) > std::abs(bins[size_t(k + 1)]))
        last = k;
      if (last - k < k - nearest[size_t(k)])
        nearest[size_t(k)] = last;
    }
    for (int v = 0; v < 7; ++v) {
      bool enabled = v == 0   ? (blend.getCurrentValue() > .00001f ||
                                 blend.getTargetValue() > .00001f)
                     : v <= 4 ? (p[HarmonyOn] > .5f && v <= int(p[VoiceCount]))
                              : p[DoublesOn] > .5f;
      if (!enabled) {
        seeded[size_t(v)] = false;
        continue;
      }
      float ratio = std::clamp(ratios[size_t(v)], .25f, 4.f);
      float formant = std::exp2(
          (p[Formant] + (v ? (.16f * std::sin(float(clock) * .1f + v)) : 0)) /
          12.f);
      mag.fill(0);
      freq.fill(0);
      weight.fill(0);
      winner.fill(0);
      synth.fill({});
      for (int k = 1; k < B - 1; ++k) {
        float target = k * ratio;
        int at = int(target);
        if (at < 1 || at >= B - 1)
          continue;
        float f = target - at;
        for (int j = 0; j < 2; ++j) {
          int d = at + j;
          float e = std::clamp(d / formant, 0.f, float(B - 1));
          int a = int(e), b = std::min(B - 1, a + 1);
          float desired = envelope[size_t(a)] +
                          (e - a) * (envelope[size_t(b)] - envelope[size_t(a)]);
          float compensation =
              std::clamp(desired / (envelope[size_t(k)] + 1e-8f), .18f, 5.f);
          float m = std::abs(bins[size_t(k)]) * compensation * (j ? f : 1 - f);
          mag[size_t(d)] += m;
          freq[size_t(d)] += m * omega[size_t(k)] * ratio;
          if (m > weight[size_t(d)]) {
            weight[size_t(d)] = m;
            winner[size_t(d)] = k;
          }
        }
      }
      for (int k = 1; k < B - 1; ++k) {
        int source = winner[size_t(k)];
        if (!seeded[size_t(v)])
          phase[size_t(v)][size_t(k)] = lastPhase[size_t(source)];
        else if (mag[size_t(k)] > 1e-9f)
          phase[size_t(v)][size_t(k)] =
              std::remainder(phase[size_t(v)][size_t(k)] +
                                 H * freq[size_t(k)] / mag[size_t(k)],
                             6.28318530718f);
      }
      for (int k = 1; k < B - 1; ++k) {
        int source = winner[size_t(k)];
        float angle = phase[size_t(v)][size_t(k)];
        if (source > 0) {
          int peak = nearest[size_t(source)],
              dest = std::clamp(int(std::round(peak * ratio)), 1, B - 2);
          if (mag[size_t(dest)] > 1e-7f)
            angle = phase[size_t(v)][size_t(dest)] +
                    std::remainder(lastPhase[size_t(source)] -
                                       lastPhase[size_t(peak)],
                                   6.28318530718f);
        }
        synth[size_t(k)] = std::polar(mag[size_t(k)], angle);
        synth[size_t(N - k)] = std::conj(synth[size_t(k)]);
      }
      seeded[size_t(v)] = true;
      fft.perform(synth.data(), time.data(), true);
      for (int i = 0; i < N; ++i)
        ola[size_t(v)][size_t((position + i) & (N - 1))] +=
            time[size_t(i)].real() * window[size_t(i)] * .25f;
    }
  }
  double rate = 48000, clock = 0;
  int position = 0, hop = 0, postPos = 0;
  bool hasVoice = false;
  float fundamental = 220;
  Values p = defaults();
  std::array<float, 7> ratios{1, 1, 1, 1, 1, 1, 1};
  juce::dsp::FFT fft{Order};
  juce::SmoothedValue<float> blend, level, bypassLevel;
  std::array<juce::SmoothedValue<float>, 6> voiceGains;
  std::array<VoiceFX, 6> fx;
  std::array<std::array<float, N>, 2> history{};
  std::array<std::array<float, N>, 7> ola{};
  std::array<std::array<float, DelaySize>, 6> post{};
  std::array<std::array<float, B>, 7> phase{};
  std::array<bool, 7> seeded{};
  std::array<float, N> window{};
  std::array<float, B> lastPhase{}, omega{}, envelope{}, mag{}, freq{},
      weight{};
  std::array<int, B> nearest{}, winner{};
  std::array<std::complex<float>, N> time{}, bins{}, temp{}, cep{}, synth{};
};
} // namespace tune
