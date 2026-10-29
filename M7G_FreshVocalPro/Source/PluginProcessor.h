#pragma once
#include "../../Source/V11License.h"
#include "DSP/Chain.h"
#include "DSP/PresetManager.h"
#include <juce_audio_utils/juce_audio_utils.h>
class FreshProcessor final
    : public juce::AudioProcessor,
      private juce::Timer,
      private juce::AudioProcessorValueTreeState::Listener {
public:
  FreshProcessor();
  ~FreshProcessor() override;
  const juce::String getName() const override {
    return "M7G Fresh Vocal Pro Beast";
  }
  void prepareToPlay(double, int) override;
  void releaseResources() override { chain.reset(); }
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  void processBlockBypassed(juce::AudioBuffer<float> &,
                            juce::MidiBuffer &) override;
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 12; }
  int getNumPrograms() override {
    return int(fresh::PresetManager::factory().size());
  }
  int getCurrentProgram() override { return selectedPreset.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int i) override {
    return fresh::PresetManager::factory()[size_t(juce::jlimit(
                                               0, getNumPrograms() - 1, i))]
        .name;
  }
  void changeProgramName(int, const juce::String &) override {}
  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor *createEditor() override;
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  juce::AudioProcessorParameter *getBypassParameter() const override {
    return state.getParameter("bypass");
  }
  fresh::Values settings() const;
  void set(fresh::Param, float);
  void apply(const fresh::Values &, juce::String, bool morph = true);
  void choosePreset(int, bool morph = true);
  void switchAB();
  bool loadPreset(const juce::File &);
  bool savePreset(const juce::File &);
  void setEQStyle(int);
  void reorder(int from, int to);
  fresh::Meters &meters();
  juce::String presetName() const;
  juce::UndoManager undo;
  juce::AudioProcessorValueTreeState state;
  std::atomic<int> viewMode{0}, selectedTab{0}, selectedPreset{3};
  std::atomic<bool> bankB{false}, morphing{false};
  bool authorized = false;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  void parameterChanged(const juce::String &, float) override;
  void timerCallback() override;
  void process(juce::AudioBuffer<float> &, juce::MidiBuffer &, bool);
  std::array<std::atomic<float> *, fresh::count> raw{};
  fresh::Chain chain;
  double rate = 48000;
  int maximum = 512;
  juce::CriticalSection nameLock;
  juce::String currentName = "Default";
  fresh::Values morphFrom{}, morphTo{}, a{}, b{};
  double morphStart = 0;
  std::atomic<bool> morphWriting{false};
  V11License license;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreshProcessor)
};
