#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace tune;
juce::AudioProcessorValueTreeState::ParameterLayout TuneProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout l;
  auto d = defaults();
  const char *captions[]{"Correccion IA", "Human a Robot", "Retune ms",
                         "Humanize",      "Formante",      "Mix armonias"};
  for (int i = 0; i < 6; ++i)
    l.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids[size_t(i)], 1}, captions[i],
        juce::NormalisableRange<float>(i == Formant ? -6.f : 0.f,
                                       i == Retune    ? 300.f
                                       : i == Formant ? 6.f
                                                      : 100.f,
                                       i == Retune ? 1.f : .1f),
        d[size_t(i)]));
  auto choice = [&](int i, const char *name, const juce::StringArray &a,
                    int def) {
    l.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids[size_t(i)], 1}, name, a, def));
  };
  choice(Model, "Perfil de afinacion",
         {"ALIEN NATURAL", "REGGAETON FLOW", "TRAP AUTO", "POP CRYSTAL",
          "CLASSIC VINTAGE"},
         0);
  choice(Key, "Tonica",
         {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 0);
  choice(ScaleKind, "Escala", {"Cromatica", "Mayor", "Menor", "Menor armonica"},
         1);
  choice(Quality, "Calidad", {"ESTUDIO", "MONITOR"}, 0);
  choice(Interval, "Armonia",
         {"3a + 5a", "5a + octava", "Octava arriba", "Octava abajo",
          "Coro completo", "Personalizada"},
         0);
  for (auto i : {AutoKey, HarmonyOn, DoublesOn, Bypass})
    l.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ids[size_t(i)], 1}, ids[size_t(i)],
        d[size_t(i)] > .5f));
  l.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID{"voices", 1}, "Voces de armonia", 1, 4, 3));
  l.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID{"custom", 1}, "Intervalo personalizado", -24, 24, 7));
  l.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"doubleHuman", 1}, "Humanizar dobles", 0, 100, 70));
  for (int v = 0; v < 6; ++v)
    for (int f = 0; f < 6; ++f) {
      auto id = "v" + juce::String(v + 1) + "_" + fxIds[size_t(f)];
      l.add(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{id, 1}, id,
          juce::NormalisableRange<float>(
              f == 5 ? -1.f : 0.f, f == 5 ? 1.f : 100.f, f == 5 ? .01f : .1f),
          d[size_t(FirstFX + v * 6 + f)]));
    }
  return l;
}
juce::String TuneProcessor::parameterID(int i) const {
  return i < FirstFX ? juce::String(ids[size_t(i)])
                     : "v" + juce::String((i - FirstFX) / 6 + 1) + "_" +
                           fxIds[size_t((i - FirstFX) % 6)];
}
TuneProcessor::TuneProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Voz", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "M7G_TUNE", layout()) {
  for (int i = 0; i < Count; ++i)
    raw[size_t(i)] = state.getRawParameterValue(parameterID(i));
  for (int i = 0; i < 4; ++i)
    bankPrograms[size_t(i)].store(i * 25);
  neural = std::make_unique<NeuralWorker>();
  engine = std::make_unique<Engine>();
  license.reload();
  authorized.store(license.authorized);
  favourites.addLines(
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("M7G/Tune-V1-favourites.txt")
          .loadFileAsString());
  compare[0] = state.copyState();
  compare[1] = state.copyState();
}
void TuneProcessor::setValue(const juce::String &id, float v) {
  if (auto *p = state.getParameter(id))
    p->setValueNotifyingHost(p->convertTo0to1(v));
}
Values TuneProcessor::parameters() const {
  Values p{};
  for (int i = 0; i < Count; ++i)
    p[size_t(i)] = raw[size_t(i)]->load();
  return p;
}
void TuneProcessor::setCurrentProgram(int i) {
  i = juce::jlimit(0, 99, i);
  auto q = presets()[size_t(i)].p;
  q[Quality] = value("quality");
  q[Bypass] = 0;
  for (int j = 0; j < Count; ++j)
    setValue(parameterID(j), q[size_t(j)]);
  program.store(i);
  browserBank.store(i / 25);
  bankPrograms[size_t(i / 25)].store(i);
}
void TuneProcessor::setBank(int b) {
  setCurrentProgram(bankPrograms[size_t(juce::jlimit(0, 3, b))].load());
}
bool TuneProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  auto a = b.getMainInputChannelSet(), o = b.getMainOutputChannelSet();
  return (a == juce::AudioChannelSet::mono() ||
          a == juce::AudioChannelSet::stereo()) &&
         (o == a || (a == juce::AudioChannelSet::mono() &&
                     o == juce::AudioChannelSet::stereo()));
}
void TuneProcessor::prepareToPlay(double sr, int) {
  rate = sr;
  int q = int(value("quality"));
  engine->prepare(sr, q);
  activeQuality.store(q);
  setLatencySamples(engine->latency());
  scopeCount = 0;
  scopePeak = 0;
  previousBlock = 0;
  knownPosition = false;
  resetRequested.store(false);
}
void TuneProcessor::processBlock(juce::AudioBuffer<float> &b,
                                 juce::MidiBuffer &) {
  juce::ScopedNoDenormals quiet;
  int n = b.getNumSamples(),
      channels = juce::jmin(2, getTotalNumOutputChannels());
  if (!n || !channels)
    return;
  if (getChannelCountOfBus(true, 0) == 1 && channels == 2)
    b.copyFrom(1, 0, b, 0, 0, n);
  bool run = false, seek = false;
  if (auto *h = getPlayHead())
    if (auto pos = h->getPosition()) {
      run = pos->getIsPlaying();
      if (auto s = pos->getTimeInSamples()) {
        seek = knownPosition && run &&
               std::abs(double(*s) - (previousSample + previousBlock)) >
                   juce::jmax(n * 2, 8);
        previousSample = double(*s);
        previousBlock = n;
        knownPosition = true;
      }
    }
  playing.store(run);
  int q = int(raw[Quality]->load());
  if (q != activeQuality.load() && !run) {
    activeQuality.store(q);
    engine->setQuality(q);
    triggerAsyncUpdate();
  }
  if (resetRequested.exchange(false) || seek)
    engine->reset();
  engine->profileAir = neural->acoustic[2].load();
  engine->profileRoughness = neural->acoustic[3].load();
  auto p = parameters();
  if (hostBypass)
    p[Bypass] = 1;
  engine->set(p, authorized.load());
  double in = 0, out = 0;
  int scopeHop = std::max(1, int(rate * .01));
  for (int i = 0; i < n; ++i) {
    float l = b.getSample(0, i), r = b.getSample(channels - 1, i);
    if (!std::isfinite(l))
      l = 0;
    if (!std::isfinite(r))
      r = 0;
    in += (l * l + r * r) * .5;
    auto result = engine->process(l, r, neural.get());
    b.setSample(0, i,
                channels == 1 ? (result[0] + result[1]) * .5f : result[0]);
    if (channels == 2)
      b.setSample(1, i, result[1]);
    out += (result[0] * result[0] + result[1] * result[1]) * .5;
    if (engine->updated) {
      int head = noteHead.load();
      auto &c = roll[size_t(head)];
      c.notes[0].store(midi(engine->analysis.hz));
      for (int v = 0; v < 7; ++v)
        c.notes[size_t(v + 1)].store(engine->voiced ? engine->notes[size_t(v)]
                                                    : 0);
      c.strength.store(engine->voiced ? engine->analysis.confidence : 0);
      noteHead.store((head + 1) & 511);
    }
    scopePeak =
        std::max(scopePeak, std::max(std::abs(result[0]), std::abs(result[1])));
    if (++scopeCount >= scopeHop) {
      int h = waveHead.load();
      wave[size_t(h)].store(scopePeak);
      waveHead.store((h + 1) & 511);
      scopeCount = 0;
      scopePeak = 0;
    }
  }
  auto &a = engine->analysis;
  detectedHz.store(a.hz);
  confidence.store(a.confidence);
  inputNote.store(midi(a.hz));
  targetNote.store(engine->target);
  correctedNote.store(engine->notes[0]);
  cents.store(engine->correction);
  currentRoot.store(engine->scale.root);
  currentScale.store(engine->scale.kind);
  for (int v = 0; v < 6; ++v)
    voiceEnergy[size_t(v)].store(engine->wet[size_t(v)]);
  inputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(in / std::max(1, n))), -100.f));
  outputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(out / std::max(1, n))), -100.f));
}
bool TuneProcessor::isFavourite(int i) const {
  return favourites.contains(juce::String(i));
}
void TuneProcessor::toggleFavourite(int i) {
  auto s = juce::String(i);
  if (favourites.contains(s))
    favourites.removeString(s);
  else
    favourites.add(s);
  auto f =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("M7G/Tune-V1-favourites.txt");
  f.getParentDirectory().createDirectory();
  f.replaceWithText(favourites.joinIntoString("\n"));
}
void TuneProcessor::copyCompare() {
  const juce::ScopedLock lock(compareLock);
  int slot = compareIndex.load();
  compare[size_t(1 - slot)] = state.copyState();
  compareProgram[size_t(1 - slot)] = program.load();
}
void TuneProcessor::selectCompare(int i) {
  i = juce::jlimit(0, 1, i);
  const juce::ScopedLock lock(compareLock);
  int previous = compareIndex.load();
  if (previous == i)
    return;
  compare[size_t(previous)] = state.copyState();
  compareProgram[size_t(previous)] = program.load();
  state.replaceState(compare[size_t(i)].createCopy());
  program.store(compareProgram[size_t(i)]);
  browserBank.store(program.load() / 25);
  compareIndex.store(i);
}
void TuneProcessor::getStateInformation(juce::MemoryBlock &dest) {
  auto s = state.copyState();
  s.setProperty("program", program.load(), nullptr);
  s.setProperty("creator", "Makkine Seven Golden", nullptr);
  s.setProperty("compareIndex", compareIndex.load(), nullptr);
  for (int i = 0; i < 4; ++i)
    s.setProperty("bank" + juce::String(i), bankPrograms[size_t(i)].load(),
                  nullptr);
  {
    const juce::ScopedLock lock(compareLock);
    for (int i = 0; i < 2; ++i) {
      auto c = (i == compareIndex.load() ? state.copyState()
                                         : compare[size_t(i)].createCopy());
      c.setProperty("program",
                    i == compareIndex.load() ? program.load()
                                             : compareProgram[size_t(i)],
                    nullptr);
      auto item = juce::ValueTree(i == 0 ? "COMPARE_A" : "COMPARE_B");
      item.addChild(c, -1, nullptr);
      s.addChild(item, -1, nullptr);
    }
  }
  if (auto xml = s.createXml())
    copyXmlToBinary(*xml, dest);
}
void TuneProcessor::setStateInformation(const void *d, int size) {
  auto x = getXmlFromBinary(d, size);
  if (!x || !x->hasTagName("M7G_TUNE"))
    return;
  auto s = juce::ValueTree::fromXml(*x);
  int pr = juce::jlimit(0, 99, int(s.getProperty("program", 0)));
  for (int i = 0; i < 4; ++i)
    bankPrograms[size_t(i)].store(
        juce::jlimit(i * 25, i * 25 + 24,
                     int(s.getProperty("bank" + juce::String(i), i * 25))));
  {
    const juce::ScopedLock lock(compareLock);
    for (int i = 0; i < 2; ++i) {
      auto c = s.getChildWithName(i == 0 ? "COMPARE_A" : "COMPARE_B");
      compare[size_t(i)] =
          c.getNumChildren() ? c.getChild(0).createCopy() : s.createCopy();
      compareProgram[size_t(i)] = juce::jlimit(
          0, 99, int(compare[size_t(i)].getProperty("program", pr)));
    }
    compareIndex.store(
        juce::jlimit(0, 1, int(s.getProperty("compareIndex", 0))));
  }
  s.removeChild(s.getChildWithName("COMPARE_A"), nullptr);
  s.removeChild(s.getChildWithName("COMPARE_B"), nullptr);
  state.replaceState(s);
  program.store(pr);
  browserBank.store(pr / 25);
  resetRequested.store(true);
}
juce::String TuneProcessor::profileText() const {
  if (detectedHz.load() < 50)
    return "ESPERANDO VOZ";
  if (!neural->ready.load())
    return "REGISTRO ESTIMADO / YIN";
  auto low = neural->acoustic[0].load(), high = neural->acoustic[1].load();
  juce::String s = low > .6f    ? "REGISTRO GRAVE"
                   : high > .6f ? "REGISTRO AGUDO"
                                : "REGISTRO MEDIO";
  s += neural->acoustic[2].load() > .5f ? " / CON AIRE" : " / CLARO";
  if (neural->acoustic[3].load() > .5f)
    s += " / ASPERO";
  return s;
}
juce::String TuneProcessor::keyText() const {
  const char *names[]{"C",  "C#", "D",  "D#", "E",  "F",
                      "F#", "G",  "G#", "A",  "A#", "B"};
  int kind = currentScale.load();
  if (kind == 0)
    return value("autoKey") > .5f ? "TONO: ESCUCHANDO / CROMATICA"
                                  : "CROMATICA";
  return "TONO: " +
         juce::String(names[juce::jlimit(0, 11, currentRoot.load())]) +
         (kind == 1   ? " MAYOR"
          : kind == 2 ? " MENOR"
                      : " MENOR ARMONICA") +
         (value("autoKey") > .5f ? " (ESTIMADO)" : "");
}
juce::AudioProcessorEditor *TuneProcessor::createEditor() {
  return new TuneEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new TuneProcessor();
}
