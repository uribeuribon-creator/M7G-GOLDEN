#pragma once
#include <array>
#include <juce_dsp/juce_dsp.h>

// Original colour/dynamics algorithms; no claim of circuit-level hardware
// emulation.
class AnalogColour {
public:
  AnalogColour()
      : oversampling(2, 2,
                     juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                     true, true) {}
  void prepare(double sr) {
    rate = sr;
    oversampling.initProcessing(1);
    oversampling.reset();
    drive.reset(sr * 4, .04);
    bias.reset(sr * 4, .04);
    drive.setCurrentAndTargetValue(1);
    bias.setCurrentAndTargetValue(0);
    fast = slow = reduction = memory = 0;
    dcX.fill(0);
    dcY.fill(0);
    dcPole = std::exp(-2 * juce::MathConstants<float>::pi * 12 / float(sr * 4));
  }
  int latency() const {
    return int(std::round(oversampling.getLatencyInSamples()));
  }
  void setColour(float warmth, float amount) {
    const float w = juce::jlimit(0.0f, 1.0f, warmth * .01f) * amount;
    drive.setTargetValue(1 + 2.6f * w);
    bias.setTargetValue(.18f * w);
  }
  void colour(float *frame, int channels) {
    float samples[2]{frame[0], channels == 2 ? frame[1] : 0};
    float *pointers[2]{&samples[0], &samples[1]};
    juce::dsp::AudioBlock<float> input(pointers, 2, 1);
    auto up = oversampling.processSamplesUp(input);
    for (size_t i = 0; i < up.getNumSamples(); ++i) {
      const float d = drive.getNextValue(), b = bias.getNextValue(),
                  offset = std::tanh(b);
      const float normalization = 1 / (d * (1 - offset * offset));
      for (size_t c = 0; c < 2; ++c) {
        float x = up.getSample(int(c), int(i));
        // Bias creates controlled even-order content. Normalize small-signal
        // gain.
        x = (std::tanh(d * x + b) - offset) * normalization;
        const float y = x - dcX[c] + dcPole * dcY[c];
        dcX[c] = x;
        dcY[c] = y;
        up.setSample(int(c), int(i), y);
      }
    }
    oversampling.processSamplesDown(input);
    for (int c = 0; c < channels; ++c)
      frame[c] = samples[c];
  }
  void compress(float *frame, int channels, float threshold, float ratio,
                bool fastMode) {
    float power = 0;
    for (int c = 0; c < channels; ++c)
      power += frame[c] * frame[c];
    power /= float(channels);
    const float a = std::exp(-1 / float(rate * .008)),
                s = std::exp(-1 / float(rate * .15));
    fast = a * fast + (1 - a) * power;
    slow = s * slow + (1 - s) * power;
    const float level =
        10 * std::log10(juce::jmax(1.0e-12f, juce::jmax(fast, .65f * slow)));
    const float excess = level - threshold, knee = 6, factor = 1 - 1 / ratio;
    float wanted = 0;
    if (excess >= knee * .5f)
      wanted = factor * excess;
    else if (excess > -knee * .5f)
      wanted = factor * std::pow(excess + knee * .5f, 2) / (2 * knee);
    const float m = std::exp(-1 / float(rate * .7));
    memory = m * memory + (1 - m) * wanted;
    const float seconds =
        wanted > reduction
            ? (fastMode ? .0015f : .006f)
            : (.075f + .045f * juce::jlimit(0.0f, 10.0f, memory));
    const float g = std::exp(-1 / float(rate * seconds));
    reduction = g * reduction + (1 - g) * wanted;
    const float gain = juce::Decibels::decibelsToGain(-reduction);
    for (int c = 0; c < channels; ++c)
      frame[c] *= gain;
  }

private:
  juce::dsp::Oversampling<float> oversampling;
  juce::SmoothedValue<float> drive, bias;
  std::array<float, 2> dcX{}, dcY{};
  double rate = 48000;
  float dcPole = 0, fast = 0, slow = 0, reduction = 0, memory = 0;
};
