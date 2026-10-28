#pragma once
#include "Wave.h"

namespace one {
inline float variation(uint64_t version, int note, int purpose) {
  uint64_t x = version * 0x9e3779b97f4a7c15ull +
               uint64_t(note + 1) * 0xbf58476d1ce4e5b9ull +
               uint64_t(purpose + 1) * 0x94d049bb133111ebull;
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ull;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebull;
  x ^= x >> 31;
  return float((x >> 40) & 0xffffff) / 16777216.f;
}
inline Tonality inferKey(const Analysis &analysis, Origin origin) {
  if (origin.hostRoot >= 0 && origin.hostRoot < 12 && origin.hostScale >= 1 &&
      origin.hostScale <= 2)
    return {origin.hostRoot, origin.hostScale, 1, true};
  static constexpr float major[]{6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
                                 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f};
  static constexpr float minor[]{6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f,
                                 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f};
  std::array<double, 12> histogram{};
  for (const auto &region : analysis.notes) {
    int pc = ((int(std::round(region.centre)) % 12) + 12) % 12;
    histogram[size_t(pc)] += (region.end - region.begin) * analysis.hopSeconds;
  }
  double total = std::accumulate(histogram.begin(), histogram.end(), 0.);
  int distinct = 0;
  for (auto x : histogram)
    if (x > total * .025)
      ++distinct;
  if (total < .3 || distinct < 3)
    return {};
  double mean = total / 12, energy = 0;
  for (auto x : histogram)
    energy += (x - mean) * (x - mean);
  float best = -2, second = -2;
  Tonality result;
  for (int root = 0; root < 12; ++root)
    for (int mode = 1; mode <= 2; ++mode) {
      const auto *profile = mode == 1 ? major : minor;
      double pmean = 0;
      for (int k = 0; k < 12; ++k)
        pmean += profile[k] / 12.;
      double score = 0, power = 0, coverage = 0;
      for (int k = 0; k < 12; ++k) {
        double v = profile[(k - root + 12) % 12] - pmean;
        score += (histogram[size_t(k)] - mean) * v;
        power += v * v;
        if (allowed(k, mode, root))
          coverage += histogram[size_t(k)];
      }
      float value = float(score / std::sqrt(energy * power + 1e-20)) * .75f +
                    float(coverage / total) * .25f;
      if (value > best) {
        second = best;
        best = value;
        result.root = root;
        result.scale = mode;
      } else
        second = std::max(second, value);
    }
  result.confidence =
      juce::jlimit(0.f, 1.f, (best - second) * 7.f + (best - .65f) * .7f);
  if (result.confidence < .22f)
    result.scale = 0;
  return result;
}
inline std::vector<Note> automaticNotes(const Input &input, uint64_t version) {
  const auto &regions = input.analysis.notes;
  int count = int(regions.size());
  std::vector<Note> notes;
  notes.reserve(regions.size());
  if (count == 0)
    return notes;
  constexpr int choices = 5;
  std::vector<std::array<float, choices>> cost(static_cast<size_t>(count));
  std::vector<std::array<int, choices>> previous(static_cast<size_t>(count));
  std::vector<std::array<int, choices>> candidates(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    const auto &region = regions[size_t(i)];
    int centre = int(std::round(region.centre));
    double duration = (region.end - region.begin) * input.analysis.hopSeconds;
    for (int j = 0; j < choices; ++j) {
      int target = centre + j - 2;
      candidates[size_t(i)][size_t(j)] = target;
      float distance = region.centre - target;
      bool inScale = allowed(target, input.key.scale, input.key.root);
      float emission =
          distance * distance * 5.f + (!inScale && duration > .14 ? .9f : 0.f);
      emission += variation(version, i, j) * .008f;
      float best = 1e20f;
      int bestIndex = 0;
      if (i == 0)
        best = 0;
      else
        for (int k = 0; k < choices; ++k) {
          float jump = float(target - candidates[size_t(i - 1)][size_t(k)]);
          float sourceJump = region.centre - regions[size_t(i - 1)].centre;
          float intentionPenalty = std::pow(jump - sourceJump, 2) *
                                   (.09f + .03f * variation(version, i, 7));
          float trial = cost[size_t(i - 1)][size_t(k)] + intentionPenalty;
          if (trial < best) {
            best = trial;
            bestIndex = k;
          }
        }
      cost[size_t(i)][size_t(j)] = best + emission;
      previous[size_t(i)][size_t(j)] = bestIndex;
    }
  }
  int selected = int(std::min_element(cost.back().begin(), cost.back().end()) -
                     cost.back().begin());
  std::vector<int> target(static_cast<size_t>(count));
  for (int i = count - 1; i >= 0; --i) {
    target[size_t(i)] = candidates[size_t(i)][size_t(selected)];
    selected = previous[size_t(i)][size_t(selected)];
  }
  for (int i = 0; i < count; ++i) {
    const auto &r = regions[size_t(i)];
    Note n;
    n.id = i;
    n.begin = input.analysis.frames[size_t(r.begin)].time;
    n.end = std::min(input.audio.getNumSamples() / input.rate,
                     input.analysis.frames[size_t(r.end - 1)].time +
                         input.analysis.hopSeconds);
    n.original = r.centre;
    n.target = float(target[size_t(i)]);
    n.vibrato = .92f + .14f * variation(version, i, 9);
    notes.push_back(n);
  }
  return notes;
}
inline Plan makePlan(const Input &input, const std::vector<Note> &notes,
                     uint64_t version) {
  const auto &a = input.analysis;
  Plan p;
  p.delta.resize(a.frames.size(), 0);
  p.wet.resize(a.frames.size(), 0);
  for (const auto &note : notes) {
    const auto &region = a.notes[size_t(note.id)];
    for (int i = region.begin; i < region.end; ++i) {
      const auto &f = a.frames[size_t(i)];
      int radius = 26;
      double mean = 0;
      int observations = 0;
      for (int j = std::max(region.begin, i - radius);
           j < std::min(region.end, i + radius + 1); ++j) {
        mean += a.frames[size_t(j)].note;
        ++observations;
      }
      float trend = observations ? float(mean / observations) : region.centre;
      float expression = f.note - trend;
      // Stable centres stay on musical notes; interpretations change human
      // motion, transition handling and vibrato, never add a static random
      // detune.
      float micro = 0;
      double duration = note.end - note.begin;
      if (!region.vibrato && duration > .45 && version > 1) {
        double t = f.time - note.begin;
        double phase = 2 * juce::MathConstants<double>::pi *
                       (4.8 + variation(version, note.id, 2)) * t;
        double envelope = std::pow(
            std::sin(juce::MathConstants<double>::pi * t / duration), 2);
        micro = float(std::sin(phase) * envelope) *
                (.008f + .018f * variation(version, note.id, 3));
      }
      p.delta[size_t(i)] = juce::jlimit(
          -2.f, 2.f, note.target - f.note + expression * note.vibrato + micro);
      float wet = 1;
      int edge = 3;
      for (int j = std::max(0, i - edge);
           j < std::min(int(a.frames.size()), i + edge + 1); ++j)
        if (!a.frames[size_t(j)].voiced)
          wet = std::min(wet, float(std::abs(i - j)) / edge);
      int protect = 6;
      for (int j = std::max(0, i - protect);
           j < std::min(int(a.frames.size()), i + protect + 1); ++j)
        if (a.frames[size_t(j)].attack || a.frames[size_t(j)].air > .24f)
          wet = std::min(
              wet, juce::jlimit(
                       0.f, 1.f,
                       (float(std::abs(i - j)) * float(a.hopSeconds) - .012f) /
                           .018f));
      p.wet[size_t(i)] =
          .5f - .5f * std::cos(juce::MathConstants<float>::pi * wet);
    }
  }
  float seconds = .012f + .016f * variation(version, 0, 19);
  float alpha = 1 - std::exp(-float(a.hopSeconds) / seconds);
  for (size_t i = 1; i < p.delta.size(); ++i)
    p.delta[i] = p.delta[i - 1] + alpha * (p.delta[i] - p.delta[i - 1]);
  for (int i = int(p.delta.size()) - 2; i >= 0; --i)
    p.delta[size_t(i)] = p.delta[size_t(i + 1)] +
                         alpha * (p.delta[size_t(i)] - p.delta[size_t(i + 1)]);
  return p;
}
inline void applyEdits(juce::AudioBuffer<float> &audio, double rate,
                       const std::vector<Note> &notes) {
  // Gain and time edits are optional. The automatic path has zero time
  // shift/gain.
  juce::AudioBuffer<float> untouched(audio);
  int n = audio.getNumSamples();
  for (const auto &note : notes) {
    int first = juce::jlimit(0, n, int(std::llround(note.begin * rate))),
        last = juce::jlimit(first, n, int(std::llround(note.end * rate)));
    int length = last - first;
    if (length <= 0)
      continue;
    float gain = juce::Decibels::decibelsToGain(note.gainDb);
    int shift = int(std::llround(note.timeShift * rate));
    int fade = std::min(length / 2, std::max(1, int(rate * .008)));
    if (shift == 0 && std::abs(note.gainDb) < .001)
      continue;
    for (int i = 0; i < length; ++i) {
      float envelope = juce::jlimit(
          0.f, 1.f, std::min(float(i) / fade, float(length - 1 - i) / fade));
      int dest = first + i + shift;
      for (int c = 0; c < audio.getNumChannels(); ++c) {
        float original = untouched.getSample(c, first + i);
        if (shift == 0)
          audio.addSample(c, first + i, original * (gain - 1) * envelope);
        else {
          audio.addSample(c, first + i, -original * envelope);
          if (dest >= 0 && dest < n)
            audio.addSample(c, dest, original * envelope * gain);
        }
      }
    }
  }
}
inline std::shared_ptr<Input> readInput(const juce::File &file, Origin origin,
                                        const Cancel &cancel) {
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(
      formats.createReaderFor(file));
  if (!reader || reader->numChannels < 1 || reader->numChannels > 2 ||
      !std::isfinite(reader->sampleRate) || reader->sampleRate < 22050 ||
      reader->sampleRate > 192000 || reader->lengthInSamples <= 0 ||
      reader->lengthInSamples > reader->sampleRate * 600 ||
      uint64_t(reader->lengthInSamples) * reader->numChannels * 4 >
          180ull * 1024 * 1024)
    throw std::runtime_error("Carga/captura una vocal mono o estereo de hasta "
                             "10 minutos (180 MB PCM)");
  if (!origin.positionKnown && reader->metadataValues.containsKey(
                                   juce::WavAudioFormat::bwavTimeReference)) {
    origin.seconds =
        reader->metadataValues[juce::WavAudioFormat::bwavTimeReference]
            .getLargeIntValue() /
        reader->sampleRate;
    origin.positionKnown = true;
  }
  auto p = std::make_shared<Input>();
  p->rate = reader->sampleRate;
  p->audio.setSize(int(reader->numChannels), int(reader->lengthInSamples));
  p->origin = origin;
  p->rawFile = file;
  if (!reader->read(&p->audio, 0, p->audio.getNumSamples(), 0, true, true))
    throw std::runtime_error("No se pudo leer la captura completa");
  for (int c = 0; c < p->audio.getNumChannels(); ++c)
    for (int i = 0; i < p->audio.getNumSamples(); ++i)
      if (!std::isfinite(p->audio.getSample(c, i)))
        throw std::runtime_error("La captura contiene audio no finito");
  p->analysis = Analyzer().run(p->audio, p->rate, cancel);
  if (cancel())
    return {};
  p->key = inferKey(p->analysis, origin);
  p->waveform = overview(p->audio);
  return p;
}
inline std::shared_ptr<Result>
generate(std::shared_ptr<const Input> input, uint64_t version,
         const std::vector<Note> &retouch, const juce::File &folder,
         const Cancel &cancel,
         const std::function<void(float, const juce::String &)> &progress) {
  auto p = std::make_shared<Result>();
  p->input = std::move(input);
  p->interpretation = version;
  p->notes = retouch.empty() ? automaticNotes(*p->input, version) : retouch;
  progress(.12f, "IA: afinando centros musicales y conservando vibrato...");
  auto plan = makePlan(*p->input, p->notes, version);
  p->audio =
      render(p->input->audio, p->input->rate, p->input->analysis, plan, cancel);
  if (cancel())
    return {};
  applyEdits(p->audio, p->input->rate, p->notes);
  progress(.65f, "IA: comprobando el resultado vocal...");
  p->measured = Analyzer().run(p->audio, p->input->rate, cancel);
  if (cancel())
    return {};
  p->waveform = overview(p->audio);
  progress(.80f, "Preparando WAV 24-bit / 48 kHz con origen de la toma...");
  exportResult(*p, folder, cancel);
  if (cancel())
    return {};
  progress(1, "Version IA " + juce::String(version) +
                  " lista. Escucha o arrastra al proyecto.");
  return p;
}
} // namespace one
