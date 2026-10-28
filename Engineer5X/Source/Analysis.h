#pragma once
#include <functional>
#include <juce_dsp/juce_dsp.h>
#include <numeric>

namespace engineer {
using Cancel = std::function<bool()>;
struct Frame {
  double time = 0;
  float note = 0, confidence = 0, rms = 0, air = 0;
  bool voiced = false, attack = false;
  int phrase = -1, region = -1;
};
struct Region {
  int begin = 0, end = 0;
  float centre = 0;
  bool vibrato = false;
};
struct Analysis {
  std::vector<Frame> frames;
  std::vector<Region> notes;
  std::vector<std::pair<int, int>> phrases;
  float noiseDb = -78;
  double hopSeconds = .005;
};
inline float median(std::vector<float> x) {
  if (x.empty())
    return 0;
  auto middle = x.begin() + std::ptrdiff_t(x.size() / 2);
  std::nth_element(x.begin(), middle, x.end());
  return *middle;
}
inline float noteFromHz(double f) {
  return float(69 + 12 * std::log2(f / 440.));
}
inline bool allowed(int note, int scale, int root) {
  if (scale == 0)
    return true;
  static constexpr int major[]{0, 2, 4, 5, 7, 9, 11},
      minor[]{0, 2, 3, 5, 7, 8, 10};
  int pc = ((note - root) % 12 + 12) % 12;
  for (int n : (scale == 1 ? major : minor))
    if (n == pc)
      return true;
  return false;
}
inline float nearest(float note, int scale, int root) {
  int centre = int(std::round(note)), best = centre;
  float distance = 100;
  for (int n = centre - 2; n <= centre + 2; ++n)
    if (allowed(n, scale, root) && std::abs(note - n) < distance) {
      distance = std::abs(note - n);
      best = n;
    }
  return float(best);
}

// Centred offline YIN, FFT autocorrelation and confidence/continuity decisions.
// No cloud, generative model, voice identification or proprietary Melodyne
// code.
class Analyzer {
public:
  Analysis run(
      const juce::AudioBuffer<float> &source, double sr,
      const Cancel &cancel = [] { return false; }) {
    Analysis result;
    const int n = source.getNumSamples(), channels = source.getNumChannels();
    if (n <= 0 || channels <= 0)
      return result;
    const int factor = std::max(1, int(std::round(sr / 12000.)));
    const double ar = sr / factor;
    std::vector<float> mono(size_t(n), 0);
    double sumEnergy = 0, leftEnergy = 0;
    for (int i = 0; i < n; ++i) {
      if ((i & 65535) == 0 && cancel())
        return {};
      float a = source.getSample(0, i), b = source.getSample(channels - 1, i);
      mono[size_t(i)] = (a + b) * .5f;
      sumEnergy += mono[size_t(i)] * mono[size_t(i)];
      leftEnergy += a * a;
    }
    if (sumEnergy < leftEnergy * .04)
      std::copy(source.getReadPointer(0), source.getReadPointer(0) + n,
                mono.begin());
    const int energyHop = std::max(1, int(sr * .01));
    std::vector<float> powers;
    for (int i = 0; i < n; i += energyHop) {
      double sum = 0;
      int count = std::min(energyHop, n - i);
      for (int j = 0; j < count; ++j)
        sum += mono[size_t(i + j)] * mono[size_t(i + j)];
      powers.push_back(float(sum / std::max(1, count)));
    }
    auto sorted = powers;
    std::sort(sorted.begin(), sorted.end());
    float quiet = sorted[sorted.size() / 10],
          loud = sorted[sorted.size() * 9 / 10];
    if (loud > quiet * 10 + 1e-8f)
      result.noiseDb =
          juce::jlimit(-90.f, -24.f, 10 * std::log10(quiet + 1e-12f));
    float threshold =
        std::max(.0012f, std::pow(10.f, result.noiseDb / 20) * 2.7f);
    // Hysteresis and 240 ms pause grouping; retain all silence in the source.
    bool open = false;
    int first = 0, last = 0;
    for (int k = 0; k < int(powers.size()); ++k) {
      float rms = std::sqrt(powers[size_t(k)]);
      if (rms > threshold) {
        if (!open) {
          first = std::max(0, k * energyHop - int(sr * .008));
          open = true;
        }
        last = std::min(n, (k + 1) * energyHop);
      } else if (open && rms > threshold * .55f)
        last = std::min(n, (k + 1) * energyHop);
      if (open && k * energyHop - last > sr * .24) {
        result.phrases.emplace_back(first, last);
        open = false;
      }
    }
    if (open)
      result.phrases.emplace_back(first, last);
    juce::dsp::IIR::Filter<float> lp1, lp2;
    lp1.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sr, ar * .38, .5411961f);
    lp2.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sr, ar * .38, 1.306563f);
    std::vector<float> reduced;
    reduced.reserve(size_t(n / factor + 1));
    for (int i = 0; i < n; ++i) {
      float v = lp2.processSample(lp1.processSample(mono[size_t(i)]));
      if (i % factor == 0)
        reduced.push_back(v);
    }
    const int hop = std::max(1, int(ar * .005));
    float previousHz = 0;
    result.hopSeconds = hop / ar;
    juce::dsp::FFT fft(11);
    std::array<juce::dsp::Complex<float>, 2048> in{}, spectrum{}, acf{};
    std::array<double, 1025> energy{};
    std::array<float, 513> cmnd{}, corr{};
    int phrase = 0;
    for (int centre = 0; centre < int(reduced.size()); centre += hop) {
      if (cancel())
        return {};
      Frame f;
      f.time = centre / ar;
      int sample = std::min(n - 1, int(std::round(f.time * sr)));
      int e = std::min(int(powers.size()) - 1, sample / energyHop);
      f.rms = std::sqrt(std::max(0.f, powers[size_t(e)]));
      float prev = e > 1 ? powers[size_t(e - 2)] : 0;
      f.attack = f.rms > .006f && powers[size_t(e)] > prev * 3.5f + 1e-6f;
      while (phrase < int(result.phrases.size()) &&
             sample >= result.phrases[size_t(phrase)].second)
        ++phrase;
      if (phrase < int(result.phrases.size()) &&
          sample >= result.phrases[size_t(phrase)].first)
        f.phrase = phrase;
      if (f.rms < threshold || f.phrase < 0) {
        previousHz = 0;
        result.frames.push_back(f);
        continue;
      }
      int window = previousHz > 0
                       ? juce::jlimit(256, 1024, int(ar / previousHz * 4))
                       : 1024;
      in.fill({});
      double mean = 0;
      int firstSample = centre - window / 2 + 4;
      for (int j = 0; j < window; ++j) {
        int at = firstSample + j;
        if (at >= 0 && at < int(reduced.size()))
          mean += reduced[size_t(at)];
      }
      mean /= window;
      energy[0] = 0;
      for (int j = 0; j < window; ++j) {
        int at = firstSample + j;
        float x =
            (at >= 0 && at < int(reduced.size()) ? reduced[size_t(at)] : 0) -
            float(mean);
        in[size_t(j)] = {x, 0};
        energy[size_t(j + 1)] = energy[size_t(j)] + x * x;
      }
      fft.perform(in.data(), spectrum.data(), false);
      double total = 0, high = 0;
      for (int k = 1; k < 1024; ++k) {
        double p = std::norm(spectrum[size_t(k)]);
        total += p;
        if (k * ar / 2048. > 3000)
          high += p;
      }
      f.air = float(high / (total + 1e-20));
      for (int k = 0; k < 2048; ++k)
        spectrum[size_t(k)] = {std::norm(spectrum[size_t(k)]), 0};
      fft.perform(spectrum.data(), acf.data(), true);
      double cumulative = 0;
      cmnd[0] = corr[0] = 1;
      int lo = std::max(2, int(ar / 1100)),
          hi = std::min(window / 2 - 2, int(ar / 55));
      for (int lag = 1; lag <= window / 2; ++lag) {
        double a = energy[size_t(window - lag)],
               b = energy[window] - energy[size_t(lag)],
               r = acf[size_t(lag)].real();
        double d = std::max(0., a + b - 2 * r);
        cumulative += d;
        cmnd[size_t(lag)] = float(d * lag / (cumulative + 1e-20));
        corr[size_t(lag)] = float(r / std::sqrt(a * b + 1e-20));
      }
      int chosen = 0;
      for (int lag = lo + 1; lag < hi; ++lag)
        if (cmnd[size_t(lag)] < .16f &&
            cmnd[size_t(lag)] <= cmnd[size_t(lag - 1)] &&
            cmnd[size_t(lag)] < cmnd[size_t(lag + 1)]) {
          chosen = lag;
          break;
        }
      if (chosen) {
        float a = cmnd[size_t(chosen - 1)], b = cmnd[size_t(chosen)],
              c = cmnd[size_t(chosen + 1)], den = a - 2 * b + c;
        float fraction = std::abs(den) > 1e-9f
                             ? juce::jlimit(-.5f, .5f, .5f * (a - c) / den)
                             : 0;
        f.confidence =
            juce::jlimit(0.f, 1.f, .7f * corr[size_t(chosen)] + .3f * (1 - b));
        float hz = float(ar / (chosen + fraction));
        f.voiced =
            f.confidence > .83f && f.air < .48f && hz >= 55 && hz <= 1100;
        if (f.voiced)
          f.note = noteFromHz(hz);
      }
      previousHz = f.voiced ? float(440 * std::exp2((f.note - 69) / 12)) : 0;
      result.frames.push_back(f);
    }
    auto original = result.frames;
    for (size_t i = 1; i + 1 < result.frames.size(); ++i) {
      auto &f = result.frames[i];
      const auto &a = original[i - 1];
      const auto &b = original[i + 1];
      if (f.voiced && a.voiced && b.voiced && std::abs(a.note - b.note) < .5f)
        f.note = median({a.note, original[i].note, b.note});
      if (f.voiced && !a.voiced && !b.voiced)
        f.voiced = false;
    }
    // Breath/noise can cross the energy gate. Only confident voiced frames
    // extend a musical phrase, so a noisy gap cannot glue two phrases together.
    result.phrases.clear();
    int previousVoiced = -1000000;
    for (int i = 0; i < int(result.frames.size()); ++i) {
      auto &f = result.frames[size_t(i)];
      f.phrase = -1;
      if (!f.voiced)
        continue;
      int sample = int(std::round(f.time * sr));
      if ((i - previousVoiced) * result.hopSeconds > .24)
        result.phrases.emplace_back(sample, sample);
      f.phrase = int(result.phrases.size()) - 1;
      result.phrases.back().second =
          std::min(n, sample + int(sr * result.hopSeconds));
      previousVoiced = i;
    }
    for (size_t i = 0; i < result.frames.size();) {
      if (!result.frames[i].voiced) {
        ++i;
        continue;
      }
      size_t end = i + 1;
      std::vector<float> pitches{result.frames[i].note};
      while (end < result.frames.size() && result.frames[end].voiced &&
             result.frames[end].phrase == result.frames[i].phrase) {
        float current =
            median(pitches.size() > 41
                       ? std::vector<float>(pitches.end() - 41, pitches.end())
                       : pitches);
        if (std::abs(result.frames[end].note - current) > .7f)
          break;
        pitches.push_back(result.frames[end].note);
        ++end;
      }
      Region r;
      r.begin = int(i);
      r.end = int(end);
      r.centre = median(pitches);
      float crossings = 0, last = 0, variance = 0;
      for (size_t j = i; j < end; ++j) {
        result.frames[j].region = int(result.notes.size());
        float x = result.frames[j].note - r.centre;
        if (j > i && x * last < 0)
          ++crossings;
        last = x;
        variance += x * x;
      }
      double duration = (end - i) * result.hopSeconds;
      float depth = std::sqrt(variance / std::max(1, int(end - i)));
      float hz = float(crossings / (2 * std::max(.01, duration)));
      r.vibrato =
          duration > .3 && depth > .035f && depth < .65f && hz >= 3 && hz <= 10;
      result.notes.push_back(r);
      i = end;
    }
    return result;
  }
};
} // namespace engineer
