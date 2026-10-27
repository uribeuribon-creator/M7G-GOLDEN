#pragma once
#include "../../Source/V11License.h"
#include "Engine.h"
#include <juce_audio_utils/juce_audio_utils.h>
class TransitionsProcessor final : public juce::AudioProcessor {
public:
  TransitionsProcessor();
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
    return "M7G ALIEN TRANSITIONS GOD V1";
  }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return tailSeconds.load(); }
  int getNumPrograms() override { return 150; }
  int getCurrentProgram() override { return program.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int i) override {
    return transition::recipes()[size_t(juce::jlimit(0, 149, i))].name;
  }
  void changeProgramName(int, const juce::String &) override {}
  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor *createEditor() override;
  juce::AudioProcessorParameter *getBypassParameter() const override {
    return state.getParameter("bypass");
  }
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  void setBank(int);
  int bank() const { return int(raw[4]->load()); }
  void setValue(const char *, float);
  float value(const char *id) const {
    return state.getRawParameterValue(id)->load();
  }
  void selectCompare(int);
  void copyCompare();
  int compareSlot() const { return compareIndex.load(); }
  bool isFavourite(int) const;
  void toggleFavourite(int);
  void stopEffects() { stopRequest.store(true); }
  bool importLicense(const juce::File &f) {
    bool result = license.importFile(f);
    authorized.store(license.authorized);
    return result;
  }
  juce::String explanation() const;
  juce::AudioProcessorValueTreeState state;
  V11License license;
  std::atomic<bool> authorized{false}, idle{true};
  std::atomic<float> inputDB{-100}, outputDB{-100}, tempo{120}, activity{0},
      progress{0}, gap{1000};
  std::atomic<int> activeVoices{0}, mode{-1}, phraseCount{0}, overflows{0};
  std::array<std::atomic<float>, 512> waveform;
  std::array<std::atomic<int>, 512> waveColour;
  std::atomic<int> waveHead{0};
  std::atomic<int64_t> lastEventStart{-1};
  transition::Engine engine;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  std::array<std::atomic<float> *, 6> raw{};
  std::array<std::atomic<int>, 3> bankPrograms;
  std::atomic<int> program{0}, compareIndex{0};
  juce::CriticalSection compareLock;
  std::array<juce::ValueTree, 2> compare;
  mutable juce::CriticalSection favouriteLock;
  juce::StringArray favourites;
  juce::ValueTree parameterState();
  std::atomic<bool> resetRequest{false}, stopRequest{false};
  std::atomic<double> tailSeconds{10};
  juce::SmoothedValue<float> amountSmooth, mixSmooth, bypassSmooth;
  double rate = 48000, previousHostSample = 0;
  int previousBlock = 0, scopeCount = 0;
  float scopePeak = 0;
  int scopeClass = 0;
  bool hostPositionKnown = false, wasPlaying = false, hostBypass = false;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransitionsProcessor)
};
