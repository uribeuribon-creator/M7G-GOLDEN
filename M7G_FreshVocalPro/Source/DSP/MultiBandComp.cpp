#include "MultiBandComp.h"
// El allpass de la banda baja compensa la segunda division LR; las tres bandas
// suman planas.
namespace fresh {
void MultiBandComp::prepare(double sr, int channels, int maximum) {
  oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
      size_t(channels), 1,
      juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
  oversampling->initProcessing(size_t(maximum));
  delay = int(std::llround(oversampling->getLatencyInSamples()));
  rate = sr * 2;
  juce::dsp::ProcessSpec spec{rate, juce::uint32(maximum * 2),
                              juce::uint32(channels)};
  for (auto *f : {&splitLow, &splitHigh, &lowPhase})
    f->prepare(spec);
  splitLow.setCutoffFrequency(200);
  splitHigh.setCutoffFrequency(2000);
  lowPhase.setCutoffFrequency(2000);
  lowPhase.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
  mix.reset(rate, .025);
  color.reset(rate, .025);
  mix.setCurrentAndTargetValue(.189f);
  color.setCurrentAndTargetValue(.5f);
  reset();
}
void MultiBandComp::reset() {
  if (oversampling)
    oversampling->reset();
  splitLow.reset();
  splitHigh.reset();
  lowPhase.reset();
  envelope = {};
  gain = {{1, 1, 1}};
  gr = 0;
}
void MultiBandComp::process(juce::dsp::AudioBlock<float> b, float amount,
                            float colour, int style) {
  mix.setTargetValue(amount * .01f);
  color.setTargetValue(colour * .01f);
  auto up = oversampling->processSamplesUp(b);
  int channels = int(up.getNumChannels());
  style = juce::jlimit(0, 2, style);
  const float ratios[]{2, 4, 8}, attacks[]{.03f, .01f, .001f},
      releases[]{.2f, .1f, .05f};
  float a = float(1 - std::exp(-1 / (rate * attacks[style]))),
        r = float(1 - std::exp(-1 / (rate * releases[style]))), maximum = 0;
  for (size_t i = 0; i < up.getNumSamples(); ++i) {
    float bands[2][3]{};
    for (int c = 0; c < channels; ++c) {
      float lo, hi;
      splitLow.processSample(c, up.getSample(size_t(c), i), lo, hi);
      bands[c][0] = lowPhase.processSample(c, lo);
      splitHigh.processSample(c, hi, bands[c][1], bands[c][2]);
    }
    float blend = mix.getNextValue(), warm = color.getNextValue();
    float totalGR = 0;
    for (int k = 0; k < 3; ++k) {
      float detector = 0;
      for (int c = 0; c < channels; ++c)
        detector = std::max(detector, std::abs(bands[c][k]));
      envelope[size_t(k)] += (detector - envelope[size_t(k)]) *
                             (detector > envelope[size_t(k)] ? a : r);
      float level = db(envelope[size_t(k)]), over = level + 24, knee = 6,
            down = over <= -knee / 2 ? 0
                   : over >= knee / 2
                       ? (1 - 1 / ratios[style]) * over
                       : (1 - 1 / ratios[style]) *
                             std::pow(over + knee / 2, 2) / (2 * knee);
      down = juce::jlimit(0.f, 20.f, down);
      float upward =
          level > -60 ? juce::jlimit(0.f, 6.f, (-24 - level) * .12f) * warm : 0;
      float makeup = (1 - 1 / ratios[style]) * 2.4f;
      float target = juce::Decibels::decibelsToGain(-down + upward + makeup);
      gain[size_t(k)] +=
          (target - gain[size_t(k)]) * (target < gain[size_t(k)] ? a : r);
      totalGR += down * blend;
    }
    maximum = std::max(maximum, totalGR);
    for (int c = 0; c < channels; ++c) {
      float sum = 0;
      for (int k = 0; k < 3; ++k) {
        float dry = bands[c][k], wet = dry * gain[size_t(k)];
        float kDrive = 1 + warm * .35f;
        wet = std::tanh(wet * kDrive) / kDrive;
        sum += dry + (wet - dry) * blend;
      }
      up.setSample(size_t(c), i, sum);
    }
  }
  oversampling->processSamplesDown(b);
  gr = maximum;
}
} // namespace fresh
