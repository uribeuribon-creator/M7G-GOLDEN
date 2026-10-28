#pragma once
#include "../../Source/V11License.h"
#include "Engine.h"
class TuneProcessor final : public juce::AudioProcessor,
                            private juce::AsyncUpdater {
public:
  TuneProcessor();
  ~TuneProcessor() override { cancelPendingUpdate(); }
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
    return "M7G ALIEN TUNE GOD V1";
  }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 3.5; }
  int getNumPrograms() override { return 100; }
  int getCurrentProgram() override { return program.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int i) override {
    return tune::presets()[size_t(juce::jlimit(0, 99, i))].name;
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
  void setBank(int);
  int bank() const { return browserBank.load(); }
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
  juce::String profileText() const;
  juce::String keyText() const;
  juce::AudioProcessorValueTreeState state;
  V11License license;
  std::unique_ptr<tune::NeuralWorker> neural;
  std::unique_ptr<tune::Engine> engine;
  std::atomic<bool> authorized{false}, playing{false};
  std::atomic<int> activeQuality{0};
  std::atomic<float> detectedHz{0}, inputNote{0}, targetNote{0},
      correctedNote{0}, cents{0}, confidence{0}, inputDB{-100}, outputDB{-100};
  std::atomic<int> currentRoot{0}, currentScale{0};
  std::array<std::atomic<float>, 6> voiceEnergy{};
  struct Column {
    std::array<std::atomic<float>, 8> notes{};
    std::atomic<float> strength{0};
  };
  std::array<Column, 512> roll;
  std::atomic<int> noteHead{0};
  std::array<std::atomic<float>, 512> wave{};
  std::atomic<int> waveHead{0};

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  void handleAsyncUpdate() override {
    setLatencySamples(activeQuality.load() == 0 ? 2048 : 512);
  }
  juce::String parameterID(int) const;
  tune::Values parameters() const;
  std::array<std::atomic<float> *, tune::Count> raw{};
  std::atomic<int> program{0}, browserBank{0}, compareIndex{0};
  std::array<std::atomic<int>, 4> bankPrograms{};
  juce::CriticalSection compareLock;
  std::array<juce::ValueTree, 2> compare;
  std::array<int, 2> compareProgram{0, 0};
  juce::StringArray favourites;
  std::atomic<bool> resetRequested{false};
  double rate = 48000, previousSample = 0;
  int previousBlock = 0, scopeCount = 0;
  float scopePeak = 0;
  bool knownPosition = false, hostBypass = false;
};
