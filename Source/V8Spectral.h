#pragma once
#include "RackParameters.h"
#include <juce_dsp/juce_dsp.h>
// Stereo-linked WOLA: one mask for both channels, bounded attenuation/boost.
class V8Spectral {
public:
  static constexpr int N = 1024, H = 256, B = N / 2 + 1;
  explicit V8Spectral(bool room) : dereverb(room) {}
  void prepare(double sr) {
    rate = sr;
    position = hop = 0;
    for (auto &a : input)
      a.fill(0);
    for (auto &a : ola)
      a.fill(0);
    late.fill(0);
    gains.fill(1);
    previous.fill(0);
    persistence.fill(0);
    for (int i = 0; i < N; ++i)
      window[size_t(i)] = std::sqrt(
          .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i / N));
    reduction = 0;
  }
  void setControls(const rack::Values &values) { p = values; }
  void setPitch(float f, float c) {
    fundamental = f;
    certainty = c;
  }
  void process(float *frame, int channels) {
    for (int c = 0; c < channels; ++c) {
      input[size_t(c)][size_t(position)] = frame[c];
      frame[c] = ola[size_t(c)][size_t(position)];
      ola[size_t(c)][size_t(position)] = 0;
    }
    position = (position + 1) % N;
    if (++hop == H) {
      hop = 0;
      transform(channels);
    }
  }
  float gainReduction() const { return reduction; }

private:
  void transform(int channels) {
    mag.fill(0);
    float maximum = 0;
    for (int c = 0; c < channels; ++c) {
      for (int i = 0; i < N; ++i)
        time[size_t(i)] = {input[size_t(c)][size_t((position + i) % N)] *
                               window[size_t(i)],
                           0};
      fft.perform(time.data(), bins[size_t(c)].data(), false);
      for (int k = 0; k < B; ++k)
        mag[size_t(k)] +=
            std::norm(bins[size_t(c)][size_t(k)]) / float(channels);
    }
    float flux = 0, total = 0;
    for (int k = 0; k < B; ++k) {
      mag[size_t(k)] = std::sqrt(mag[size_t(k)]);
      maximum = juce::jmax(maximum, mag[size_t(k)]);
      flux += juce::jmax(0.0f, mag[size_t(k)] - previous[size_t(k)]);
      total += mag[size_t(k)];
    }
    flux = juce::jlimit(0.0f, 1.0f, flux / (total + 1e-12f));
    float average = 0;
    int valid = 0;
    float sculptMean = 0, sculptWeight = 0;
    if (!dereverb && p[rack::SculptorOn] > .5f) {
      logPrefix[0] = 0;
      for (int k = 0; k < B; ++k)
        logPrefix[size_t(k + 1)] =
            logPrefix[size_t(k)] + std::log(juce::jmax(1e-7f, mag[size_t(k)]));
      for (int k = 1; k < B; ++k) {
        float center = float(k) * float(rate) / N;
        int radius = juce::jmax(3, int(k * .23f)),
            lo = juce::jmax(1, k - radius), hi = juce::jmin(B - 1, k + radius);
        broad[size_t(k)] = 20 / std::log(10.0f) *
                           (logPrefix[size_t(hi + 1)] - logPrefix[size_t(lo)]) /
                           float(hi - lo + 1);
        if (center > 150 && center < 12000) {
          average += broad[size_t(k)];
          ++valid;
        }
      }
      average /= juce::jmax(1, valid);
      for (int k = 1; k < B; ++k) {
        float freq = float(k) * float(rate) / N;
        auto bell = [&](float center, float width) {
          float x = std::log2(juce::jmax(1.0f, freq) / center) / width;
          return std::exp(-.5f * x * x);
        };
        float shape =
            p[rack::SculptorTilt] * std::log2(juce::jmax(80.0f, freq) / 1000) +
            p[rack::SculptorBody] * bell(320, .75f) +
            p[rack::SculptorPresence] * bell(3000, .7f) +
            p[rack::SculptorAir] * bell(11000, .8f);
        sculptDB[size_t(k)] =
            juce::jlimit(-8.0f, 3.0f, shape - (broad[size_t(k)] - average));
        if (freq > 80 && freq < 18000) {
          float weight = mag[size_t(k)] * mag[size_t(k)];
          sculptMean += weight * sculptDB[size_t(k)];
          sculptWeight += weight;
        }
      }
      sculptMean /= sculptWeight + 1e-20f;
    }
    float maxReduction = 0;
    float decay = std::exp(-H / float(rate * p[rack::DereverbTail] * .001));
    for (int k = 0; k < B; ++k) {
      float magnitude = mag[size_t(k)], target = 1,
            freq = float(k) * float(rate) / N;
      if (dereverb && p[rack::DereverbOn] > .5f) {
        float tail = late[size_t(k)],
              onset = juce::jlimit(0.0f, 1.0f,
                                   (magnitude - tail) / (magnitude + 1e-8f));
        float protect = 1 - p[rack::TransientProtect] * .01f * onset;
        float fraction = juce::jlimit(0.0f, .9f, tail / (magnitude + 1e-8f));
        target = 1 + p[rack::RoomParallel] * .01f *
                         (std::sqrt(juce::jmax(
                              .04f, 1 - p[rack::DereverbAmount] * .009f *
                                            fraction * protect)) -
                          1);
      }
      if (!dereverb && p[rack::ResonanceOn] > .5f && k > 3) {
        float local = 0;
        int count = 0,
            radius = juce::jlimit(3, 18, int(16 / p[rack::ResonanceFocus]));
        for (int j = juce::jmax(1, k - radius);
             j <= juce::jmin(B - 1, k + radius); ++j)
          if (std::abs(j - k) > 1) {
            local += mag[size_t(j)];
            ++count;
          }
        local /= juce::jmax(1, count);
        float excess = juce::jmax(
            0.0f, 20 * std::log10((magnitude + 1e-8f) / (local + 1e-8f)) - 8);
        persistence[size_t(k)] += .15f * (excess - persistence[size_t(k)]);
        float protection = 1;
        if (fundamental > 35 && certainty > .8f) {
          float harmonic = freq / fundamental;
          float distance = std::abs(harmonic - std::round(harmonic));
          if (distance < juce::jmin(.24f, 1.4f * float(rate) / N / fundamental))
            protection -= p[rack::HarmonicProtect] * .0095f;
        }
        float transient = 1 - p[rack::TransientProtect] * .01f * flux;
        float db =
            -juce::jmin(14.0f, excess * p[rack::ResonanceAmount] * .01f) *
            protection * transient *
            (.4f + .6f * juce::jlimit(0.0f, 1.0f, persistence[size_t(k)] / 8));
        target *= 1 + p[rack::ResonanceParallel] * .01f *
                          (juce::Decibels::decibelsToGain(db) - 1);
      }
      if (!dereverb && p[rack::SculptorOn] > .5f && freq > 80 && freq < 18000 &&
          magnitude > maximum * .0001f) {
        float wanted =
            juce::jlimit(-8.0f, 3.0f, sculptDB[size_t(k)] - sculptMean);
        float transient = 1 - p[rack::SculptorProtect] * .01f * flux;
        target *=
            1 + p[rack::SculptorParallel] * .01f *
                    (juce::Decibels::decibelsToGain(
                         wanted * p[rack::SculptorAmount] * .01f * transient) -
                     1);
      }
      if (!dereverb && int(p[rack::EngineMode]) == 2 && freq > 0) {
        float db = 0;
        if (p[rack::EQOn] > .5f) {
          float x = std::log2(freq / p[rack::EQFreq]) * p[rack::EQQ] / .65f;
          db += p[rack::EQGain] * std::exp(-.5f * x * x);
        }
        if (p[rack::AirOn] > .5f) {
          float ratio = 10000 / freq;
          db += (p[rack::AirAmount] * .045f + p[rack::AirGain]) /
                (1 + ratio * ratio * ratio * ratio);
          if (freq > p[rack::AirFilter])
            db -= 24 * std::log2(freq / p[rack::AirFilter]);
          db *= p[rack::AirParallel] * .01f;
        }
        target *= juce::Decibels::decibelsToGain(db);
      }
      late[size_t(k)] = juce::jmax(magnitude, decay * late[size_t(k)]);
      previous[size_t(k)] = magnitude;
      gains[size_t(k)] += (target < gains[size_t(k)] ? .35f : .07f) *
                          (target - gains[size_t(k)]);
      maxReduction =
          juce::jmax(maxReduction,
                     -juce::Decibels::gainToDecibels(gains[size_t(k)], -80.0f));
      for (int c = 0; c < channels; ++c) {
        bins[size_t(c)][size_t(k)] *= gains[size_t(k)];
        if (k > 0 && k < N / 2)
          bins[size_t(c)][size_t(N - k)] =
              std::conj(bins[size_t(c)][size_t(k)]);
      }
    }
    reduction = .8f * reduction + .2f * maxReduction;
    for (int c = 0; c < channels; ++c) {
      fft.perform(bins[size_t(c)].data(), time.data(), true);
      for (int i = 0; i < N; ++i)
        ola[size_t(c)][size_t((position + i) % N)] +=
            time[size_t(i)].real() * window[size_t(i)] * .5f;
    }
  }
  bool dereverb;
  double rate = 48000;
  int position = 0, hop = 0;
  float reduction = 0, fundamental = 0, certainty = 0;
  rack::Values p = rack::defaults();
  juce::dsp::FFT fft{10};
  std::array<float, N> window{};
  std::array<std::array<float, N>, 2> input{}, ola{};
  std::array<float, B + 1> logPrefix{};
  std::array<float, B> sculptDB{};
  std::array<float, B> late{}, gains{}, mag{}, previous{}, persistence{},
      broad{};
  std::array<std::complex<float>, N> time{};
  std::array<std::array<std::complex<float>, N>, 2> bins{};
};
