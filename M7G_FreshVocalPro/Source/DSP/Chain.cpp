#include "Chain.h"
// Los bypass conservan retardos fijos. Reordenar modulos mantiene la latencia
// comunicada al host.
namespace fresh {
void Chain::prepare(double sr, int count, int block) {
  rate = sr;
  channels = count;
  maximum = std::max(32, block);
  deEsser.prepare(sr, count);
  compressor.prepare(sr, count, maximum);
  eq.prepare(sr);
  saturation.prepare(sr, count, maximum);
  space.prepare(sr, count);
  std::array<int, 4> delays{
      {deEsser.latency() + compressor.latency(), 0, 0, saturation.latency()}};
  int limiterDelay = int(std::round(sr * .001));
  limiterLookahead = limiterDelay;
  totalLatency = limiterDelay;
  for (int i = 0; i < 4; ++i) {
    bypassLines[size_t(i)].prepare(count, delays[size_t(i)]);
    enabled[size_t(i)].reset(sr, .02);
    enabled[size_t(i)].setCurrentAndTargetValue(1);
    totalLatency += delays[size_t(i)];
  }
  limiterLine.prepare(count, limiterDelay);
  globalLine.prepare(count, totalLatency);
  dry.setSize(count, maximum);
  globalDry.setSize(count, maximum);
  for (auto *g : {&inputGain, &outputGain, &globalWet})
    g->reset(sr, .025);
  inputGain.setCurrentAndTargetValue(1);
  outputGain.setCurrentAndTargetValue(1);
  globalWet.setCurrentAndTargetValue(1);
  reset();
}
void Chain::reset() {
  deEsser.reset();
  compressor.reset();
  eq.reset();
  saturation.reset();
  space.reset();
  for (auto &d : bypassLines)
    d.reset();
  limiterLine.reset();
  globalLine.reset();
  limiterGain = 1;
  limiterHold = 0;
  meters.in.store(0);
  meters.out.store(0);
  meters.compression.store(0);
  meters.deEss.store(0);
  meters.duck.store(0);
  meters.limiting.store(0);
}
void Chain::process(juce::AudioBuffer<float> &b, const Values &p, double bpm,
                    bool hostBypass) {
  juce::dsp::AudioBlock<float> block(b);
  process(block, p, bpm, hostBypass);
}
void Chain::process(juce::dsp::AudioBlock<float> full, const Values &p,
                    double bpm, bool hostBypass) {
  float peakIn = 0, peakOut = 0, limiting = 0;
  inputGain.setTargetValue(juce::Decibels::decibelsToGain(p[input]));
  outputGain.setTargetValue(juce::Decibels::decibelsToGain(p[output]));
  globalWet.setTargetValue(hostBypass || p[bypass] > .5f ? 0 : 1);
  const int switches[]{dynOn, eqOn, spaceOn, charOn};
  for (int s = 0; s < 4; ++s)
    enabled[size_t(s)].setTargetValue(p[size_t(switches[s])]);
  auto sequence = orders()[size_t(juce::jlimit(0, 23, int(p[order])))];
  for (size_t start = 0; start < full.getNumSamples();
       start += size_t(maximum)) {
    auto b = full.getSubBlock(
        start, std::min(size_t(maximum), full.getNumSamples() - start));
    int n = int(b.getNumSamples());
    for (int i = 0; i < n; ++i) {
      float original[2]{};
      float g = inputGain.getNextValue();
      for (int c = 0; c < channels; ++c) {
        float x = b.getSample(size_t(c), size_t(i));
        if (!std::isfinite(x))
          x = 0;
        original[c] = x;
        b.setSample(size_t(c), size_t(i), juce::jlimit(-32.f, 32.f, x) * g);
        peakIn = std::max(peakIn, std::abs(x * g));
      }
      globalLine.frame(original, channels);
      for (int c = 0; c < channels; ++c)
        globalDry.setSample(c, i, original[c]);
    }
    if (globalWet.getCurrentValue() > .00001f ||
        globalWet.getTargetValue() > .5f) {
      wasGlobalActive = true;
      for (int stage : sequence) {
        for (int i = 0; i < n; ++i) {
          float x[2]{};
          for (int c = 0; c < channels; ++c)
            x[c] = b.getSample(size_t(c), size_t(i));
          bypassLines[size_t(stage)].frame(x, channels);
          for (int c = 0; c < channels; ++c)
            dry.setSample(c, i, x[c]);
        }
        bool run = enabled[size_t(stage)].getCurrentValue() > .00001f ||
                   enabled[size_t(stage)].getTargetValue() > .5f;
        if (run) {
          if (stage == 0) {
            deEsser.process(b, p[focus]);
            compressor.process(b, p[comp], p[color], int(p[dynStyle]));
          }
          if (stage == 1)
            eq.process(b, p);
          if (stage == 2)
            space.process(b, p, bpm);
          if (stage == 3)
            saturation.process(b, p);
        }
        if (!run && wasActive[size_t(stage)]) {
          if (stage == 0) {
            deEsser.reset();
            compressor.reset();
          }
          if (stage == 1)
            eq.reset();
          if (stage == 2)
            space.reset();
          if (stage == 3)
            saturation.reset();
        }
        wasActive[size_t(stage)] = run;
        for (int i = 0; i < n; ++i) {
          float wet = enabled[size_t(stage)].getNextValue();
          for (int c = 0; c < channels; ++c) {
            float d = dry.getSample(c, i);
            b.setSample(size_t(c), size_t(i),
                        d + (b.getSample(size_t(c), size_t(i)) - d) * wet);
          }
        }
      }
      float release = float(std::exp(-1 / (rate * .05))),
            ceiling = juce::Decibels::decibelsToGain(-.5f);
      for (int i = 0; i < n; ++i) {
        float x[2]{}, g = outputGain.getNextValue(), peak = 0;
        for (int c = 0; c < channels; ++c) {
          x[c] = b.getSample(size_t(c), size_t(i)) * g;
          peak = std::max(peak, std::abs(x[c]));
        }
        float target = std::min(1.f, ceiling / std::max(1e-8f, peak));
        if (target < limiterGain) {
          limiterGain = target;
          limiterHold = limiterLookahead;
        } else if (limiterHold > 0)
          --limiterHold;
        else
          limiterGain = target + (limiterGain - target) * release;
        limiterLine.frame(x, channels);
        for (int c = 0; c < channels; ++c)
          b.setSample(size_t(c), size_t(i),
                      juce::jlimit(-ceiling, ceiling, x[c] * limiterGain));
        limiting = std::max(limiting, -db(limiterGain));
      }
    } else {
      outputGain.skip(n);
      if (wasGlobalActive) {
        deEsser.reset();
        compressor.reset();
        eq.reset();
        saturation.reset();
        space.reset();
        limiterLine.reset();
        limiterGain = 1;
        limiterHold = 0;
      }
      wasGlobalActive = false;
    }
    for (int i = 0; i < n; ++i) {
      float wet = globalWet.getNextValue();
      for (int c = 0; c < channels; ++c) {
        float d = globalDry.getSample(c, i),
              x = d + (b.getSample(size_t(c), size_t(i)) - d) * wet;
        if (!std::isfinite(x))
          x = 0;
        b.setSample(size_t(c), size_t(i), x);
        peakOut = std::max(peakOut, std::abs(x));
      }
    }
  }
  meters.in.store(std::max(peakIn, meters.in.load() * .86f));
  meters.out.store(std::max(peakOut, meters.out.load() * .86f));
  meters.compression.store(p[dynOn] > .5f ? compressor.reduction() : 0);
  meters.deEss.store(p[dynOn] > .5f ? deEsser.reduction() : 0);
  meters.duck.store(p[spaceOn] > .5f ? space.duckReduction() : 0);
  meters.limiting.store(limiting);
}
} // namespace fresh
