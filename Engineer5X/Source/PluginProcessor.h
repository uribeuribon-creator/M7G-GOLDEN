#pragma once
#include "../../Source/V11License.h"
#include "Render.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class EngineerProcessor final
    : public juce::AudioProcessor,
      private juce::Thread,
      private juce::Timer,
      private juce::AudioProcessorValueTreeState::Listener {
public:
  EngineerProcessor();
  ~EngineerProcessor() override;
  const juce::String getName() const override {
    return "Makkine Engineer 5X - AI Vocal Tuner Evolution";
  }
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return "Engineer 5X"; }
  void changeProgramName(int, const juce::String &) override {}
  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor *createEditor() override;
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  void loadFile(const juce::File &);
  void rebuild();
  void cancelRender();
  void play(int version);
  void stop();
  void seek(double seconds);
  void setSolo(int);
  void setMute(int, bool);
  bool isMuted(int i) const { return mute[size_t(i)].load(); }
  std::shared_ptr<const engineer::Project> snapshot() const;
  juce::String statusText() const;
  juce::String sourcePath() const;
  engineer::Settings settings() const;
  juce::AudioProcessorValueTreeState state;
  std::atomic<float> progress{0}, meter{0};
  std::atomic<bool> busy{false}, playing{false}, original{false};
  std::atomic<int> selected{0}, solo{-1};
  std::atomic<double> playhead{0};
  bool authorized = false;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  void run() override;
  void timerCallback() override;
  void parameterChanged(const juce::String &, float) override;
  void publish(std::shared_ptr<engineer::Project>);
  mutable juce::CriticalSection lock;
  std::shared_ptr<engineer::Project> owner;
  std::vector<std::shared_ptr<engineer::Project>> retired;
  std::atomic<const engineer::Project *> current{nullptr}, hazard{nullptr};
  std::array<std::atomic<bool>, 5> mute{};
  juce::File pendingFile, lastFile;
  engineer::Settings pendingSettings;
  juce::String message =
      "Carga una vocal cruda para generar las cinco versiones.";
  bool request = false;
  std::atomic<uint64_t> generation{0}, publishedSerial{0};
  std::atomic<bool> parameterDirty{false};
  std::atomic<uint32_t> parameterTime{0};
  std::atomic<double> requestedSeek{0};
  std::atomic<uint64_t> seekSerial{0};
  uint64_t appliedSeek = 0, audioSerial = 0;
  double cursor = 0, hostRate = 48000;
  float fade = 0, muteFade = 1, modeFade = 1;
  int audioVersion = 0, oldVersion = 0;
  bool audioRaw = false, oldRaw = false;
  const engineer::Project *audioProject = nullptr;
  V11License license;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerProcessor)
};
