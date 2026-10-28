#include "FreshSpace.h"
// Retornos en paralelo: la envolvente de la voz reduce solo reverb y delay, no
// la senal directa.
namespace fresh {
void FreshSpace::prepare(double sr, int count) {
  rate = sr;
  channels = count;
  for (int c = 0; c < channels; ++c) {
    for (auto &r : combs[size_t(c)])
      r.prepare(int(sr * .18) + 8);
    for (auto &r : diffusion[size_t(c)])
      r.prepare(int(sr * .035) + 8);
    pre[size_t(c)].prepare(int(sr * .09) + 8);
    echo[size_t(c)].prepare(int(sr * 2.2) + 8);
  }
  verbLow.set(Biquad::highpass, sr, 200, .707);
  verbHigh.set(Biquad::lowpass, sr, 10000, .707);
  echoLow.set(Biquad::highpass, sr, 400, .707);
  echoHigh.set(Biquad::lowpass, sr, 8000, .707);
  for (auto *s : {&verbMix, &echoMix, &size, &echoSamples})
    s->reset(sr, .05);
  verbMix.setCurrentAndTargetValue(.241f);
  echoMix.setCurrentAndTargetValue(.06f);
  size.setCurrentAndTargetValue(.6f);
  echoSamples.setCurrentAndTargetValue(float(sr * .5));
  reset();
}
void FreshSpace::reset() {
  for (auto &channels_ : combs)
    for (auto &r : channels_)
      r.clear();
  for (auto &channels_ : diffusion)
    for (auto &r : channels_)
      r.clear();
  for (auto &r : pre)
    r.clear();
  for (auto &r : echo)
    r.clear();
  verbLow.reset();
  verbHigh.reset();
  echoLow.reset();
  echoHigh.reset();
  envelope = 0;
  duck = 1;
  duckDb = 0;
  phase = 0;
}
void FreshSpace::process(juce::dsp::AudioBlock<float> b, const Values &p,
                         double bpm) {
  const double times[]{.0297, .0371, .0411, .0437, .0531, .0617, .0719, .0793},
      allpass[]{.0051, .0077, .0113, .0179}, spaces[]{.3, .6, 1.},
      decays[]{1.1, .8, 2.5}, predelays[]{.02, .01, .04};
  int mode = juce::jlimit(0, 2, int(p[spaceMode])),
      type = juce::jlimit(0, 2, int(p[verbType]));
  double decay =
      .2 +
      7.8 * (1 - std::exp(-std::max(0., double(p[time])) * decays[type] / 8.));
  verbMix.setTargetValue(p[reverb] * .01f);
  echoMix.setTargetValue(p[delay] * .01f);
  size.setTargetValue(float(spaces[mode]));
  echoSamples.setTargetValue(float(delaySeconds(int(p[division]), bpm) * rate));
  float feedback[2][8]{};
  for (int c = 0; c < channels; ++c)
    for (int k = 0; k < 8; ++k)
      feedback[c][k] = float(
          std::pow(10., -3 * (times[k] * spaces[mode] + c * .00073) / decay));
  float damping = float(
            1 - std::exp(-2 * juce::MathConstants<double>::pi * 5000 / rate)),
        attack = float(1 - std::exp(-1 / (rate * .01))),
        release = float(1 - std::exp(-1 / (rate * .2)));
  float maxDuck = 0;
  for (size_t i = 0; i < b.getNumSamples(); ++i) {
    float direct[2]{};
    float detector = 0;
    for (int c = 0; c < channels; ++c) {
      direct[c] = b.getSample(size_t(c), i);
      detector = std::max(detector, std::abs(direct[c]));
    }
    envelope +=
        (detector - envelope) * (detector > envelope ? attack : release);
    float reduction = juce::jlimit(0.f, 12.f, (db(envelope) + 20) * .75f);
    float target = juce::Decibels::decibelsToGain(-reduction);
    duck += (target - duck) * (target < duck ? attack : release);
    maxDuck = std::max(maxDuck, -db(duck));
    float amount = verbMix.getNextValue(), echoAmount = echoMix.getNextValue(),
          room = size.getNextValue(), delayAt = echoSamples.getNextValue();
    float echoes[2]{};
    for (int c = 0; c < channels; ++c)
      echoes[c] = echo[size_t(c)].read(delayAt);
    for (int c = 0; c < channels; ++c) {
      auto &predelay = pre[size_t(c)];
      float feed = predelay.read(predelays[type] * rate);
      predelay.push(direct[c]);
      float wet = 0;
      for (int k = 0; k < 8; ++k) {
        auto &comb = combs[size_t(c)][size_t(k)];
        double modulation = std::sin(phase + k * .83 + c * 1.1) * rate * .00035;
        float read =
            comb.read((times[k] * room + c * .00073) * rate + modulation);
        comb.damp += damping * (read - comb.damp);
        comb.push(feed * .22f + comb.damp * feedback[c][k]);
        wet += read * .32f;
      }
      for (int k = 0; k < 4; ++k) {
        auto &all = diffusion[size_t(c)][size_t(k)];
        float read = all.read((allpass[k] + c * .00031) * rate),
              out = read - wet * .5f;
        all.push(wet + read * .5f);
        wet = out;
      }
      wet = verbHigh.process(c, verbLow.process(c, wet));
      float delayed = echoHigh.process(c, echoLow.process(c, echoes[c]));
      int other = channels == 2 ? 1 - c : c;
      echo[size_t(c)].push(direct[c] + echoes[other] * .25f);
      b.setSample(size_t(c), i,
                  direct[c] + (wet * amount + delayed * echoAmount) * duck);
    }
    phase += 2 * juce::MathConstants<double>::pi * .13 / rate;
    if (phase > 2 * juce::MathConstants<double>::pi)
      phase -= 2 * juce::MathConstants<double>::pi;
  }
  duckDb = maxDuck;
}
} // namespace fresh
