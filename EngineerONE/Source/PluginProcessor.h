#pragma once
#include "../../Source/V11License.h"
#include "ARA.h"
#include "Capture.h"

class OneProcessor final : public juce::AudioProcessor,
                           public juce::AudioProcessorARAExtension,
                           private juce::Thread,
                           private juce::Timer,
                           private juce::ARAPlaybackRegion::Listener {
public:
  OneProcessor();
  ~OneProcessor() override;
  const juce::String getName() const override {
    return "Makkine Engineer ONE - Studio One Edition";
  }
  void prepareToPlay(double, int) override;
  void releaseResources() override { releaseResourcesForARA(); }
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override {
    return "Ingeniero automatico";
  }
  void changeProgramName(int, const juce::String &) override {}
  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor *createEditor() override;
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  void capture();
  void finishCapture();
  void loadFile(const juce::File &, one::Origin = {});
  void engineer();
  void retouch(std::vector<one::Note>);
  void cancel();
  void play(bool raw = false);
  void stop();
  void seek(double);
  bool applyToEvent();
  bool araAvailable() const { return isPlaybackRenderer() || isEditorView(); }
  std::shared_ptr<const one::Input> inputSnapshot() const;
  std::shared_ptr<const one::Result> snapshot() const {
    return results.snapshot();
  }
  juce::String statusText() const;
  void setMessage(juce::String);
  one::Capture recorder;
  one::ResultStore results;
  std::atomic<bool> busy{false}, playing{false}, original{false},
      applyOnTrack{false}, mute{false}, exportStale{false};
  std::atomic<float> progress{0}, meter{0};
  std::atomic<double> playhead{0};
  bool authorized = false;

private:
  enum class Job { none, read, generate, retouch, ara, reopen };
  one::Controller *araController() const;
  void run() override;
  void timerCallback() override;
  void queue(Job);
  bool captureARA();
  void attach(juce::ARAPlaybackRegion *);
  void willDestroyPlaybackRegion(juce::ARAPlaybackRegion *) override;
  void didUpdatePlaybackRegionProperties(juce::ARAPlaybackRegion *) override;
  mutable juce::CriticalSection lock;
  std::shared_ptr<const one::Input> input;
  Job job = Job::none;
  juce::File pendingFile;
  one::Origin pendingOrigin;
  std::vector<one::Note> pendingNotes;
  juce::String message =
      "CAPTURA una linea, pulsa IA y escucha la afinacion automatica.";
  std::atomic<uint64_t> generation{0};
  uint64_t nextVersion = 0, pendingVersion = 1;
  std::shared_ptr<one::Result> completedARA;
  std::shared_ptr<const one::Result> pendingReopen;
  bool restoreAfterRead = false, restoreTrack = false;
  uint64_t restoreVersion = 1;
  std::vector<one::Note> restoreNotes;
  juce::ARAPlaybackRegion *region = nullptr;
  std::vector<std::unique_ptr<juce::ARAAudioSourceReader>> sourceReaders;
  juce::ARAAudioSourceReader *pendingReader = nullptr;
  int64_t araFirst = 0, araLength = 0;
  double araRate = 48000;
  int araChannels = 1;
  double hostRate = 48000, cursor = 0;
  float fade = 0, abFade = 1;
  bool previousRaw = false, audioRaw = false;
  std::atomic<double> seekTo{0};
  std::atomic<uint64_t> seekSerial{0};
  uint64_t seekApplied = 0, audioSerial = 0;
  const one::Result *audioResult = nullptr;
  V11License license;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OneProcessor)
};
