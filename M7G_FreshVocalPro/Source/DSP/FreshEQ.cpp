#include "FreshEQ.h"
namespace fresh {
void FreshEQ::prepare(double sr) {
  rate = sr;
  const int ids[]{eqLow, eqLowMid, eqHighMid, eqHigh,
                  eq160, eq350,    eq1500,    eq6000};
  for (int i = 0; i < 8; ++i) {
    gain[size_t(i)].reset(sr, .025);
    gain[size_t(i)].setCurrentAndTargetValue(specs[size_t(ids[i])].initial);
  }
  reset();
}
void FreshEQ::reset() {
  for (auto &f : filters)
    f.reset();
  untilUpdate = 0;
}
void FreshEQ::process(juce::dsp::AudioBlock<float> b, const Values &p) {
  const int ids[]{eqLow, eqLowMid, eqHighMid, eqHigh,
                  eq160, eq350,    eq1500,    eq6000};
  const double frequencies[]{80, 700, 2500, 12000, 160, 350, 1500, 6000};
  for (int i = 0; i < 8; ++i)
    gain[size_t(i)].setTargetValue(p[size_t(ids[i])]);
  for (size_t n = 0; n < b.getNumSamples(); ++n) {
    if (--untilUpdate <= 0) {
      untilUpdate = 32;
      for (int i = 0; i < 8; ++i) {
        auto type = i == 0   ? Biquad::lowShelf
                    : i == 3 ? Biquad::highShelf
                             : Biquad::peak;
        filters[size_t(i)].set(type, rate, frequencies[i],
                               i == 2 && int(p[eqStyle]) == 2 ? .7 : 1,
                               gain[size_t(i)].getCurrentValue());
      }
    }
    for (auto &g : gain)
      g.getNextValue();
    for (size_t c = 0; c < b.getNumChannels(); ++c) {
      float x = b.getSample(c, n);
      for (auto &f : filters)
        x = f.process(int(c), x);
      b.setSample(c, n, x);
    }
  }
}
} // namespace fresh
