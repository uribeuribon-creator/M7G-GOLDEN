#pragma once
#include "Model.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace one {
struct Transport {
  bool playing = false, playStateKnown = false, positionKnown = false,
       tempoKnown = false;
  int64_t sample = 0;
  double bpm = 120;
};
class Capture final : private juce::Thread {
public:
  enum State { idle, armed, recording, finishing };
  Capture()
      : Thread("Makkine track capture writer"), fifo(capacity),
        ring(2, capacity), block(2, 8192) {
    ring.clear();
    startThread(juce::Thread::Priority::normal);
  }
  ~Capture() override { shutdown(); }
  void shutdown() {
    if (shutdownRequested.exchange(true))
      return;
    signalThreadShouldExit();
    notify();
    waitForThreadToExit(-1);
  }
  void prepare(double rate, int channels) {
    nextRate.store(rate);
    nextChannels.store(juce::jlimit(1, 2, channels));
  }
  bool arm() {
    if (state.load() != idle)
      return false;
    fifo.reset();
    frames.store(0);
    overrun.store(false);
    stopRequested.store(false);
    state.store(armed);
    return true;
  }
  void stop() {
    if (state.load() == armed)
      state.store(idle);
    else
      stopRequested.store(true);
  }
  bool active() const { return state.load() != idle; }
  std::atomic<int> state{idle};
  std::atomic<int64_t> frames{0};
  std::atomic<bool> overrun{false};
  std::function<void(const juce::File &, Origin, const juce::String &)>
      onFinished;
  void process(const juce::AudioBuffer<float> &audio, Transport transport) {
    producerBusy.store(true);
    auto done = juce::ScopeGuard([&] { producerBusy.store(false); });
    int status = state.load();
    if (status == idle || status == finishing || audio.getNumSamples() == 0)
      return;
    if (status == armed) {
      if (transport.playStateKnown && !transport.playing)
        return;
      captureRate = nextRate.load();
      captureChannels = nextChannels.load();
      originSeconds =
          transport.positionKnown ? transport.sample / captureRate : 0;
      originKnown = transport.positionKnown;
      tempoKnown = transport.tempoKnown;
      originBpm = transport.bpm;
      expected = transport.sample;
      state.store(recording);
      status = recording;
    }
    if (status != recording)
      return;
    if (stopRequested.load() ||
        (transport.playStateKnown && !transport.playing) ||
        (transport.positionKnown && originKnown &&
         std::llabs(transport.sample - expected) >
             std::max(8, audio.getNumSamples() / 2))) {
      state.store(finishing);
      return;
    }
    int n = audio.getNumSamples();
    if (frames.load() + n > int64_t(captureRate * 600) ||
        uint64_t(frames.load() + n) * captureChannels * 4 >
            180ull * 1024 * 1024) {
      stopRequested.store(true);
      state.store(finishing);
      return;
    }
    int a, na, b, nb;
    fifo.prepareToWrite(n, a, na, b, nb);
    if (na + nb != n) {
      overrun.store(true);
      state.store(finishing);
      return;
    }
    for (int c = 0; c < captureChannels; ++c) {
      int input = std::min(c, audio.getNumChannels() - 1);
      ring.copyFrom(c, a, audio, input, 0, na);
      if (nb)
        ring.copyFrom(c, b, audio, input, na, nb);
    }
    fifo.finishedWrite(n);
    frames.fetch_add(n);
    expected = transport.sample + n;
  }

private:
  void run() override {
    std::unique_ptr<juce::AudioFormatWriter> writer;
    juce::File file;
    Origin origin;
    juce::String error;
    while (!threadShouldExit()) {
      int current = state.load();
      if ((current == recording || current == finishing) && !writer) {
        origin.seconds = originSeconds;
        origin.positionKnown = originKnown;
        origin.bpm = originBpm;
        origin.tempoKnown = tempoKnown;
        auto folder = juce::File::getSpecialLocation(
                          juce::File::userApplicationDataDirectory)
                          .getChildFile("M7G/EngineerONE/Capturas");
        folder.createDirectory();
        file =
            folder.getChildFile("Captura-" + juce::Uuid().toString() + ".wav");
        auto stream = file.createOutputStream();
        juce::WavAudioFormat wav;
        juce::StringPairArray metadata;
        if (origin.positionKnown && origin.seconds >= 0)
          metadata.set(juce::WavAudioFormat::bwavTimeReference,
                       juce::String(int64_t(
                           std::llround(origin.seconds * captureRate))));
        if (stream)
          writer.reset(wav.createWriterFor(stream.get(), captureRate,
                                           unsigned(captureChannels), 32,
                                           metadata, 0));
        if (!writer) {
          error = "No se pudo escribir la captura en disco";
          state.store(finishing);
        } else
          stream.release();
      }
      if (stopRequested.load() && current == recording)
        state.compare_exchange_strong(current, finishing);
      if (writer) {
        int available = std::min(8192, fifo.getNumReady());
        if (available) {
          int a, na, b, nb;
          fifo.prepareToRead(available, a, na, b, nb);
          for (int c = 0; c < captureChannels; ++c) {
            block.copyFrom(c, 0, ring, c, a, na);
            if (nb)
              block.copyFrom(c, na, ring, c, b, nb);
          }
          if (!writer->writeFromAudioSampleBuffer(block, 0, available)) {
            error = "No se completo la escritura de captura";
            state.store(finishing);
          }
          fifo.finishedRead(available);
        }
      }
      if (state.load() == finishing && !producerBusy.load() &&
          fifo.getNumReady() == 0) {
        writer.reset();
        if (overrun.load())
          error = "Se detuvo la captura: el disco no recibio todos los "
                  "bloques. No se descarto audio en silencio.";
        if (frames.load() < int64_t(captureRate * .05))
          error =
              "Captura demasiado corta; reproduce una linea vocal completa.";
        if (onFinished && !shutdownRequested.load())
          onFinished(file, origin, error);
        error.clear();
        stopRequested.store(false);
        state.store(idle);
      }
      if (!writer && state.load() == finishing && !producerBusy.load()) {
        fifo.reset();
        if (onFinished && !shutdownRequested.load())
          onFinished(file, origin,
                     error.isEmpty() ? "Captura no disponible" : error);
        error.clear();
        state.store(idle);
      }
      wait(state.load() == idle || state.load() == armed ? 40 : 3);
    }
  }
  static constexpr int capacity = 524288;
  juce::AbstractFifo fifo;
  juce::AudioBuffer<float> ring, block;
  std::atomic<double> nextRate{48000};
  std::atomic<int> nextChannels{2};
  std::atomic<bool> producerBusy{false}, stopRequested{false},
      shutdownRequested{false};
  double captureRate = 48000, originSeconds = 0, originBpm = 120;
  int captureChannels = 2;
  bool originKnown = false, tempoKnown = false;
  int64_t expected = 0;
};
} // namespace one
