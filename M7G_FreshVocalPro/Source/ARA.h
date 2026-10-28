#pragma once
#include "DSP/Chain.h"
#include <juce_audio_utils/juce_audio_utils.h>
namespace fresh {
class Modification final : public juce::ARAAudioModification {
public:
  Modification(juce::ARAAudioSource *s, ARA::ARAAudioModificationHostRef r,
               const juce::ARAAudioModification *clone)
      : juce::ARAAudioModification(s, r, clone) {
    if (auto *m = dynamic_cast<const Modification *>(clone)) {
      parameters.store(m->parameters.load());
      initialised = m->initialised;
    }
  }
  Mailbox parameters;
  bool initialised = false;
  void apply(const Values &p) {
    parameters.store(p);
    initialised = true;
    notifyContentChanged(juce::ARAContentUpdateScopes::samplesAreAffected(),
                         true);
    for (auto *r : getPlaybackRegions())
      r->notifyContentChanged(
          juce::ARAContentUpdateScopes::samplesAreAffected(), true);
  }
};
class Controller;
class Renderer final : public juce::ARAPlaybackRenderer {
public:
  std::atomic<bool> hostBypass{false};
  Renderer(ARA::PlugIn::DocumentController *, Controller &);
  ~Renderer() override;
  void prepareToPlay(double, int, int,
                     juce::AudioProcessor::ProcessingPrecision,
                     AlwaysNonRealtime) override;
  void releaseResources() override;
  bool
  processBlock(juce::AudioBuffer<float> &, juce::AudioProcessor::Realtime,
               const juce::AudioPlayHead::PositionInfo &) noexcept override;

private:
  struct Source {
    std::unique_ptr<juce::BufferingAudioReader> reader;
    juce::AudioBuffer<float> pcm;
    double rate = 48000;
    std::vector<std::array<float, 64>> sinc;
  };
  struct Voice {
    std::unique_ptr<Chain> chain;
    juce::AudioBuffer<float> audio;
  };
  Controller &controller;
  std::map<juce::ARAAudioSource *, Source> readers;
  std::map<juce::ARAPlaybackRegion *, Voice> voices;
  juce::TimeSliceThread thread{"Fresh Vocal ARA read-ahead"};
  double rate = 48000;
  int maximum = 512, channels = 2;
  int64_t expected = std::numeric_limits<int64_t>::min();
};
class Controller final : public juce::ARADocumentControllerSpecialisation {
public:
  using juce::ARADocumentControllerSpecialisation::
      ARADocumentControllerSpecialisation;
  juce::ReadWriteLock modelLock;
  Meters meters;

protected:
  void willBeginEditing(juce::ARADocument *) override {
    modelLock.enterWrite();
  }
  void didEndEditing(juce::ARADocument *) override { modelLock.exitWrite(); }
  juce::ARAAudioModification *doCreateAudioModification(
      juce::ARAAudioSource *s, ARA::ARAAudioModificationHostRef r,
      const juce::ARAAudioModification *clone) noexcept override {
    return new Modification(s, r, clone);
  }
  juce::ARAPlaybackRenderer *doCreatePlaybackRenderer() noexcept override {
    return new Renderer(getDocumentController(), *this);
  }
  void doGetPlaybackRegionHeadAndTailTime(
      const ARA::PlugIn::PlaybackRegion *, ARA::ARATimeDuration *head,
      ARA::ARATimeDuration *tail) noexcept override {
    if (head)
      *head = 0;
    if (tail)
      *tail = 12;
  }
  bool doStoreObjectsToStream(
      juce::ARAOutputStream &out,
      const juce::ARAStoreObjectsFilter *filter) noexcept override {
    const auto &mods = filter->getAudioModificationsToStore<Modification>();
    if (!out.writeInt(0x46565031) || !out.writeInt(int(mods.size())))
      return false;
    for (auto *mod : mods) {
      if (!out.writeString(mod->getPersistentID().c_str()))
        return false;
      auto values = mod->parameters.load();
      for (auto x : values)
        if (!out.writeFloat(x))
          return false;
    }
    return true;
  }
  bool doRestoreObjectsFromStream(
      juce::ARAInputStream &in,
      const juce::ARARestoreObjectsFilter *filter) noexcept override {
    if (in.readInt() != 0x46565031)
      return false;
    int n = in.readInt();
    if (n < 0 || n > 4096)
      return false;
    for (int j = 0; j < n; ++j) {
      auto id = in.readString();
      Values v{};
      for (auto &x : v) {
        x = in.readFloat();
        if (!std::isfinite(x))
          return false;
      }
      if (auto *mod =
              filter->getAudioModificationToRestoreStateWithID<Modification>(
                  id.getCharPointer())) {
        mod->parameters.store(sanitise(v));
        mod->initialised = true;
        mod->notifyContentChanged(
            juce::ARAContentUpdateScopes::samplesAreAffected(), false);
      }
    }
    return !in.failed();
  }
};
} // namespace fresh
