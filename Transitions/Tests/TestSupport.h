#pragma once
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
inline void requireTransition(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
struct TestPlayHead : juce::AudioPlayHead {
  double bpm = 120;
  int64_t sample = 0;
  double rate = 48000;
  bool playing = true;
  juce::Optional<PositionInfo> getPosition() const override {
    PositionInfo p;
    p.setBpm(bpm);
    p.setTimeInSamples(sample);
    p.setTimeInSeconds(sample / rate);
    p.setPpqPosition(sample / rate * bpm / 60);
    p.setIsPlaying(playing);
    return p;
  }
};
inline float testVoice(double t) {
  double local = -1;
  if (t >= .45 && t < 1.85)
    local = t - .45;
  else if (t >= 2.4 && t < 3.3)
    local = t - 2.4;
  else
    return 0;
  double length = t < 2 ? 1.4 : .9;
  float env = float(std::min(1.0, std::min(local, length - local) / .012));
  float v = 0;
  for (int n = 1; n <= 18; ++n) {
    float frequency = float(n * 197.0);
    float envelope = .08f +
                     .9f * std::exp(-std::pow((frequency - 800) / 320, 2)) +
                     .55f * std::exp(-std::pow((frequency - 2400) / 510, 2));
    v += std::sin(float(juce::MathConstants<double>::twoPi * frequency * t)) *
         envelope / float(n);
  }
  return env * .22f * v;
}
struct TestRender {
  std::vector<float> audio;
  double delta = 0, energy = 0;
  uint64_t hash = 1469598103934665603ull;
  int phrases = 0;
};
inline TestRender renderTransition(int preset, double sr = 48000,
                                   int block = 256, float amount = 100,
                                   float mix = 100, double bpm = 120,
                                   double seconds = 6.8) {
  auto processor = std::make_unique<TransitionsProcessor>();
  processor->setCurrentProgram(preset);
  processor->setValue("amount", amount);
  processor->setValue("mix", mix);
  processor->setRateAndBufferSizeDetails(sr, block);
  processor->prepareToPlay(sr, block);
  TestPlayHead head;
  head.bpm = bpm;
  head.rate = sr;
  processor->setPlayHead(&head);
  juce::MidiBuffer midi;
  TestRender out;
  int count = int(sr * seconds);
  out.audio.reserve(size_t(count));
  for (int start = 0; start < count; start += block) {
    int n = std::min(block, count - start);
    juce::AudioBuffer<float> b(2, n);
    for (int i = 0; i < n; ++i) {
      float x = testVoice((start + i) / sr);
      b.setSample(0, i, x);
      b.setSample(1, i, x * .93f);
    }
    head.sample = start;
    processor->processBlock(b, midi);
    for (int i = 0; i < n; ++i) {
      float x = b.getSample(0, i);
      requireTransition(std::isfinite(x) && std::abs(x) <= 1.001f,
                        "Nonfinite or excessive effect output");
      float dry = start + i >= int(sr * 2)
                      ? testVoice((start + i - int(sr * 2)) / sr)
                      : 0;
      out.delta += std::pow(double(x - dry), 2);
      out.energy += x * x;
      int quant = int(std::round(x * 1000000));
      out.hash ^= uint32_t(quant);
      out.hash *= 1099511628211ull;
      out.audio.push_back(x);
    }
  }
  out.phrases = processor->phraseCount.load();
  processor->setPlayHead(nullptr);
  return out;
}
