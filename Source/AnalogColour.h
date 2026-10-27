#pragma once
#include "RackParameters.h"
#include <juce_dsp/juce_dsp.h>
class AnalogColour {
public:
  AnalogColour()
      : oversampling(2, 3,
                     juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                     true, true),
        linear(2, 3,
               juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
               true, true) {}
  void prepare(double sr, int quality = 1) {
    mode = quality;
    legacy = false;
    iron.fill(0);
    loaded.fill(0);
    rate = sr;
    compFastPole = std::exp(-1 / float(sr * .008));
    compSlowPole = std::exp(-1 / float(sr * .15));
    compMemoryPole = std::exp(-1 / float(sr * .7));
    oversampling.initProcessing(1);
    oversampling.reset();
    linear.initProcessing(1);
    linear.reset();
    drive.reset(sr * (mode ? 8 : 1), .04);
    bias.reset(sr * (mode ? 8 : 1), .04);
    warmMix.reset(sr * (mode ? 8 : 1), .025);
    drive.setCurrentAndTargetValue(1);
    bias.setCurrentAndTargetValue(0);
    warmMix.setCurrentAndTargetValue(1);
    fast = slow = reduction = memory = 0;
    dcX.fill(0);
    dcY.fill(0);
    dcPole = std::exp(-2 * juce::MathConstants<float>::pi * 12 /
                      float(sr * (mode ? 8 : 1)));
    for (auto &f : tone)
      f.reset();
    for (auto &f : exciter)
      f.reset();
    auto hi = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(
        sr * (mode ? 8 : 1), 8000);
    for (auto &f : exciter)
      *f.coefficients = hi;
    lastTone = 999;
    shimmer = 0;
    warmOutput = 1;
    setTone(0);
  }
  int latency() const {
    return mode == 0 ? 0
                     : int(std::round((mode == 2 ? linear : oversampling)
                                          .getLatencyInSamples()));
  }
  void setMode(int quality) {
    mode = quality;
    lastTone = 999;
  }
  void setColour(float warmth, float amount) {
    legacy = true;
    float w = juce::jlimit(0.0f, 1.0f, warmth * .01f) * amount;
    drive.setTargetValue(1 + 2.6f * w);
    bias.setTargetValue(.18f * w);
    warmMix.setTargetValue(1);
  }
  void setControls(const rack::Values &p) {
    legacy = false;
    style = juce::jlimit(0, 5, int(p[rack::PreampStyle]));
    constexpr float slopes[]{1, 1.08f, 1.1f, .88f, .93f, 1.18f};
    constexpr float cutoffs[]{18000, 21000, 22000, 23000, 16000, 13500};
    float z = p[rack::PreampImpedance];
    inputGain =
        juce::Decibels::decibelsToGain(p[rack::PreampGain] * slopes[style] -
                                       (p[rack::PreampPad] > .5f ? 20.0f : 0));
    load = 1.09375f * z / (z + 150);
    rail = p[rack::PhantomModel] > .5f ? 1.15f : .8f;
    impedancePole =
        std::exp(-juce::MathConstants<float>::twoPi * cutoffs[style] *
                 std::sqrt(z / (z + 350)) / float(rate * (mode ? 8 : 1)));
    constexpr float recovery[]{.002f, .0004f, .0003f, .001f, .003f, .004f};
    magneticPole =
        std::exp(-1 / float(rate * (mode ? 8 : 1) * recovery[style]));
    magneticLoad = juce::jlimit(.6f, 2.0f, 1 + .12f * (1600 / z - 1));
    drive.setTargetValue(1 + 2.6f * p[rack::WarmDrive] * .01f);
    bias.setTargetValue(.003f * p[rack::WarmBias]);
    warmMix.setTargetValue(p[rack::WarmOn] > .5f ? p[rack::WarmParallel] * .01f
                                                 : 0.0f);
    warmOutput = juce::Decibels::decibelsToGain(p[rack::WarmOutput]);
    setTone(p[rack::WarmTone]);
    shimmer = p[rack::AirOn] > .5f ? p[rack::AirShimmer] * .007f : 0;
  }
  void colour(float *frame, int channels) {
    float samples[2]{frame[0], channels == 2 ? frame[1] : 0};
    float *pointers[2]{&samples[0], &samples[1]};
    juce::dsp::AudioBlock<float> input(pointers, 2, 1);
    auto &selected = mode == 2 ? linear : oversampling;
    auto up = mode == 0 ? input : selected.processSamplesUp(input);
    for (size_t i = 0; i < up.getNumSamples(); ++i) {
      float d = drive.getNextValue(), b = bias.getNextValue(),
            offset = std::tanh(b), norm = 1 / (d * (1 - offset * offset)),
            wet = warmMix.getNextValue();
      for (size_t c = 0; c < 2; ++c) {
        float dry = up.getSample(int(c), int(i));
        float shaped = 0;
        if (legacy)
          shaped = (std::tanh(d * dry + b) - offset) * norm;
        else {
          constexpr float extra[]{1.25f, 1.6f, 1.1f, 1, 1.4f, 2};
          constexpr float biases[]{.7f, .2f, .35f, 1.1f, 1.5f, 1.9f};
          constexpr float magnetic[]{.15f, .06f, .02f, .03f, .08f, .3f};
          float x = dry * inputGain * load;
          loaded[c] = impedancePole * loaded[c] + (1 - impedancePole) * x;
          x = loaded[c] / rail;
          iron[c] = magneticPole * iron[c] + (1 - magneticPole) * x;
          float k = d * extra[style], bb = b * biases[style], o = std::tanh(bb),
                mag = magnetic[style] * magneticLoad;
          if (style == 3 || style == 4) {
            float y = x;
            for (int iteration = 0; iteration < 5; ++iteration) {
              float t = std::tanh(k * y + bb);
              y -= (y + .18f * (t - o) - x) / (1 + .18f * k * (1 - t * t));
            }
            shaped = y * (1 + .18f * k * (1 - o * o));
          } else
            shaped = (std::tanh(k * (x + mag * iron[c]) + bb) - o) /
                         (k * (1 - o * o)) -
                     mag * iron[c];
          shaped *= rail;
        }
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
    if (mode != 0)
      selected.processSamplesDown(input);
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
    float a = compFastPole, s = compSlowPole;
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
    float m = compMemoryPole;
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
        rate * (mode ? 8 : 1), 1800, .707f, juce::Decibels::decibelsToGain(db));
    for (auto &f : tone)
      *f.coefficients = coeff;
  }
  juce::dsp::Oversampling<float> oversampling, linear;
  int mode = 1, style = 0;
  bool legacy = false;
  std::array<float, 2> iron{}, loaded{};
  float inputGain = 1, load = 1, rail = 1, impedancePole = 0, magneticLoad = 1,
        magneticPole = 0, compFastPole = 0, compSlowPole = 0,
        compMemoryPole = 0;
  juce::SmoothedValue<float> drive, bias, warmMix;
  std::array<juce::dsp::IIR::Filter<float>, 2> tone, exciter;
  std::array<float, 2> dcX{}, dcY{};
  double rate = 48000;
  float dcPole = 0, fast = 0, slow = 0, reduction = 0, memory = 0,
        lastTone = 999, warmOutput = 1, shimmer = 0;
};
