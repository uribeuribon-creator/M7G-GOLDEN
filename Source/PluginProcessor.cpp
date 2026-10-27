#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
struct PitchDetector {
  float confidence = 0;
  float detect(const float *data, int n, float sr) {
    int lo = juce::jmax(2, int(sr / 1400)),
        hi = juce::jmin(n / 2, int(sr / 35));
    double energy = 0;
    for (int i = 0; i < n; ++i)
      energy += data[i] * data[i];
    if (energy / n < .000005)
      return 0;
    float best = 0, previous = 0, previous2 = 0;
    int lagBest = 0;
    for (int lag = lo; lag <= hi; ++lag) {
      double xy = 0, xx = 0, yy = 0;
      for (int i = 0; i < n - hi; ++i) {
        float x = data[i], y = data[i + lag];
        xy += x * y;
        xx += x * x;
        yy += y * y;
      }
      float c = float(xy / std::sqrt(xx * yy + 1e-20));
      if (previous > .88f && previous > previous2 && previous > c) {
        confidence = previous;
        float denominator = previous2 - 2 * previous + c;
        float offset = std::abs(denominator) > 1e-8f
                           ? .5f * (previous2 - c) / denominator
                           : 0;
        return sr / (float(lag - 1) + juce::jlimit(-.5f, .5f, offset));
      }
      if (c > best) {
        best = c;
        lagBest = lag;
      }
      previous2 = previous;
      previous = c;
    }
    confidence = best;
    return best > .8f && lagBest > 0 ? sr / float(lagBest) : 0;
  }
};
} // namespace
M7GProcessor::M7GProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "M7G", rack::layout()) {
  for (size_t i = 0; i < rack::Count; ++i)
    parameter[i] = apvts.getRawParameterValue(rack::specs()[i].id);
  for (auto &x : spectrum)
    x.store(-100);
  for (auto &x : outputChannels)
    x.store(-100);
  setCurrentProgram(0);
  compare[0] = parameterState();
  compare[1] = parameterState();
}
float M7GProcessor::value(rack::Param i) const {
  return parameter[size_t(i)]->load();
}
void M7GProcessor::setValue(rack::Param i, float v) {
  auto *p = apvts.getParameter(rack::specs()[size_t(i)].id);
  const auto &s = rack::specs()[size_t(i)];
  p->setValueNotifyingHost(p->convertTo0to1(juce::jlimit(s.min, s.max, v)));
}
void M7GProcessor::setCurrentProgram(int i) {
  analysisRequest.store(false);
  analysisState.store(0);
  analysisProgress.store(0);
  i = juce::jlimit(0, 49, i);
  auto v = rack::presetValues(i);
  for (size_t k = 0; k < v.size(); ++k)
    setValue(static_cast<rack::Param>(k), v[k]);
  program.store(i);
}
bool M7GProcessor::isBusesLayoutSupported(const BusesLayout &l) const {
  auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
  return (in == juce::AudioChannelSet::mono() ||
          in == juce::AudioChannelSet::stereo()) &&
         (out == in || (in == juce::AudioChannelSet::mono() &&
                        out == juce::AudioChannelSet::stereo()));
}
const char *M7GProcessor::action(int i) {
  static const char *a[]{"PRINCIPAL", "DOBLES",   "REFUERZOS",
                         "ADLIBS",    "ARMONIAS", "BUS VOCAL"};
  return a[juce::jlimit(0, 5, i)];
}
void M7GProcessor::prepareToPlay(double sr, int block) {
  rate = sr;
  decimation = juce::jmax(1, int(sr / 22050));
  decimatePos = pitchPos = pitchCount = pitchHop = write = fftPos =
      filterClock = 0;
  phase = autoGainDB = 0;
  pitchWindow.fill(0);
  fftData.fill(0);
  pitchDelay.setSize(2, 32768);
  pitchDelay.clear();
  dryDelay.setSize(2, 32768);
  dryDelay.clear();
  neural.prepare(sr);
  tuner.reset();
  analogue.prepare(sr);
  cleaner.prepare(sr, block);
  limiter.prepare(sr);
  space.prepare(sr);
  stereo.prepare(sr);
  setLatencySamples(32 + int(sr * .02) + neural.latency() + analogue.latency() +
                    limiter.latency());
  for (size_t k = 0; k < rack::Count; ++k)
    settings[k] = parameter[k]->load();
  lastSettings.fill(-999);
  for (auto *f : {&presence, &air, &airCut})
    for (auto &channel : *f)
      channel.reset();
  refreshFilters();
  ratioSmooth.reset(sr, .03);
  ratioSmooth.setCurrentAndTargetValue(1);
  mixSmooth.reset(sr, .03);
  mixSmooth.setCurrentAndTargetValue(settings[rack::Mix] * .01f);
  inputSmooth.reset(sr, .03);
  inputSmooth.setCurrentAndTargetValue(
      juce::Decibels::decibelsToGain(settings[rack::InputTrim]));
  outputSmooth.reset(sr, .03);
  outputSmooth.setCurrentAndTargetValue(
      juce::Decibels::decibelsToGain(settings[rack::OutputTrim]));
  bypassSmooth.reset(sr, .02);
  bypassSmooth.setCurrentAndTargetValue(settings[rack::Bypass]);
}
void M7GProcessor::refreshFilters() {
  const auto &p = settings;
  bool eq = p[rack::EQOn] > .5f, hi = p[rack::AirOn] > .5f;
  auto eqc = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
      rate, juce::jmin(double(p[rack::EQFreq]), rate * .4), p[rack::EQQ],
      juce::Decibels::decibelsToGain(eq ? p[rack::EQGain] : 0));
  auto airc = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
      rate, juce::jmin(10000.0, rate * .4), .707f,
      juce::Decibels::decibelsToGain(
          hi ? p[rack::AirAmount] * .045f + p[rack::AirGain] : 0));
  auto cut = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
      rate, juce::jmin(double(p[rack::AirFilter]), rate * .47), .707f);
  for (size_t c = 0; c < 2; ++c) {
    *presence[c].coefficients = eqc;
    *air[c].coefficients = airc;
    *airCut[c].coefficients = cut;
  }
  lastSettings = p;
}
void M7GProcessor::meterFFT() {
  for (int i = 0; i < 512; ++i)
    fftData[size_t(i)] *=
        .5f -
        .5f * std::cos(juce::MathConstants<float>::twoPi * float(i) / 511);
  fft.performFrequencyOnlyForwardTransform(fftData.data());
  for (int b = 0; b < 32; ++b) {
    float low = 30 * std::pow(650.0f, float(b) / 32),
          high = 30 * std::pow(650.0f, float(b + 1) / 32);
    int lo = juce::jlimit(1, 255, int(low * 512 / float(rate))),
        hi = juce::jlimit(lo, 255, int(high * 512 / float(rate)));
    float peak = 0;
    for (int k = lo; k <= hi; ++k)
      peak = juce::jmax(peak, fftData[size_t(k)] / 128);
    spectrum[size_t(b)].store(juce::Decibels::gainToDecibels(peak, -100.0f));
  }
  fftData.fill(0);
}
void M7GProcessor::processBlock(juce::AudioBuffer<float> &b,
                                juce::MidiBuffer &) {
  juce::ScopedNoDenormals guard;
  int n = b.getNumSamples(), channels = juce::jmin(2, b.getNumChannels());
  if (n == 0 || channels == 0)
    return;
  if (getTotalNumInputChannels() == 1 && channels == 2)
    b.copyFrom(1, 0, b, 0, 0, n);
  for (size_t k = 0; k < rack::Count; ++k)
    settings[k] = parameter[k]->load();
  const auto &p = settings;
  int voice = juce::jlimit(0, 5, int(p[rack::VoiceType]));
  double tempo = 120;
  if (auto *head = getPlayHead())
    if (auto pos = head->getPosition())
      if (auto bpm = pos->getBpm())
        tempo = *bpm;
  if (filterClock <= 0 || p[rack::EQOn] != lastSettings[rack::EQOn] ||
      p[rack::AirOn] != lastSettings[rack::AirOn]) {
    refreshFilters();
    filterClock = 512;
  }
  filterClock -= n;
  cleaner.setControls(p);
  analogue.setControls(p);
  space.setControls(p, tempo);
  stereo.setControls(p);
  limiter.setControls(p);
  double e = 0;
  for (int c = 0; c < channels; ++c) {
    float r = b.getRMSLevel(c, 0, n);
    e += r * r;
  }
  float rms = std::sqrt(float(e / channels));
  float rawDB = juce::Decibels::gainToDecibels(rms, -100.0f);
  if (rawDB > -55)
    autoGainDB += (1 - std::exp(-float(n) / float(rate))) *
                  (juce::jlimit(-12.0f, 12.0f, -18 - rawDB) - autoGainDB);
  float automatic = p[rack::AutoGain] > .5f ? autoGainDB : 0;
  inputSmooth.setTargetValue(
      juce::Decibels::decibelsToGain(p[rack::InputTrim] + automatic));
  outputSmooth.setTargetValue(
      juce::Decibels::decibelsToGain(p[rack::OutputTrim]));
  mixSmooth.setTargetValue(p[rack::Mix] * .01f);
  bypassSmooth.setTargetValue(p[rack::Bypass]);
  if (analysisRequest.exchange(false)) {
    analysisState.store(1);
    analysisSamples = 0;
    analysisEnergy = analysisHigh = 0;
    analysisPeak = 0;
    analysisPrevious = 0;
    analysisLowDB = 0;
    analysisProgress.store(0);
  }
  double inEnergy = 0, outEnergy = 0, leftEnergy = 0, rightEnergy = 0,
         cross = 0;
  const float window = float(rate * .04), base = 32;
  PitchDetector detector;
  for (int i = 0; i < n; ++i) {
    float original[2]{};
    float mono = 0;
    for (int c = 0; c < channels; ++c) {
      original[c] = b.getSample(c, i);
      dryDelay.setSample(c, write, original[c]);
      mono += original[c] / float(channels);
    }
    leftEnergy += original[0] * original[0];
    rightEnergy += original[channels - 1] * original[channels - 1];
    cross += original[0] * original[channels - 1];
    if (analysisState.load() == 1) {
      analysisEnergy += mono * mono;
      analysisPeak = juce::jmax(analysisPeak, std::abs(mono));
      {
        float d = mono - analysisPrevious;
        analysisHigh += d * d;
        analysisPrevious = mono;
      }
      ++analysisSamples;
      analysisProgress.store(
          juce::jmin(1.0f, float(analysisSamples) / float(rate * 3)));
      if (analysisSamples >= int(rate * 3)) {
        analysisLevelDB = juce::Decibels::gainToDecibels(
            float(std::sqrt(analysisEnergy / analysisSamples)), -100.0f);
        analysisCrest = juce::Decibels::gainToDecibels(analysisPeak, -100.0f) -
                        analysisLevelDB;
        analysisHiss = float(analysisHigh / (analysisEnergy + 1e-15));
        analysisProcessedDB = analysisLevelDB + p[rack::InputTrim] + automatic;
        analysisState.store(analysisLevelDB > -65 ? 2 : -1);
      }
    }
    fftData[size_t(fftPos++)] = mono;
    if (fftPos == 512) {
      meterFFT();
      fftPos = 0;
    }
    if (++decimatePos >= decimation) {
      decimatePos = 0;
      pitchWindow[size_t(pitchPos)] = mono;
      pitchPos = (pitchPos + 1) % 2048;
      pitchCount = juce::jmin(2048, pitchCount + 1);
      ++pitchHop;
    }
    if (pitchHop >= 512 && pitchCount == 2048) {
      pitchHop = 0;
      std::array<float, 2048> ordered{};
      for (int k = 0; k < 2048; ++k)
        ordered[size_t(k)] = pitchWindow[size_t((pitchPos + k) % 2048)];
      float f = detector.detect(ordered.data(), 2048,
                                float(rate) / float(decimation));
      pitchHz.store(f);
      float target = tuner.target(
          f, detector.confidence, 512 / (float(rate) / float(decimation)),
          int(p[rack::PitchKey]), int(p[rack::PitchScale]));
      targetHz.store(target);
      float correction = 1;
      if (target > 35 && voice != 5 && p[rack::TuneOn] > .5f && f > 35)
        correction = juce::jlimit(
            .94f, 1.06f, 1 + (target / f - 1) * p[rack::TuneAmount] * .01f);
      float time = p[rack::TuneSpeed] * .001f;
      float current = ratioSmooth.getCurrentValue();
      ratioSmooth.setCurrentAndTargetValue(current);
      ratioSmooth.reset(rate, time);
      ratioSmooth.setTargetValue(correction);
    }
    float ratio = ratioSmooth.getNextValue(),
          inputGain = inputSmooth.getNextValue(),
          outputGain = outputSmooth.getNextValue(),
          mix = mixSmooth.getNextValue();
    phase += (1 - ratio) / window;
    phase -= std::floor(phase);
    float cleaned[2]{};
    for (int c = 0; c < channels; ++c) {
      cleaned[c] = original[c] * inputGain;
      inEnergy += cleaned[c] * cleaned[c];
    }
    neural.processFrame(
        cleaned, channels,
        p[rack::NoiseOn] > .5f && voice != 5 ? p[rack::NoiseAmount] * .01f : 0);
    cleaner.cleanFrame(cleaned, channels);
    for (int c = 0; c < channels; ++c)
      pitchDelay.setSample(c, write, cleaned[c]);
    auto read = [&](int c, float delay) {
      float at = float(write) - delay;
      while (at < 0)
        at += 32768;
      int a = int(at) & 32767;
      float f = at - std::floor(at);
      return pitchDelay.getSample(c, a) * (1 - f) +
             pitchDelay.getSample(c, (a + 1) & 32767) * f;
    };
    float second = std::fmod(phase + .5f, 1.0f),
          weight =
              .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * phase),
          frame[2]{};
    for (int c = 0; c < channels; ++c)
      frame[c] =
          p[rack::TuneOn] > .5f && p[rack::TuneAmount] > .001f && voice != 5
              ? read(c, base + phase * window) * weight +
                    read(c, base + second * window) * (1 - weight)
              : read(c, base + window * .5f);
    analogue.colour(frame, channels);
    for (int c = 0; c < channels; ++c) {
      auto k = size_t(c);
      frame[c] = presence[k].processSample(frame[c]);
      float airy = air[k].processSample(frame[c]);
      float filtered = airCut[k].processSample(airy);
      if (p[rack::AirOn] > .5f)
        frame[c] = filtered;
    }
    analogue.compress(frame, channels, -6 - .3f * p[rack::CompReduction],
                      p[rack::CompRatio], voice == 3, p[rack::CompRelease],
                      p[rack::CompOn] > .5f, p[rack::CompGain]);
    space.process(frame, channels);
    stereo.process(frame, channels);
    int dryIndex =
        (write + 32768 - getLatencySamples() + limiter.latency()) & 32767;
    for (int c = 0; c < channels; ++c) {
      float dry = dryDelay.getSample(c, dryIndex) *
                  juce::Decibels::decibelsToGain(p[rack::InputTrim]);
      float processed = frame[c];
      frame[c] = (p[rack::Solo] > .5f ? dry - processed
                                      : dry + mix * (processed - dry)) *
                 outputGain;
    }
    limiter.process(frame, channels);
    float bypass = bypassSmooth.getNextValue();
    int bypassIndex = (write + 32768 - getLatencySamples()) & 32767;
    for (int c = 0; c < channels; ++c) {
      float x =
          frame[c] + bypass * (dryDelay.getSample(c, bypassIndex) - frame[c]);
      b.setSample(c, i, x);
      outEnergy += x * x;
    }
    write = (write + 1) & 32767;
  }
  for (int c = 0; c < 2; ++c)
    outputChannels[size_t(c)].store(juce::Decibels::gainToDecibels(
        b.getRMSLevel(juce::jmin(c, channels - 1), 0, n), -100.0f));
  inputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(inEnergy / (n * channels))), -100.0f));
  outputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(outEnergy / (n * channels))), -100.0f));
  reductionDB.store(p[rack::CompOn] > .5f ? analogue.gainReduction() : 0);
  deEssDB.store(cleaner.getDeEssReduction());
  gateGR.store(p[rack::GateOn] > .5f ? cleaner.getGateReduction() : 0);
  limitGR.store(p[rack::LimitOn] > .5f ? limiter.reduction() : 0);
  revDuckDB.store(space.reverbDucking());
  delDuckDB.store(space.delayDucking());
  modPhase.store(stereo.lfoPhase());
  correlation.store(float(cross / std::sqrt(leftEnergy * rightEnergy + 1e-20)));
  double feedback = juce::jmax(.001, double(p[rack::DelayFeedback] * .01f));
  double delayTail = 60 / juce::jlimit(30.0, 300.0, tempo) * 2 *
                     std::log(.001) / std::log(feedback);
  tailSeconds.store(
      juce::jmin(180.0, (p[rack::ReverbOn] > .5f ? p[rack::ReverbDecay] : 0) +
                            (p[rack::DelayOn] > .5f ? delayTail : 0) + .2));
}
juce::ValueTree M7GProcessor::parameterState() {
  auto state = apvts.copyState();
  state.setProperty("program", program.load(), nullptr);
  return state;
}
void M7GProcessor::selectCompare(int slot) {
  const juce::ScopedLock lock(compareLock);
  slot = juce::jlimit(0, 1, slot);
  if (slot == activeCompare)
    return;
  compare[size_t(activeCompare)] = parameterState();
  auto next = compare[size_t(slot)].createCopy();
  program.store(int(next.getProperty("program", 0)));
  apvts.replaceState(next);
  activeCompare = slot;
}
void M7GProcessor::copyCompare() {
  const juce::ScopedLock lock(compareLock);
  compare[size_t(1 - activeCompare)] = parameterState();
}
void M7GProcessor::getStateInformation(juce::MemoryBlock &data) {
  const juce::ScopedLock lock(compareLock);
  auto state = parameterState();
  compare[size_t(activeCompare)] = state.createCopy();
  state.setProperty("compare", activeCompare.load(), nullptr);
  for (int k = 0; k < 2; ++k) {
    juce::ValueTree child(k == 0 ? "COMPARE_A" : "COMPARE_B");
    child.addChild(compare[size_t(k)].createCopy(), -1, nullptr);
    state.addChild(child, -1, nullptr);
  }
  copyXmlToBinary(*state.createXml(), data);
}
void M7GProcessor::setStateInformation(const void *data, int bytes) {
  const juce::ScopedLock lock(compareLock);
  auto xml = getXmlFromBinary(data, bytes);
  if (!xml || !xml->hasTagName("M7G"))
    return;
  auto state = juce::ValueTree::fromXml(*xml);
  for (int k = 0; k < 2; ++k) {
    auto child = state.getChildWithName(k == 0 ? "COMPARE_A" : "COMPARE_B");
    if (child.isValid() && child.getNumChildren() > 0)
      compare[size_t(k)] = child.getChild(0).createCopy();
    if (child.isValid())
      state.removeChild(child, nullptr);
  }
  activeCompare = juce::jlimit(0, 1, int(state.getProperty("compare", 0)));
  program.store(juce::jlimit(0, 49, int(state.getProperty("program", 0))));
  apvts.replaceState(state);
}
void M7GProcessor::beginAnalysis(int module) {
  requestedModule.store(juce::jlimit(0, 12, module));
  analysisState.store(1);
  analysisProgress.store(0);
  analysisRequest.store(true);
}
bool M7GProcessor::applyAnalysis() {
  if (analysisState.load() != 2)
    return false;
  int m = requestedModule.load();
  auto selected = [&](int x) { return m == 12 || m == x; };
  if (m < 12) {
    static const rack::Param enable[]{
        rack::GateOn,  rack::DeessOn,  rack::WarmOn,  rack::EQOn,
        rack::AirOn,   rack::CompOn,   rack::LimitOn, rack::ReverbOn,
        rack::DelayOn, rack::StereoOn, rack::ModOn,   rack::TuneOn};
    setValue(enable[m], 1);
    if (m == 7 && value(rack::ReverbMix) < .1f)
      setValue(rack::ReverbMix, 8);
    if (m == 8 && value(rack::DelayMix) < .1f)
      setValue(rack::DelayMix, 6);
  }
  float hiss = juce::jlimit(0.0f, 1.0f, analysisHiss * .3f);
  float processedDB = analysisProcessedDB;
  if (m == 12) {
    setValue(rack::InputTrim,
             juce::jlimit(-12.0f, 12.0f, -18 - analysisLevelDB));
    setValue(rack::AutoGain, 0);
    processedDB = analysisLevelDB + value(rack::InputTrim);
  }
  if (selected(0)) {
    setValue(rack::GateThreshold,
             juce::jlimit(-65.0f, -35.0f, processedDB - 28));
    setValue(rack::GateRange, 18 + 10 * hiss);
    setValue(rack::GateRelease, analysisCrest > 12 ? 140 : 100);
  }
  if (selected(1)) {
    setValue(rack::DeessAmount, juce::jlimit(25.0f, 85.0f, 40 + 45 * hiss));
    setValue(rack::DeessThreshold,
             juce::jlimit(-45.0f, -20.0f, processedDB - 12));
  }
  if (selected(2)) {
    setValue(rack::WarmDrive, juce::jlimit(25.0f, 75.0f, 65 - analysisCrest));
    setValue(rack::WarmBias, 35);
  }
  if (selected(3)) {
    setValue(rack::EQGain, juce::jlimit(-2.0f, 3.0f, 3 - 5 * hiss));
    setValue(rack::EQFreq, 2800 + 800 * hiss);
  }
  if (selected(4)) {
    setValue(rack::AirAmount, juce::jlimit(15.0f, 65.0f, 58 - 40 * hiss));
    setValue(rack::AirGain, 1.0f);
  }
  if (selected(5)) {
    setValue(rack::CompReduction,
             juce::jlimit(25.0f, 65.0f, 30 + analysisCrest));
    setValue(rack::CompRatio,
             juce::jlimit(2.0f, 5.0f, 2 + analysisCrest * .15f));
  }
  if (selected(6)) {
    setValue(rack::LimitCeiling, -1);
    setValue(rack::LimitLookahead, 1);
    setValue(rack::LimitRelease, 80);
  }
  if (selected(7)) {
    setValue(rack::ReverbDuck, 75);
    setValue(rack::ReverbPredelay, 28);
    setValue(rack::ReverbDecay, value(rack::VoiceType) == 3 ? 2.4f : 1.5f);
  }
  if (selected(8)) {
    setValue(rack::DelayDuck, 82);
    setValue(rack::DelayNote, analysisCrest > 12 ? 1 : 3);
    setValue(rack::DelayFeedback, 26);
  }
  if (selected(9)) {
    setValue(rack::Width, correlation.load() < 0 ? 85.0f : 110.0f);
    setValue(rack::Spread, correlation.load() > .95f ? 15.0f : 5.0f);
  }
  if (selected(10)) {
    setValue(rack::ModDepth, 20);
    setValue(rack::ModRate, .6f);
    setValue(rack::ModMix, value(rack::VoiceType) == 0 ? 5.0f : 12.0f);
  }
  if (selected(11)) {
    setValue(rack::TuneOn, value(rack::VoiceType) == 5 ? 0.0f : 1.0f);
    setValue(rack::TuneAmount, value(rack::VoiceType) == 3 ? 95.0f : 70.0f);
    setValue(rack::PitchKey, 0);
    setValue(rack::PitchScale, 0);
  }
  analysisState.store(0);
  return true;
}
juce::AudioProcessorEditor *M7GProcessor::createEditor() {
  return new M7GEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new M7GProcessor;
}
