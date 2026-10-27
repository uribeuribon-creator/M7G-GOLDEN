#pragma once
#include "RackParameters.h"
#include <juce_dsp/juce_dsp.h>
class V8Harmony {
public:
  static constexpr int N = 2048, H = 256, B = N / 2 + 1;
  void prepare(double sr) {
    rate = sr;
    position = hop = 0;
    clock = 0;
    lastPhase.fill(0);
    seeded.fill(false);
    for (auto &a : history)
      a.fill(0);
    for (auto &a : ola)
      a.fill(0);
    for (auto &a : phase)
      a.fill(0);
    for (int i = 0; i < N; ++i)
      window[size_t(i)] = std::sqrt(
          .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i / N));
    mainBlend.reset(sr, .03);
    mainBlend.setCurrentAndTargetValue(0);
    harmonyLevel.reset(sr, .04);
    harmonyLevel.setCurrentAndTargetValue(0);
  }
  void setControls(const rack::Values &values, float ratio, float fundamental,
                   float certainty) {
    p = values;
    correction = ratio;
    f0 = fundamental;
    bool bus = int(p[rack::VoiceType]) == 5;
    mainActive =
        p[rack::TuneOn] > .5f && !bus && certainty > .78f && fundamental > 35;
    harmonyActive = p[rack::HarmonyOn] > .5f && !bus && certainty > .65f &&
                    fundamental > 35;
    mainBlend.setTargetValue(mainActive ? p[rack::PitchParallel] * .01f : 0.0f);
    harmonyLevel.setTargetValue(harmonyActive ? p[rack::HarmonyMix] * .01f
                                              : 0.0f);
  }
  void process(float *frame, int channels) {
    float delayed[2]{};
    for (int c = 0; c < 2; ++c) {
      delayed[c] = history[size_t(c)][size_t(position)];
      history[size_t(c)][size_t(position)] = frame[c < channels ? c : 0];
    }
    float mid = (delayed[0] + delayed[1]) * .5f;
    float lead = ola[0][size_t(position)];
    ola[0][size_t(position)] = 0;
    float wetL = 0, wetR = 0;
    for (int v = 0; v < 8; ++v) {
      float sample = ola[size_t(v + 1)][size_t(position)];
      ola[size_t(v + 1)][size_t(position)] = 0;
      float angle = juce::MathConstants<float>::halfPi * (v + .5f) / 8;
      wetL += sample * std::cos(angle) * .35355339f;
      wetR += sample * std::sin(angle) * .35355339f;
    }
    float blend = mainBlend.getNextValue(), level = harmonyLevel.getNextValue();
    for (int c = 0; c < channels; ++c)
      frame[c] =
          delayed[c] + blend * (lead - mid) +
          level * (channels == 1 ? (wetL + wetR) * .7071f : (c ? wetR : wetL));
    position = (position + 1) % N;
    clock += 1 / rate;
    if (++hop == H) {
      hop = 0;
      transform();
    }
  }
  void setRatio(float r) { correction = r; }
  int voices() const { return harmonyActive ? 8 : 0; }

private:
  void envelope() {
    for (int k = 0; k < N; ++k)
      temp[size_t(k)] = {std::log(juce::jmax(1e-5f, std::abs(bins[size_t(k)]))),
                         0};
    fft.perform(temp.data(), cepstrum.data(), true);
    for (int q = 31; q < N - 30; ++q)
      cepstrum[size_t(q)] = {};
    fft.perform(cepstrum.data(), temp.data(), false);
    for (int k = 0; k < B; ++k)
      env[size_t(k)] =
          std::exp(juce::jlimit(-12.0f, 12.0f, temp[size_t(k)].real()));
  }
  void transform() {
    if (!mainActive && !harmonyActive && mainBlend.getCurrentValue() < .0001f &&
        harmonyLevel.getCurrentValue() < .0001f)
      return;
    for (int i = 0; i < N; ++i) {
      int at = (position + i) % N;
      time[size_t(i)] = {(history[0][size_t(at)] + history[1][size_t(at)]) *
                             .5f * window[size_t(i)],
                         0};
    }
    fft.perform(time.data(), bins.data(), false);
    envelope();
    for (int k = 0; k < B; ++k) {
      float angle = std::arg(bins[size_t(k)]),
            expected = juce::MathConstants<float>::twoPi * k * H / N;
      float delta = std::remainder(angle - lastPhase[size_t(k)] - expected,
                                   juce::MathConstants<float>::twoPi);
      frequency[size_t(k)] =
          juce::MathConstants<float>::twoPi * k / N + delta / H;
      lastPhase[size_t(k)] = angle;
    }
    static constexpr float notes[4][8] = {{-12, 12, -24, 24, -12, 12, -24, 24},
                                          {-12, 7, 12, 19, -5, 7, 12, 19},
                                          {0, 0, 7, 7, 12, 12, -12, -12},
                                          {7, 12, 19, 24, 7, 12, 19, 24}};
    static constexpr float detune[]{-1, 1, -.7f, .7f, -.4f, .4f, -.2f, .2f};
    for (int v = 0; v < 9; ++v) {
      if (v > 0 && !harmonyActive && harmonyLevel.getCurrentValue() < .0001f)
        continue;
      float shift = correction;
      if (v > 0) {
        float semis =
            notes[juce::jlimit(0, 3, int(p[rack::HarmonyPattern]))][v - 1];
        if (p[rack::PitchKey] > 0 && p[rack::PitchScale] >= 2 && f0 > 35) {
          int root = int(p[rack::PitchKey]) - 1,
              note = juce::roundToInt(69 + 12 * std::log2(f0 / 440)),
              wanted = note + int(semis);
          constexpr bool major[]{true,  false, true,  false, true,  true,
                                 false, true,  false, true,  false, true};
          constexpr bool minor[]{true,  false, true, true,  false, true,
                                 false, true,  true, false, true,  false};
          auto allowed = [&](int n) {
            int pc = (n - root + 1200) % 12;
            return int(p[rack::PitchScale]) == 3 ? minor[pc] : major[pc];
          };
          if (!allowed(wanted)) {
            if (allowed(wanted - 1))
              --wanted;
            else if (allowed(wanted + 1))
              ++wanted;
          }
          semis = float(wanted - note);
        }
        semis += (detune[v - 1] * p[rack::HarmonyDetune] +
                  .7f * std::sin(float(clock) * (.43f + v * .07f) + v)) *
                 .01f;
        shift *= std::pow(2.0f, semis / 12);
      }
      float formant = std::pow(
          2.0f,
          (v == 0 ? p[rack::PitchFormant] : p[rack::HarmonyFormant]) / 12);
      mag.fill(0);
      omega.fill(0);
      initial.fill(0);
      for (int k = 1; k < B - 1; ++k) {
        float dest = k * shift;
        int d = int(dest);
        if (d < 1 || d >= B - 1)
          continue;
        float fraction = dest - d;
        for (int j = 0; j < 2; ++j) {
          int at = d + j;
          float eAt = juce::jlimit(0.0f, float(B - 1), at / formant);
          int a = int(eAt), b = juce::jmin(B - 1, a + 1);
          float desired =
              env[size_t(a)] + (eAt - a) * (env[size_t(b)] - env[size_t(a)]);
          float compensation =
              juce::jlimit(.15f, 6.0f, desired / (env[size_t(k)] + 1e-8f));
          float m = std::abs(bins[size_t(k)]) * compensation *
                    (j ? fraction : 1 - fraction);
          mag[size_t(at)] += m;
          omega[size_t(at)] += m * frequency[size_t(k)] * shift;
          initial[size_t(at)] = lastPhase[size_t(k)];
        }
      }
      synth.fill({});
      for (int k = 1; k < B - 1; ++k) {
        if (!seeded[size_t(v)])
          phase[size_t(v)][size_t(k)] = initial[size_t(k)];
        else if (mag[size_t(k)] > 1e-8f)
          phase[size_t(v)][size_t(k)] =
              std::remainder(phase[size_t(v)][size_t(k)] +
                                 H * omega[size_t(k)] / mag[size_t(k)],
                             juce::MathConstants<float>::twoPi);
        synth[size_t(k)] =
            std::polar(mag[size_t(k)], phase[size_t(v)][size_t(k)]);
        synth[size_t(N - k)] = std::conj(synth[size_t(k)]);
      }
      seeded[size_t(v)] = true;
      fft.perform(synth.data(), time.data(), true);
      for (int i = 0; i < N; ++i)
        ola[size_t(v)][size_t((position + i) % N)] +=
            time[size_t(i)].real() * window[size_t(i)] * .25f;
    }
  }
  rack::Values p = rack::defaults();
  double rate = 48000, clock = 0;
  int position = 0, hop = 0;
  float correction = 1, f0 = 0;
  bool mainActive = false, harmonyActive = false;
  juce::dsp::FFT fft{11};
  juce::SmoothedValue<float> mainBlend, harmonyLevel;
  std::array<std::array<float, N>, 2> history{};
  std::array<std::array<float, N>, 9> ola{};
  std::array<std::array<float, B>, 9> phase{};
  std::array<bool, 9> seeded{};
  std::array<float, N> window{};
  std::array<float, B> lastPhase{}, frequency{}, env{}, mag{}, omega{},
      initial{};
  std::array<std::complex<float>, N> time{}, bins{}, temp{}, cepstrum{},
      synth{};
};
