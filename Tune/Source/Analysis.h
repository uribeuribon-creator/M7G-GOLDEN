#pragma once
#include "Neural.h"
#include <juce_dsp/juce_dsp.h>
namespace tune {
inline float midi(float hz) {
  return hz > 0 ? 69.f + 12.f * std::log2(hz / 440.f) : 0;
}
inline float frequency(float note) {
  return 440.f * std::exp2((note - 69.f) / 12.f);
}
class Analysis {
public:
  void prepare(double sr) {
    rate = sr;
    factor = std::max(1, int(std::round(sr / 12000.)));
    ar = sr / factor;
    auto c1 = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sr, std::min(4300., ar * .39), .5411961f);
    auto c2 = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sr, std::min(4300., ar * .39), 1.306563f);
    lp1.coefficients = c1;
    lp2.coefficients = c2;
    reset();
  }
  void reset() {
    ring.fill(0);
    hist.fill(0);
    pos = hop = decimate = observations = 0;
    hz = confidence = 0;
    lp1.reset();
    lp2.reset();
    previousHz = 0;
    keyRoot = 0;
    keyMinor = false;
    keyConfidence = 0;
    keyHold = 0;
  }
  bool process(float x, NeuralWorker *neural) {
    x = lp2.processSample(lp1.processSample(x));
    if (++decimate < factor)
      return false;
    decimate = 0;
    ring[size_t(pos)] = x;
    pos = (pos + 1) & 1023;
    if (++hop < 128)
      return false;
    hop = 0;
    detect(neural);
    return true;
  }
  float hz = 0, confidence = 0, air = 0, roughness = 0;
  int keyRoot = 0;
  bool keyMinor = false;
  float keyConfidence = 0;
  std::array<float, 12> hist{};

private:
  void detect(NeuralWorker *neural) {
    ++serial;
    int n = previousHz >= 170 ? 512 : previousHz >= 95 ? 768 : 1024;
    double mean = 0;
    for (int i = 0; i < n; ++i)
      mean += ring[size_t((pos + 1024 - n + i) & 1023)];
    mean /= n;
    in.fill({});
    prefix[0] = 0;
    for (int i = 0; i < n; ++i) {
      float v = ring[size_t((pos + 1024 - n + i) & 1023)] - float(mean);
      in[size_t(i)] = {v, 0};
      prefix[size_t(i + 1)] = prefix[size_t(i)] + v * v;
    }
    confidence = hz = 0;
    if (prefix[n] / n < .000002) {
      previousHz = 0;
      return;
    }
    fft.perform(in.data(), out.data(), false);
    double bands[3]{}, centroid = 0, total = 0;
    for (int k = 1; k < 1024; ++k) {
      double p = std::norm(out[size_t(k)]);
      double f = k * ar / 2048.;
      total += p;
      centroid += p * f;
      bands[f < 900 ? 0 : f < 2600 ? 1 : 2] += p;
    }
    for (auto &v : out)
      v = {std::norm(v), 0};
    fft.perform(out.data(), in.data(), true);
    double sum = 0;
    acf[0] = cmnd[0] = 1;
    int lo = std::max(2, int(ar / 1100)),
        hi = std::min(n / 2 - 1, int(ar / 50));
    for (int t = 1; t <= n / 2; ++t) {
      double ex = prefix[n - t], ey = prefix[n] - prefix[t],
             r = in[size_t(t)].real();
      acf[size_t(t)] = float(r / std::sqrt(ex * ey + 1e-20));
      double d = std::max(0., ex + ey - 2 * r);
      sum += d;
      cmnd[size_t(t)] = float(d * t / (sum + 1e-20));
    }
    int candidate = 0;
    for (int t = lo + 1; t < hi; ++t)
      if (cmnd[size_t(t)] < .16f && cmnd[size_t(t)] <= cmnd[size_t(t - 1)] &&
          cmnd[size_t(t)] < cmnd[size_t(t + 1)]) {
        candidate = t;
        break;
      }
    if (!candidate) {
      float best = .78f;
      for (int t = lo + 1; t < hi; ++t)
        if (acf[size_t(t)] > best && acf[size_t(t)] > acf[size_t(t - 1)] &&
            acf[size_t(t)] > acf[size_t(t + 1)]) {
          candidate = t;
          best = acf[size_t(t)];
        }
    }
    if (!candidate)
      return;
    NeuralPacket packet;
    packet.serial = serial;
    std::array<int, 3> candidates{};
    float best = -100;
    int chosen = candidate;
    for (int c = 0; c < 3; ++c) {
      int t = std::clamp(int(std::round(candidate * (c == 0   ? 1.f
                                                     : c == 1 ? .5f
                                                              : 2.f))),
                         lo + 1, hi - 1);
      int optimum = t;
      for (int k = std::max(lo + 1, t - 2); k <= std::min(hi - 1, t + 2); ++k)
        if (cmnd[size_t(k)] < cmnd[size_t(optimum)])
          optimum = k;
      t = optimum;
      candidates[size_t(c)] = t;
      for (int i = 0; i < 32; ++i) {
        float at =
            std::clamp(float(t) * (.5f + 2.f * i / 31), 1.f, float(n / 2));
        int a = int(at), b = std::min(n / 2, a + 1);
        float f = at - a;
        packet.pitch[size_t(c * 64 + i)] =
            acf[size_t(a)] * (1 - f) + acf[size_t(b)] * f;
        packet.pitch[size_t(c * 64 + 32 + i)] =
            1 - std::min(2.f, cmnd[size_t(a)] * (1 - f) + cmnd[size_t(b)] * f);
      }
      float score = .7f * acf[size_t(t)] + .3f * (1 - cmnd[size_t(t)]);
      if (c)
        score -= .025f;
      if (neural && neural->ready.load() && neural->runs.load() > 0 &&
          neural->sourceSerial.load() <= serial &&
          serial - neural->sourceSerial.load() <= 8 &&
          std::abs(midi(neural->sourceHz.load()) -
                   midi(float(ar / candidate))) < .8f)
        score = .65f * score + .35f * neural->scores[size_t(c)].load();
      if (score > best) {
        best = score;
        chosen = t;
      }
    }
    float a = cmnd[size_t(chosen - 1)], b = cmnd[size_t(chosen)],
          c = cmnd[size_t(chosen + 1)], den = a - 2 * b + c;
    float fraction =
        std::abs(den) > 1e-9f ? std::clamp(.5f * (a - c) / den, -.5f, .5f) : 0;
    confidence =
        std::clamp(.7f * acf[size_t(chosen)] + .3f * (1 - b), 0.f, 1.f);
    if (confidence < .79f)
      return;
    hz = float(ar / (chosen + fraction));
    if (hz < 50 || hz > 1100) {
      hz = 0;
      return;
    }
    previousHz = hz;
    float note = midi(hz);
    air = float(bands[2] / (total + 1e-20));
    roughness = 1 - confidence;
    packet.hz = float(ar / candidate);
    packet.profile = {std::clamp((note - 30) / 60, 0.f, 1.f),
                      confidence,
                      std::clamp(air * 3, 0.f, 1.f),
                      std::clamp(roughness * 4, 0.f, 1.f),
                      float(centroid / (total + 1e-20) / 5500),
                      float(bands[0] / (total + 1e-20)),
                      float(bands[1] / (total + 1e-20)),
                      air,
                      0.f,
                      0.f,
                      std::clamp(float(std::sqrt(prefix[n] / n)) * 3, 0.f, 1.f),
                      0.f};
    if (neural && neural->ready.load())
      neural->submit(packet);
    int pc = (int(std::round(note)) % 12 + 12) % 12;
    for (float &v : hist)
      v *= .997f;
    hist[size_t(pc)] += confidence;
    ++observations;
    if (observations % 12 == 0)
      estimateKey();
  }
  void estimateKey() {
    constexpr float major[]{6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
                            2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f};
    constexpr float minor[]{6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f,
                            2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f};
    float best = -1e9f, second = -1e9f;
    int winner = 0;
    float hmean = 0;
    for (float h : hist)
      hmean += h / 12;
    for (int k = 0; k < 24; ++k) {
      auto *w = k < 12 ? major : minor;
      float wm = 0;
      for (int i = 0; i < 12; ++i)
        wm += w[i] / 12;
      float dot = 0, hs = 0, ws = 0;
      for (int i = 0; i < 12; ++i) {
        float h = hist[size_t((i + k) % 12)] - hmean, v = w[i] - wm;
        dot += h * v;
        hs += h * h;
        ws += v * v;
      }
      float score = dot / std::sqrt(hs * ws + 1e-12f);
      if (score > best) {
        second = best;
        best = score;
        winner = k;
      } else
        second = std::max(second, score);
    }
    keyConfidence =
        observations < 120 ? 0 : std::clamp((best - second) * 3, 0.f, 1.f);
    if (observations > 120 && (keyHold <= 0 || winner % 12 == keyRoot)) {
      keyRoot = winner % 12;
      keyMinor = winner >= 12;
      keyHold = 240;
    } else
      keyHold -= 12;
  }
  double rate = 48000, ar = 12000;
  int factor = 4, pos = 0, hop = 0, decimate = 0, observations = 0, keyHold = 0;
  float previousHz = 0;
  uint64_t serial = 0;
  juce::dsp::IIR::Filter<float> lp1, lp2;
  juce::dsp::FFT fft{11};
  std::array<float, 1024> ring{};
  std::array<double, 1025> prefix{};
  std::array<float, 513> acf{}, cmnd{};
  std::array<std::complex<float>, 2048> in{}, out{};
};
struct Scale {
  int root = 0, kind = 0; // 0 chromatic, 1 major, 2 minor, 3 harmonic minor
  bool allows(int note) const {
    int p = (note - root + 1200) % 12;
    constexpr int major[]{0, 2, 4, 5, 7, 9, 11}, minor[]{0, 2, 3, 5, 7, 8, 10},
        harm[]{0, 2, 3, 5, 7, 8, 11};
    if (kind == 0)
      return true;
    auto *notes = kind == 1 ? major : kind == 2 ? minor : harm;
    for (int i = 0; i < 7; ++i)
      if (notes[i] == p)
        return true;
    return false;
  }
  int nearest(float note) const {
    int centre = int(std::round(note)), best = centre;
    float dist = 100;
    for (int k = centre - 6; k <= centre + 6; ++k)
      if (allows(k) && std::abs(note - k) < dist) {
        dist = std::abs(note - k);
        best = k;
      }
    return best;
  }
  int degree(int lead, int offset) const {
    lead = nearest(float(lead));
    int direction = offset < 0 ? -1 : 1;
    for (int i = 0; i < std::abs(offset); ++i) {
      lead += direction;
      while (!allows(lead))
        lead += direction;
    }
    return lead;
  }
};
} // namespace tune
