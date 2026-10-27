#pragma once
#include "SpaceProcessor.h"
class V10SmartDelay {
public:
  V10SmartDelay()
      : iir(2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true, true),
        fir(2, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true, true) {}
  void prepare(double sr, int m) {
    rate = sr;
    mode = m;
    for (auto &b : buffer)
      b.assign(size_t(sr * 4.3) + 8, 0);
    for (auto *o : {&iir, &fir}) {
      o->initProcessing(1);
      o->reset();
    }
    write = 0;
    env = 0;
    low.fill(0);
    outputLow.fill(0);
    dc.fill(0);
    phase = 0;
    quiet = 0;
    send.reset(sr, .03);
    send.setCurrentAndTargetValue(0);
    throwGate.reset(sr, .02);
    throwGate.setCurrentAndTargetValue(0);
    setControls(rack::defaults(), 120);
  }
  void setMode(int m) { mode = m; }
  void setControls(const rack::Values &values, double bpm) {
    p = values;
    constexpr float beats[]{.25f, .5f, .75f, 1, 1.5f, 2};
    float seconds = p[rack::DelaySync] > .5f
                        ? float(60 / juce::jlimit(30.0, 300.0, bpm)) *
                              beats[juce::jlimit(0, 5, int(p[rack::DelayNote]))]
                        : p[rack::DelayTime] * .001f;
    if (int(p[rack::DelayStyle]) == 0)
      seconds = p[rack::SlapTime] * .001f;
    delay =
        juce::jlimit(2.0f, float(buffer[0].size() - 2), float(rate) * seconds);
    pole = std::exp(-juce::MathConstants<float>::twoPi *
                    juce::jmin(p[rack::DelayTone], float(rate) * .4f) /
                    float(rate));
    attack = std::exp(-1 / float(rate * .005));
    release = std::exp(-1 / float(rate * .09));
    send.setTargetValue(p[rack::DelayOn] > .5f ? p[rack::DelayMix] * .01f : 0);
  }
  void wet(const float *input, float *output, int channels) {
    float power =
        (input[0] * input[0] + input[channels - 1] * input[channels - 1]) * .5f;
    float a = power > env ? attack : release;
    env = a * env + (1 - a) * power;
    float db = juce::Decibels::gainToDecibels(std::sqrt(env + 1e-20f), -120.0f);
    bool active = db > -42;
    quiet = active ? 0 : juce::jmin(int(rate * 2), quiet + 1);
    int style = juce::jlimit(0, 4, int(p[rack::DelayStyle]));
    throwGate.setTargetValue(
        style != 1 || quiet > int(rate * p[rack::ThrowSensitivity] * .001)
            ? 1.0f
            : 0.0f);
    duck = juce::jlimit(0.0f, 1.0f, (db + 48) / 30) * p[rack::DelayDuck] * .24f;
    float gain = send.getNextValue() * throwGate.getNextValue() *
                 juce::Decibels::decibelsToGain(-duck);
    phase += juce::MathConstants<float>::twoPi * .6f / float(rate);
    if (phase > juce::MathConstants<float>::twoPi)
      phase -= juce::MathConstants<float>::twoPi;
    float wobble = style == 3 ? std::sin(phase) * float(rate) * .0003f : 0;
    float taps[2]{};
    int size = int(buffer[0].size());
    for (int c = 0; c < 2; ++c) {
      float at = float(write) - delay + wobble;
      while (at < 0)
        at += size;
      int i = int(at) % size;
      float f = at - std::floor(at);
      taps[c] = buffer[size_t(c)][size_t(i)] * (1 - f) +
                buffer[size_t(c)][size_t((i + 1) % size)] * f;
    }
    float feedback = p[rack::DelayFeedback] * .01f;
    if (style == 0)
      feedback = juce::jmin(.2f, feedback);
    bool ping = style == 2 || style == 4;
    float mono = (input[0] + input[channels - 1]) * .5f;
    float samples[2]{};
    for (int c = 0; c < 2; ++c) {
      float t = taps[ping ? 1 - c : c];
      low[size_t(c)] = pole * low[size_t(c)] + (1 - pole) * t;
      dc[size_t(c)] += .002f * (low[size_t(c)] - dc[size_t(c)]);
      samples[c] = (ping ? (c == 0 ? mono : 0) : input[c < channels ? c : 0]) +
                   feedback * (low[size_t(c)] - dc[size_t(c)]);
    }
    float *ptr[]{&samples[0], &samples[1]};
    juce::dsp::AudioBlock<float> base(ptr, 2, 1);
    auto &os = mode == 2 ? fir : iir;
    auto up = mode == 0 ? base : os.processSamplesUp(base);
    float amount = p[rack::DelayDrive] * .01f, drive = 1 + amount * 4;
    for (size_t i = 0; i < up.getNumSamples(); ++i)
      for (int c = 0; c < 2; ++c) {
        float x = up.getSample(c, int(i));
        up.setSample(c, int(i),
                     x + amount * (std::tanh(drive * x) / drive - x));
      }
    if (mode != 0)
      os.processSamplesDown(base);
    for (int c = 0; c < 2; ++c)
      buffer[size_t(c)][size_t(write)] = samples[c];
    write = (write + 1) % size;
    for (int c = 0; c < 2; ++c) {
      outputLow[size_t(c)] = pole * outputLow[size_t(c)] + (1 - pole) * taps[c];
      taps[c] = outputLow[size_t(c)];
    }
    output[0] = taps[0] * gain;
    output[1] = (channels == 1 ? (taps[0] + taps[1]) * .5f : taps[1]) * gain;
    if (channels == 1)
      output[0] = (taps[0] + taps[1]) * .5f * gain;
  }
  float reduction() const { return duck; }

private:
  rack::Values p = rack::defaults();
  double rate = 48000;
  int mode = 1, write = 0, quiet = 0;
  float env = 0, pole = 0, attack = 0, release = 0, delay = 18000, duck = 0,
        phase = 0;
  std::array<std::vector<float>, 2> buffer;
  std::array<float, 2> low{}, dc{}, outputLow{};
  juce::dsp::Oversampling<float> iir, fir;
  juce::SmoothedValue<float> send, throwGate;
};
class V10TripleSpace {
public:
  void prepare(double sr, int mode) {
    rate = sr;
    for (auto &layer : layers)
      layer.prepare(sr);
    delay.prepare(sr, mode);
    levels.fill(0);
    setControls(rack::defaults(), 120);
  }
  void setMode(int mode) { delay.setMode(mode); }
  void setControls(const rack::Values &p, double bpm) {
    settings = p;
    delay.setControls(p, bpm);
    for (int i = 0; i < 3; ++i) {
      auto v = p;
      const rack::Param mix[]{rack::VerbAMix, rack::VerbBMix, rack::VerbCMix},
          pred[]{rack::VerbAPredelay, rack::VerbBPredelay, rack::VerbCPredelay},
          decay[]{rack::VerbADecay, rack::VerbBDecay, rack::VerbCDecay},
          filter[]{rack::VerbAFilter, rack::VerbBFilter, rack::VerbCFilter},
          mod[]{rack::VerbAMod, rack::VerbBMod, rack::VerbCMod};
      float minimum[]{8, 7, 4};
      float amount = p[mix[i]];
      if (p[rack::GodSpace] > .5f)
        amount = juce::jmax(amount, minimum[i]);
      v[rack::ReverbMix] =
          juce::jlimit(0.0f, 40.0f, amount * p[rack::ReverbMix] / 12);
      v[rack::ReverbPredelay] = p[pred[i]] + p[rack::ReverbPredelay];
      v[rack::ReverbDecay] = p[decay[i]] * (p[rack::ReverbDecay] / 1.5f);
      v[rack::ReverbStyle] =
          i == 0 ? 0.0f
                 : float(i == 1 ? (int(p[rack::ReverbStyle]) + 1) % 3 : 1);
      v[rack::ReverbShimmer] =
          i == 2 ? juce::jmax(p[rack::ReverbShimmer],
                              p[rack::GodSpace] > .5f ? 30.0f : 0.0f)
                 : 0.0f;
      v[rack::DelayOn] = 0;
      layers[size_t(i)].configureLayer(p[filter[i]], p[mod[i]]);
      layers[size_t(i)].setControls(v, bpm);
    }
  }
  void process(float *frame, int channels) {
    float dry[2]{frame[0], frame[channels - 1]}, echo[2]{};
    delay.wet(dry, echo, channels);
    float excitation[2]{dry[0], dry[1]};
    if (int(settings[rack::DelayStyle]) == 4) {
      excitation[0] += echo[0];
      excitation[1] += echo[1];
    }
    float wet[2]{};
    for (int i = 0; i < 3; ++i) {
      float x[2]{excitation[0], excitation[1]};
      layers[size_t(i)].process(x, channels);
      float e = 0;
      for (int c = 0; c < channels; ++c) {
        float sample = x[c] - excitation[c];
        wet[c] += sample;
        e += sample * sample;
      }
      levels[size_t(i)] =
          .999f * levels[size_t(i)] + .001f * e / float(channels);
    }
    for (int c = 0; c < channels; ++c)
      frame[c] = dry[c] + echo[c] + wet[c];
  }
  float reverbDucking() const { return layers[0].reverbDucking(); }
  float delayDucking() const { return delay.reduction(); }
  float layerDB(int i) const {
    return juce::Decibels::gainToDecibels(std::sqrt(levels[size_t(i)]),
                                          -100.0f);
  }

private:
  double rate = 48000;
  rack::Values settings = rack::defaults();
  std::array<SpaceProcessor, 3> layers;
  V10SmartDelay delay;
  std::array<float, 3> levels{};
};
