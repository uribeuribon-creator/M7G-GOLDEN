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
      key.intervals[i] =
          (i == 0 || i == 2 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10)
              ? kARAKeySignatureIntervalUsed
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
    check(factory, "ARA factory missing");
    SizedStruct<ARA_STRUCT_MEMBER(ARAInterfaceConfiguration,
                                  assertFunctionAddress)>
        config{kARAAPIGeneration_2_0_Final, nullptr};
    factory->initializeARAWithConfiguration(&config);
    AudioHost audio;
    ArchiveHost archive;
    archive.id = factory->documentArchiveID;
    Host::DocumentControllerHostInstance host(&audio, &archive, nullptr,
                                              nullptr, nullptr);
    SizedStruct<ARA_STRUCT_MEMBER(ARADocumentProperties, name)> dp{
        "Fresh Vocal event test"};
    const auto *instance =
        factory->createDocumentControllerWithDocument(&host, &dp);
    Host::DocumentController controller(instance);
    controller.beginEditing();
    SizedStruct<ARA_STRUCT_MEMBER(ARAMusicalContextProperties, color)> cp{};
    cp.name = "Song";
    auto context = controller.createMusicalContext(
        reinterpret_cast<ARAMusicalContextHostRef>(&audio), &cp);
    SizedStruct<ARA_STRUCT_MEMBER(ARARegionSequenceProperties, color)> sq{};
    sq.name = "Lead";
    sq.musicalContextRef = context;
    auto sequence = controller.createRegionSequence(
        reinterpret_cast<ARARegionSequenceHostRef>(&audio), &sq);
    SizedStruct<ARA_STRUCT_MEMBER(ARAAudioSourceProperties, channelArrangement)>
        sp{};
    sp.name = "Vocal";
    sp.persistentID = "fresh-source";
    sp.sampleCount = audio.pcm.getNumSamples();
    sp.sampleRate = 48000;
    sp.channelCount = 1;
    sp.channelArrangementDataType = kARAChannelArrangementUndefined;
    auto source = controller.createAudioSource(
        reinterpret_cast<ARAAudioSourceHostRef>(&audio), &sp);
    SizedStruct<ARA_STRUCT_MEMBER(ARAAudioModificationProperties, persistentID)>
        mp{"Fresh FX", "fresh-mod"};
    auto modification = controller.createAudioModification(
        source, reinterpret_cast<ARAAudioModificationHostRef>(&archive), &mp);
    SizedStruct<ARA_STRUCT_MEMBER(ARAPlaybackRegionProperties, color)> rp{};
    rp.startInModificationTime = 0;
    rp.durationInModificationTime = audio.pcm.getNumSamples() / 48000.;
    rp.startInPlaybackTime = 14.5;
    rp.durationInPlaybackTime = rp.durationInModificationTime;
    rp.regionSequenceRef = sequence;
    auto region = controller.createPlaybackRegion(
        modification, reinterpret_cast<ARAPlaybackRegionHostRef>(&audio), &rp);
    controller.endEditing();
    controller.enableAudioSourceSamplesAccess(source, true);
    {
      FreshProcessor p;
      auto roles = kARAPlaybackRendererRole | kARAEditorViewRole;
      check(p.bindToARA(instance->documentControllerRef, roles, roles),
            "ARA role binding failed");
      auto *renderer = p.getPlaybackRenderer<fresh::Renderer>();
      renderer->addPlaybackRegion(region);
      p.prepareToPlay(48000, 256);
      dispatch(120);
      p.set(fresh::reverb, 0);
      p.set(fresh::delay, 0);
      p.set(fresh::character, 65);
      p.set(fresh::comp, 80);
      dispatch(120);
      auto *model =
          static_cast<juce::ARAPlaybackRegion *>(ARA::PlugIn::fromRef(region));
      auto *mod = model->getAudioModification<fresh::Modification>();
      check(mod->initialised && mod->parameters.load()[fresh::character] == 65,
            "Editor parameters do not reach ARA event modification");
      juce::AudioPlayHead::PositionInfo position;
      position.setIsPlaying(true);
      position.setBpm(120);
      juce::AudioBuffer<float> block(2, 256);
      double energy = 0;
      for (int i = 0; i < 24; ++i) {
        position.setTimeInSamples(int64_t((14.5 + .8) * 48000) + i * 256);
        check(renderer->processBlock(block, juce::AudioProcessor::Realtime::no,
                                     position),
              "ARA source rendering failed");
        energy += block.getRMSLevel(0, 0, 256);
      }
      check(energy > .05, "ARA event output is silent");
      check(p.getLatencySamples() > 0 && p.getLatencySamples() < 1024,
            "ARA host latency report invalid");
      p.releaseResources();
      p.prepareToPlay(44100, 256);
      double convertedEnergy = 0;
      for (int i = 0; i < 24; ++i) {
        position.setTimeInSamples(int64_t((14.5 + .8) * 44100) + i * 256);
        check(renderer->processBlock(block, juce::AudioProcessor::Realtime::no,
                                     position),
              "ARA bandlimited source conversion failed");
        convertedEnergy += block.getRMSLevel(0, 0, 256);
      }
      check(convertedEnergy > .05, "ARA 48k source into 44.1k host is silent");
      p.releaseResources();
      p.prepareToPlay(48000, 256);
      auto expected = mod->parameters.load();
      check(controller.storeObjectsToArchive(
                reinterpret_cast<ARAArchiveWriterHostRef>(&archive), nullptr),
            "ARA event archive failed");
      mod->parameters.store(fresh::defaults());
      controller.beginEditing();
      check(controller.restoreObjectsFromArchive(
                reinterpret_cast<ARAArchiveReaderHostRef>(&archive), nullptr),
            "ARA event archive restore failed");
      controller.endEditing();
      check(mod->parameters.load() == expected,
            "ARA archive loses complete chain parameters");
      position.setTimeInSamples(
          int64_t((14.5 + rp.durationInPlaybackTime + 20) * 48000));
      renderer->processBlock(block, juce::AudioProcessor::Realtime::no,
                             position);
      check(block.getMagnitude(0, 256) == 0,
            "ARA outputs outside event and tail");
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
    std::cout << "PASS: actual ARA factory/roles, selected-event parameters, "
                 "nonzero-position audio processing, host latency, event "
                 "archive restore and bounded tail\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
