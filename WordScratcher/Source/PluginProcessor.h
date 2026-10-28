#pragma once
#include "../../Source/V11License.h"
#include "PresetManager.h"
#include "VinylEngine.h"
class ScratchProcessor final : public juce::AudioProcessor,
                               private juce::AsyncUpdater {
public:
  ScratchProcessor();
  ~ScratchProcessor() override { cancelPendingUpdate(); }
  using juce::AudioProcessor::processBlock;
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  void processBlockBypassed(juce::AudioBuffer<float> &b,
                            juce::MidiBuffer &m) override {
    hostBypass = true;
    processBlock(b, m);
    hostBypass = false;
  }
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  const juce::String getName() const override {
    return "WORD SCRATCHER PRO - 300";
  }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return tail.load(); }
  int getNumPrograms() override { return 300; }
  int getCurrentProgram() override { return program.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int i) override {
    return manager.presets[size_t(juce::jlimit(0, 299, i))].name;
  }
  void changeProgramName(int, const juce::String &) override {}
  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor *createEditor() override;
  juce::AudioProcessorParameter *getBypassParameter() const override {
    return state.getParameter("bypass");
  }
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  void setValue(const juce::String &, float);
  float value(const juce::String &id) const {
    return state.getRawParameterValue(id)->load();
  }
  juce::String parameterID(int i) const;
  void captureWord() { captureRequest.store(true); }
  void playScratch() { playRequest.store(true); }
  void stopScratch() { stopRequest.store(true); }
  void cue(int i) { cueRequest.store(i); }
  void learnNoise() { learnRequest.store(true); }
  bool isFavourite(int) const;
  void toggleFavourite(int);
  void selectCompare(int);
  void copyCompare();
  int compareSlot() const { return compareIndex.load(); }
  bool importLicense(const juce::File &f) {
    bool ok = license.importFile(f);
    authorized.store(license.authorized);
    return ok;
  }
  juce::String statusText() const;
  scratch::PresetManager manager;
  juce::AudioProcessorValueTreeState state;
  V11License license;
  std::unique_ptr<scratch::VinylEngine> engine;
  std::atomic<bool> authorized{false}, hasWord{false}, scratching{false},
      noiseLearning{false};
  std::atomic<float> platterPosition{.5f}, playbackRate{0}, gate{0},
      inputDB{-100}, outputDB{-100}, activity{0}, wordMilliseconds{0},
      noiseFloor{-70};
  std::atomic<int> wordCount{0}, digitalUnderflows{0};
  std::array<std::atomic<float>, 512> wordWave{}, outputWave{};
  std::atomic<int> waveHead{0};

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  scratch::Values parameters() const;
  void handleAsyncUpdate() override {
    setValue("noiseFloor", noiseFloor.load());
  }
  std::array<std::atomic<float> *, scratch::Count> raw{};
  std::atomic<int> program{0}, compareIndex{0};
  juce::CriticalSection compareLock;
  std::array<juce::ValueTree, 2> compare;
  std::array<int, 2> compareProgram{0, 0};
  juce::StringArray favourites;
  std::atomic<bool> captureRequest{false}, playRequest{false},
      stopRequest{false}, learnRequest{false}, presetRequest{false},
      resetRequest{false};
  std::atomic<int> cueRequest{-1};
  std::atomic<double> tail{3};
  double rate = 48000, previousSample = 0;
  int previousBlock = 0, scopeCount = 0, lastSerial = 0;
  float scopePeak = 0;
  bool knownPosition = false, hostBypass = false;
};
