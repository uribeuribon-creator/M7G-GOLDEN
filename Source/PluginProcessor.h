#pragma once
#include "AnalogColour.h"
#include "MusicalTuner.h"
#include "NeuralDenoiser.h"
#include "VocalFinisher.h"
#include <array>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
class M7GProcessor final : public juce::AudioProcessor {
public:
  M7GProcessor();
  using juce::AudioProcessor::processBlock;
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "MAKKINE VOICE 7"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 6.0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  juce::AudioProcessorValueTreeState apvts;
  std::atomic<float> inputDB{-100}, outputDB{-100}, reductionDB{0}, pitchHz{0},
      deEssDB{0};
  static const char *action(int);

private:
  using Filter = juce::dsp::IIR::Filter<float>;
  std::array<Filter, 2> low, presence, high;
  AnalogColour analogue;
  juce::dsp::Limiter<float> limiter;
  VocalFinisher finisher;
  NeuralDenoiser neural;
  MusicalTuner tuner;
  juce::AudioBuffer<float> delay, dryDelay;
  std::array<float, 2048> analysis{};
  int decimation = 1, decimationPhase = 0;
  int analysisPos = 0, analysisHop = 0, analysisCount = 0, writePos = 0,
      controlCounter = 0;
  double rate = 44100;
  float inputGainDB = 0, pitchRatio = 1, phase = 0;
  juce::SmoothedValue<float> wet, ratioSmooth, gainSmooth;
  void updateProfile(int, float);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M7GProcessor)
};
