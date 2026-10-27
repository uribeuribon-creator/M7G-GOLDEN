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
#include "V11Automation.h"
#include "V11License.h"
#include "V11NoiseLearn.h"
#include "V11Pitch.h"
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
    return "M7G ALIEN VOICE V11 - ALIEN INTELLIGENCE";
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
  void controlTick();
  bool automatic() const { return value(rack::AlienAutomation) > .5f; }
  float effectiveValue(rack::Param k) const {
    return effective[size_t(k)].load();
  }
  void learnNoise() {
    noiseLearnRequest.store(true);
    noiseLearnState.store(1);
    noiseLearnProgress.store(0);
  }
  std::atomic<int> noiseLearnState{0};
  std::atomic<float> noiseLearnProgress{0}, pitchCorrection{0},
      rawInputDB{-100};
  std::atomic<bool> sleeping{false}, licensed{false};
  V11License license;
  juce::String intelligenceText() const;
  bool importLicense(const juce::File &file) {
    bool ok = license.importFile(file);
    licensed.store(ok);
    return ok;
  }
  bool isFavourite(int) const;
  void toggleFavourite(int);
  std::atomic<int> automationUpdates{0};
  int compareSlot() const { return activeCompare; }
  void copyCompare();
  static constexpr int analysisAll = 26;
  void beginAnalysis(int module);
  int engineMode() const { return activeMode; }
  std::atomic<float> dereverbGR{0}, resonanceGR{0}, fetGR{0}, pitchNN{0},
      producerTrim{0};
  std::atomic<int> harmonyVoices{0};
  std::atomic<float> alignOffset{0}, alignConfidence{0};
  std::atomic<bool> referencePresent{false};
  std::atomic<bool> analysisReferenceSeen{false};
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
  void timerCallback() override { controlTick(); }
  std::array<std::atomic<float>, rack::Count> effective;
  std::array<std::atomic<float>, 8> modelTargets;
  std::atomic<float> liveCrest{0}, liveBrightness{0}, learnedNoise{-100};
  std::atomic<int> manualCommit{0};
  std::atomic<bool> publishedAutomatic{false};
  std::atomic<float> publishedMakeupDB{0};
  rack::Values runtime = rack::defaults(), frozen = rack::defaults();
  bool wasAutomatic = false, runtimeReady = false;
  std::atomic<bool> runtimeReset{false};
  double quietSamples = 0;
  V11Pitch intelligentPitch;
  V11NoiseLearn noiseLearner;
  bool learning = false;
  std::atomic<bool> noiseLearnRequest{false};
  int controlClock = 0;
  mutable juce::CriticalSection favouriteLock;
  juce::StringArray favourites;
  void persistFavourites();
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
  float producerMakeupDB = 0;
  std::array<float, 8> autoTargets{};
  void configureMode(int);
  void applyAutoProducer(rack::Values &, float);
  void refreshFilters();
  void meterFFT();
  juce::ValueTree parameterState();
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M7GProcessor)
};
