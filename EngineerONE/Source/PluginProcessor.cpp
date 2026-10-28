#include "PluginProcessor.h"
#include "PluginEditor.h"
OneProcessor::OneProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Vocal", juce::AudioChannelSet::stereo(), true)
              .withOutput("Vocal", juce::AudioChannelSet::stereo(), true)),
      Thread("Makkine automatic vocal engineer") {
  license.reload();
  authorized = license.authorized;
  recorder.onFinished = [this](const juce::File &file, one::Origin origin,
                               const juce::String &error) {
    if (error.isNotEmpty())
      setMessage(error);
    else
      loadFile(file, origin);
  };
  startThread(juce::Thread::Priority::low);
  startTimerHz(10);
}
OneProcessor::~OneProcessor() {
  stop();
  stopTimer();
  recorder.shutdown();
  generation.fetch_add(1);
  signalThreadShouldExit();
  notify();
  waitForThreadToExit(-1);
  if (region)
    region->removeListener(this);
}
bool OneProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
  return (in == juce::AudioChannelSet::mono() ||
          in == juce::AudioChannelSet::stereo()) &&
         (out == in || out == juce::AudioChannelSet::stereo());
}
void OneProcessor::prepareToPlay(double sr, int block) {
  hostRate = sr;
  recorder.prepare(sr, getTotalNumInputChannels());
  cursor = 0;
  fade = 0;
  setLatencySamples(0);
  prepareToPlayForARA(sr, block, getTotalNumOutputChannels(),
                      getProcessingPrecision());
}
juce::String OneProcessor::statusText() const {
  juce::ScopedLock guard(lock);
  return message;
}
void OneProcessor::setMessage(juce::String text) {
  juce::ScopedLock guard(lock);
  message = std::move(text);
}
std::shared_ptr<const one::Input> OneProcessor::inputSnapshot() const {
  juce::ScopedLock guard(lock);
  return input;
}
one::Controller *OneProcessor::araController() const {
  auto *doc = getDocumentController();
  return doc ? juce::ARADocumentControllerSpecialisation::
                   getSpecialisedDocumentController<one::Controller>(doc)
             : nullptr;
}
void OneProcessor::stop() {
  playing.store(false);
  if (auto *c = araController())
    if (c->preview.controlOwner.load() == this)
      c->preview.active.store(false);
}
void OneProcessor::queue(Job next) {
  if (auto *c = araController())
    c->preview.active.store(false);
  generation.fetch_add(1);
  completedARA.reset();
  job = next;
  busy.store(true);
  progress.store(0);
  playing.store(false);
  notify();
}
void OneProcessor::loadFile(const juce::File &f, one::Origin origin) {
  applyOnTrack.store(false);
  juce::ScopedLock guard(lock);
  pendingFile = f;
  pendingOrigin = origin;
  restoreAfterRead = false;
  pendingOrigin.name = f.getFileNameWithoutExtension();
  queue(Job::read);
  message = "Analizando notas, frases, ataques y vibrato...";
}
void OneProcessor::capture() {
  stop();
  applyOnTrack.store(false);
  if (araAvailable() && captureARA())
    return;
  if (recorder.arm())
    setMessage("CAPTURA preparada: reproduce la pista. STOP termina la linea.");
}
void OneProcessor::finishCapture() {
  recorder.stop();
  setMessage("Terminando captura y preparando analisis...");
}
void OneProcessor::engineer() {
  juce::ScopedLock guard(lock);
  if (!input) {
    message = "Primero captura o importa una vocal.";
    return;
  }
  pendingVersion = ++nextVersion;
  pendingNotes.clear();
  queue(Job::generate);
  message = "IA: eligiendo notas e intencion musical automaticamente...";
}
void OneProcessor::retouch(std::vector<one::Note> notes) {
  juce::ScopedLock guard(lock);
  if (!input)
    return;
  pendingNotes = std::move(notes);
  pendingVersion = std::max<uint64_t>(1, nextVersion);
  queue(Job::retouch);
  message =
      "Aplicando retoque opcional; la afinacion automatica ya esta hecha.";
}
void OneProcessor::cancel() {
  juce::ScopedLock guard(lock);
  generation.fetch_add(1);
  job = Job::none;
  busy.store(false);
  message = "Proceso cancelado. Puedes escuchar la version anterior.";
}
void OneProcessor::play(bool raw) {
  original.store(raw);
  if (auto p = snapshot()) {
    if (playhead.load() >= p->audio.getNumSamples() / p->input->rate)
      seek(0);
    if (auto *c = araController())
      if (c->editorRenderers.load() > 0 && region) {
        c->preview.controlOwner.store(this);
        c->preview.results.publish(p);
        c->preview.modification.store(
            region->getAudioModification<one::Modification>());
        c->preview.raw.store(raw);
        c->preview.mute.store(mute.load());
        c->preview.seekTo.store(playhead.load());
        c->preview.seekSerial.fetch_add(1);
        c->preview.active.store(true);
        if (auto *host = getDocumentController()->getHostPlaybackController())
          host->requestStopPlayback();
        playing.store(false);
        return;
      }
    playing.store(true);
  } else
    setMessage("Pulsa IA para crear una version afinada.");
}
void OneProcessor::seek(double seconds) {
  if (auto *c = araController(); c && c->preview.controlOwner.load() == this) {
    c->preview.seekTo.store(std::max(0., seconds));
    c->preview.seekSerial.fetch_add(1);
  }
  seekTo.store(std::max(0., seconds));
  seekSerial.fetch_add(1);
  playhead.store(std::max(0., seconds));
}
void OneProcessor::run() {
  while (!threadShouldExit()) {
    Job task;
    juce::File file;
    one::Origin origin;
    uint64_t token, version;
    std::vector<one::Note> notes;
    std::shared_ptr<const one::Input> source;
    juce::ARAAudioSourceReader *reader;
    int64_t first, length;
    double rate;
    int channels;
    std::shared_ptr<const one::Result> reopen;
    {
      juce::ScopedLock guard(lock);
      task = job;
      job = Job::none;
      file = pendingFile;
      origin = pendingOrigin;
      token = generation.load();
      version = pendingVersion;
      notes = pendingNotes;
      source = input;
      reader = pendingReader;
      first = araFirst;
      length = araLength;
      rate = araRate;
      channels = araChannels;
      reopen = pendingReopen;
    }
    if (task == Job::none) {
      wait(100);
      continue;
    }
    auto cancelled = [&] {
      return threadShouldExit() || generation.load() != token;
    };
    auto folder =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("M7G/EngineerONE/Exports")
            .getChildFile(juce::Uuid().toString());
    try {
      if (task == Job::reopen && reopen) {
        auto fresh = std::make_shared<one::Input>(*reopen->input);
        fresh->analysis =
            one::Analyzer().run(fresh->audio, fresh->rate, cancelled);
        fresh->key = one::inferKey(fresh->analysis, fresh->origin);
        fresh->waveform = one::overview(fresh->audio);
        auto restored = std::make_shared<one::Result>(*reopen);
        restored->input = fresh;
        restored->measured =
            one::Analyzer().run(restored->audio, fresh->rate, cancelled);
        restored->waveform = one::overview(restored->audio);
        one::exportResult(*restored, folder, cancelled);
        if (cancelled()) {
          folder.deleteRecursively();
          continue;
        }
        juce::ScopedLock guard(lock);
        if (cancelled())
          continue;
        input = fresh;
        nextVersion = restored->interpretation;
        results.publish(restored);
        busy.store(false);
        message = "Evento ARA recuperado con su afinacion y WAV listos.";
        continue;
      }
      if (task == Job::ara) {
        if (!reader || length < 1 || length > rate * 600 ||
            uint64_t(length) * channels * 4 > 180ull * 1024 * 1024)
          throw std::runtime_error(
              "El evento excede 10 minutos o 180 MB de audio");
        juce::AudioBuffer<float> pcm(channels, int(length));
        for (int i = 0; i < pcm.getNumSamples(); i += 8192) {
          if (cancelled())
            break;
          int n = std::min(8192, pcm.getNumSamples() - i);
          if (!reader->read(&pcm, i, n, first + i, true, true))
            throw std::runtime_error("Studio One cambio o no permite leer el "
                                     "evento ARA. Vuelve a CAPTURAR.");
        }
        if (cancelled())
          continue;
        auto dir = juce::File::getSpecialLocation(
                       juce::File::userApplicationDataDirectory)
                       .getChildFile("M7G/EngineerONE/Capturas");
        dir.createDirectory();
        file = dir.getChildFile("ARA-" + juce::Uuid().toString() + ".wav");
        auto stream = file.createOutputStream();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            stream ? wav.createWriterFor(stream.get(), rate, unsigned(channels),
                                         32, {}, 0)
                   : nullptr);
        if (!writer)
          throw std::runtime_error("No se pudo guardar la captura ARA");
        stream.release();
        if (!writer->writeFromAudioSampleBuffer(pcm, 0, pcm.getNumSamples()))
          throw std::runtime_error("Captura ARA incompleta");
        writer.reset();
      }
      if (task == Job::read || task == Job::ara) {
        auto loaded = one::readInput(file, origin, cancelled);
        if (cancelled())
          continue;
        juce::ScopedLock guard(lock);
        if (cancelled())
          continue;
        input = std::move(loaded);
        nextVersion = 0;
        results.publish(nullptr);
        busy.store(false);
        message = "Analisis listo: " + one::keyName(input->key) +
                  ". Pulsa IA: afina toda la voz sola.";
        if (restoreAfterRead) {
          restoreAfterRead = false;
          bool valid = true;
          for (auto &note : restoreNotes)
            valid &=
                note.id >= 0 && note.id < int(input->analysis.notes.size());
          if (!valid)
            restoreNotes.clear();
          pendingNotes = restoreNotes;
          pendingVersion = restoreVersion;
          nextVersion = restoreVersion;
          queue(Job::retouch);
          applyOnTrack.store(restoreTrack);
        }
        continue;
      }
      if (!source)
        throw std::runtime_error("No hay captura para afinar");
      auto result = one::generate(
          source, version,
          task == Job::retouch ? notes : std::vector<one::Note>{}, folder,
          cancelled, [&](float value, const juce::String &text) {
            if (!cancelled()) {
              juce::ScopedLock guard(lock);
              if (!cancelled()) {
                progress.store(value);
                message = text;
              }
            }
          });
      if (cancelled()) {
        folder.deleteRecursively();
        continue;
      }
      {
        juce::ScopedLock guard(lock);
        if (cancelled()) {
          folder.deleteRecursively();
          continue;
        }
        results.publish(result);
        exportStale.store(false);
        completedARA = result;
        busy.store(false);
        seek(0);
      }
    } catch (const std::exception &e) {
      folder.deleteRecursively();
      juce::ScopedLock guard(lock);
      if (!cancelled()) {
        busy.store(false);
        message = "No se completo: " + juce::String::fromUTF8(e.what());
      }
    }
  }
}
void OneProcessor::timerCallback() {
  if (auto *c = araController(); c && c->preview.controlOwner.load() == this) {
    c->preview.mute.store(mute.load());
    c->preview.raw.store(original.load());
    if (c->preview.active.load())
      playhead.store(c->preview.position.load());
    c->preview.results.collect();
  }
  std::shared_ptr<one::Result> ready;
  {
    juce::ScopedLock guard(lock);
    ready = std::move(completedARA);
  }
  if (ready && region && ready->input->origin.fromARA &&
      ready->input == inputSnapshot()) {
    auto *mod = region->getAudioModification<one::Modification>();
    mod->apply(ready);
  }
  results.collect();
  if (!busy.load() && !inputSnapshot() && isEditorView()) {
    if (!region) {
      if (auto *renderer = getPlaybackRenderer()) {
        auto regions = renderer->getPlaybackRegions();
        if (!regions.empty())
          attach(regions.front());
      }
      if (!region)
        if (auto *view = getEditorView()) {
          const auto &regions =
              view->getViewSelection()
                  .getEffectivePlaybackRegions<juce::ARAPlaybackRegion>();
          if (!regions.empty())
            attach(static_cast<juce::ARAPlaybackRegion *>(regions.front()));
        }
    }
    if (region)
      if (auto restored = region->getAudioModification<one::Modification>()
                              ->results.snapshot()) {
        juce::ScopedLock guard(lock);
        pendingReopen = restored;
        queue(Job::reopen);
        message = "Recuperando visual y exportaciones del evento ARA...";
      }
  }
}
void OneProcessor::attach(juce::ARAPlaybackRegion *p) {
  if (region == p)
    return;
  if (region)
    region->removeListener(this);
  region = p;
  if (region)
    region->addListener(this);
}
void OneProcessor::willDestroyPlaybackRegion(juce::ARAPlaybackRegion *p) {
  if (auto *c = araController())
    c->preview.active.store(false);
  if (region == p) {
    generation.fetch_add(1);
    region = nullptr;
    busy.store(false);
    setMessage("El evento ARA fue eliminado. Selecciona otro y captura.");
  }
}
void OneProcessor::didUpdatePlaybackRegionProperties(
    juce::ARAPlaybackRegion *) {
  exportStale.store(true);
  generation.fetch_add(1);
  busy.store(false);
  setMessage("Cambio de evento ARA: CAPTURA otra vez para actualizar los WAV y "
             "su timestamp.");
}
bool OneProcessor::captureARA() {
  juce::ARAPlaybackRegion *selected = nullptr;
  if (auto *view = getEditorView()) {
    const auto &selectedRegions =
        view->getViewSelection()
            .getEffectivePlaybackRegions<juce::ARAPlaybackRegion>();
    if (!selectedRegions.empty())
      selected =
          static_cast<juce::ARAPlaybackRegion *>(selectedRegions.front());
  }
  if (!selected)
    if (auto *renderer = getPlaybackRenderer()) {
      auto regions = renderer->getPlaybackRegions();
      if (!regions.empty())
        selected = regions.front();
    }
  if (!selected) {
    setMessage(
        "Selecciona un evento en Studio One para capturarlo mediante ARA.");
    return true;
  }
  attach(selected);
  auto *source = selected->getAudioModification()->getAudioSource();
  if (!source->isSampleAccessEnabled()) {
    setMessage("Studio One aun no permite leer audio del evento ARA.");
    return true;
  }
  auto reader = std::make_unique<juce::ARAAudioSourceReader>(source);
  auto *ptr = reader.get();
  sourceReaders.push_back(std::move(reader));
  one::Origin origin;
  origin.fromARA = true;
  origin.positionKnown = true;
  origin.seconds = selected->getStartInPlaybackTime();
  origin.modificationStart = selected->getStartInAudioModificationTime();
  if (auto *sequence = selected->getRegionSequence()) {
    auto *context = sequence->getMusicalContext();
    if (context) {
      double quarter = 0;
      ARA::PlugIn::HostContentReader<ARA::kARAContentTypeTempoEntries> tempos(
          context);
      if (tempos.getEventCount() > 1) {
        int i = 0;
        while (i + 2 < tempos.getEventCount() &&
               tempos[i + 1].timePosition <= origin.seconds)
          ++i;
        auto a = tempos[i], b = tempos[i + 1];
        if (b.timePosition > a.timePosition) {
          double slope = (b.quarterPosition - a.quarterPosition) /
                         (b.timePosition - a.timePosition);
          quarter =
              a.quarterPosition + (origin.seconds - a.timePosition) * slope;
          origin.bpm = slope * 60;
          origin.tempoKnown = true;
        }
      }
      ARA::PlugIn::HostContentReader<ARA::kARAContentTypeKeySignatures> keys(
          context);
      int keyIndex = 0;
      for (const auto &k : keys) {
        if (keyIndex++ > 0 && k.position > quarter)
          break;
        int root = ((int(k.root) * 7) % 12 + 12) % 12;
        bool major = true, minor = true;
        for (int i = 0; i < 12; ++i) {
          bool used = k.intervals[i] != ARA::kARAKeySignatureIntervalUnused;
          major &= used == one::allowed(i, 1, 0);
          minor &= used == one::allowed(i, 2, 0);
        }
        if (major || minor) {
          origin.hostRoot = root;
          origin.hostScale = major ? 1 : 2;
        }
      }
    }
  }
  {
    juce::ScopedLock guard(lock);
    pendingReader = ptr;
    pendingOrigin = origin;
    araFirst = selected->getStartInAudioModificationSamples();
    araLength =
        int64_t(std::llround(selected->getDurationInAudioModificationTime() *
                             source->getSampleRate()));
    araRate = source->getSampleRate();
    araChannels = juce::jlimit(1, 2, int(source->getChannelCount()));
    queue(Job::ara);
    message = "Leyendo el evento ARA en su posicion original...";
  }
  return true;
}
bool OneProcessor::applyToEvent() {
  auto result = snapshot();
  if (!result)
    return false;
  if (region && result->input->origin.fromARA) {
    region->getAudioModification<one::Modification>()->apply(
        std::make_shared<one::Result>(*result));
    setMessage("Version aplicada al evento ARA en su posicion original (no "
               "destructivo).");
    return true;
  }
  if (result->input->origin.positionKnown) {
    applyOnTrack.store(!applyOnTrack.load());
    setMessage(
        applyOnTrack.load()
            ? "Correccion activa en esta pista durante el tramo capturado."
            : "Correccion de pista desactivada.");
    return true;
  }
  setMessage("Posicion de host desconocida. Arrastra el WAV o usa ARA.");
  return false;
}
void OneProcessor::processBlock(juce::AudioBuffer<float> &b,
                                juce::MidiBuffer &midi) {
  juce::ScopedNoDenormals noDenormals;
  midi.clear();
  one::Transport transport;
  juce::AudioPlayHead::PositionInfo position;
  if (auto *head = getPlayHead())
    if (auto p = head->getPosition()) {
      position = *p;
      transport.playing = p->getIsPlaying();
      transport.playStateKnown = true;
      if (auto samples = p->getTimeInSamples()) {
        transport.sample = *samples;
        transport.positionKnown = true;
      }
      if (auto bpm = p->getBpm()) {
        transport.bpm = *bpm;
        transport.tempoKnown = true;
      }
    }
  recorder.process(b, transport);
  bool ara = processBlockForARA(
      b, isNonRealtime() ? Realtime::no : Realtime::yes, position);
  for (int c = ara ? b.getNumChannels() : getTotalNumInputChannels();
       c < b.getNumChannels(); ++c)
    b.clear(c, 0, b.getNumSamples());
  const auto *result = results.acquire();
  auto serial = results.serial.load();
  if (serial != audioSerial || result != audioResult) {
    audioResult = result;
    audioSerial = serial;
    cursor = 0;
    fade = 0;
  }
  auto requested = seekSerial.load();
  if (requested != seekApplied) {
    seekApplied = requested;
    cursor = seekTo.load() * (result ? result->input->rate : hostRate);
    fade = 0;
  }
  bool raw = original.load();
  if (raw != audioRaw) {
    previousRaw = audioRaw;
    audioRaw = raw;
    abFade = 0;
  }
  if (result && authorized) {
    double increment = result->input->rate / hostRate;
    const auto &pcm = audioRaw ? result->input->audio : result->audio;
    const auto &previous = previousRaw ? result->input->audio : result->audio;
    for (int i = 0; i < b.getNumSamples(); ++i) {
      bool listening = playing.load() && cursor < pcm.getNumSamples();
      if (cursor >= pcm.getNumSamples())
        playing.store(false);
      float target = listening ? 1.f : 0.f;
      fade += (target - fade) * float(1 - std::exp(-1 / (hostRate * .004)));
      abFade = std::min(1.f, abFade + float(1 / (hostRate * .008)));
      bool track = applyOnTrack.load() && !ara && transport.positionKnown &&
                   transport.playing && !recorder.active();
      double at = (double(transport.sample + i) / hostRate -
                   result->input->origin.seconds) *
                  result->input->rate;
      track = track && at >= 0 && at < result->audio.getNumSamples();
      for (int c = 0; c < b.getNumChannels(); ++c) {
        float dry = b.getSample(c, i);
        if (track)
          dry = one::sample(result->audio, c, at);
        auto read = [&](const juce::AudioBuffer<float> &data) {
          return b.getNumChannels() == 1 && data.getNumChannels() == 2
                     ? .5f * (one::sample(data, 0, cursor) +
                              one::sample(data, 1, cursor))
                     : one::sample(data, c, cursor);
        };
        float wet = (read(previous) * (1 - abFade) + read(pcm) * abFade) *
                    float(result->exportGain);
        float output = dry * (1 - fade) + (mute.load() ? 0 : wet) * fade;
        b.setSample(c, i, std::isfinite(output) ? output : 0);
      }
      if (listening || fade > .0001f)
        cursor += increment;
    }
    playhead.store(
        std::min(cursor / result->input->rate,
                 result->audio.getNumSamples() / result->input->rate));
  }
  results.release();
  float peak = 0;
  for (int c = 0; c < b.getNumChannels(); ++c)
    for (int i = 0; i < b.getNumSamples(); ++i) {
      float x = b.getSample(c, i);
      if (!std::isfinite(x))
        b.setSample(c, i, 0);
      else
        peak = std::max(peak, std::abs(x));
    }
  meter.store(std::max(peak, meter.load() * .85f));
}
void OneProcessor::getStateInformation(juce::MemoryBlock &dest) {
  juce::ValueTree tree("MAKKINE_ENGINEER_ONE");
  auto source = inputSnapshot();
  if (source) {
    tree.setProperty("file", source->rawFile.getFullPathName(), nullptr);
    tree.setProperty("seconds", source->origin.seconds, nullptr);
    tree.setProperty("known", source->origin.positionKnown, nullptr);
    tree.setProperty("bpm", source->origin.bpm, nullptr);
  }
  if (auto p = snapshot()) {
    tree.setProperty("version", int64_t(p->interpretation), nullptr);
    tree.setProperty(
        "notes", juce::JSON::toString(one::notesJSON(p->notes), true), nullptr);
    tree.setProperty("track", applyOnTrack.load(), nullptr);
  }
  if (auto xml = tree.createXml())
    copyXmlToBinary(*xml, dest);
}
void OneProcessor::setStateInformation(const void *data, int bytes) {
  if (auto xml = getXmlFromBinary(data, bytes)) {
    auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.hasType("MAKKINE_ENGINEER_ONE"))
      return;
    auto f = juce::File(tree["file"].toString());
    if (f.existsAsFile()) {
      one::Origin origin;
      origin.seconds = double(tree["seconds"]);
      origin.positionKnown = bool(tree["known"]);
      origin.bpm = double(tree["bpm"]);
      juce::ScopedLock guard(lock);
      loadFile(f, origin);
      if (tree.hasProperty("version")) {
        restoreAfterRead = true;
        restoreVersion =
            uint64_t(std::max<int64_t>(1, int64_t(tree["version"])));
        restoreTrack = bool(tree["track"]);
        try {
          restoreNotes =
              one::readNotes(juce::JSON::parse(tree["notes"].toString()));
        } catch (...) {
          restoreNotes.clear();
        }
      }
    }
  }
}
juce::AudioProcessorEditor *OneProcessor::createEditor() {
  return new OneEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new OneProcessor();
}
