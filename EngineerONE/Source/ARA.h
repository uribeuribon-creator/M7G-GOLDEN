#pragma once
#include "Engineer.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace one {
class Modification final : public juce::ARAAudioModification {
public:
  Modification(juce::ARAAudioSource *source,
               ARA::ARAAudioModificationHostRef ref,
               const juce::ARAAudioModification *clone)
      : juce::ARAAudioModification(source, ref, clone) {
    if (auto *other = dynamic_cast<const Modification *>(clone))
      if (auto result = other->results.snapshot())
        results.publish(std::make_shared<Result>(*result));
  }
  ResultStore results;
  void apply(std::shared_ptr<Result> result) {
    results.publish(std::move(result));
    notifyContentChanged(juce::ARAContentUpdateScopes::samplesAreAffected(),
                         true);
    for (auto *region : getPlaybackRegions())
      region->notifyContentChanged(
          juce::ARAContentUpdateScopes::samplesAreAffected(), true);
  }
};
class Controller;
class ARARenderer final : public juce::ARAPlaybackRenderer {
public:
  ARARenderer(ARA::PlugIn::DocumentController *, Controller &);
  ~ARARenderer() override;
  void prepareToPlay(double, int, int,
                     juce::AudioProcessor::ProcessingPrecision,
                     AlwaysNonRealtime) override;
  void releaseResources() override;
  bool
  processBlock(juce::AudioBuffer<float> &, juce::AudioProcessor::Realtime,
               const juce::AudioPlayHead::PositionInfo &) noexcept override;

private:
  struct Reader {
    std::unique_ptr<juce::BufferingAudioReader> reader;
    juce::AudioBuffer<float> buffer;
    double rate = 48000;
    int channels = 2;
  };
  Controller &controller;
  double rate = 48000;
  int maximum = 512, channels = 2;
  juce::TimeSliceThread readThread{"Makkine ARA source cache"};
  std::map<juce::ARAAudioSource *, Reader> readers;
};
struct PreviewState {
  std::atomic<const void *> controlOwner{nullptr};
  ResultStore results;
  std::atomic<bool> active{false}, raw{false}, mute{false};
  std::atomic<double> position{0}, seekTo{0};
  std::atomic<uint64_t> seekSerial{0};
  std::atomic<const Modification *> modification{nullptr};
};
class EditorRenderer final : public juce::ARAEditorRenderer {
public:
  EditorRenderer(ARA::PlugIn::DocumentController *, Controller &);
  ~EditorRenderer() override;
  void prepareToPlay(double, int, int,
                     juce::AudioProcessor::ProcessingPrecision,
                     AlwaysNonRealtime) override;
  bool
  processBlock(juce::AudioBuffer<float> &, juce::AudioProcessor::Realtime,
               const juce::AudioPlayHead::PositionInfo &) noexcept override;

private:
  Controller &controller;
  double rate = 48000, cursor = 0;
  uint64_t seekApplied = 0, serial = 0;
  float fade = 0, muteFade = 1, blend = 1;
  bool raw = false, previousRaw = false;
};
inline juce::var notesJSON(const std::vector<Note> &notes) {
  juce::Array<juce::var> array;
  for (const auto &n : notes) {
    auto *o = new juce::DynamicObject;
    o->setProperty("id", n.id);
    o->setProperty("begin", n.begin);
    o->setProperty("end", n.end);
    o->setProperty("original", n.original);
    o->setProperty("target", n.target);
    o->setProperty("vibrato", n.vibrato);
    o->setProperty("gain", n.gainDb);
    o->setProperty("time", n.timeShift);
    o->setProperty("touched", n.touched);
    array.add(juce::var(o));
  }
  return juce::var(array);
}
inline std::vector<Note> readNotes(const juce::var &data) {
  std::vector<Note> notes;
  if (auto *array = data.getArray())
    for (const auto &v : *array) {
      Note n;
      n.id = int(v["id"]);
      n.begin = double(v["begin"]);
      n.end = double(v["end"]);
      n.original = float(v["original"]);
      n.target = float(v["target"]);
      n.vibrato = float(v["vibrato"]);
      n.gainDb = float(v["gain"]);
      n.timeShift = double(v["time"]);
      n.touched = bool(v["touched"]);
      if (n.id < 0 || !std::isfinite(n.begin) || !std::isfinite(n.end) ||
          !std::isfinite(n.original) || n.begin < 0 || n.end < n.begin ||
          n.end > 600 || !std::isfinite(n.target) || n.target < 20 ||
          n.target > 110 || !std::isfinite(n.gainDb) || n.gainDb < -24 ||
          n.gainDb > 18 || !std::isfinite(n.timeShift) ||
          std::abs(n.timeShift) > 600 || !std::isfinite(n.vibrato) ||
          n.vibrato < 0 || n.vibrato > 2)
        throw std::runtime_error("Estado de notas invalido");
      notes.push_back(n);
    }
  return notes;
}
class Controller final : public juce::ARADocumentControllerSpecialisation {
public:
  using juce::ARADocumentControllerSpecialisation::
      ARADocumentControllerSpecialisation;
  juce::ReadWriteLock modelLock;
  PreviewState preview;
  std::atomic<int> editorRenderers{0};

protected:
  void willBeginEditing(juce::ARADocument *) override {
    modelLock.enterWrite();
  }
  void didEndEditing(juce::ARADocument *) override { modelLock.exitWrite(); }
  juce::ARAAudioModification *doCreateAudioModification(
      juce::ARAAudioSource *source, ARA::ARAAudioModificationHostRef ref,
      const juce::ARAAudioModification *clone) noexcept override {
    return new Modification(source, ref, clone);
  }
  juce::ARAPlaybackRenderer *doCreatePlaybackRenderer() noexcept override {
    return new ARARenderer(getDocumentController(), *this);
  }
  juce::ARAEditorRenderer *doCreateEditorRenderer() noexcept override {
    return new EditorRenderer(getDocumentController(), *this);
  }
  bool doStoreObjectsToStream(
      juce::ARAOutputStream &output,
      const juce::ARAStoreObjectsFilter *filter) noexcept override {
    try {
      const auto &mods = filter->getAudioModificationsToStore<Modification>();
      if (!output.writeInt(0x4d4f4e31) || !output.writeInt(int(mods.size())))
        return false;
      for (auto *mod : mods) {
        if (!output.writeString(mod->getPersistentID().c_str()))
          return false;
        auto p = mod->results.snapshot();
        if (!output.writeBool(bool(p)))
          return false;
        if (!p)
          continue;
        auto &raw = p->input->audio;
        int n = raw.getNumSamples(), c = raw.getNumChannels();
        if (!output.writeDouble(p->input->rate) || !output.writeInt(n) ||
            !output.writeInt(c) ||
            !output.writeInt64(int64_t(p->interpretation)) ||
            !output.writeDouble(p->input->origin.seconds) ||
            !output.writeBool(p->input->origin.positionKnown) ||
            !output.writeDouble(p->input->origin.modificationStart) ||
            !output.writeString(
                juce::JSON::toString(notesJSON(p->notes), true)))
          return false;
        for (int channel = 0; channel < c; ++channel)
          if (!output.write(raw.getReadPointer(channel),
                            size_t(n) * sizeof(float)))
            return false;
        for (int channel = 0; channel < c; ++channel)
          if (!output.write(p->audio.getReadPointer(channel),
                            size_t(n) * sizeof(float)))
            return false;
      }
      return true;
    } catch (...) {
      return false;
    }
  }
  bool doRestoreObjectsFromStream(
      juce::ARAInputStream &input,
      const juce::ARARestoreObjectsFilter *filter) noexcept override {
    try {
      if (input.readInt() != 0x4d4f4e31)
        return false;
      int count = input.readInt();
      if (count < 0 || count > 256)
        return false;
      for (int i = 0; i < count; ++i) {
        auto id = input.readString();
        bool present = input.readBool();
        if (!present)
          continue;
        double sr = input.readDouble();
        int n = input.readInt(), ch = input.readInt();
        auto version = input.readInt64();
        double origin = input.readDouble();
        bool known = input.readBool();
        double offset = input.readDouble();
        auto notes = readNotes(juce::JSON::parse(input.readString()));
        if (!std::isfinite(origin) || !std::isfinite(offset) ||
            !std::isfinite(sr) || sr < 22050 || sr > 192000 || ch < 1 ||
            ch > 2 || n < 1 || n > sr * 600 ||
            uint64_t(n) * ch * 4 > 180ull * 1024 * 1024)
          return false;
        auto source = std::make_shared<Input>();
        source->rate = sr;
        source->audio.setSize(ch, n);
        source->origin.seconds = origin;
        source->origin.positionKnown = known;
        source->origin.modificationStart = offset;
        source->origin.fromARA = true;
        auto result = std::make_shared<Result>();
        result->input = source;
        result->interpretation = uint64_t(version);
        result->notes = std::move(notes);
        result->audio.setSize(ch, n);
        for (int c = 0; c < ch; ++c)
          if (input.read(source->audio.getWritePointer(c),
                         n * int(sizeof(float))) != n * int(sizeof(float)))
            return false;
        for (int c = 0; c < ch; ++c)
          if (input.read(result->audio.getWritePointer(c),
                         n * int(sizeof(float))) != n * int(sizeof(float)))
            return false;
        for (int c = 0; c < ch; ++c)
          for (int j = 0; j < n; ++j)
            if (!std::isfinite(source->audio.getSample(c, j)) ||
                !std::isfinite(result->audio.getSample(c, j)))
              return false;
        if (auto *mod =
                filter->getAudioModificationToRestoreStateWithID<Modification>(
                    id.getCharPointer())) {
          mod->results.publish(result);
          mod->notifyContentChanged(
              juce::ARAContentUpdateScopes::samplesAreAffected(), false);
          for (auto *region : mod->getPlaybackRegions())
            region->notifyContentChanged(
                juce::ARAContentUpdateScopes::samplesAreAffected(), false);
        }
      }
      return !input.failed();
    } catch (...) {
      return false;
    }
  }
};
inline EditorRenderer::EditorRenderer(ARA::PlugIn::DocumentController *doc,
                                      Controller &c)
    : juce::ARAEditorRenderer(doc), controller(c) {
  controller.editorRenderers.fetch_add(1);
}
inline EditorRenderer::~EditorRenderer() {
  controller.editorRenderers.fetch_sub(1);
}
inline void
EditorRenderer::prepareToPlay(double sr, int, int,
                              juce::AudioProcessor::ProcessingPrecision,
                              AlwaysNonRealtime) {
  rate = sr;
  cursor = 0;
  fade = 0;
}
inline bool EditorRenderer::processBlock(
    juce::AudioBuffer<float> &output, juce::AudioProcessor::Realtime,
    const juce::AudioPlayHead::PositionInfo &position) noexcept {
  if (position.getIsPlaying())
    return true;
  juce::ScopedTryReadLock model(controller.modelLock);
  if (!model.isLocked())
    return true;
  auto &preview = controller.preview;
  auto *mod = preview.modification.load();
  bool assigned = false;
  for (auto *region : getPlaybackRegions())
    assigned |= region->getAudioModification() == mod;
  for (auto *sequence : getRegionSequences())
    for (auto *region : sequence->getPlaybackRegions())
      assigned |= region->getAudioModification() == mod;
  if (!assigned || (!preview.active.load() && fade < .00001f))
    return true;
  auto *result = preview.results.acquire();
  if (!result) {
    preview.results.release();
    return true;
  }
  auto publication = preview.results.serial.load();
  auto seek = preview.seekSerial.load();
  if (publication != serial || seek != seekApplied) {
    serial = publication;
    seekApplied = seek;
    cursor = preview.seekTo.load() * result->input->rate;
    fade = 0;
  }
  bool nextRaw = preview.raw.load();
  if (nextRaw != raw) {
    previousRaw = raw;
    raw = nextRaw;
    blend = 0;
  }
  const auto &pcm = raw ? result->input->audio : result->audio;
  const auto &previous = previousRaw ? result->input->audio : result->audio;
  float coefficient = float(1 - std::exp(-1 / (rate * .004)));
  double increment = result->input->rate / rate;
  for (int i = 0; i < output.getNumSamples(); ++i) {
    bool on = preview.active.load() && cursor < pcm.getNumSamples();
    if (cursor >= pcm.getNumSamples())
      preview.active.store(false);
    fade += ((on ? 1.f : 0.f) - fade) * coefficient;
    muteFade += ((preview.mute.load() ? 0.f : 1.f) - muteFade) * coefficient;
    blend = std::min(1.f, blend + float(1 / (rate * .008)));
    for (int c = 0; c < output.getNumChannels(); ++c) {
      auto read = [&](const juce::AudioBuffer<float> &data) {
        return output.getNumChannels() == 1 && data.getNumChannels() == 2
                   ? .5f * (sample(data, 0, cursor) + sample(data, 1, cursor))
                   : sample(data, c, cursor);
      };
      float wet = (read(previous) * (1 - blend) + read(pcm) * blend) *
                  float(result->exportGain) * muteFade;
      output.setSample(c, i, output.getSample(c, i) * (1 - fade) + wet * fade);
    }
    if (on || fade > .00001f)
      cursor += increment;
  }
  preview.position.store(std::min(cursor / result->input->rate,
                                  pcm.getNumSamples() / result->input->rate));
  preview.results.release();
  return true;
}
inline ARARenderer::ARARenderer(ARA::PlugIn::DocumentController *doc,
                                Controller &owner)
    : juce::ARAPlaybackRenderer(doc), controller(owner) {
  readThread.startThread(juce::Thread::Priority::normal);
}
inline ARARenderer::~ARARenderer() {
  releaseResources();
  readThread.stopThread(-1);
}
inline void ARARenderer::releaseResources() { readers.clear(); }
inline void
ARARenderer::prepareToPlay(double sr, int blockSize, int count,
                           juce::AudioProcessor::ProcessingPrecision,
                           AlwaysNonRealtime) {
  rate = sr;
  maximum = blockSize;
  channels = count;
  readers.clear();
  for (auto *region : getPlaybackRegions()) {
    auto *source = region->getAudioModification()->getAudioSource();
    if (readers.find(source) != readers.end())
      continue;
    Reader item;
    item.rate = source->getSampleRate();
    item.channels = juce::jlimit(1, 2, int(source->getChannelCount()));
    item.reader = std::make_unique<juce::BufferingAudioReader>(
        new juce::ARAAudioSourceReader(source), readThread,
        std::max(32768, int(item.rate * 2)));
    item.buffer.setSize(item.channels,
                        std::max(8192, int(blockSize * item.rate / rate) + 16));
    item.reader->setReadTimeout(2000);
    int64_t at =
        std::max<int64_t>(0, region->getStartInAudioModificationSamples() - 4);
    item.reader->read(&item.buffer, 0,
                      std::min(1024, item.buffer.getNumSamples()), at, true,
                      true);
    item.reader->setReadTimeout(0);
    readers.emplace(source, std::move(item));
  }
}
inline bool ARARenderer::processBlock(
    juce::AudioBuffer<float> &output, juce::AudioProcessor::Realtime realtime,
    const juce::AudioPlayHead::PositionInfo &position) noexcept {
  juce::ScopedTryReadLock model(controller.modelLock);
  if (!model.isLocked()) {
    output.clear();
    return true;
  }
  auto time = position.getTimeInSamples();
  if (!time.hasValue())
    return false;
  output.clear();
  if (!position.getIsPlaying())
    return true;
  bool success = true;
  for (auto *region : getPlaybackRegions()) {
    auto *mod = region->getAudioModification<Modification>();
    auto *source = mod->getAudioSource();
    auto reader = readers.find(source);
    const Result *corrected = mod->results.acquire();
    double start = region->getStartInPlaybackTime(),
           end = region->getEndInPlaybackTime();
    double duration = region->getDurationInPlaybackTime();
    double ratio = duration > 0
                       ? region->getDurationInAudioModificationTime() / duration
                       : 1;
    for (int block = 0; block < output.getNumSamples(); block += maximum) {
      int n = std::min(maximum, output.getNumSamples() - block);
      double blockTime = (*time + block) / rate;
      if (blockTime >= end || blockTime + n / rate <= start)
        continue;
      double sourceFirst = (region->getStartInAudioModificationTime() +
                            (blockTime - start) * ratio) *
                           source->getSampleRate();
      int64_t readAt = int64_t(std::floor(sourceFirst)) - 2;
      bool ready = false;
      if (reader != readers.end()) {
        auto &item = reader->second;
        int required = int(std::ceil(n * item.rate / rate * ratio)) + 6;
        if (required <= item.buffer.getNumSamples()) {
          item.reader->setReadTimeout(
              realtime == juce::AudioProcessor::Realtime::no ? 2000 : 0);
          ready =
              item.reader->read(&item.buffer, 0, required, readAt, true, true);
        }
      }
      for (int i = 0; i < n; ++i) {
        double timeline = blockTime + i / rate;
        if (timeline < start || timeline >= end)
          continue;
        double sourceTime = region->getStartInAudioModificationTime() +
                            (timeline - start) * ratio;
        double correctedAt =
            corrected
                ? (sourceTime - corrected->input->origin.modificationStart) *
                      corrected->input->rate
                : -1;
        bool usesCorrection = corrected && correctedAt >= 0 &&
                              correctedAt < corrected->audio.getNumSamples();
        for (int c = 0; c < output.getNumChannels(); ++c) {
          float x = 0;
          if (usesCorrection) {
            x = sample(corrected->audio, c, correctedAt);
            if (output.getNumChannels() == 1 &&
                corrected->audio.getNumChannels() == 2)
              x = .5f * (sample(corrected->audio, 0, correctedAt) +
                         sample(corrected->audio, 1, correctedAt));
          } else if (ready)
            x = sample(reader->second.buffer, c,
                       sourceTime * source->getSampleRate() - readAt);
          else
            success = false;
          output.addSample(c, block + i, x);
        }
      }
    }
    mod->results.release();
  }
  return success;
}
} // namespace one
