#pragma once
#include "PitchRender.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <mutex>

namespace one {
struct Origin {
  double seconds = 0, bpm = 120;
  bool positionKnown = false, tempoKnown = false;
  double modificationStart = 0;
  bool fromARA = false;
  juce::String name = "Vocal capturada";
  int hostRoot = -1, hostScale = 0;
};
struct Note {
  int id = 0;
  double begin = 0, end = 0;
  float original = 0, target = 0, vibrato = 1, gainDb = 0;
  double timeShift = 0;
  bool touched = false;
};
struct Tonality {
  int root = 0, scale = 0;
  float confidence = 0;
  bool fromHost = false;
};
struct Input {
  juce::AudioBuffer<float> audio;
  double rate = 48000;
  Analysis analysis;
  Origin origin;
  Tonality key;
  juce::File rawFile;
  std::vector<float> waveform;
};
struct Result {
  std::shared_ptr<const Input> input;
  juce::AudioBuffer<float> audio;
  Analysis measured;
  std::vector<Note> notes;
  std::vector<float> waveform;
  juce::File lineFile;
  std::vector<juce::File> noteFiles;
  uint64_t interpretation = 1;
  double exportGain = 1;
};
// Ownership never leaves the UI/worker. Audio uses a hazard-protected raw view.
class ResultStore {
public:
  const Result *acquire() {
    readers.fetch_add(1);
    return current.load();
  }
  void release() { readers.fetch_sub(1); }
  std::shared_ptr<const Result> snapshot() const {
    std::lock_guard<std::mutex> guard(lock);
    return owner;
  }
  void publish(std::shared_ptr<const Result> p) {
    std::lock_guard<std::mutex> guard(lock);
    if (owner)
      retired.push_back(owner);
    owner = std::move(p);
    serial.fetch_add(1);
    current.store(owner.get());
    collectLocked();
  }
  void collect() {
    std::lock_guard<std::mutex> guard(lock);
    collectLocked();
  }
  std::atomic<uint64_t> serial{0};

private:
  void collectLocked() {
    if (readers.load() == 0)
      retired.clear();
  }
  mutable std::mutex lock;
  std::shared_ptr<const Result> owner;
  std::vector<std::shared_ptr<const Result>> retired;
  std::atomic<const Result *> current{nullptr};
  std::atomic<unsigned> readers{0};
};
inline float sample(const juce::AudioBuffer<float> &b, int channel,
                    double position) {
  if (position < 0 || position >= b.getNumSamples() || b.getNumSamples() == 0)
    return 0;
  int i = int(position);
  float f = float(position - i);
  auto get = [&](int x) {
    return b.getSample(std::min(channel, b.getNumChannels() - 1),
                       juce::jlimit(0, b.getNumSamples() - 1, x));
  };
  return get(i - 1) * (-f * (f - 1) * (f - 2) / 6) +
         get(i) * ((f + 1) * (f - 1) * (f - 2) / 2) +
         get(i + 1) * (-(f + 1) * f * (f - 2) / 2) +
         get(i + 2) * ((f + 1) * f * (f - 1) / 6);
}
inline std::vector<float> overview(const juce::AudioBuffer<float> &b) {
  std::vector<float> data(4096, 0);
  float peak = 0;
  for (int i = 0; i < b.getNumSamples(); ++i) {
    float x = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
      x = std::max(x, std::abs(b.getSample(c, i)));
    auto bin = std::min(size_t(4095), size_t(int64_t(i) * 4096 /
                                             std::max(1, b.getNumSamples())));
    data[bin] = std::max(data[bin], x);
    peak = std::max(peak, x);
  }
  if (peak > 0)
    for (auto &x : data)
      x /= peak;
  return data;
}
inline juce::String keyName(Tonality k) {
  static const char *names[]{"C",  "C#", "D",  "D#", "E",  "F",
                             "F#", "G",  "G#", "A",  "A#", "B"};
  if (k.scale == 0)
    return "Cromatica segura (tonalidad ambigua)";
  return juce::String(names[k.root % 12]) +
         (k.scale == 1 ? " mayor" : " menor") +
         (k.fromHost ? " / proyecto ARA" : " / estimada del audio");
}
} // namespace one
