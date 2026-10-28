#include "DeEsser.h"
// Detector separado de la ruta vocal: anticipacion fija y reduccion enlazada
// sin reservar memoria.
namespace fresh {
void DeEsser::prepare(double sr, int channels) {
  rate = sr;
  lookahead = int(std::round(sr * .002));
  line.prepare(channels, lookahead);
  high.set(Biquad::highpass, sr, 4000, .707);
  low.set(Biquad::lowpass, sr, 4000, .707);
  upper.set(Biquad::lowpass, sr, 10000, .707);
  reset();
}
void DeEsser::reset() {
  line.reset();
  high.reset();
  upper.reset();
  low.reset();
  band.reset();
  rmsHigh = rmsLow = peak = 0;
  gain = 1;
  hold = 0;
  gr = 0;
  lastFocus = -1;
}
void DeEsser::process(juce::dsp::AudioBlock<float> b, float focusHz) {
  if (focusHz != lastFocus) {
    band.set(Biquad::bandpass, rate, focusHz, 2);
    lastFocus = focusHz;
  }
  float rmsA = float(1 - std::exp(-1 / (rate * .05))),
        peakRelease = float(std::exp(-1 / (rate * .005))),
        attack = float(1 - std::exp(-1 / (rate * .001))),
        release = float(1 - std::exp(-1 / (rate * .04)));
  float maximum = 0;
  int ch = int(b.getNumChannels());
  for (size_t i = 0; i < b.getNumSamples(); ++i) {
    float h = 0, l = 0, p = 0;
    float x[2]{};
    for (int c = 0; c < ch; ++c) {
      x[c] = b.getSample(size_t(c), i);
      h += std::pow(upper.process(c, high.process(c, x[c])), 2);
      l += std::pow(low.process(c, x[c]), 2);
      p = std::max(p, std::abs(band.process(c, x[c])));
    }
    h /= ch;
    l /= ch;
    rmsHigh += rmsA * (h - rmsHigh);
    rmsLow += rmsA * (l - rmsLow);
    peak = std::max(p, peak * peakRelease);
    bool sibilant = rmsHigh > rmsLow * 3.981f && peak > .1f;
    hold = sibilant ? hold + 1 : 0;
    float target = 1;
    if (hold > int(rate * .02))
      target = juce::Decibels::decibelsToGain(
          -juce::jlimit(0.f, 12.f, (db(peak) + 20) * .75f + 2.f));
    gain += (target - gain) * (target < gain ? attack : release);
    line.frame(x, ch);
    for (int c = 0; c < ch; ++c)
      b.setSample(size_t(c), i, x[c] * gain);
    maximum = std::max(maximum, -db(gain));
  }
  gr = maximum;
}
} // namespace fresh
