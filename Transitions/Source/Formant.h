#pragma once
#include <juce_dsp/juce_dsp.h>
namespace transition {
// Cepstral envelope transposition, with unchanged harmonic-bin phases.
// This shifts formants rather than transposing the fundamental frequency.
class Formant {
public:
  static constexpr int N = 1024, H = 128;
  void prepare() {
    for (int i = 0; i < N; ++i)
      window[size_t(i)] = std::sqrt(
          .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i / N));
    reset();
  }
  void reset() {
    position = hop = 0;
    for (auto &b : input)
      b.fill(0);
    for (auto &b : ola)
      b.fill(0);
  }
  void process(float *frame, float ratio) {
    for (int c = 0; c < 2; ++c) {
      input[size_t(c)][size_t(position)] = frame[c];
      frame[c] = ola[size_t(c)][size_t(position)];
      ola[size_t(c)][size_t(position)] = 0;
    }
    position = (position + 1) % N;
    if (++hop != H)
      return;
    hop = 0;
    for (int c = 0; c < 2; ++c) {
      for (int i = 0; i < N; ++i)
        time[size_t(i)] = {input[size_t(c)][size_t((position + i) % N)] *
                               window[size_t(i)],
                           0};
      fft.perform(time.data(), bins.data(), false);
      for (int i = 0; i < N; ++i)
        time[size_t(i)] = {std::log(std::max(1e-7f, std::abs(bins[size_t(i)]))),
                           0};
      fft.perform(time.data(), cepstrum.data(), true);
      for (int i = 25; i < N - 24; ++i)
        cepstrum[size_t(i)] = {};
      fft.perform(cepstrum.data(), time.data(), false);
      for (int i = 0; i <= N / 2; ++i)
        envelope[size_t(i)] =
            std::exp(std::clamp(time[size_t(i)].real(), -14.0f, 14.0f));
      for (int k = 0; k <= N / 2; ++k) {
        float pos = std::clamp(k / ratio, 0.0f, float(N / 2));
        int a = int(pos), b = std::min(a + 1, N / 2);
        float env = envelope[size_t(a)] +
                    (pos - a) * (envelope[size_t(b)] - envelope[size_t(a)]);
        float gain =
            std::clamp(env / (envelope[size_t(k)] + 1e-8f), .15f, 5.0f);
        bins[size_t(k)] *= gain;
        if (k > 0 && k < N / 2)
          bins[size_t(N - k)] = std::conj(bins[size_t(k)]);
      }
      fft.perform(bins.data(), time.data(), true);
      for (int i = 0; i < N; ++i)
        ola[size_t(c)][size_t((position + i) % N)] +=
            time[size_t(i)].real() * window[size_t(i)] * .25f;
    }
  }

private:
  juce::dsp::FFT fft{10};
  int position = 0, hop = 0;
  std::array<float, N> window{};
  std::array<float, N / 2 + 1> envelope{};
  std::array<std::array<float, N>, 2> input{}, ola{};
  std::array<std::complex<float>, N> time{}, bins{}, cepstrum{};
};
} // namespace transition
