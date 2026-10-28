#pragma once
#include "Model.h"

namespace one {
// Offline bandlimited conversion, 128 taps, Blackman window, polyphase table.
// This is never called from the audio callback or mouseDrag.
inline juce::AudioBuffer<float> at48k(const juce::AudioBuffer<float> &source,
                                      double rate, const Cancel &cancel) {
  int count = int(std::llround(source.getNumSamples() * 48000. / rate));
  if (std::abs(rate - 48000) < .01)
    return juce::AudioBuffer<float>(source);
  juce::AudioBuffer<float> output(source.getNumChannels(), count);
  output.clear();
  constexpr int taps = 128, phases = 1024;
  std::vector<std::array<float, taps>> table(phases);
  double cutoff = .94 * std::min(1., 48000. / rate);
  for (int phase = 0; phase < phases; ++phase) {
    double sum = 0;
    for (int j = 0; j < taps; ++j) {
      double x = j - (taps / 2 - 1) - double(phase) / phases;
      double sinc =
          std::abs(x) < 1e-10
              ? cutoff
              : std::sin(juce::MathConstants<double>::pi * x * cutoff) /
                    (juce::MathConstants<double>::pi * x);
      double window =
          .42 -
          .5 * std::cos(2 * juce::MathConstants<double>::pi * j / (taps - 1)) +
          .08 * std::cos(4 * juce::MathConstants<double>::pi * j / (taps - 1));
      table[size_t(phase)][size_t(j)] = float(sinc * window);
      sum += sinc * window;
    }
    for (auto &w : table[size_t(phase)])
      w /= float(sum);
  }
  for (int i = 0; i < count; ++i) {
    if ((i & 4095) == 0 && cancel())
      return {};
    double pos = i * rate / 48000.;
    int at = int(pos),
        phase = juce::jlimit(0, phases - 1, int((pos - at) * phases));
    for (int c = 0; c < source.getNumChannels(); ++c) {
      double sum = 0;
      for (int j = 0; j < taps; ++j) {
        int index = at + j - (taps / 2 - 1);
        if (index >= 0 && index < source.getNumSamples())
          sum += source.getSample(c, index) * table[size_t(phase)][size_t(j)];
      }
      output.setSample(c, i, float(sum));
    }
  }
  return output;
}
inline double headroom(const juce::AudioBuffer<float> &b) {
  float peak = 0;
  for (int c = 0; c < b.getNumChannels(); ++c)
    peak = std::max(peak, b.getMagnitude(c, 0, b.getNumSamples()));
  return peak > .999f ? .966 / peak : 1.;
}
inline void write24(const juce::File &file,
                    const juce::AudioBuffer<float> &audio,
                    double absoluteSeconds, bool known, double gain = 1) {
  juce::StringPairArray meta;
  meta.set(juce::WavAudioFormat::bwavDescription,
           "Makkine Engineer ONE - vocal edit - 48000 Hz 24-bit");
  meta.set(juce::WavAudioFormat::bwavOriginator, "Makkine Seven Golden");
  if (known && absoluteSeconds >= 0)
    meta.set(juce::WavAudioFormat::bwavTimeReference,
             juce::String(int64_t(std::llround(absoluteSeconds * 48000))));
  auto stream = file.createOutputStream();
  if (!stream)
    throw std::runtime_error("No se puede crear el WAV temporal");
  stream->setPosition(0);
  stream->truncate();
  juce::WavAudioFormat format;
  std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(
      stream.get(), 48000, unsigned(audio.getNumChannels()), 24, meta, 0));
  if (!writer)
    throw std::runtime_error("No se pudo crear el WAV 24-bit");
  stream.release();
  juce::AudioBuffer<float> block(audio.getNumChannels(), 4096);
  uint32_t random = 0x57415631;
  auto uniform = [&]() {
    random = random * 1664525u + 1013904223u;
    return double(random) / 4294967296.;
  };
  for (int i = 0; i < audio.getNumSamples(); i += 4096) {
    int n = std::min(4096, audio.getNumSamples() - i);
    for (int c = 0; c < audio.getNumChannels(); ++c)
      for (int j = 0; j < n; ++j) {
        float x = float(audio.getSample(c, i + j) * gain);
        if (!std::isfinite(x) || std::abs(x) > 1.0001f)
          throw std::runtime_error("Nivel de exportacion invalido");
        double dither = x == 0 ? 0 : (uniform() - uniform()) / 8388608.;
        block.setSample(c, j, float(x + dither));
      }
    if (!writer->writeFromAudioSampleBuffer(block, 0, n))
      throw std::runtime_error("No se completo el WAV exportado");
  }
}
inline void exportResult(Result &result, const juce::File &folder,
                         const Cancel &cancel) {
  if (folder.createDirectory().failed())
    throw std::runtime_error("No se pudo crear la carpeta temporal");
  auto converted = at48k(result.audio, result.input->rate, cancel);
  if (cancel())
    return;
  result.exportGain = headroom(converted);
  result.lineFile = folder.getChildFile(
      "Makkine-ONE-linea-IA-" + juce::String(result.interpretation) + ".wav");
  write24(result.lineFile, converted, result.input->origin.seconds,
          result.input->origin.positionKnown, result.exportGain);
  result.noteFiles.clear();
  for (const auto &note : result.notes) {
    if (cancel())
      return;
    int first =
        juce::jlimit(0, converted.getNumSamples(),
                     int(std::llround((note.begin + note.timeShift) * 48000)));
    int last =
        juce::jlimit(first, converted.getNumSamples(),
                     int(std::llround((note.end + note.timeShift) * 48000)));
    int n = last - first;
    juce::AudioBuffer<float> single(converted.getNumChannels(), std::max(1, n));
    single.clear();
    if (n > 0)
      for (int c = 0; c < single.getNumChannels(); ++c)
        single.copyFrom(c, 0, converted, c, first, n);
    int fade = std::min(n / 2, 240);
    for (int c = 0; c < single.getNumChannels(); ++c) {
      single.applyGainRamp(c, 0, fade, 0, 1);
      single.applyGainRamp(c, std::max(0, n - fade), fade, 1, 0);
    }
    auto file = folder.getChildFile("Makkine-ONE-nota-" +
                                    juce::String(note.id + 1) + ".wav");
    write24(file, single, result.input->origin.seconds + first / 48000.,
            result.input->origin.positionKnown, result.exportGain);
    result.noteFiles.push_back(file);
  }
}
} // namespace one
