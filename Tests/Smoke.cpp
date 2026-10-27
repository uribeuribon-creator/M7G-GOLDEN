#include "../Source/PluginProcessor.h"
#include <iostream>
int main() {
  M7GProcessor p;
  juce::MidiBuffer midi;
  for (double sr : {44100.0, 48000.0, 96000.0})
    for (int size : {64, 256, 1024}) {
      p.prepareToPlay(sr, size);
      for (int voice = 0; voice < 6; ++voice) {
        p.apvts.getParameter("voicetype")
            ->setValueNotifyingHost(float(voice) / 5);
        double energy = 0;
        for (int block = 0; block < 100; ++block) {
          juce::AudioBuffer<float> b(2, size);
          for (int c = 0; c < 2; ++c)
            for (int i = 0; i < size; ++i)
              b.setSample(
                  c, i,
                  .15f * std::sin(float(2 * juce::MathConstants<double>::pi *
                                        223 * (block * size + i) / sr)));
          p.processBlock(b, midi);
          for (int c = 0; c < 2; ++c)
            for (int i = 0; i < size; ++i) {
              float x = b.getSample(c, i);
              if (!std::isfinite(x) || std::abs(x) > .892f)
                return 1;
              energy += x * x;
            }
        }
        if (energy < .001)
          return 2;
      }
      const int tailBlocks = int(sr * 7 / size) + 1;
      for (int block = 0; block < tailBlocks; ++block) {
        juce::AudioBuffer<float> b(2, size);
        b.clear();
        p.processBlock(b, midi);
        if (block == tailBlocks - 1 && b.getRMSLevel(0, 0, size) > .0001f)
          return 3;
      }
    }

  // Exercise actual recurrent-model inference, not only the heuristic gate.
  for (double neuralRate : {44100.0, 48000.0, 96000.0}) {
    NeuralDenoiser denoiser;
    denoiser.prepare(neuralRate);
    uint32_t randomState = 1;
    double noiseIn = 0, noiseOut = 0;
    const int samples = int(neuralRate * 8);
    for (int i = 0; i < samples; ++i) {
      randomState = randomState * 1664525u + 1013904223u;
      float x = (float(randomState >> 8) / 16777216.0f - .5f) * .02f;
      float frame[2]{x, x};
      denoiser.processFrame(frame, 2);
      if (!std::isfinite(frame[0]))
        return 10;
      if (i > int(neuralRate * 5)) {
        noiseIn += x * x;
        noiseOut += frame[0] * frame[0];
      }
    }
    const double reduction = 10 * std::log10(noiseOut / noiseIn);
    std::cout << "RNNoise " << neuralRate
              << " Hz: " << denoiser.framesProcessed()
              << " frames; noise reduction " << reduction << " dB\n";
    if (denoiser.framesProcessed() < 799 || reduction > -3)
      return 11;
  }

  // Independent measurements of the newly added cleanup stages.
  auto cleanupRms = [](float frequency, float amplitude) {
    VocalFinisher f;
    f.prepare(48000, 256);
    double energy = 0;
    for (int i = 0; i < 48000; ++i) {
      float frame[2]{amplitude *
                         std::sin(float(2 * juce::MathConstants<double>::pi *
                                        frequency * i / 48000)),
                     0};
      frame[1] = frame[0];
      f.cleanFrame(frame, 2);
      if (i >= 24000)
        energy += frame[0] * frame[0];
    }
    return std::sqrt(energy / 24000);
  };
  const double cleanSpeech = cleanupRms(1000, .1f),
               cleanEss = cleanupRms(8000, .1f);
  const double essDb = 20 * std::log10(cleanEss / cleanSpeech);
  const double quietDb =
      20 * std::log10(cleanupRms(1000, .0001f) / (.0001 / std::sqrt(2.0)));
  std::cout << "Cleanup: high-band reduction " << essDb
            << " dB; quiet-signal reduction " << quietDb << " dB\n";
  if (essDb > -3 || quietDb > -6)
    return 7;
  // Native mono layout and variable host block lengths, including one sample.
  M7GProcessor mono;
  auto layout = mono.getBusesLayout();
  layout.inputBuses.set(0, juce::AudioChannelSet::mono());
  layout.outputBuses.set(0, juce::AudioChannelSet::mono());
  if (!mono.setBusesLayout(layout))
    return 8;
  mono.prepareToPlay(48000, 256);
  for (int k = 0; k < 500; ++k) {
    const int count = k % 2 ? 1 : 256;
    juce::AudioBuffer<float> b(1, count);
    for (int i = 0; i < count; ++i)
      b.setSample(0, i, .1f);
    mono.processBlock(b, midi);
    for (int i = 0; i < count; ++i)
      if (!std::isfinite(b.getSample(0, i)))
        return 9;
  }

  // Character-stage spectral checks and soft-knee transient behaviour.
  AnalogColour colour;
  colour.prepare(48000);
  colour.setColour(100, 1);
  double ar = 0, ai = 0, nr = 0, ni = 0;
  for (int i = 0; i < 48000; ++i) {
    const float x =
        .7f *
        std::sin(float(2 * juce::MathConstants<double>::pi * 5000 * i / 48000));
    float frame[2]{x, x};
    colour.colour(frame, 2);
    if (i >= 24000) {
      double a = 2 * juce::MathConstants<double>::pi * 23000 * i / 48000;
      double naive = (std::tanh(3.6 * x + .18) - std::tanh(.18)) /
                     (3.6 * (1 - std::pow(std::tanh(.18), 2)));
      ar += frame[0] * std::cos(a);
      ai += frame[0] * std::sin(a);
      nr += naive * std::cos(a);
      ni += naive * std::sin(a);
    }
  }
  const double aliasImprovement =
      10 * std::log10((nr * nr + ni * ni) / (ar * ar + ai * ai + 1e-25));
  std::cout << "4x colour: alias reduction at 23 kHz " << aliasImprovement
            << " dB versus direct waveshaping\n";
  if (aliasImprovement < 12)
    return 12;
  colour.prepare(48000);
  float first = 0, last = 0;
  for (int i = 0; i < 48000; ++i) {
    float frame[2]{.5f, .5f};
    colour.compress(frame, 2, -18, 4, false);
    if (i == 0)
      first = frame[0];
    last = frame[0];
  }
  if (last >= first * .75f || last <= 0 || first > .501f)
    return 13;
  MusicalTuner musical;
  musical.reset();
  const float a440 = musical.target(443, .99f, .025f);
  if (std::abs(a440 - 440) > .1f || musical.hasScale())
    return 14;
  if (musical.target(440, .3f, .025f) != 0)
    return 15;
  // Polyphonic bus keeps all three fundamental notes.
  M7GProcessor bus;
  bus.apvts.getParameter("voicetype")->setValueNotifyingHost(1);
  bus.apvts.getParameter("intensity")->setValueNotifyingHost(1);
  bus.prepareToPlay(48000, 256);
  double chordPower[3]{};
  double chordRe[3]{}, chordIm[3]{};
  const double notes[]{220, 277.182631, 329.627557};
  for (int block = 0; block < 400; ++block) {
    juce::AudioBuffer<float> b(2, 256);
    for (int i = 0; i < 256; ++i) {
      float x = 0;
      for (auto f : notes)
        x += .07f * std::sin(float(2 * juce::MathConstants<double>::pi * f *
                                   (block * 256 + i) / 48000));
      b.setSample(0, i, x);
      b.setSample(1, i, x);
    }
    bus.processBlock(b, midi);
    if (block >= 200)
      for (int i = 0; i < 256; ++i)
        for (int k = 0; k < 3; ++k) {
          double a = 2 * juce::MathConstants<double>::pi * notes[k] *
                     (block * 256 + i) / 48000;
          chordRe[k] += b.getSample(0, i) * std::cos(a);
          chordIm[k] += b.getSample(0, i) * std::sin(a);
        }
  }
  for (int k = 0; k < 3; ++k) {
    chordPower[k] = chordRe[k] * chordRe[k] + chordIm[k] * chordIm[k];
    if (chordPower[k] < 100)
      return 16;
  }
  std::cout << "Polyphonic bus: all three input fundamentals preserved\n";

  // Verify tuning on a sustained detuned note after detector settling.
  p.apvts.getParameter("voicetype")->setValueNotifyingHost(.6f);
  p.apvts.getParameter("intensity")->setValueNotifyingHost(1);
  p.prepareToPlay(48000, 256);
  std::vector<float> rendered;
  for (int block = 0; block < 600; ++block) {
    juce::AudioBuffer<float> b(2, 256);
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < 256; ++i)
        b.setSample(c, i,
                    .1f * std::sin(float(2 * juce::MathConstants<double>::pi *
                                         223 * (block * 256 + i) / 48000)));
    p.processBlock(b, midi);
    if (block >= 400)
      for (int i = 0; i < 256; ++i)
        rendered.push_back(b.getSample(0, i));
  }
  if (std::abs(p.pitchHz.load() - 223) > 3)
    return 5;
  double strongest = 0, bestFrequency = 0;
  for (double f = 217; f <= 226; f += .1) {
    double re = 0, im = 0;
    for (size_t i = 0; i < rendered.size(); ++i) {
      double a = 2 * juce::MathConstants<double>::pi * f * double(i) / 48000;
      double w = .5 - .5 * std::cos(2 * juce::MathConstants<double>::pi *
                                    double(i) / double(rendered.size()));
      re += rendered[i] * std::cos(a) * w;
      im += rendered[i] * std::sin(a) * w;
    }
    double power = re * re + im * im;
    if (power > strongest) {
      strongest = power;
      bestFrequency = f;
    }
  }
  std::cout << "Tuning: input 223 Hz; output peak " << bestFrequency
            << " Hz (target 220 Hz)\n";
  if (std::abs(bestFrequency - 220) >= 2.0)
    return 6;
  p.apvts.getParameter("intensity")->setValueNotifyingHost(.42f);
  juce::MemoryBlock state;
  p.getStateInformation(state);
  M7GProcessor restored;
  restored.setStateInformation(state.getData(), int(state.getSize()));
  if (std::abs(*restored.apvts.getRawParameterValue("intensity") - 42) > .01f)
    return 4;
  std::cout << "PASS: 54 profile/rate/block combinations; finite output, "
               "ceiling, signal, silence, state round trip\n";
}
