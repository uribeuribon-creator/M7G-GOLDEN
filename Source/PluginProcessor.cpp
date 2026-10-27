#include "PluginProcessor.h"
#include "PluginEditor.h"
M7GProcessor::M7GProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withInput("Lead reference", juce::AudioChannelSet::stereo(),
                         false)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "M7G", rack::layout()) {
  for (size_t i = 0; i < rack::Count; ++i)
    parameter[i] = apvts.getRawParameterValue(rack::specs()[i].id);
  for (auto &x : spectrum)
    x.store(-100);
  for (auto &x : outputChannels)
    x.store(-100);
  for (auto &x : layerMeters)
    x.store(-100);
  setCurrentProgram(0);
  compare[0] = parameterState();
  compare[1] = parameterState();
  startTimerHz(20);
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
  i = juce::jlimit(0, rack::presetCount - 1, i);
  auto v = rack::presetValues(i);
  for (size_t k = 0; k < v.size(); ++k)
    setValue(static_cast<rack::Param>(k), v[k]);
  program.store(i);
}
bool M7GProcessor::isBusesLayoutSupported(const BusesLayout &l) const {
  auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
  if (l.inputBuses.size() > 1) {
    auto ref = l.inputBuses[1];
    if (!ref.isDisabled() && ref != juce::AudioChannelSet::mono() &&
        ref != juce::AudioChannelSet::stereo())
      return false;
  }
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
      filterClock = neuralClock = 0;
  phase = autoGainDB = sinoMakeupDB = 0;
  pitchWindow.fill(0);
  fftData.fill(0);
  pitchHz.store(0);
  targetHz.store(0);
  dryDelay.setSize(2, 32768);
  dryDelay.clear();
  eqDryDelay.setSize(2, 32768);
  eqDryDelay.clear();
  referenceScratch.setSize(2, juce::jmax(32768, block));
  referenceScratch.clear();
  neural.prepare(sr);
  for (size_t i = 0; i < 2; ++i) {
    pitchAntiAlias[i].reset();
    *pitchAntiAlias[i].coefficients =
        juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
            sr, juce::jmin(9000.0, sr / decimation * .4),
            i == 0 ? .5411961f : 1.306563f);
  }
  tuner.reset();
  activeMode = juce::jlimit(0, 2, int(value(rack::EngineMode)));
  analogue.prepare(sr, activeMode);
  saturation.prepare(sr, activeMode);
  fet.prepare(sr);
  cleaner.prepare(sr, block);
  gate.prepare(sr, block);
  limiter.prepare(sr, activeMode == 0);
  space.prepare(sr, activeMode);
  stereo.prepare(sr);
  roomCleaner.prepare(sr);
  resonance.prepare(sr);
  leadTuner.prepare(sr);
  harmony.prepare(sr);
  aligner.prepare(sr);
  alignmentActive = activeMode != 0 && value(rack::AlignOn) > .5f;
  setLatencySamples(activeMode == 0
                        ? 0
                        : neural.latency() + V8Spectral::N * 2 +
                              V8Harmony::N * 2 + analogue.latency() +
                              saturation.latency() + limiter.latency() +
                              (alignmentActive ? aligner.latency() : 0));
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
  autoTargets =
      v8::autoMix({.7f, .4f, .2f, .1f, .2f, .2f, .2f, .2f, .1f, 0, .2f, .5f});
}
void M7GProcessor::configureMode(int mode) {
  if (mode == activeMode)
    return;
  activeMode = mode;
  analogue.setMode(mode);
  saturation.setMode(mode);
  limiter.setMode(mode == 0);
  space.setMode(mode);
  roomCleaner.prepare(rate);
  resonance.prepare(rate);
  leadTuner.prepare(rate);
  harmony.prepare(rate);
  filterClock = 0;
  setLatencySamples(mode == 0 ? 0
                              : neural.latency() + V8Spectral::N * 2 +
                                    V8Harmony::N * 2 + analogue.latency() +
                                    saturation.latency() + limiter.latency() +
                                    (alignmentActive ? aligner.latency() : 0));
}
void M7GProcessor::applySino(rack::Values &p, float rawDB) {
  float s = p[rack::Sino] * .01f;
  if (s <= 0)
    return;
  auto blend = [&](rack::Param k, float target) {
    p[k] += s * (target - p[k]);
  };
  bool bus = int(p[rack::VoiceType]) == 5;
  blend(rack::WarmDrive, 25 + autoTargets[0] * 55);
  blend(rack::AirAmount, 15 + autoTargets[1] * 45);
  blend(rack::CompReduction, 25 + autoTargets[2] * 45);
  blend(rack::NoiseAmount, 25 + autoTargets[3] * 65);
  blend(rack::DereverbAmount, autoTargets[4] * 70);
  blend(rack::ResonanceAmount, autoTargets[5] * 70);
  blend(rack::SculptorAmount, 20 + autoTargets[5] * 30);
  blend(rack::ReverbMix, 4 + autoTargets[6] * 14);
  blend(rack::CompParallel, 65 + autoTargets[2] * 20);
  blend(rack::SatParallel, 65);
  blend(rack::EQParallel, 82);
  blend(rack::SatLow, 25);
  blend(rack::SatMid, 30);
  blend(rack::SatHigh, 15);
  blend(rack::TuneAmount, p[rack::PitchStyle] > .5f ? 100.0f : 85.0f);
  blend(rack::HarmonyMix, bus ? 0 : 3 + autoTargets[7] * 7);
  blend(rack::DelayDuck, 90);
  blend(rack::ReverbDuck, 85);
  p[rack::WarmOn] = p[rack::CompOn] = p[rack::FetOn] = p[rack::SaturationOn] =
      p[rack::SculptorOn] = p[rack::ResonanceOn] = 1;
  p[rack::DereverbOn] = p[rack::NoiseOn] = p[rack::TuneOn] =
      p[rack::HarmonyOn] = bus ? 0 : 1;
  if (s >= .8f) {
    p[rack::GodSpace] = p[rack::ReverbOn] = p[rack::DelayOn] = 1;
    p[rack::VerbAMix] = juce::jmax(8.0f, p[rack::VerbAMix]);
    p[rack::VerbBMix] = juce::jmax(7.0f, p[rack::VerbBMix]);
    p[rack::VerbCMix] = juce::jmax(4.0f, p[rack::VerbCMix]);
  }
  float manual = p[rack::InputTrim];
  float target =
      rawDB > -60 ? juce::jlimit(-12.0f, 12.0f, -18 - rawDB) : manual;
  p[rack::InputTrim] =
      juce::jlimit(-24.0f, 24.0f, manual + s * (target - manual));
  p[rack::AutoGain] = 0;
  sinoTrim.store(p[rack::InputTrim] - manual);
}
void M7GProcessor::refreshFilters() {
  const auto &p = settings;
  auto eqc = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
      rate, juce::jmin(double(p[rack::EQFreq]), rate * .4), p[rack::EQQ],
      juce::Decibels::decibelsToGain(p[rack::EQOn] > .5f ? p[rack::EQGain]
                                                         : 0));
  auto airc = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
      rate, juce::jmin(10000.0, rate * .4), .707f,
      juce::Decibels::decibelsToGain(
          p[rack::AirOn] > .5f ? p[rack::AirAmount] * .045f + p[rack::AirGain]
                               : 0));
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
        .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i / 511);
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
  juce::ScopedNoDenormals noDenormals;
  int n = b.getNumSamples(),
      channels = juce::jmin(2, getTotalNumOutputChannels());
  if (n == 0 || channels == 0)
    return;
  auto referenceInput = getBusBuffer(b, true, 1);
  bool referenceFits = n <= referenceScratch.getNumSamples();
  if (referenceFits)
    for (int c = 0; c < 2; ++c) {
      if (referenceInput.getNumChannels() > 0)
        referenceScratch.copyFrom(
            c, 0, referenceInput,
            juce::jmin(c, referenceInput.getNumChannels() - 1), 0, n);
      else
        referenceScratch.clear(c, 0, n);
    }
  if (getChannelCountOfBus(true, 0) == 1 && channels == 2)
    b.copyFrom(1, 0, b, 0, 0, n);
  for (size_t k = 0; k < rack::Count; ++k)
    settings[k] = parameter[k]->load();
  configureMode(juce::jlimit(0, 2, int(settings[rack::EngineMode])));
  bool wantsAlignment = activeMode != 0 && settings[rack::AlignOn] > .5f;
  if (wantsAlignment != alignmentActive) {
    alignmentActive = wantsAlignment;
    setLatencySamples(activeMode == 0
                          ? 0
                          : neural.latency() + V8Spectral::N * 2 +
                                V8Harmony::N * 2 + analogue.latency() +
                                saturation.latency() + limiter.latency() +
                                (alignmentActive ? aligner.latency() : 0));
  }
  aligner.setControls(settings);
  auto &reference = referenceScratch;
  bool hasReference =
      reference.getNumChannels() > 0 && reference.getRMSLevel(0, 0, n) > .0005f;
  referencePresent.store(hasReference);
  double tempo = 120;
  if (auto *head = getPlayHead())
    if (auto pos = head->getPosition())
      if (auto bpm = pos->getBpm())
        tempo = *bpm;
  double inputEnergy = 0;
  for (int c = 0; c < channels; ++c) {
    float r = b.getRMSLevel(c, 0, n);
    inputEnergy += r * r;
  }
  float rawDB = juce::Decibels::gainToDecibels(
      float(std::sqrt(inputEnergy / channels)), -100.0f);
  if (neuralClock <= 0) {
    std::array<float, 12> x{};
    x[0] = juce::jlimit(0.0f, 1.0f, (rawDB + 60) / 60);
    float peak = 0;
    for (int c = 0; c < channels; ++c)
      peak = juce::jmax(peak, b.getMagnitude(c, 0, n));
    x[1] = juce::jlimit(
        0.0f, 1.0f,
        (juce::Decibels::gainToDecibels(peak, -100.0f) - rawDB) / 24);
    x[2] = juce::jlimit(0.0f, 1.0f, analysisHiss * .3f);
    float total = 0;
    for (int i = 0; i < 6; ++i) {
      float e = 0;
      for (int k = i * 5; k < juce::jmin(32, i * 5 + 5); ++k)
        e += juce::Decibels::decibelsToGain(spectrum[size_t(k)].load());
      x[size_t(3 + i)] = e;
      total += e;
    }
    for (int i = 3; i < 9; ++i)
      x[size_t(i)] /= total + 1e-12f;
    x[9] = pitchHz.load() > 35 ? pitchDetector.confidence : 0;
    x[10] = rawDB < previousBlockDB
                ? juce::jlimit(0.0f, 1.0f, (previousBlockDB - rawDB) / 18)
                : 0;
    x[11] = x[6] * .4f + x[7] * .7f + x[8];
    auto target = v8::autoMix(x);
    for (size_t i = 0; i < 8; ++i)
      autoTargets[i] = .65f * autoTargets[i] + .35f * target[i];
    previousBlockDB = rawDB;
    neuralClock = int(rate * .5);
  }
  neuralClock -= n;
  applySino(settings, rawDB);
  const auto &p = settings;
  int voice = juce::jlimit(0, 5, int(p[rack::VoiceType]));
  if (filterClock <= 0 || p[rack::EQOn] != lastSettings[rack::EQOn] ||
      p[rack::AirOn] != lastSettings[rack::AirOn]) {
    refreshFilters();
    filterClock = 512;
  }
  filterClock -= n;
  auto gateSettings = p;
  gateSettings[rack::DeessOn] = gateSettings[rack::EQOn] =
      gateSettings[rack::DeessListen] = 0;
  gate.setControls(gateSettings);
  auto essSettings = p;
  essSettings[rack::GateOn] = essSettings[rack::EQOn] = 0;
  cleaner.setControls(essSettings);
  analogue.setControls(p);
  saturation.setControls(p);
  fet.setControls(p);
  space.setControls(p, tempo);
  stereo.setControls(p);
  limiter.setControls(p);
  roomCleaner.setControls(p);
  resonance.setControls(p);
  resonance.setPitch(pitchHz.load() * ratioSmooth.getCurrentValue(),
                     pitchDetector.confidence);
  auto lead = p;
  lead[rack::HarmonyOn] = 0;
  leadTuner.setControls(lead, ratioSmooth.getCurrentValue(), pitchHz.load(),
                        pitchDetector.confidence);
  auto harmonySettings = p;
  harmonySettings[rack::TuneOn] = 0;
  harmony.setControls(harmonySettings, 1,
                      pitchHz.load() * ratioSmooth.getCurrentValue(),
                      pitchDetector.confidence);
  if (rawDB > -55)
    autoGainDB += (1 - std::exp(-float(n) / float(rate))) *
                  (juce::jlimit(-12.0f, 12.0f, -18 - rawDB) - autoGainDB);
  float automatic = p[rack::AutoGain] > .5f ? autoGainDB : 0;
  inputSmooth.setTargetValue(
      juce::Decibels::decibelsToGain(p[rack::InputTrim] + automatic));
  outputSmooth.setTargetValue(juce::Decibels::decibelsToGain(
      p[rack::OutputTrim] + sinoMakeupDB * p[rack::Sino] * .01f));
  mixSmooth.setTargetValue(p[rack::Mix] * .01f);
  bypassSmooth.setTargetValue(p[rack::Bypass]);
  if (analysisRequest.exchange(false)) {
    analysisState.store(1);
    analysisSamples = 0;
    analysisEnergy = analysisHigh = 0;
    analysisPeak = analysisPrevious = analysisLowDB = 0;
    analysisProgress.store(0);
  }
  double inEnergy = 0, outEnergy = 0, leftEnergy = 0, rightEnergy = 0,
         cross = 0;
  for (int i = 0; i < n; ++i) {
    float original[2]{}, mono = 0;
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
      float d = mono - analysisPrevious;
      analysisHigh += d * d;
      analysisPrevious = mono;
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
    float ratio = ratioSmooth.getNextValue(), gain = inputSmooth.getNextValue(),
          outputGain = outputSmooth.getNextValue(),
          mix = mixSmooth.getNextValue();
    float frame[2]{};
    for (int c = 0; c < channels; ++c) {
      frame[c] = original[c] * gain;
      inEnergy += frame[c] * frame[c];
    }
    float referenceFrame[2]{};
    if (referenceFits && referenceInput.getNumChannels() > 0)
      for (int c = 0; c < channels; ++c)
        referenceFrame[c] = reference.getSample(
            juce::jmin(c, reference.getNumChannels() - 1), i);
    aligner.process(frame, referenceFrame, channels, alignmentActive);
    float pitchFiltered = (frame[0] + frame[channels - 1]) * .5f;
    for (auto &filter : pitchAntiAlias)
      pitchFiltered = filter.processSample(pitchFiltered);
    if (++decimatePos >= decimation) {
      decimatePos = 0;
      pitchWindow[size_t(pitchPos)] = pitchFiltered;
      pitchPos = (pitchPos + 1) % 2048;
      pitchCount = juce::jmin(2048, pitchCount + 1);
      pitchHop = juce::jmin(512, pitchHop + 1);
    }
    if (activeMode != 0 && pitchHop >= 512 && pitchCount == 2048) {
      pitchHop = 0;
      std::array<float, 2048> ordered{};
      for (int k = 0; k < 2048; ++k)
        ordered[size_t(k)] = pitchWindow[size_t((pitchPos + k) % 2048)];
      float f =
          pitchDetector.detect(ordered.data(), 2048, float(rate) / decimation);
      pitchHz.store(f);
      pitchNN.store(pitchDetector.neuralConfidence);
      float target = tuner.target(
          f, pitchDetector.confidence, 512 / (float(rate) / decimation),
          int(p[rack::PitchKey]), int(p[rack::PitchScale]));
      targetHz.store(target);
      float correction = 1;
      if (target > 35 && voice != 5 && p[rack::TuneOn] > .5f && f > 35)
        correction = juce::jlimit(
            .90f, 1.12f, 1 + (target / f - 1) * p[rack::TuneAmount] * .01f);
      float current = ratioSmooth.getCurrentValue();
      ratioSmooth.setCurrentAndTargetValue(current);
      ratioSmooth.reset(
          rate, p[rack::PitchStyle] > .5f ? .002 : p[rack::TuneSpeed] * .001);
      ratioSmooth.setTargetValue(correction);
      leadTuner.setControls(lead, current, f, pitchDetector.confidence);
      harmony.setControls(harmonySettings, 1, f * current,
                          pitchDetector.confidence);
      resonance.setPitch(f * current, pitchDetector.confidence);
    }

    float denoised[2]{frame[0], frame[channels - 1]};
    neural.processFrame(
        denoised, channels,
        p[rack::NoiseOn] > .5f && voice != 5 ? p[rack::NoiseAmount] * .01f : 0);
    if (activeMode != 0) {
      for (int c = 0; c < channels; ++c)
        frame[c] = denoised[c];
      roomCleaner.process(frame, channels);
    }
    float gateDry[2]{frame[0], frame[channels - 1]};
    gate.cleanFrame(frame, channels);
    for (int c = 0; c < channels; ++c)
      frame[c] =
          gateDry[c] + p[rack::GateParallel] * .01f * (frame[c] - gateDry[c]);
    if (activeMode != 0) {
      leadTuner.setRatio(ratio);
      leadTuner.process(frame, channels);
    }
    analogue.colour(frame, channels);
    float compDry[2]{frame[0], frame[channels - 1]};
    analogue.compress(frame, channels, -6 - .3f * p[rack::CompReduction],
                      p[rack::CompRatio], voice == 3, p[rack::CompRelease],
                      p[rack::CompOn] > .5f, p[rack::CompGain]);
    fet.process(frame, channels);
    for (int c = 0; c < channels; ++c)
      frame[c] =
          compDry[c] + p[rack::CompParallel] * .01f * (frame[c] - compDry[c]);
    for (int c = 0; c < channels; ++c)
      eqDryDelay.setSample(c, write, frame[c]);
    if (activeMode != 2)
      for (int c = 0; c < channels; ++c) {
        auto k = size_t(c);
        frame[c] = presence[k].processSample(frame[c]);
        float airy = air[k].processSample(frame[c]),
              filtered = airCut[k].processSample(airy);
        if (p[rack::AirOn] > .5f)
          frame[c] += p[rack::AirParallel] * .01f * (filtered - frame[c]);
      }
    if (activeMode != 0)
      resonance.process(frame, channels);
    float eqDry[2]{};
    int eqRead = (write + 32768 - (activeMode ? V8Spectral::N : 0)) & 32767;
    for (int c = 0; c < channels; ++c)
      eqDry[c] = eqDryDelay.getSample(c, eqRead);
    saturation.process(frame, channels, eqDry);
    float essDry[2]{frame[0], frame[channels - 1]};
    cleaner.cleanFrame(frame, channels);
    for (int c = 0; c < channels; ++c)
      frame[c] =
          essDry[c] + p[rack::DeessParallel] * .01f * (frame[c] - essDry[c]);
    stereo.process(frame, channels);
    if (activeMode != 0)
      harmony.process(frame, channels);
    space.process(frame, channels);
    int dryIndex =
        (write + 32768 - getLatencySamples() + limiter.latency()) & 32767;
    for (int c = 0; c < channels; ++c) {
      float dry = dryDelay.getSample(c, dryIndex) *
                  juce::Decibels::decibelsToGain(p[rack::InputTrim]);
      frame[c] = (p[rack::Solo] > .5f ? dry - frame[c]
                                      : dry + mix * (frame[c] - dry)) *
                 outputGain;
    }
    limiter.process(frame, channels);
    if (p[rack::LimitOn] > .5f) {
      float ceiling = juce::Decibels::decibelsToGain(p[rack::LimitCeiling]);
      for (int c = 0; c < channels; ++c)
        frame[c] = juce::jlimit(-ceiling, ceiling, frame[c]);
    }
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
  alignOffset.store(alignmentActive ? aligner.offsetMs() : 0);
  alignConfidence.store(alignmentActive ? aligner.certainty() : 0);
  for (int c = 0; c < 2; ++c)
    outputChannels[size_t(c)].store(juce::Decibels::gainToDecibels(
        b.getRMSLevel(juce::jmin(c, channels - 1), 0, n), -100.0f));
  inputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(inEnergy / (n * channels))), -100.0f));
  outputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(outEnergy / (n * channels))), -100.0f));
  if (p[rack::Sino] > .01f && p[rack::Bypass] < .5f && rawDB > -50 &&
      outputDB.load() > -60) {
    float error = -18 - (outputDB.load() - p[rack::OutputTrim]);
    sinoMakeupDB = juce::jlimit(
        -6.0f, 12.0f,
        sinoMakeupDB + (1 - std::exp(-float(n) / float(rate * .8))) * error);
  } else if (p[rack::Sino] < .01f)
    sinoMakeupDB *= std::exp(-float(n) / float(rate * .3));
  reductionDB.store(p[rack::CompOn] > .5f ? analogue.gainReduction() : 0);
  fetGR.store(fet.gainReduction());
  deEssDB.store(cleaner.getDeEssReduction());
  gateGR.store(p[rack::GateOn] > .5f ? gate.getGateReduction() : 0);
  limitGR.store(p[rack::LimitOn] > .5f ? limiter.reduction() : 0);
  dereverbGR.store(activeMode ? roomCleaner.gainReduction() : 0);
  resonanceGR.store(activeMode ? resonance.gainReduction() : 0);
  harmonyVoices.store(activeMode ? harmony.voices() : 0);
  revDuckDB.store(space.reverbDucking());
  delDuckDB.store(space.delayDucking());
  for (int j = 0; j < 3; ++j)
    layerMeters[size_t(j)].store(space.layerDB(j));
  modPhase.store(stereo.lfoPhase());
  correlation.store(float(cross / std::sqrt(leftEnergy * rightEnergy + 1e-20)));
  double feedback = juce::jmax(.001, double(p[rack::DelayFeedback] * .01f)),
         delayTail = 60 / juce::jlimit(30.0, 300.0, tempo) * 2 *
                     std::log(.001) / std::log(feedback);
  double verbTail =
      juce::jmax(p[rack::VerbADecay],
                 juce::jmax(p[rack::VerbBDecay], p[rack::VerbCDecay])) *
      p[rack::ReverbDecay] / 1.5;
  tailSeconds.store(
      juce::jmin(180.0, (p[rack::ReverbOn] > .5f ? verbTail : 0) +
                            (p[rack::DelayOn] > .5f ? delayTail : 0) +
                            getLatencySamples() / rate + .2));
}
juce::ValueTree M7GProcessor::parameterState() {
  auto state = apvts.copyState();
  state.setProperty("program", program.load(), nullptr);
  state.setProperty("factoryCreator", rack::presetCreator, nullptr);
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
  program.store(juce::jlimit(0, rack::presetCount - 1,
                             int(state.getProperty("program", 0))));
  apvts.replaceState(state);
}
void M7GProcessor::beginAnalysis(int module) {
  requestedModule.store(juce::jlimit(0, analysisAll, module));
  analysisState.store(1);
  analysisProgress.store(0);
  analysisRequest.store(true);
}
bool M7GProcessor::applyAnalysis() {
  if (analysisState.load() != 2)
    return false;
  int m = requestedModule.load();
  auto selected = [&](int x) { return m == analysisAll || m == x; };
  static const rack::Param enable[]{
      rack::GateOn,     rack::DeessOn,     rack::WarmOn,       rack::EQOn,
      rack::AirOn,      rack::CompOn,      rack::LimitOn,      rack::ReverbOn,
      rack::DelayOn,    rack::StereoOn,    rack::ModOn,        rack::TuneOn,
      rack::DereverbOn, rack::ResonanceOn, rack::SaturationOn, rack::HarmonyOn,
      rack::FetOn,      rack::NoiseOn,     rack::WarmOn,       rack::CompOn,
      rack::ReverbOn,   rack::ReverbOn,    rack::ReverbOn,     rack::DelayOn,
      rack::SculptorOn, rack::AlignOn};
  if (m < analysisAll)
    setValue(enable[m], 1);
  std::array<float, 12> measured{};
  measured[0] = juce::jlimit(0.0f, 1.0f, (analysisLevelDB + 60) / 60);
  measured[1] = juce::jlimit(0.0f, 1.0f, analysisCrest / 24);
  measured[2] = juce::jlimit(0.0f, 1.0f, analysisHiss * .3f);
  float total = 0;
  for (int i = 0; i < 6; ++i) {
    for (int k = i * 5; k < juce::jmin(32, i * 5 + 5); ++k)
      measured[size_t(3 + i)] +=
          juce::Decibels::decibelsToGain(spectrum[size_t(k)].load());
    total += measured[size_t(3 + i)];
  }
  for (int i = 3; i < 9; ++i)
    measured[size_t(i)] /= total + 1e-15f;
  measured[9] = pitchHz.load() > 35 ? 1.0f : 0.0f;
  measured[10] = .3f;
  measured[11] = measured[7] + measured[8];
  const auto recommendations = v8::autoMix(measured);
  float hiss = juce::jlimit(0.0f, 1.0f, analysisHiss * .3f),
        processedDB = analysisProcessedDB;
  if (m == analysisAll) {
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
    setValue(rack::AirGain, 1);
  }
  if (selected(5)) {
    setValue(rack::CompReduction,
             juce::jlimit(25.0f, 65.0f, 30 + analysisCrest));
    setValue(rack::CompRatio,
             juce::jlimit(2.0f, 5.0f, 2 + analysisCrest * .15f));
  }
  if (selected(6))
    setValue(rack::LimitCeiling, -1);
  if (selected(7)) {
    setValue(rack::ReverbDecay,
             (value(rack::VoiceType) == 3 ? 1.7f : 1.0f) + recommendations[6]);
    setValue(rack::ReverbDuck, 85);
    if (value(rack::ReverbMix) < .1f)
      setValue(rack::ReverbMix, 9);
  }
  if (selected(8)) {
    setValue(rack::DelayFeedback, 18 + 24 * recommendations[6]);
    setValue(rack::DelayDuck, 90);
    if (value(rack::DelayMix) < .1f)
      setValue(rack::DelayMix, 6);
  }
  if (selected(9))
    setValue(rack::Width, correlation.load() < 0 ? 85.0f : 110.0f);
  if (selected(10))
    setValue(rack::ModMix,
             (value(rack::VoiceType) == 0 ? 3.0f : 8.0f) + 4 * measured[9]);
  if (selected(11))
    setValue(rack::TuneAmount, value(rack::PitchStyle) > .5f ? 100.0f : 82.0f);
  if (selected(12))
    setValue(rack::DereverbAmount, 20 + 45 * recommendations[4]);
  if (selected(13))
    setValue(rack::ResonanceAmount, 25 + 45 * recommendations[5]);
  if (selected(14)) {
    setValue(rack::SatMid, 20 + 25 * recommendations[0]);
    setValue(rack::SatHigh, 12 + 15 * recommendations[1]);
  }
  if (selected(15)) {
    setValue(rack::HarmonyMix, value(rack::VoiceType) == 5 ? 0.0f : 16.0f);
    if (value(rack::VoiceType) == 5)
      setValue(rack::HarmonyOn, 0);
  }
  if (selected(16))
    setValue(rack::FetThreshold, juce::jlimit(-28.0f, -10.0f, processedDB - 3));
  if (selected(17))
    setValue(rack::NoiseAmount, 35 + 50 * recommendations[3]);
  if (selected(18)) {
    setValue(rack::PreampImpedance, 1600 + 1000 * hiss);
    setValue(rack::PreampGain, analysisCrest > 12 ? 0 : 2);
  }
  if (selected(19))
    setValue(rack::CompParallel,
             juce::jlimit(60.0f, 85.0f, 88 - analysisCrest));
  if (selected(20))
    setValue(rack::VerbAMix, 7 + 3 * recommendations[6]);
  if (selected(21))
    setValue(rack::VerbBMix, 5 + 3 * recommendations[6]);
  if (selected(22))
    setValue(rack::VerbCMix, 2 + 3 * recommendations[6]);
  if (selected(23))
    setValue(rack::ThrowSensitivity, 150);
  if (selected(25)) {
    if (referencePresent.load()) {
      setValue(rack::AlignAmount, 90);
      setValue(rack::AlignRange, 50);
    } else {
      setValue(rack::AlignOn, 0);
      if (m == 25) {
        analysisState.store(-2);
        return true;
      }
    }
  }
  if (selected(24))
    setValue(rack::SculptorAmount, 25 + 30 * recommendations[5]);
  analysisState.store(0);
  analysisProgress.store(0);
  return true;
}
juce::AudioProcessorEditor *M7GProcessor::createEditor() {
  return new M7GEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new M7GProcessor();
}
