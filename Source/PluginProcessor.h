#pragma once
#include "AnalogColour.h"
#include "MusicalTuner.h"
#include "NeuralDenoiser.h"
#include "PresetBank.h"
#include "RackLimiter.h"
#include "SpaceProcessor.h"
#include "StereoModulation.h"
#include "V10Align.h"
#include "V10Space.h"
#include "V8Harmony.h"
#include "V8PitchDetector.h"
#include "V8Saturation.h"
#include "V8Spectral.h"
#include "VocalFinisher.h"
#include <juce_audio_utils/juce_audio_utils.h>
class M7GProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
  M7GProcessor();
  ~M7GProcessor() override { stopTimer(); }
  using juce::AudioProcessor::processBlock;
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override {
    return "M7G ALIEN VOICE V10 TRIPLE GOD ORIGIN";
  }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return tailSeconds.load(); }
  int getNumPrograms() override { return rack::presetCount; }
  int getCurrentProgram() override { return program.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int i) override {
    return rack::presets()[size_t(juce::jlimit(0, rack::presetCount - 1, i))]
        .name;
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
  static constexpr int analysisAll = 26;
  void beginAnalysis(int module);
  int engineMode() const { return activeMode; }
  std::atomic<float> dereverbGR{0}, resonanceGR{0}, fetGR{0}, pitchNN{0},
      sinoTrim{0};
  std::atomic<int> harmonyVoices{0};
  std::atomic<float> alignOffset{0}, alignConfidence{0};
  std::atomic<bool> referencePresent{false};
  float verbLayerDB(int i) const { return layerMeters[size_t(i)].load(); }
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
  void timerCallback() override { applyAnalysis(); }
  std::array<juce::dsp::IIR::Filter<float>, 2> pitchAntiAlias;
  using Filter = juce::dsp::IIR::Filter<float>;
  std::array<Filter, 2> presence, air, airCut;
  AnalogColour analogue;
  VocalFinisher cleaner, gate;
  NeuralDenoiser neural;
  MusicalTuner tuner;
  V8PitchDetector pitchDetector;
  V8Spectral roomCleaner{true}, resonance{false};
  V8Harmony leadTuner, harmony;
  V8Saturation saturation;
  V8FetCompressor fet;
  RackLimiter limiter;
  V10TripleSpace space;
  V10Align aligner;
  bool alignmentActive = false;
  juce::AudioBuffer<float> eqDryDelay, referenceScratch;
  std::array<std::atomic<float>, 3> layerMeters;
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
  std::atomic<int> requestedModule{analysisAll};
  std::atomic<bool> analysisRequest{false};
  int analysisSamples = 0;
  double analysisEnergy = 0, analysisHigh = 0;
  float analysisPeak = 0, analysisLowDB = 0, analysisLevelDB = 0,
        analysisCrest = 0, analysisHiss = 0, analysisPrevious = 0,
        analysisProcessedDB = 0;
  std::atomic<double> tailSeconds{6};
  std::atomic<int> activeMode{1};
  int neuralClock = 0;
  float previousBlockDB = -100, sinoMakeupDB = 0;
  std::array<float, 8> autoTargets{};
  void configureMode(int);
  void applySino(rack::Values &, float);
  void refreshFilters();
  void meterFFT();
  juce::ValueTree parameterState();
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M7GProcessor)
};
