#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace scratch;
juce::AudioProcessorValueTreeState::ParameterLayout ScratchProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout l;
  auto d = defaults();
  auto number = [&](int i, const char *name, float lo, float hi, float step) {
    l.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids[size_t(i)], 1}, name,
        juce::NormalisableRange<float>(lo, hi, step), d[size_t(i)]));
  };
  number(Mix, "Mezcla", 0, 100, .1f);
  number(Intensity, "Intensidad", 0, 150, .1f);
  number(Speed, "Agite", .25f, 2.5f, .01f);
  number(Tension, "Tension", 0, 100, .1f);
  number(Inertia, "Inercia", .85f, .98f, .0001f);
  number(Friction, "Friccion", .0001f, .006f, .00001f);
  number(MotorTorque, "Torque del motor", 0, 1.5f, .01f);
  number(ReleaseTime, "Soltar ms", 40, 1600, 1);
  number(PitchRange, "Rango de velocidad", 0, 100, .1f);
  number(VinylNoise, "Ruido de vinilo", 0, 100, .1f);
  number(Crackle, "Polvo y crujidos", 0, 100, .1f);
  number(TransformerRate, "Cortes por segundo", 1, 40, .1f);
  number(EchoDelay, "Eco ms", 40, 1000, 1);
  number(EchoFeedback, "Repeticiones de eco", 0, .8f, .001f);
  number(EchoMix, "Mezcla del eco", 0, 100, .1f);
  number(Variant, "Variacion", 0, 29, 1);
  number(Depth, "Recorrido", 10, 100, .1f);
  number(Duration, "Duracion ms", 180, 3000, 1);
  number(LoFi, "Color LoFi", 0, 100, .1f);
  number(Drive, "Saturacion", 0, 100, .1f);
  number(Sensitivity, "Sensibilidad de palabra", 0, 100, .1f);
  number(Gap, "Silencio entre palabras ms", 50, 400, 1);
  number(Position, "Posicion del disco", 0, 1, .00001f);
  number(Crossfader, "Crossfader", 0, 1, .001f);
  number(Output, "Salida dB", -12, 12, .1f);
  number(NoiseFloor, "Perfil de ruido dB", -85, -18, .1f);
  auto choice = [&](int i, const char *name, juce::StringArray a) {
    l.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids[size_t(i)], 1}, name, a, int(d[size_t(i)])));
  };
  choice(CrossfaderCurve, "Curva del fader", {"CUT", "SMOOTH", "HAMSTER"});
  choice(Pattern, "Gesto",
         {"CHIRP", "TRANSFORMER", "FLARE / CRAB", "WORD CHOP", "BABY",
          "TEAR / ORBIT", "SLOW DRAG", "LOFI SWEEP", "EXTREME", "SIGNATURE"});
  choice(Rpm, "RPM", {"33 1/3", "45"});
  for (int i : {VinylMode, AutoScratch, Intelligent, Touch, Hold, Bypass,
                Reverse, AutoFader, Cut, Brake, ReleaseAction})
    l.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ids[size_t(i)], 1}, ids[size_t(i)],
        d[size_t(i)] > .5f));
  for (int cue = 0; cue < 8; ++cue) {
    auto id = "cue" + juce::String(cue + 1);
    l.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{id, 1}, id, 0, 1, d[size_t(FirstCue + cue)]));
  }
  return l;
}
juce::String ScratchProcessor::parameterID(int i) const {
  return i < FirstCue ? juce::String(ids[size_t(i)])
                      : "cue" + juce::String(i - FirstCue + 1);
}
ScratchProcessor::ScratchProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Voz", juce::AudioChannelSet::stereo(), true)
              .withOutput("Scratch", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "WORD_SCRATCHER_300", layout()) {
  for (int i = 0; i < Count; ++i)
    raw[size_t(i)] = state.getRawParameterValue(parameterID(i));
  engine = std::make_unique<VinylEngine>();
  license.reload();
  authorized.store(license.authorized);
  favourites.addLines(
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("M7G/WordScratcher300-favourites.txt")
          .loadFileAsString());
  setCurrentProgram(0);
  compare[0] = state.copyState();
  compare[1] = state.copyState();
}
void ScratchProcessor::setValue(const juce::String &id, float v) {
  if (auto *p = state.getParameter(id))
    p->setValueNotifyingHost(p->convertTo0to1(v));
}
Values ScratchProcessor::parameters() const {
  Values v{};
  for (int i = 0; i < Count; ++i)
    v[size_t(i)] = raw[size_t(i)]->load();
  return v;
}
void ScratchProcessor::setCurrentProgram(int i) {
  i = juce::jlimit(0, 299, i);
  auto p = manager.presets[size_t(i)].values;
  p[NoiseFloor] = value("noiseFloor");
  p[Touch] = 0;
  p[Bypass] = 0;
  for (int j = 0; j < Count; ++j)
    setValue(parameterID(j), p[size_t(j)]);
  program.store(i);
  presetRequest.store(true);
}
bool ScratchProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  auto a = b.getMainInputChannelSet(), o = b.getMainOutputChannelSet();
  return (a == juce::AudioChannelSet::mono() ||
          a == juce::AudioChannelSet::stereo()) &&
         (o == a || (a == juce::AudioChannelSet::mono() &&
                     o == juce::AudioChannelSet::stereo()));
}
void ScratchProcessor::prepareToPlay(double sr, int) {
  rate = sr;
  engine->prepare(sr);
  setLatencySamples(VinylEngine::latencyFor(sr));
  scopeCount = lastSerial = 0;
  scopePeak = 0;
  knownPosition = false;
  resetRequest.store(false);
}
void ScratchProcessor::processBlock(juce::AudioBuffer<float> &b,
                                    juce::MidiBuffer &) {
  juce::ScopedNoDenormals noDenormals;
  int n = b.getNumSamples(),
      channels = juce::jmin(2, getTotalNumOutputChannels());
  if (!n || !channels)
    return;
  if (getChannelCountOfBus(true, 0) == 1 && channels == 2)
    b.copyFrom(1, 0, b, 0, 0, n);
  double bpm = 120;
  bool seek = false;
  if (auto *h = getPlayHead())
    if (auto pos = h->getPosition()) {
      if (auto tempo = pos->getBpm())
        bpm = *tempo;
      if (auto at = pos->getTimeInSamples()) {
        seek = knownPosition && pos->getIsPlaying() &&
               std::abs(double(*at) - (previousSample + previousBlock)) >
                   juce::jmax(8, n * 2);
        previousSample = double(*at);
        previousBlock = n;
        knownPosition = true;
      }
    }
  if (resetRequest.exchange(false) || seek)
    engine->reset();
  auto p = parameters();
  bool preset = presetRequest.exchange(false);
  if (hostBypass || !authorized.load())
    p[Bypass] = 1;
  engine->controls(p, bpm, preset);
  if (captureRequest.exchange(false))
    engine->requestCapture();
  if (playRequest.exchange(false) || (preset && engine->haveWord))
    engine->requestPlay();
  if (stopRequest.exchange(false))
    engine->requestStop();
  if (learnRequest.exchange(false))
    engine->requestLearn();
  int cue = cueRequest.exchange(-1);
  if (cue >= 0)
    engine->requestCue(cue);
  double in = 0, out = 0;
  int hop = std::max(1, int(rate * .01));
  for (int i = 0; i < n; ++i) {
    float l = b.getSample(0, i), r = b.getSample(channels - 1, i);
    if (!std::isfinite(l))
      l = 0;
    if (!std::isfinite(r))
      r = 0;
    in += (l * l + r * r) * .5;
    auto y = engine->process(l, r);
    b.setSample(0, i, channels == 1 ? (y[0] + y[1]) * .5f : y[0]);
    if (channels == 2)
      b.setSample(1, i, y[1]);
    out += (y[0] * y[0] + y[1] * y[1]) * .5;
    scopePeak = std::max(scopePeak, std::max(std::abs(y[0]), std::abs(y[1])));
    if (++scopeCount >= hop) {
      int h = waveHead.load();
      outputWave[size_t(h)].store(scopePeak);
      waveHead.store((h + 1) & 511);
      scopeCount = 0;
      scopePeak = 0;
    }
  }
  if (engine->serial != lastSerial) {
    for (int i = 0; i < 512; ++i)
      wordWave[size_t(i)].store(engine->wordPeak(i));
    lastSerial = engine->serial;
  }
  if (engine->learnReady) {
    engine->learnReady = false;
    noiseFloor.store(engine->learnedFloor);
    triggerAsyncUpdate();
  }
  platterPosition.store(float(engine->position));
  playbackRate.store(engine->outputRate);
  gate.store(engine->currentGate);
  activity.store(engine->currentActivity);
  hasWord.store(engine->haveWord);
  scratching.store(engine->playing);
  wordCount.store(engine->words);
  wordMilliseconds.store(float(engine->wordSize() * 1000 / rate));
  digitalUnderflows.store(engine->digital.underflows);
  noiseLearning.store(engine->detector.learning);
  inputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(in / std::max(1, n))), -100.f));
  outputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(out / std::max(1, n))), -100.f));
  double echoTime = p[EchoMix] > .001f
                        ? p[EchoDelay] * .001 * std::log(.001) /
                              std::log(std::max(.05f, p[EchoFeedback]))
                        : 0;
  tail.store(std::min(
      60., (p[Hold] > .5f ? 15. : p[Duration] * .001 + p[ReleaseTime] * .001) +
               echoTime + VinylEngine::latencyFor(rate) / rate));
}
bool ScratchProcessor::isFavourite(int i) const {
  return favourites.contains(juce::String(i));
}
void ScratchProcessor::toggleFavourite(int i) {
  auto s = juce::String(i);
  if (favourites.contains(s))
    favourites.removeString(s);
  else
    favourites.add(s);
  auto f =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("M7G/WordScratcher300-favourites.txt");
  f.getParentDirectory().createDirectory();
  f.replaceWithText(favourites.joinIntoString("\n"));
}
void ScratchProcessor::copyCompare() {
  const juce::ScopedLock lock(compareLock);
  int slot = compareIndex.load();
  compare[size_t(1 - slot)] = state.copyState();
  compareProgram[size_t(1 - slot)] = program.load();
}
void ScratchProcessor::selectCompare(int i) {
  i = juce::jlimit(0, 1, i);
  const juce::ScopedLock lock(compareLock);
  int previous = compareIndex.load();
  if (previous == i)
    return;
  compare[size_t(previous)] = state.copyState();
  compareProgram[size_t(previous)] = program.load();
  state.replaceState(compare[size_t(i)].createCopy());
  program.store(compareProgram[size_t(i)]);
  compareIndex.store(i);
  presetRequest.store(true);
}
void ScratchProcessor::getStateInformation(juce::MemoryBlock &dest) {
  auto s = state.copyState();
  s.setProperty("program", program.load(), nullptr);
  s.setProperty("creator", "Makkine Seven Golden", nullptr);
  s.setProperty("compareIndex", compareIndex.load(), nullptr);
  for (auto child : s)
    if (child.getProperty("id").toString() == "touch")
      child.setProperty("value", 0, nullptr);
  {
    const juce::ScopedLock lock(compareLock);
    for (int i = 0; i < 2; ++i) {
      auto c = i == compareIndex.load() ? state.copyState()
                                        : compare[size_t(i)].createCopy();
      for (auto child : c)
        if (child.getProperty("id").toString() == "touch")
          child.setProperty("value", 0, nullptr);
      c.setProperty("program",
                    i == compareIndex.load() ? program.load()
                                             : compareProgram[size_t(i)],
                    nullptr);
      auto wrap = juce::ValueTree(i == 0 ? "COMPARE_A" : "COMPARE_B");
      wrap.addChild(c, -1, nullptr);
      s.addChild(wrap, -1, nullptr);
    }
  }
  if (auto x = s.createXml())
    copyXmlToBinary(*x, dest);
}
void ScratchProcessor::setStateInformation(const void *d, int size) {
  auto x = getXmlFromBinary(d, size);
  if (!x || !x->hasTagName("WORD_SCRATCHER_300"))
    return;
  auto s = juce::ValueTree::fromXml(*x);
  int pr = juce::jlimit(0, 299, int(s.getProperty("program", 0)));
  {
    const juce::ScopedLock lock(compareLock);
    for (int i = 0; i < 2; ++i) {
      auto c = s.getChildWithName(i == 0 ? "COMPARE_A" : "COMPARE_B");
      compare[size_t(i)] =
          c.getNumChildren() ? c.getChild(0).createCopy() : s.createCopy();
      compareProgram[size_t(i)] = juce::jlimit(
          0, 299, int(compare[size_t(i)].getProperty("program", pr)));
    }
    compareIndex.store(
        juce::jlimit(0, 1, int(s.getProperty("compareIndex", 0))));
  }
  s.removeChild(s.getChildWithName("COMPARE_A"), nullptr);
  s.removeChild(s.getChildWithName("COMPARE_B"), nullptr);
  state.replaceState(s);
  program.store(pr);
  resetRequest.store(true);
}
juce::String ScratchProcessor::statusText() const {
  if (noiseLearning.load())
    return "APRENDIENDO RUIDO: deja sonar solo el cuarto";
  if (!hasWord.load())
    return "ESCUCHANDO: reproduce una palabra o pulsa CAPTURAR";
  if (scratching.load())
    return value("vinylMode") > .5f
               ? "SCRATCH VINYL: tono y reversa siguen el disco"
               : "SCRATCH DIGITAL: Rubber Band conserva el tono";
  return "PALABRA LISTA / " + juce::String(int(wordMilliseconds.load())) +
         " ms / arrastra el vinilo o pulsa SCRATCH";
}
juce::AudioProcessorEditor *ScratchProcessor::createEditor() {
  return new ScratchEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new ScratchProcessor();
}
