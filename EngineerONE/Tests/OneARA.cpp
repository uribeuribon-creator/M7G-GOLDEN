#include "Support.h"
#include <ARA_Library/Dispatch/ARAHostDispatch.cpp>
#include <ARA_Library/Dispatch/ARAHostDispatch.h>
extern const ARA::ARAFactory *JUCE_CALLTYPE createARAFactory();
using namespace ARA;
struct AudioHost final : Host::AudioAccessControllerInterface {
  juce::AudioBuffer<float> pcm = vocal(48000, 1);
  ARAAudioReaderHostRef
  createAudioReaderForSource(ARAAudioSourceHostRef,
                             bool wide) noexcept override {
    return wide ? nullptr : reinterpret_cast<ARAAudioReaderHostRef>(this);
  }
  bool readAudioSamples(ARAAudioReaderHostRef, ARASamplePosition first,
                        ARASampleCount n,
                        void *const channels[]) noexcept override {
    for (int64_t i = 0; i < n; ++i)
      static_cast<float *>(channels[0])[i] =
          first + i >= 0 && first + i < pcm.getNumSamples()
              ? pcm.getSample(0, int(first + i))
              : 0;
    return true;
  }
  void destroyAudioReader(ARAAudioReaderHostRef) noexcept override {}
};
struct ArchiveHost final : Host::ArchivingControllerInterface {
  std::vector<ARAByte> bytes;
  const char *id = nullptr;
  ARASize getArchiveSize(ARAArchiveReaderHostRef) noexcept override {
    return bytes.size();
  }
  bool readBytesFromArchive(ARAArchiveReaderHostRef, ARASize first, ARASize n,
                            ARAByte out[]) noexcept override {
    if (first + n > bytes.size())
      return false;
    std::memcpy(out, bytes.data() + first, size_t(n));
    return true;
  }
  bool writeBytesToArchive(ARAArchiveWriterHostRef, ARASize first, ARASize n,
                           const ARAByte data[]) noexcept override {
    try {
      if (first + n > bytes.size())
        bytes.resize(size_t(first + n));
      std::memcpy(bytes.data() + first, data, size_t(n));
      return true;
    } catch (...) {
      return false;
    }
  }
  void notifyDocumentArchivingProgress(float) noexcept override {}
  void notifyDocumentUnarchivingProgress(float) noexcept override {}
  ARAPersistentID
  getDocumentArchiveID(ARAArchiveReaderHostRef) noexcept override {
    return id;
  }
};
struct ContentHost final : Host::ContentAccessControllerInterface {
  ARAContentTempoEntry tempos[2]{{0, 0}, {100, 200}};
  ARAContentKeySignature key{};
  ContentHost() {
    key.root = 0;
    for (int i = 0; i < 12; ++i)
      key.intervals[i] = one::allowed(i, 2, 0) ? kARAKeySignatureIntervalUsed
                                               : kARAKeySignatureIntervalUnused;
    key.position = 0;
  }
  bool isMusicalContextContentAvailable(ARAMusicalContextHostRef,
                                        ARAContentType type) noexcept override {
    return type == kARAContentTypeKeySignatures ||
           type == kARAContentTypeTempoEntries;
  }
  ARAContentGrade
  getMusicalContextContentGrade(ARAMusicalContextHostRef,
                                ARAContentType) noexcept override {
    return kARAContentGradeAdjusted;
  }
  ARAContentReaderHostRef createMusicalContextContentReader(
      ARAMusicalContextHostRef, ARAContentType type,
      const ARAContentTimeRange *) noexcept override {
    return reinterpret_cast<ARAContentReaderHostRef>(
        intptr_t(type == kARAContentTypeKeySignatures ? 1 : 2));
  }
  bool isAudioSourceContentAvailable(ARAAudioSourceHostRef,
                                     ARAContentType) noexcept override {
    return false;
  }
  ARAContentGrade getAudioSourceContentGrade(ARAAudioSourceHostRef,
                                             ARAContentType) noexcept override {
    return kARAContentGradeInitial;
  }
  ARAContentReaderHostRef createAudioSourceContentReader(
      ARAAudioSourceHostRef, ARAContentType,
      const ARAContentTimeRange *) noexcept override {
    return nullptr;
  }
  ARAInt32
  getContentReaderEventCount(ARAContentReaderHostRef ref) noexcept override {
    return intptr_t(ref) == 1 ? 1 : 2;
  }
  const void *getContentReaderDataForEvent(ARAContentReaderHostRef ref,
                                           ARAInt32 i) noexcept override {
    return intptr_t(ref) == 1 ? static_cast<const void *>(&key)
                              : static_cast<const void *>(&tempos[i]);
  }
  void destroyContentReader(ARAContentReaderHostRef) noexcept override {}
};
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    const auto *factory = createARAFactory();
    check(factory && factory->highestSupportedApiGeneration >=
                         kARAAPIGeneration_2_0_Final,
          "ARA factory missing");
    SizedStruct<ARA_STRUCT_MEMBER(ARAInterfaceConfiguration,
                                  assertFunctionAddress)>
        config{kARAAPIGeneration_2_0_Final, nullptr};
    factory->initializeARAWithConfiguration(&config);
    AudioHost audio;
    ArchiveHost archive;
    archive.id = factory->documentArchiveID;
    ContentHost content;
    Host::DocumentControllerHostInstance host(&audio, &archive, &content,
                                              nullptr, nullptr);
    SizedStruct<ARA_STRUCT_MEMBER(ARADocumentProperties, name)> doc{
        "Makkine Studio One event test"};
    const auto *instance =
        factory->createDocumentControllerWithDocument(&host, &doc);
    check(instance, "ARA document creation failed");
    Host::DocumentController controller(instance);
    controller.beginEditing();
    SizedStruct<ARA_STRUCT_MEMBER(ARAMusicalContextProperties, color)> cp{};
    cp.name = "Song";
    cp.orderIndex = 0;
    auto context = controller.createMusicalContext(
        reinterpret_cast<ARAMusicalContextHostRef>(&content), &cp);
    SizedStruct<ARA_STRUCT_MEMBER(ARARegionSequenceProperties, color)> seq{};
    seq.name = "Vocal";
    seq.musicalContextRef = context;
    auto sequence = controller.createRegionSequence(
        reinterpret_cast<ARARegionSequenceHostRef>(&content), &seq);
    SizedStruct<ARA_STRUCT_MEMBER(ARAAudioSourceProperties, channelArrangement)>
        sp{};
    sp.name = "Raw vocal";
    sp.persistentID = "source-1";
    sp.sampleCount = audio.pcm.getNumSamples();
    sp.sampleRate = 48000;
    sp.channelCount = 1;
    sp.merits64BitSamples = false;
    sp.channelArrangementDataType = kARAChannelArrangementUndefined;
    auto source = controller.createAudioSource(
        reinterpret_cast<ARAAudioSourceHostRef>(&audio), &sp);
    SizedStruct<ARA_STRUCT_MEMBER(ARAAudioModificationProperties, persistentID)>
        mp{"IA version", "mod-1"};
    auto modification = controller.createAudioModification(
        source, reinterpret_cast<ARAAudioModificationHostRef>(&archive), &mp);
    SizedStruct<ARA_STRUCT_MEMBER(ARAPlaybackRegionProperties, color)> rp{};
    rp.transformationFlags = 0;
    rp.startInModificationTime = 0;
    rp.durationInModificationTime = audio.pcm.getNumSamples() / 48000.;
    rp.startInPlaybackTime = 21.25;
    rp.durationInPlaybackTime = rp.durationInModificationTime;
    rp.regionSequenceRef = sequence;
    rp.name = "Original event";
    auto region = controller.createPlaybackRegion(
        modification, reinterpret_cast<ARAPlaybackRegionHostRef>(&audio), &rp);
    controller.endEditing();
    controller.enableAudioSourceSamplesAccess(source, true);
    {
      OneProcessor p;
      auto roles = kARAPlaybackRendererRole | kARAEditorViewRole;
      check(p.bindToARA(instance->documentControllerRef, roles, roles),
            "ARA instance binding failed");
      auto *renderer = p.getPlaybackRenderer<one::ARARenderer>();
      check(renderer, "ARA renderer missing");
      renderer->addPlaybackRegion(region);
      p.prepareToPlay(48000, 256);
      auto *modelRegion =
          static_cast<juce::ARAPlaybackRegion *>(ARA::PlugIn::fromRef(region));
      check(modelRegion, "ARA region model missing");
      p.capture();
      auto wait = [&] {
        auto start = juce::Time::getMillisecondCounter();
        while (p.busy.load() &&
               juce::Time::getMillisecondCounter() - start < 120000)
          dispatch(20);
        check(!p.busy.load(), "ARA analysis timeout");
      };
      wait();
      check(bool(p.inputSnapshot()), p.statusText().toRawUTF8());
      check(p.inputSnapshot()->origin.seconds == 21.25 &&
                p.inputSnapshot()->origin.fromARA,
            "ARA capture loses source event position");
      check(p.inputSnapshot()->key.fromHost &&
                p.inputSnapshot()->key.scale == 2,
            "Project ARA key signature not used");
      p.engineer();
      wait();
      dispatch(150);
      auto result = p.snapshot();
      check(bool(result), p.statusText().toRawUTF8());
      check(bool(modelRegion->getAudioModification<one::Modification>()
                     ->results.snapshot()),
            "IA not applied automatically to original ARA event");
      juce::AudioBuffer<float> block(1, 256);
      juce::AudioPlayHead::PositionInfo pos;
      pos.setIsPlaying(true);
      pos.setTimeInSamples(int64_t((21.25 + .8) * 48000));
      check(renderer->processBlock(block, juce::AudioProcessor::Realtime::no,
                                   pos),
            "ARA event rendering failed");
      double error = 0;
      for (int i = 0; i < 256; ++i)
        error += std::abs(block.getSample(0, i) -
                          result->audio.getSample(0, int(.8 * 48000) + i));
      check(error < .01, "ARA playback is not aligned to source event sample");
      auto writer = reinterpret_cast<ARAArchiveWriterHostRef>(&archive);
      check(controller.storeObjectsToArchive(writer, nullptr),
            "ARA archive failed");
      auto *mod = modelRegion->getAudioModification<one::Modification>();
      mod->results.publish(nullptr);
      controller.beginEditing();
      check(controller.restoreObjectsFromArchive(
                reinterpret_cast<ARAArchiveReaderHostRef>(&archive), nullptr),
            "ARA archive restore failed");
      controller.endEditing();
      check(mod->results.snapshot() &&
                fingerprint(mod->results.snapshot()->audio) ==
                    fingerprint(result->audio),
            "Self-contained ARA restore lost corrected audio");
      check(fingerprint(mod->results.snapshot()->input->audio) ==
                fingerprint(audio.pcm),
            "ARA archive does not contain original source");
      {
        OneProcessor audition;
        check(audition.bindToARA(instance->documentControllerRef,
                                 kARAEditorRendererRole,
                                 kARAEditorRendererRole),
              "Separate ARA editor-renderer binding failed");
        auto *ear = audition.getEditorRenderer<one::EditorRenderer>();
        ear->addPlaybackRegion(region);
        audition.prepareToPlay(48000, 256);
        p.play();
        p.seek(.8);
        juce::AudioPlayHead::PositionInfo stopped;
        stopped.setIsPlaying(false);
        double energy = 0;
        for (int i = 0; i < 20; ++i) {
          block.clear();
          ear->processBlock(block, juce::AudioProcessor::Realtime::no, stopped);
          energy += block.getRMSLevel(0, 0, block.getNumSamples());
        }
        check(energy > .05, "ARA preview is silent when editor view and audio "
                            "renderer are separate instances");
        p.stop();
        for (int i = 0; i < 30; ++i) {
          block.clear();
          ear->processBlock(block, juce::AudioProcessor::Realtime::no, stopped);
        }
        check(block.getMagnitude(0, block.getNumSamples()) < .00001f,
              "Separate ARA preview STOP failed");
        audition.releaseResources();
        ear->removePlaybackRegion(region);
      }
      p.releaseResources();
      renderer->removePlaybackRegion(region);
    }
    controller.beginEditing();
    controller.destroyPlaybackRegion(region);
    controller.destroyAudioModification(modification);
    controller.destroyRegionSequence(sequence);
    controller.destroyAudioSource(source);
    controller.destroyMusicalContext(context);
    controller.endEditing();
    controller.destroyDocumentController();
    factory->uninitializeARA();
    std::cout
        << "PASS: real ARA factory/document/roles, selected event capture, "
           "project key, automatic IA application, sample-aligned renderer and "
           "self-contained project archive restore\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
