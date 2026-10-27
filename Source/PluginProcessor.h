#pragma once
#include "AnalogColour.h"
#include "MusicalTuner.h"
#include "NeuralDenoiser.h"
#include "PresetBank.h"
#include "RackLimiter.h"
#include "SpaceProcessor.h"
#include "StereoModulation.h"
#include "VocalFinisher.h"
#include <juce_audio_utils/juce_audio_utils.h>
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
  const juce::String getName() const override { return "M7G ALIEN VOICE"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return tailSeconds.load(); }
  int getNumPrograms() override { return 50; }
  int getCurrentProgram() override { return program.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int i) override {
    return rack::presets()[size_t(juce::jlimit(0, 49, i))].name;
  }
  void changeProgramName(int, const juce::String &) override {}
  juce::AudioProcessorParameter *getBypassParameter() const override {
    return apvts.getParameter("bypass");
  }
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  void setValue(rack::Param, float);
  float value(rack::Param) const;
  void selectCompare(int);
  int compareSlot() const { return activeCompare; }
  void copyCompare();
  void beginAnalysis(int module);
  bool applyAnalysis();
  int analysisTarget() const { return requestedModule.load(); }
  std::atomic<int> analysisState{0};
  std::atomic<float> analysisProgress{0};
  juce::AudioProcessorValueTreeState apvts;
  std::atomic<float> inputDB{-100}, outputDB{-100}, reductionDB{0}, pitchHz{0},
      targetHz{0}, deEssDB{0}, gateGR{0}, limitGR{0}, revDuckDB{0},
      delDuckDB{0}, correlation{1}, modPhase{0};
  std::array<std::atomic<float>, 32> spectrum;
  std::array<std::atomic<float>, 2> outputChannels;
  static const char *action(int);

private:
  using Filter = juce::dsp::IIR::Filter<float>;
  std::array<Filter, 2> presence, air, airCut;
  AnalogColour analogue;
  VocalFinisher cleaner;
  NeuralDenoiser neural;
  MusicalTuner tuner;
  RackLimiter limiter;
  SpaceProcessor space;
  StereoModulation stereo;
  std::array<std::atomic<float> *, rack::Count> parameter{};
  rack::Values settings = rack::defaults(), lastSettings = rack::defaults();
  juce::AudioBuffer<float> pitchDelay, dryDelay;
  std::array<float, 2048> pitchWindow{};
  std::array<float, 1024> fftData{};
  juce::dsp::FFT fft{9};
  int pitchPos = 0, pitchCount = 0, pitchHop = 0, decimation = 2,
      decimatePos = 0, write = 0, fftPos = 0, filterClock = 0;
  double rate = 48000;
  float phase = 0, autoGainDB = 0;
  juce::SmoothedValue<float> ratioSmooth, mixSmooth, inputSmooth, outputSmooth,
      bypassSmooth;
  std::atomic<int> program{0};
  std::atomic<int> activeCompare{0};
  juce::CriticalSection compareLock;
  std::array<juce::ValueTree, 2> compare;
  std::atomic<int> requestedModule{12};
  std::atomic<bool> analysisRequest{false};
  int analysisSamples = 0;
  double analysisEnergy = 0, analysisHigh = 0;
  float analysisPeak = 0, analysisLowDB = 0, analysisLevelDB = 0,
        analysisCrest = 0, analysisHiss = 0, analysisPrevious = 0,
        analysisProcessedDB = 0;
  std::atomic<double> tailSeconds{6};
  void refreshFilters();
  void meterFFT();
  juce::ValueTree parameterState();
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M7GProcessor)
};
