#include "CharacterSat.h"
// La no linealidad se procesa a 4x; el filtro DC actua solo sobre los armonicos
// anadidos.
namespace fresh {
float CharacterSat::transfer(float x, float amount, int model, int harmonic,
                             int tapeMode) {
  if (amount <= 0)
    return x;
  float drives[]{.1f, .25f, .5f};
  float k = 1 + amount * (2 + drives[juce::jlimit(0, 2, tapeMode)] * 5);
  float y = 0;
  if (model == 0)
    y = std::tanh(k * x) / k;
  else if (model == 1)
    y = (std::tanh(k * x + .18f) - std::tanh(.18f)) /
        (k * (1 - std::pow(std::tanh(.18f), 2)));
  else
    y = juce::jlimit(-1.f, 1.f, k * x) / k;
  float h = harmonic == 0 ? x * x : x * x * x;
  y += .2f * amount * h / (1 + std::abs(h));
  return x + (y - x) * amount;
}
void CharacterSat::prepare(double sr, int channels, int maximum) {
  oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
      size_t(channels), 2,
      juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
  oversampling->initProcessing(size_t(maximum));
  delay = int(std::llround(oversampling->getLatencyInSamples()));
  dc.set(Biquad::highpass, sr * 4, 18, .707);
  amount.reset(sr * 4, .025);
  amount.setCurrentAndTargetValue(0);
  reset();
}
void CharacterSat::reset() {
  if (oversampling)
    oversampling->reset();
  dc.reset();
}
void CharacterSat::process(juce::dsp::AudioBlock<float> b, const Values &p) {
  amount.setTargetValue(p[character] * .01f);
  auto up = oversampling->processSamplesUp(b);
  for (size_t i = 0; i < up.getNumSamples(); ++i) {
    float a = amount.getNextValue();
    float x[2]{};
    for (size_t c = 0; c < up.getNumChannels(); ++c) {
      float raw = up.getSample(c, i);
      float added =
          transfer(raw, a, int(p[charMode]), int(p[harmonics]), int(p[tape])) -
          raw;
      x[c] = raw + dc.process(int(c), added);
    }
    if (up.getNumChannels() == 2) {
      float mid = (x[0] + x[1]) * .5f, side = (x[0] - x[1]) * .5f;
      float w = p[width] > .5f ? 1 + a : 1 - a;
      x[0] = mid + side * w;
      x[1] = mid - side * w;
    }
    for (size_t c = 0; c < up.getNumChannels(); ++c)
      up.setSample(c, i, x[c]);
  }
  oversampling->processSamplesDown(b);
}
} // namespace fresh
