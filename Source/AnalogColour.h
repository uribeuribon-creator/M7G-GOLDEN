#pragma once
#include "RackParameters.h"
#include <juce_dsp/juce_dsp.h>
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
    warmMix.reset(sr * 4, .025);
    drive.setCurrentAndTargetValue(1);
    bias.setCurrentAndTargetValue(0);
    warmMix.setCurrentAndTargetValue(1);
    fast = slow = reduction = memory = 0;
    dcX.fill(0);
    dcY.fill(0);
    dcPole = std::exp(-2 * juce::MathConstants<float>::pi * 12 / float(sr * 4));
    for (auto &f : tone)
      f.reset();
    for (auto &f : exciter)
      f.reset();
    auto hi = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(
        sr * 4, 8000);
    for (auto &f : exciter)
      *f.coefficients = hi;
    lastTone = 999;
    shimmer = 0;
    warmOutput = 1;
    setTone(0);
  }
  int latency() const {
    return int(std::round(oversampling.getLatencyInSamples()));
  }
  void setColour(float warmth, float amount) {
    float w = juce::jlimit(0.0f, 1.0f, warmth * .01f) * amount;
    drive.setTargetValue(1 + 2.6f * w);
    bias.setTargetValue(.18f * w);
    warmMix.setTargetValue(1);
  }
  void setControls(const rack::Values &p) {
    drive.setTargetValue(1 + 2.6f * p[rack::WarmDrive] * .01f);
    bias.setTargetValue(.003f * p[rack::WarmBias]);
    warmMix.setTargetValue(p[rack::WarmOn] > .5f ? 1.0f : 0.0f);
    warmOutput = juce::Decibels::decibelsToGain(p[rack::WarmOutput]);
    setTone(p[rack::WarmTone]);
    shimmer = p[rack::AirOn] > .5f ? p[rack::AirShimmer] * .007f : 0;
  }
  void colour(float *frame, int channels) {
    float samples[2]{frame[0], channels == 2 ? frame[1] : 0};
    float *pointers[2]{&samples[0], &samples[1]};
    juce::dsp::AudioBlock<float> input(pointers, 2, 1);
    auto up = oversampling.processSamplesUp(input);
    for (size_t i = 0; i < up.getNumSamples(); ++i) {
      float d = drive.getNextValue(), b = bias.getNextValue(),
            offset = std::tanh(b), norm = 1 / (d * (1 - offset * offset)),
            wet = warmMix.getNextValue();
      for (size_t c = 0; c < 2; ++c) {
        float dry = up.getSample(int(c), int(i));
        float shaped = (std::tanh(d * dry + b) - offset) * norm;
        shaped = tone[c].processSample(shaped) * warmOutput;
        float x = dry + wet * (shaped - dry);
        float high = exciter[c].processSample(x);
        x += shimmer * (std::tanh(high * 3) / 3 - high);
        float y = x - dcX[c] + dcPole * dcY[c];
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
                bool fastMode, float recovery = 75, bool enabled = true,
                float makeup = 0) {
    float power = 0;
    for (int c = 0; c < channels; ++c)
      power += frame[c] * frame[c];
    power /= float(channels);
    float a = std::exp(-1 / float(rate * .008)),
          s = std::exp(-1 / float(rate * .15));
    fast = a * fast + (1 - a) * power;
    slow = s * slow + (1 - s) * power;
    float level =
        10 * std::log10(juce::jmax(1e-12f, juce::jmax(fast, .65f * slow)));
    float excess = level - threshold, knee = 6, factor = 1 - 1 / ratio,
          wanted = 0;
    if (excess >= knee * .5f)
      wanted = factor * excess;
    else if (excess > -knee * .5f)
      wanted = factor * std::pow(excess + knee * .5f, 2) / (2 * knee);
    float m = std::exp(-1 / float(rate * .7));
    memory = m * memory + (1 - m) * wanted;
    float seconds =
        wanted > reduction
            ? (fastMode ? .0015f : .006f)
            : (recovery * .001f + .035f * juce::jlimit(0.0f, 10.0f, memory));
    float g = std::exp(-1 / float(rate * seconds));
    reduction = g * reduction + (1 - g) * wanted;
    if (enabled) {
      float gain = juce::Decibels::decibelsToGain(makeup - reduction);
      for (int c = 0; c < channels; ++c)
        frame[c] *= gain;
    }
  }
  float gainReduction() const { return reduction; }

private:
  void setTone(float db) {
    if (db == lastTone)
      return;
    lastTone = db;
    auto coeff = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
        rate * 4, 1800, .707f, juce::Decibels::decibelsToGain(db));
    for (auto &f : tone)
      *f.coefficients = coeff;
  }
  juce::dsp::Oversampling<float> oversampling;
  juce::SmoothedValue<float> drive, bias, warmMix;
  std::array<juce::dsp::IIR::Filter<float>, 2> tone, exciter;
  std::array<float, 2> dcX{}, dcY{};
  double rate = 48000;
  float dcPole = 0, fast = 0, slow = 0, reduction = 0, memory = 0,
        lastTone = 999, warmOutput = 1, shimmer = 0;
};
