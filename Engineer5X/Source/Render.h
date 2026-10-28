#pragma once
#include "../ThirdParty/rubberband/rubberband/RubberBandStretcher.h"
#include "Analysis.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <stdexcept>

namespace engineer {
inline constexpr const char *names[]{"V1 CENTERED", "V2 NATURAL DRIFT",
                                     "V3 PHRASE LEAD", "V4 VIBRATO KEEPER",
                                     "V5 TRANSIENT SAFE"};
inline constexpr const char *shortNames[]{"CENTERED", "NATURAL DRIFT",
                                          "PHRASE LEAD", "VIBRATO KEEPER",
                                          "TRANSIENT SAFE"};
inline constexpr const char *descriptions[]{
    "Centro de la nota al 100%.", "Margen natural de 8 cents.",
    "Un ajuste por frase musical.", "Afina el centro del vibrato.",
    "Protege ataques y respiracion."};
struct Settings {
  float strength = 1;
  int scale = 0, root = 0;
};
struct Plan {
  std::vector<float> delta, wet;
};
struct Project {
  juce::File sourceFile;
  juce::AudioBuffer<float> source;
  std::array<juce::AudioBuffer<float>, 5> versions;
  Analysis original;
  std::array<Analysis, 5> measured;
  std::array<Plan, 5> plans;
  std::array<juce::File, 5> files;
  double rate = 48000;
  juce::StringPairArray metadata;
  Settings settings;
  std::vector<float> waveform;
  bool centredRefined = false;
};
inline Plan makePlan(const Analysis &a, int version, Settings settings) {
  Plan p;
  int count = int(a.frames.size());
  p.delta.resize(size_t(count), 0);
  p.wet.resize(size_t(count), 0);
  std::vector<float> base(size_t(count), 0), target(size_t(count), 0);
  for (const auto &region : a.notes)
    for (int i = region.begin; i < region.end; ++i) {
      int radius = version == 3 ? 30 : 16;
      double sum = 0;
      int observations = 0;
      for (int j = std::max(region.begin, i - radius);
           j < std::min(region.end, i + radius + 1); ++j) {
        sum += a.frames[size_t(j)].note;
        ++observations;
      }
      base[size_t(i)] =
          observations ? float(sum / observations) : region.centre;
      target[size_t(i)] = nearest(region.centre, settings.scale, settings.root);
    }
  std::vector<float> phraseOffset(a.phrases.size(), 0);
  for (size_t phrase = 0; phrase < a.phrases.size(); ++phrase) {
    std::vector<float> errors;
    for (int i = 0; i < count; ++i)
      if (a.frames[size_t(i)].voiced &&
          a.frames[size_t(i)].phrase == int(phrase))
        errors.push_back(target[size_t(i)] - base[size_t(i)]);
    phraseOffset[phrase] = median(errors);
  }
  int edge = std::max(2, int(.012 / a.hopSeconds));
  for (int i = 0; i < count; ++i) {
    const auto &f = a.frames[size_t(i)];
    if (!f.voiced || f.region < 0)
      continue;
    float error = target[size_t(i)] - f.note,
          slow = target[size_t(i)] - base[size_t(i)], delta = 0, wet = 1;
    if (version == 0)
      delta = error;
    if (version == 1) {
      bool vibrato = a.notes[size_t(f.region)].vibrato;
      float naturalError = vibrato ? slow : error;
      float e = std::abs(naturalError);
      if (e <= .08f)
        wet = 0;
      else
        delta =
            naturalError * .85f * juce::jlimit(0.f, 1.f, (e - .08f) / .025f);
    }
    if (version == 2 && f.phrase >= 0)
      delta = phraseOffset[size_t(f.phrase)];
    if (version == 3)
      delta = slow;
    if (version == 4)
      delta = error * .95f;
    p.delta[size_t(i)] = juce::jlimit(-1.5f, 1.5f, delta) * settings.strength;
    for (int j = std::max(0, i - edge); j < std::min(count, i + edge + 1); ++j)
      if (!a.frames[size_t(j)].voiced)
        wet = std::min(wet, float(std::abs(i - j)) / edge);
    if (version == 4) {
      int protect = std::max(1, int(.035 / a.hopSeconds));
      for (int j = std::max(0, i - protect);
           j < std::min(count, i + protect + 1); ++j)
        if (a.frames[size_t(j)].attack || a.frames[size_t(j)].air > .24f)
          wet = std::min(
              wet, juce::jlimit(
                       0.f, 1.f,
                       (float(std::abs(i - j)) * float(a.hopSeconds) - .018f) /
                           .017f));
    }
    p.wet[size_t(i)] =
        .5f - .5f * std::cos(juce::MathConstants<float>::pi * wet);
  }
  // Symmetric smoothing: no delay added to the correction envelope.
  float alpha = 1 - std::exp(-float(a.hopSeconds) / .010f);
  for (int i = 1; i < count; ++i)
    p.delta[size_t(i)] = p.delta[size_t(i - 1)] +
                         alpha * (p.delta[size_t(i)] - p.delta[size_t(i - 1)]);
  for (int i = count - 2; i >= 0; --i)
    p.delta[size_t(i)] = p.delta[size_t(i + 1)] +
                         alpha * (p.delta[size_t(i)] - p.delta[size_t(i + 1)]);
  if (settings.strength <= 0)
    std::fill(p.wet.begin(), p.wet.end(), 0);
  return p;
}
inline float at(const std::vector<float> &values, double index) {
  if (values.empty())
    return 0;
  index = juce::jlimit(0., double(values.size() - 1), index);
  int a = int(index), b = std::min(a + 1, int(values.size() - 1));
  float t = float(index - a);
  return values[size_t(a)] * (1 - t) + values[size_t(b)] * t;
}
inline juce::AudioBuffer<float> render(
    const juce::AudioBuffer<float> &source, double sr, const Analysis &analysis,
    const Plan &plan, const Cancel &cancel = [] { return false; }) {
  using RB = RubberBand::RubberBandStretcher;
  int channels = source.getNumChannels(), n = source.getNumSamples();
  juce::AudioBuffer<float> output(channels, n);
  output.clear();
  auto options = RB::OptionProcessRealTime | RB::OptionEngineFiner |
                 RB::OptionThreadingNever | RB::OptionPitchHighConsistency |
                 RB::OptionFormantPreserved | RB::OptionChannelsTogether;
  RB engine(size_t(sr), size_t(channels), options, 1.,
            std::exp2(at(plan.delta, 0) / 12.));
  engine.setMaxProcessSize(256);
  juce::AudioBuffer<float> zero(channels, 256), temp(channels, 4096);
  zero.clear();
  int discard = int(engine.getStartDelay()), written = 0;
  auto drain = [&] {
    int available;
    while ((available = engine.available()) > 0) {
      int got = int(engine.retrieve(temp.getArrayOfWritePointers(),
                                    size_t(std::min(4096, available))));
      if (got <= 0)
        break;
      int skip = std::min(discard, got);
      discard -= skip;
      int count = std::min(got - skip, n - written);
      if (count > 0) {
        for (int c = 0; c < channels; ++c)
          output.copyFrom(c, written, temp, c, skip, count);
        written += count;
      }
    }
  };
  const int sourcePad = int(engine.getPreferredStartPad());
  const int controlLead = sourcePad * 2;
  int pad = sourcePad;
  while (pad > 0) {
    if (cancel())
      return {};
    int block = std::min(256, pad);
    engine.process(zero.getArrayOfReadPointers(), size_t(block), false);
    drain();
    pad -= block;
  }
  std::array<const float *, 2> pointers{};
  for (int i = 0; i < n; i += 256) {
    if (cancel())
      return {};
    int block = std::min(256, n - i);
    // R3 consumes an FFT window behind its input clock. Offline lookahead
    // applies the pitch envelope to the source time being synthesised.
    double frame = (i + controlLead + block * .5) / sr / analysis.hopSeconds;
    engine.setPitchScale(std::exp2(at(plan.delta, frame) / 12.));
    for (int c = 0; c < channels; ++c)
      pointers[size_t(c)] = source.getReadPointer(c, i);
    engine.process(pointers.data(), size_t(block), false);
    drain();
  }
  // Explicit flush; trim compensated DSP output to the original exact duration.
  for (int i = 0; i < 32768; i += 256) {
    if (cancel())
      return {};
    engine.setPitchScale(1);
    engine.process(zero.getArrayOfReadPointers(), 256, i + 256 == 32768);
    drain();
  }
  if (written < n)
    throw std::runtime_error("Rubber Band no completo el render alineado");
  for (int i = 0; i < n; ++i) {
    float wet = at(plan.wet, i / sr / analysis.hopSeconds);
    for (int c = 0; c < channels; ++c) {
      float dry = source.getSample(c, i), processed = output.getSample(c, i);
      if (!std::isfinite(processed))
        throw std::runtime_error("El render contiene muestras no finitas");
      output.setSample(c, i, wet <= 0 ? dry : dry + (processed - dry) * wet);
    }
  }
  return output;
}
inline void writeWav(const juce::File &file,
                     const juce::AudioBuffer<float> &audio, double sr,
                     juce::StringPairArray metadata) {
  metadata.set(juce::WavAudioFormat::bwavDescription,
               "Makkine Engineer 5X - aligned natural vocal tuning");
  metadata.set(juce::WavAudioFormat::bwavOriginator, "Makkine Seven Golden");
  auto stream = file.createOutputStream();
  if (!stream)
    throw std::runtime_error("No se pudo crear el archivo WAV");
  stream->setPosition(0);
  stream->truncate();
  juce::WavAudioFormat format;
  std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(
      stream.get(), sr, unsigned(audio.getNumChannels()), 32, metadata, 0));
  if (!writer)
    throw std::runtime_error("No se pudo crear el WAV 32-bit float");
  stream.release();
  if (!writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()))
    throw std::runtime_error("Error escribiendo el WAV completo");
}
inline std::shared_ptr<Project>
buildProject(const juce::File &file, Settings settings,
             const juce::File &folder, const Cancel &cancel,
             const std::function<void(float, const juce::String &)> &progress) {
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(
      formats.createReaderFor(file));
  if (!reader || reader->lengthInSamples <= 0 || reader->numChannels < 1 ||
      reader->numChannels > 2 || reader->sampleRate < 22050 ||
      reader->sampleRate > 192000)
    throw std::runtime_error(
        "Carga una vocal mono/estereo WAV, AIFF o FLAC entre 22.05 y 192 kHz");
  uint64_t bytes =
      uint64_t(reader->lengthInSamples) * reader->numChannels * 4 * 6;
  if (reader->lengthInSamples > reader->sampleRate * 600 ||
      bytes > 512ull * 1024 * 1024)
    throw std::runtime_error("La vocal supera 10 minutos o 512 MB de render. "
                             "Usa un fragmento o un archivo mono");
  auto p = std::make_shared<Project>();
  p->sourceFile = file;
  p->rate = reader->sampleRate;
  p->metadata = reader->metadataValues;
  p->settings = settings;
  p->source.setSize(int(reader->numChannels), int(reader->lengthInSamples));
  if (!reader->read(&p->source, 0, p->source.getNumSamples(), 0, true, true))
    throw std::runtime_error("No se pudo leer la vocal completa");
  for (int c = 0; c < p->source.getNumChannels(); ++c)
    for (int i = 0; i < p->source.getNumSamples(); ++i)
      if (!std::isfinite(p->source.getSample(c, i)))
        throw std::runtime_error("La vocal contiene muestras no finitas");
  p->waveform.resize(4096, 0);
  float maximum = 0;
  for (int i = 0; i < p->source.getNumSamples(); ++i) {
    float peak = 0;
    for (int c = 0; c < p->source.getNumChannels(); ++c)
      peak = std::max(peak, std::abs(p->source.getSample(c, i)));
    size_t bin = std::min(
        size_t(4095), size_t(int64_t(i) * 4096 / p->source.getNumSamples()));
    p->waveform[bin] = std::max(p->waveform[bin], peak);
    maximum = std::max(maximum, peak);
  }
  if (maximum > 0)
    for (auto &peak : p->waveform)
      peak /= maximum;
  progress(.02f, "Analizando notas, frases, vibrato, pausas y ruido...");
  Analyzer analyzer;
  p->original = analyzer.run(p->source, p->rate, cancel);
  if (cancel())
    return {};
  if (folder.createDirectory().failed())
    throw std::runtime_error("No se pudo crear la carpeta de exportacion");
  for (int v = 0; v < 5; ++v) {
    progress(.10f + v * .18f,
             "Generando " + juce::String(names[v]) + " con formantes...");
    p->plans[size_t(v)] = makePlan(p->original, v, settings);
    p->versions[size_t(v)] =
        render(p->source, p->rate, p->original, p->plans[size_t(v)], cancel);
    if (cancel())
      return {};
    progress(.20f + v * .18f,
             "Midiendo el pitch generado de " + juce::String(names[v]) + "...");
    p->measured[size_t(v)] =
        analyzer.run(p->versions[size_t(v)], p->rate, cancel);
    if (cancel())
      return {};
    if (v == 0 && settings.strength > .01f) {
      // One bounded feedback step. Re-render FROM THE ORIGINAL, avoiding a
      // second resynthesis cascade. Only measured, stable voiced residuals
      // count.
      auto refined = p->plans[0];
      std::vector<float> residual(refined.delta.size(), 0);
      std::vector<uint8_t> eligible(refined.delta.size(), 0);
      double power = 0;
      int observations = 0;
      for (size_t i = 0;
           i < residual.size() && i < p->measured[0].frames.size(); ++i) {
        const auto &f = p->original.frames[i];
        const auto &actual = p->measured[0].frames[i];
        if (!f.voiced || !actual.voiced || f.region < 0 || f.attack ||
            refined.wet[i] < .99f)
          continue;
        const auto &region = p->original.notes[size_t(f.region)];
        if (int(i) < region.begin + 5 || int(i) >= region.end - 5)
          continue;
        float target = nearest(region.centre, settings.scale, settings.root);
        float destination = f.note + (target - f.note) * settings.strength;
        float error = destination - actual.note;
        if (std::abs(error) > .20f)
          continue;
        eligible[i] = 1;
        residual[i] = error * .85f * settings.strength;
        power += error * error;
        ++observations;
      }
      if (observations > 15 && std::sqrt(power / observations) > .02) {
        float alpha = 1 - std::exp(-float(p->original.hopSeconds) / .006f);
        for (size_t i = 1; i < residual.size(); ++i)
          residual[i] =
              residual[i - 1] + alpha * (residual[i] - residual[i - 1]);
        for (int i = int(residual.size()) - 2; i >= 0; --i)
          residual[size_t(i)] =
              residual[size_t(i + 1)] +
              alpha * (residual[size_t(i)] - residual[size_t(i + 1)]);
        for (size_t i = 0; i < residual.size(); ++i)
          refined.delta[i] =
              juce::jlimit(-1.5f, 1.5f, refined.delta[i] + residual[i]);
        progress(.24f, "Verificando y refinando V1 CENTERED...");
        auto improved =
            render(p->source, p->rate, p->original, refined, cancel);
        if (cancel())
          return {};
        auto measured = analyzer.run(improved, p->rate, cancel);
        if (cancel())
          return {};
        double nextPower = 0;
        int nextCount = 0;
        for (size_t i = 0; i < residual.size() && i < measured.frames.size();
             ++i)
          if (eligible[i]) {
            const auto &original = p->original.frames[i];
            float target =
                nearest(p->original.notes[size_t(original.region)].centre,
                        settings.scale, settings.root);
            float destination =
                original.note + (target - original.note) * settings.strength;
            float error = measured.frames[i].voiced
                              ? destination - measured.frames[i].note
                              : 1.f;
            nextPower += error * error;
            ++nextCount;
          }
        if (nextCount > 15 && nextPower / nextCount < power / observations) {
          p->versions[0] = std::move(improved);
          p->measured[0] = std::move(measured);
          p->plans[0] = std::move(refined);
          p->centredRefined = true;
        }
      }
    }
    juce::String suffix = juce::String(names[v]).replaceCharacter(' ', '-');
    p->files[size_t(v)] = folder.getChildFile(
        file.getFileNameWithoutExtension() + "-" + suffix + ".wav");
    writeWav(p->files[size_t(v)], p->versions[size_t(v)], p->rate, p->metadata);
  }
  progress(1, "Cinco versiones listas. Escucha, compara y arrastra el WAV.");
  return p;
}
} // namespace engineer
