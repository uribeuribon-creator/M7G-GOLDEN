#pragma once
#include "../ThirdParty/rubberband/rubberband/RubberBandStretcher.h"
#include "Analysis.h"
#include <stdexcept>
namespace one {
struct Plan {
  std::vector<float> delta, wet;
};
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

} // namespace one
