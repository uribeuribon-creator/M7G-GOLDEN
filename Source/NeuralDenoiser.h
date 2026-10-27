#pragma once
#include <array>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <mutex>
#include <rnnoise.h>

// Fixed-size RNNoise frames at 48 kHz, independent of host block boundaries.
// The default pretrained model is embedded; inference never uses the network.
class NeuralDenoiser {
public:
  void prepare(double sr) {
    static std::once_flag warmed;
    std::call_once(warmed, [] {
      State warm(rnnoise_create(nullptr), rnnoise_destroy);
      std::array<float, 480> silence{};
      rnnoise_process_frame(warm.get(), silence.data(), silence.data());
    });
    rate = sr;
    inPhase = outPhase = 0;
    fill = read = write = 0;
    frames = 0;
    previousIn.fill(0);
    previousOut.fill(0);
    for (auto &channel : queue)
      channel.fill(0);
    for (auto &channel : original)
      channel.fill(0);
    originalWrite = 0;
    count = int(std::ceil(rate * .01));
    write = count;
    const auto aa = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
        sr, juce::jmin(18000.0, sr * .42), .707f);
    for (size_t c = 0; c < 2; ++c) {
      state[c].reset(rnnoise_create(nullptr));
      if (!state[c])
        throw std::bad_alloc();
      antialias[c].reset();
      *antialias[c].coefficients = aa;
      input[c].fill(0);
      output[c].fill(0);
    }
  }
  int latency() const { return 2 * int(std::ceil(rate * .01)); }
  size_t framesProcessed() const { return frames; }
  void processFrame(float *samples, int channels, float strength = .65f) {
    std::array<float, 2> current{};
    for (int c = 0; c < channels; ++c) {
      auto k = static_cast<size_t>(c);
      original[k][static_cast<size_t>(originalWrite)] = samples[c];
      current[k] = antialias[k].processSample(samples[c]);
    }
    const double step = 48000 / rate;
    const double oldPhase = inPhase;
    inPhase += step;
    int event = 1;
    while (inPhase >= 1) {
      const float fraction =
          juce::jlimit(0.0f, 1.0f, float((double(event) - oldPhase) / step));
      for (size_t c = 0; c < 2; ++c)
        input[c][static_cast<size_t>(fill)] =
            (previousIn[c] + fraction * (current[c] - previousIn[c])) * 32768;
      ++fill;
      inPhase -= 1;
      ++event;
      if (fill == 480) {
        for (int c = 0; c < channels; ++c)
          rnnoise_process_frame(state[static_cast<size_t>(c)].get(),
                                output[static_cast<size_t>(c)].data(),
                                input[static_cast<size_t>(c)].data());
        ++frames;
        fill = 0;
        for (size_t i = 0; i < 480; ++i) {
          const double outStep = rate / 48000, oldOutPhase = outPhase;
          outPhase += outStep;
          int outEvent = 1;
          while (outPhase >= 1) {
            const float frac = juce::jlimit(
                0.0f, 1.0f, float((double(outEvent) - oldOutPhase) / outStep));
            for (size_t c = 0; c < 2; ++c) {
              const float sample = output[c][i] / 32768;
              queue[c][static_cast<size_t>(write)] =
                  previousOut[c] + frac * (sample - previousOut[c]);
            }
            write = (write + 1) % 8192;
            ++count;
            outPhase -= 1;
            ++outEvent;
          }
          for (size_t c = 0; c < 2; ++c)
            previousOut[c] = output[c][i] / 32768;
        }
      }
    }
    previousIn = current;
    const int aligned = (originalWrite + 16384 - latency()) % 16384;
    for (int c = 0; c < channels; ++c) {
      const auto k = static_cast<size_t>(c);
      const float filtered =
          count > 0 ? queue[k][static_cast<size_t>(read)] : 0;
      // Retain a dry residual to protect quiet syllables and sung overtones.
      samples[c] = strength * filtered +
                   (1 - strength) * original[k][static_cast<size_t>(aligned)];
    }
    if (count > 0) {
      read = (read + 1) % 8192;
      --count;
    }
    originalWrite = (originalWrite + 1) % 16384;
  }

private:
  using State = std::unique_ptr<DenoiseState, decltype(&rnnoise_destroy)>;
  std::array<State, 2> state{
      {State(nullptr, rnnoise_destroy), State(nullptr, rnnoise_destroy)}};
  std::array<juce::dsp::IIR::Filter<float>, 2> antialias;
  std::array<std::array<float, 480>, 2> input{}, output{};
  std::array<std::array<float, 8192>, 2> queue{};
  std::array<std::array<float, 16384>, 2> original{};
  std::array<float, 2> previousIn{}, previousOut{};
  double rate = 48000, inPhase = 0, outPhase = 0;
  int fill = 0, read = 0, write = 0, count = 0, originalWrite = 0;
  size_t frames = 0;
};
