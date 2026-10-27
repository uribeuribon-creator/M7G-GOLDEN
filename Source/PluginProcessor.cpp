#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
struct PitchDetector {
  float confidence = 0;
  float detectPitch(const float *data, int n, float sr) {
    const int lo = juce::jmax(2, int(sr / 1400)),
              hi = juce::jmin(n / 2, int(sr / 35));
    double energy = 0;
    for (int i = 0; i < n; ++i)
      energy += data[i] * data[i];
    if (energy / n < 0.00001)
      return 0;
    float best = 0;
    int period = 0;
    // First strong local peak avoids octave-down selection of periodic
    // multiples.
    float prev = 0, prevPrev = 0;
    int bestLag = 0;
    for (int lag = lo; lag <= hi; ++lag) {
      double xy = 0, xx = 0, yy = 0;
      for (int i = 0; i < n - hi; ++i) {
        const float a = data[i], b = data[i + lag];
        xy += a * b;
        xx += a * a;
        yy += b * b;
      }
      const float c = float(xy / std::sqrt(xx * yy + 1.0e-20));
      if (prev > 0.88f && prev > prevPrev && prev > c) {
        confidence = prev;
        const float denominator = prevPrev - 2 * prev + c;
        const float offset = std::abs(denominator) > 1e-8f
                                 ? .5f * (prevPrev - c) / denominator
                                 : 0;
        return sr / (float(lag - 1) + juce::jlimit(-.5f, .5f, offset));
      }
      if (c > best) {
        best = c;
        bestLag = lag;
      }
      prevPrev = prev;
      prev = c;
    }
    period = bestLag;
    confidence = best;
    return best > 0.75f && period > 0 ? sr / float(period) : 0;
  }
  float quantizeToScale(float f) {
    if (f < 60)
      return f;
    const float midi = 69 + 12 * std::log2(f / 440);
    return 440 * std::pow(2.0f, (std::round(midi) - 69) / 12);
  }
};
struct Profile {
  float warmth, presence, air, threshold, compression, low, high, tune, speed;
};
constexpr Profile profiles[] = {{65, 62, 48, -18, 4.5f, 3.5f, 4.5f, .75f, .4f},
                                {45, 35, 25, -22, 6, 1, 1, .35f, .6f},
                                {35, 25, 15, -26, 8, -1, -1, .25f, .7f},
                                {75, 78, 68, -14, 3.5f, 2, 7, .90f, .15f},
                                {55, 40, 35, -20, 5, 2.5f, 2, .50f, .5f},
                                {40, 25, 20, -14, 2, .5f, 1.5f, 0, .4f}};
juce::AudioProcessorValueTreeState::ParameterLayout parameters() {
  juce::AudioProcessorValueTreeState::ParameterLayout p;
  p.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"voicetype", 1}, "Voice type",
      juce::StringArray{"VOZ PRINCIPAL", "DOBLES / DOUBLES", "REFUERZOS / HARM",
                        "ADLIBS", "ARMONIAS / COROS", "BUS VOCAL"},
      0));
  p.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"intensity", 1}, "Intensity",
      juce::NormalisableRange<float>(0, 100), 75));
  return p;
}
} // namespace
M7GProcessor::M7GProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "M7G", parameters()) {}
bool M7GProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  auto s = b.getMainOutputChannelSet();
  return (s == juce::AudioChannelSet::mono() ||
          s == juce::AudioChannelSet::stereo()) &&
         s == b.getMainInputChannelSet();
}
const char *M7GProcessor::action(int i) {
  static const char *a[] = {"MAIN: NATURAL TUNE + WARMTH + CONTROL",
                            "DOUBLES: SOFT TUNE + DENSITY",
                            "REFUERZOS: SUBTLE TUNE + GLUE",
                            "ADLIBS: FAST TUNE + AIR",
                            "CHOIR: BALANCED TUNE + WARMTH",
                            "BUS: HARMONIES + SOFT GLUE + SPACE"};
  return a[juce::jlimit(0, 5, i)];
}
void M7GProcessor::prepareToPlay(double sr, int block) {
  rate = sr;
  decimation = juce::jmax(1, int(sr / 22050));
  decimationPhase = 0;
  neural.prepare(sr);
  tuner.reset();
  analogue.prepare(sr);
  setLatencySamples(32 + int(sr * .02) + neural.latency() + analogue.latency());
  delay.setSize(2, 32768);
  delay.clear();
  dryDelay.setSize(2, 32768);
  dryDelay.clear();
  analysis.fill(0);
  analysisPos = analysisHop = analysisCount = writePos = controlCounter = 0;
  phase = 0;
  pitchRatio = 1;
  inputGainDB = 0;
  for (int c = 0; c < 2; ++c) {
    low[static_cast<size_t>(c)].reset();
    presence[static_cast<size_t>(c)].reset();
    high[static_cast<size_t>(c)].reset();
  }
  finisher.prepare(sr, block);
  limiter.prepare(
      {sr, (juce::uint32)block, (juce::uint32)getTotalNumOutputChannels()});
  limiter.setThreshold(-1);
  limiter.setRelease(60);
  limiter.reset();
  wet.reset(sr, .05);
  wet.setCurrentAndTargetValue(*apvts.getRawParameterValue("intensity") / 100);
  ratioSmooth.reset(sr, .03);
  ratioSmooth.setCurrentAndTargetValue(1);
  gainSmooth.reset(sr, .1);
  gainSmooth.setCurrentAndTargetValue(1);
  updateProfile(int(*apvts.getRawParameterValue("voicetype")),
                wet.getCurrentValue());
}
void M7GProcessor::updateProfile(int v, float amount) {
  const auto &p = profiles[v];
  const auto l = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
      rate, 110, .8f, juce::Decibels::decibelsToGain(p.low * amount * .75f));
  const auto m = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
      rate, juce::jmin(3200.0, rate * .4), 1.1f,
      juce::Decibels::decibelsToGain(p.presence * .045f * amount));
  const auto h = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
      rate, juce::jmin(10500.0, rate * .45), .7f,
      juce::Decibels::decibelsToGain((p.high + p.air * .025f) * amount * .65f));
  for (int c = 0; c < 2; ++c) {
    *low[static_cast<size_t>(c)].coefficients = l;
    *presence[static_cast<size_t>(c)].coefficients = m;
    *high[static_cast<size_t>(c)].coefficients = h;
  }
}
void M7GProcessor::processBlock(juce::AudioBuffer<float> &b,
                                juce::MidiBuffer &) {
  juce::ScopedNoDenormals noDenormals;
  const int n = b.getNumSamples(), channels = juce::jmin(2, b.getNumChannels());
  if (n == 0 || channels == 0)
    return;
  const int v =
      juce::jlimit(0, 5, int(*apvts.getRawParameterValue("voicetype")));
  const auto &p = profiles[v];
  const float amount =
      juce::jlimit(0.0f, 1.0f, *apvts.getRawParameterValue("intensity") / 100);
  wet.setTargetValue(amount);
  analogue.setColour(p.warmth, amount);
  double inEnergy = 0, outEnergy = 0, beforeComp = 0, afterComp = 0;
  const float gainAlpha = 1 - std::exp(-float(n) / float(rate * 1.0));
  float rms = b.getRMSLevel(0, 0, n);
  const float db = juce::Decibels::gainToDecibels(rms, -100.0f);
  if (db > -55)
    inputGainDB +=
        gainAlpha * (juce::jlimit(-12.0f, 12.0f, -14 - db) - inputGainDB);
  gainSmooth.setTargetValue(
      juce::Decibels::decibelsToGain(inputGainDB * amount));
  if (controlCounter <= 0) {
    updateProfile(v, amount);
    controlCounter = 2048;
  }
  controlCounter -= n;
  PitchDetector detector;
  const float window = float(rate * .04), base = 32;
  for (int i = 0; i < n; ++i) {
    float mono = 0;
    for (int c = 0; c < channels; ++c)
      mono += b.getSample(c, i) / float(channels);
    if (++decimationPhase >= decimation) {
      decimationPhase = 0;
      analysis[static_cast<size_t>(analysisPos)] = mono;
      analysisPos = (analysisPos + 1) % 2048;
      analysisCount = juce::jmin(2048, analysisCount + 1);
      ++analysisHop;
    }
    if (analysisHop >= 512 && analysisCount == 2048) {
      analysisHop = 0;
      std::array<float, 2048> ordered{};
      for (int k = 0; k < 2048; ++k)
        ordered[static_cast<size_t>(k)] =
            analysis[static_cast<size_t>((analysisPos + k) % 2048)];
      float f = detector.detectPitch(ordered.data(), 2048,
                                     float(rate) / float(decimation));
      pitchHz.store(f);
      const float target = tuner.target(
          f, detector.confidence, 512.0f / (float(rate) / float(decimation)));
      if (target > 35 && v != 5) {
        const float desired =
            juce::jlimit(.94f, 1.06f, 1 + (target / f - 1) * p.tune * amount);
        const float response =
            1 - std::exp(-512.0f /
                         (float(rate) / float(decimation) * p.speed * .1f));
        pitchRatio += response * (desired - pitchRatio);
      } else
        pitchRatio = 1;
      ratioSmooth.setTargetValue(pitchRatio);
    }
    float ratio = ratioSmooth.getNextValue(), mix = wet.getNextValue(),
          g = gainSmooth.getNextValue();
    phase += (1 - ratio) / window;
    phase -= std::floor(phase);
    auto read = [&](int c, float d) {
      float idx = float(writePos) - d;
      while (idx < 0)
        idx += 32768;
      int a = int(idx) & 32767;
      float f = idx - std::floor(idx);
      return delay.getSample(c, a) * (1 - f) +
             delay.getSample(c, (a + 1) & 32767) * f;
    };
    const float second = std::fmod(phase + .5f, 1.0f),
                weight =
                    .5f -
                    .5f * std::cos(juce::MathConstants<float>::twoPi * phase);
    float cleaned[2]{};
    for (int c = 0; c < channels; ++c)
      cleaned[c] = b.getSample(c, i);
    neural.processFrame(cleaned, channels);
    // Bus path preserves simultaneous notes while retaining equal latency.
    if (v == 5)
      for (int c = 0; c < channels; ++c)
        cleaned[c] = dryDelay.getSample(
            c, (writePos + 32768 - neural.latency()) & 32767);
    finisher.cleanFrame(cleaned, channels);
    float colourFrame[2]{};
    for (int c = 0; c < channels; ++c) {
      const float dry = b.getSample(c, i);
      inEnergy += dry * dry;
      delay.setSample(c, writePos, cleaned[c]);
      dryDelay.setSample(c, writePos, dry);
      colourFrame[c] = g * (read(c, base + phase * window) * weight +
                            read(c, base + second * window) * (1 - weight));
    }
    analogue.colour(colourFrame, channels);
    for (int c = 0; c < channels; ++c) {
      auto k = static_cast<size_t>(c);
      colourFrame[c] = high[k].processSample(
          presence[k].processSample(low[k].processSample(colourFrame[c])));
      beforeComp += colourFrame[c] * colourFrame[c];
    }
    analogue.compress(colourFrame, channels, p.threshold, p.compression,
                      v == 3);
    for (int c = 0; c < channels; ++c) {
      afterComp += colourFrame[c] * colourFrame[c];
      const int dryIndex = (writePos + 32768 - getLatencySamples()) & 32767;
      const float dry = dryDelay.getSample(c, dryIndex);
      b.setSample(c, i, dry + (colourFrame[c] - dry) * mix);
    }
    writePos = (writePos + 1) & 32767;
  }
  double tempo = 120;
  if (auto *head = getPlayHead())
    if (auto pos = head->getPosition())
      if (auto bpm = pos->getBpm())
        tempo = *bpm;
  finisher.ambience(b, v, amount, tempo);
  deEssDB.store(finisher.getDeEssReduction() * amount);
  juce::dsp::AudioBlock<float> block(b);
  juce::dsp::ProcessContextReplacing<float> ctx(block);
  limiter.process(ctx);
  const float ceiling = juce::Decibels::decibelsToGain(-1.0f);
  for (int c = 0; c < channels; ++c)
    for (int i = 0; i < n; ++i) {
      float x = juce::jlimit(-ceiling, ceiling, b.getSample(c, i));
      b.setSample(c, i, x);
      outEnergy += x * x;
    }
  inputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(inEnergy / (n * channels))), -100.0f));
  outputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(outEnergy / (n * channels))), -100.0f));
  reductionDB.store(
      beforeComp > 1.0e-15
          ? juce::jlimit(0.0f, 60.0f,
                         float(10 * std::log10((beforeComp + 1e-20) /
                                               (afterComp + 1e-20))))
          : 0);
}
void M7GProcessor::getStateInformation(juce::MemoryBlock &d) {
  auto xml = apvts.copyState().createXml();
  copyXmlToBinary(*xml, d);
}
void M7GProcessor::setStateInformation(const void *d, int n) {
  auto xml = getXmlFromBinary(d, n);
  if (xml && xml->hasTagName(apvts.state.getType()))
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor *M7GProcessor::createEditor() {
  return new M7GEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new M7GProcessor;
}
