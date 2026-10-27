#pragma once
#include <juce_dsp/juce_dsp.h>

// Linked stereo dynamics and tempo-aware ambience; all storage is allocated in
// prepare().
class VocalFinisher {
public:
  void prepare(double sampleRate, int maxBlock) {
    rate = sampleRate;
    scratch.setSize(2, juce::jmax(1, maxBlock));
    echo.setSize(2, int(rate * 2.1) + 4);
    echo.clear();
    write = 0;
    fullEnv = essEnv = bodyEnv = harshEnv = essReduction = 0;
    noise = 0.0005f;
    noiseGain = 1;
    attack = std::exp(-1.0f / float(rate * .002));
    release = std::exp(-1.0f / float(rate * .09));
    slow = std::exp(-1.0f / float(rate * .5));
    reverb.setSampleRate(rate);
    reverb.reset();
    for (auto *bank : {&highpass, &sibilance, &body, &harsh})
      for (auto &f : *bank)
        f.reset();
    const auto hp =
        juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(rate, 75, .707f);
    const auto ss =
        juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(
            rate, juce::jmin(5500.0, rate * .35));
    const auto mud =
        juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(rate, 420, 1.4f);
    const auto edge = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(
        rate, juce::jmin(2900.0, rate * .3), 2.0f);
    for (size_t c = 0; c < 2; ++c) {
      *highpass[c].coefficients = hp;
      *sibilance[c].coefficients = ss;
      *body[c].coefficients = mud;
      *harsh[c].coefficients = edge;
    }
    ambienceMix.reset(rate, .1);
    ambienceMix.setCurrentAndTargetValue(0);
  }

  // Called before colour/compression. Dynamics use one detector for both
  // channels.
  void cleanFrame(float *frame, int channels) {
    float sum = 0, ess = 0, mud = 0, edge = 0;
    float highs[2]{}, bodies[2]{}, edges[2]{};
    for (int c = 0; c < channels; ++c) {
      const auto k = static_cast<size_t>(c);
      frame[c] = highpass[k].processSample(frame[c]);
      highs[c] = sibilance[k].processSample(frame[c]);
      bodies[c] = body[k].processSample(frame[c]);
      edges[c] = harsh[k].processSample(frame[c]);
      sum += frame[c] * frame[c];
      ess += highs[c] * highs[c];
      mud += bodies[c] * bodies[c];
      edge += edges[c] * edges[c];
    }
    auto follow = [&](float &env, float power) {
      power /= float(channels);
      const float a = power > env ? attack : release;
      env = a * env + (1 - a) * power;
    };
    follow(fullEnv, sum);
    follow(essEnv, ess);
    follow(bodyEnv, mud);
    follow(harshEnv, edge);
    const float level = std::sqrt(fullEnv + 1.0e-20f);
    // Learn only during quiet passages; never learn voiced signal as noise.
    if (level < .004f)
      noise = slow * noise + (1 - slow) * juce::jlimit(.00008f, .0015f, level);
    const float desired =
        juce::jlimit(.04f, 1.0f, level / (noise * 2 + .00001f));
    const float gate = desired * desired;
    const float a = gate > noiseGain ? attack : release;
    noiseGain = a * noiseGain + (1 - a) * gate;
    const float essRatio = std::sqrt(essEnv / (fullEnv + 1.0e-12f));
    const float s = juce::jlimit(0.0f, .7f, (essRatio - .26f) * 1.2f) *
                    juce::jlimit(0.0f, 1.0f, std::sqrt(essEnv) / .018f);
    const float m =
        juce::jlimit(0.0f, .4f, (bodyEnv / (fullEnv + 1.0e-12f) - .25f) * .9f);
    const float h =
        juce::jlimit(0.0f, .4f, (harshEnv / (fullEnv + 1.0e-12f) - .18f) * .8f);
    for (int c = 0; c < channels; ++c)
      frame[c] =
          noiseGain * (frame[c] - s * highs[c] - m * bodies[c] - h * edges[c]);
    essReduction = -juce::Decibels::gainToDecibels(juce::jmax(.1f, 1 - s));
  }

  void ambience(juce::AudioBuffer<float> &audio, int voice, float intensity,
                double bpm) {
    static constexpr float rooms[]{.065f, .09f, .11f, .17f, .14f, .035f};
    static constexpr float echoes[]{.035f, .07f, .04f, .16f, .065f, .015f};
    juce::Reverb::Parameters p;
    p.roomSize = voice == 3 || voice == 4 ? .65f : .4f;
    p.damping = .65f;
    p.wetLevel = 1;
    p.dryLevel = 0;
    p.width = 1;
    p.freezeMode = 0;
    reverb.setParameters(p);
    ambienceMix.setTargetValue(intensity);
    const int channels = audio.getNumChannels();
    const int echoSize = echo.getNumSamples();
    const int leftTime = juce::jlimit(
        1, echoSize - 2, int(rate * 60 / juce::jlimit(40.0, 240.0, bpm) * .5));
    const int rightTime =
        juce::jlimit(1, echoSize - 2, int(float(leftTime) * 1.5f));
    for (int offset = 0; offset < audio.getNumSamples();) {
      const int count =
          juce::jmin(scratch.getNumSamples(), audio.getNumSamples() - offset);
      for (int c = 0; c < channels; ++c)
        scratch.copyFrom(c, 0, audio, c, offset, count);
      if (channels == 2)
        reverb.processStereo(scratch.getWritePointer(0),
                             scratch.getWritePointer(1), count);
      else
        reverb.processMono(scratch.getWritePointer(0), count);
      for (int i = 0; i < count; ++i) {
        const float mix = ambienceMix.getNextValue();
        const float duck = 1.0f / (1 + 5 * std::sqrt(fullEnv));
        const float taps[2]{
            echo.getSample(0, (write + echoSize - leftTime) % echoSize),
            echo.getSample(1, (write + echoSize - rightTime) % echoSize)};
        for (int c = 0; c < channels; ++c) {
          const float dry = audio.getSample(c, offset + i);
          echo.setSample(c, write,
                         dry + taps[channels == 2 ? 1 - c : 0] * .23f);
          audio.setSample(c, offset + i,
                          dry + mix * duck *
                                    (rooms[voice] * scratch.getSample(c, i) +
                                     echoes[voice] * taps[c]));
        }
        write = (write + 1) % echoSize;
      }
      offset += count;
    }
  }
  float getDeEssReduction() const { return essReduction; }

private:
  using Filter = juce::dsp::IIR::Filter<float>;
  std::array<Filter, 2> highpass, sibilance, body, harsh;
  juce::Reverb reverb;
  juce::AudioBuffer<float> scratch, echo;
  juce::SmoothedValue<float> ambienceMix;
  double rate = 48000;
  int write = 0;
  float fullEnv = 0, essEnv = 0, bodyEnv = 0, harshEnv = 0;
  float attack = 0, release = 0, slow = 0, noise = .0005f, noiseGain = 1,
        essReduction = 0;
};
