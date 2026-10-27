#pragma once
#include "RackParameters.h"
#include <juce_dsp/juce_dsp.h>
// Complementary TPT bands; all waveform nonlinearities run at 8x in studio.
class V8Saturation {
public:
  V8Saturation()
      : iir(4, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true, true),
        fir(4, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true, true) {}
  void prepare(double sr, int quality) {
    rate = sr;
    mode = quality;
    for (auto *o : {&iir, &fir}) {
      o->initProcessing(1);
      o->reset();
    }
    dcX.fill(0);
    dcY.fill(0);
    low.fill(0);
    high.fill(0);
    memory.fill(0);
    for (auto &a : tape)
      a.fill(0);
    setControls(rack::defaults());
  }
  void setMode(int m) {
    mode = m;
    dcX.fill(0);
    dcY.fill(0);
    low.fill(0);
    high.fill(0);
    memory.fill(0);
  }
  int latency() const {
    return mode == 0
               ? 0
               : int(std::round((mode == 2 ? fir : iir).getLatencyInSamples()));
  }
  void setControls(const rack::Values &values) {
    p = values;
    float sr = float(rate * (mode ? 8 : 1));
    gLow = std::tan(juce::MathConstants<float>::pi * p[rack::SatCrossLow] / sr);
    gLow /= 1 + gLow;
    gHigh =
        std::tan(juce::MathConstants<float>::pi * p[rack::SatCrossHigh] / sr);
    gHigh /= 1 + gHigh;
    pole = std::exp(-1 / (sr * .002f));
    dcPole = std::exp(-juce::MathConstants<float>::twoPi * 8 / sr);
    const rack::Param ids[]{rack::SatLow, rack::SatMid, rack::SatHigh};
    for (int k = 0; k < 3; ++k) {
      amounts[size_t(k)] = p[rack::SaturationOn] > .5f ? p[ids[k]] * .01f : 0;
      drives[size_t(k)] = 1 + amounts[size_t(k)] * 5;
      biases[size_t(k)] = .12f * amounts[size_t(k)];
      offsets[size_t(k)] = std::tanh(biases[size_t(k)]);
      norms[size_t(k)] = 1 / (drives[size_t(k)] *
                              (1 - offsets[size_t(k)] * offsets[size_t(k)]));
      quantizers[size_t(k)] = std::pow(2.0f, 16 - 12 * amounts[size_t(k)]);
    }
  }
  void process(float *frame, int channels, const float *eqDry = nullptr) {
    float samples[4]{frame[0], channels == 2 ? frame[1] : frame[0],
                     eqDry ? eqDry[0] : frame[0],
                     eqDry ? eqDry[channels - 1] : frame[channels - 1]};
    float *ptr[]{&samples[0], &samples[1], &samples[2], &samples[3]};
    juce::dsp::AudioBlock<float> base(ptr, 4, 1);
    auto &os = mode == 2 ? fir : iir;
    auto up = mode == 0 ? base : os.processSamplesUp(base);
    for (size_t i = 0; i < up.getNumSamples(); ++i)
      for (size_t c = 0; c < 2; ++c) {
        float wet = up.getSample(int(c), int(i)),
              dry = up.getSample(int(c + 2), int(i));
        float amount = (p[rack::EQOn] > .5f || p[rack::ResonanceOn] > .5f ||
                        p[rack::SculptorOn] > .5f)
                           ? p[rack::EQDirty] * .01f
                           : 0;
        wet += amount * (std::tanh(2.5f * wet) / 2.5f - wet);
        float x = dry + p[rack::EQParallel] * .01f * (wet - dry);
        up.setSample(int(c), int(i), shape(x, c));
      }
    if (mode != 0)
      os.processSamplesDown(base);
    for (int c = 0; c < channels; ++c)
      frame[c] = samples[c];
  }

private:
  float shape(float x, size_t c) {
    float a = (x - low[c]) * gLow, l = a + low[c];
    low[c] = l + a;
    float b = (x - high[c]) * gHigh, h = b + high[c];
    high[c] = h + b;
    float band[]{l, h - l, x - h}, result = 0;
    for (int k = 0; k < 3; ++k) {
      float amount = amounts[size_t(k)], drive = drives[size_t(k)],
            bias = biases[size_t(k)];
      float &state = tape[size_t(k)][c];
      state = pole * state + (1 - pole) * band[k];
      float wet = band[k];
      switch (int(p[rack::SatStyle])) {
      case 0:
        wet = (std::tanh(drive * band[k] + bias) - offsets[size_t(k)]) *
              norms[size_t(k)];
        break;
      case 1:
        wet =
            std::tanh(drive * (band[k] + .25f * state)) / drive - .25f * state;
        break;
      case 2:
        wet = std::atan(drive * band[k]) / drive;
        break;
      case 3:
        wet = std::tanh(drive * (band[k] + .4f * state)) / drive - .4f * state;
        break;
      case 4: {
        float steps = quantizers[size_t(k)];
        wet = std::round(band[k] * steps) / steps;
        break;
      }
      }
      result += band[k] + amount * (wet - band[k]);
    }
    result = x + p[rack::SatParallel] * .01f * (result - x);
    // Small magnetic memory after the serial compressors, inside the same 8x
    // path.
    memory[c] = pole * memory[c] + (1 - pole) * result;
    float colour = (p[rack::CompOn] > .5f || p[rack::FetOn] > .5f)
                       ? .035f * p[rack::CompParallel] * .01f
                       : 0;
    float xOut =
        result + colour * (std::tanh(result * 1.7f + memory[c] * .2f) / 1.7f -
                           result - memory[c] * .11764706f);
    float y = xOut - dcX[c] + dcPole * dcY[c];
    dcX[c] = xOut;
    dcY[c] = y;
    return y;
  }
  rack::Values p = rack::defaults();
  double rate = 48000;
  int mode = 1;
  juce::dsp::Oversampling<float> iir, fir;
  std::array<float, 2> low{}, high{}, memory{};
  std::array<float, 3> amounts{}, drives{}, biases{}, offsets{}, norms{},
      quantizers{};
  std::array<std::array<float, 2>, 3> tape{};
  std::array<float, 2> dcX{}, dcY{};
  float dcPole = 0;
  float gLow = 0, gHigh = 0, pole = 0;
};
class V8FetCompressor {
public:
  void prepare(double sr) {
    rate = sr;
    envelope = reduction = 0;
  }
  void setControls(const rack::Values &values) {
    p = values;
    attack = std::exp(
        -1 / float(rate * (p[rack::Punch] > .5f ? .15f : p[rack::FetAttack]) *
                   .001));
    release = std::exp(
        -1 / float(rate * (p[rack::Punch] > .5f ? 55.0f : p[rack::FetRelease]) *
                   .001));
  }
  void process(float *x, int channels) {
    float peak = 0;
    for (int c = 0; c < channels; ++c)
      peak = juce::jmax(peak, std::abs(x[c]));
    float a = peak > envelope ? attack : release;
    envelope = a * envelope + (1 - a) * peak;
    float level = juce::Decibels::gainToDecibels(envelope, -120.0f);
    float excess =
              level - p[rack::FetThreshold] + (p[rack::Punch] > .5f ? 6.0f : 0),
          knee = 3, factor = 1 - 1 / p[rack::FetRatio];
    float wanted =
        excess > knee * .5f
            ? excess * factor
            : (excess > -knee * .5f
                   ? factor * std::pow(excess + knee * .5f, 2) / (2 * knee)
                   : 0);
    reduction += .15f * (wanted - reduction);
    if (p[rack::FetOn] > .5f ||
        (p[rack::Punch] > .5f && p[rack::CompOn] > .5f)) {
      float gain = juce::Decibels::decibelsToGain(p[rack::FetGain] - reduction);
      for (int c = 0; c < channels; ++c)
        x[c] += p[rack::FetParallel] * .01f * (x[c] * gain - x[c]);
    }
  }
  float gainReduction() const { return p[rack::FetOn] > .5f ? reduction : 0; }

private:
  rack::Values p = rack::defaults();
  double rate = 48000;
  float envelope = 0, reduction = 0, attack = 0, release = 0;
};
