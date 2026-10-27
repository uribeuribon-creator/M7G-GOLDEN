#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout
TransitionsProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout p;
  for (const auto &s : std::array<std::pair<const char *, const char *>, 4>{
           {{"amount", "Cantidad"},
            {"time", "Tiempo"},
            {"mix", "Mezcla"},
            {"sensitivity", "Sensibilidad de palabra"}}})
    p.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{s.first, 1}, s.second,
        juce::NormalisableRange<float>(0, 100, .1f),
        juce::String(s.first) == "time"          ? 60.0f
        : juce::String(s.first) == "sensitivity" ? 65.0f
        : juce::String(s.first) == "mix"         ? 60.0f
                                                 : 80.0f));
  p.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"mode", 1}, "Modo",
      juce::StringArray{"INTRO", "CAMBIO", "SALIDA"}, 0));
  p.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"bypass", 1}, "Bypass", false));
  return p;
}
TransitionsProcessor::TransitionsProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Vocal", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "M7G_TRANSITIONS", layout()) {
  const char *ids[]{"amount", "time", "mix", "sensitivity", "mode", "bypass"};
  for (int i = 0; i < 6; ++i)
    raw[size_t(i)] = state.getRawParameterValue(ids[i]);
  for (int i = 0; i < 3; ++i)
    bankPrograms[size_t(i)].store(i * 50);
  for (auto &x : waveform)
    x.store(0);
  for (auto &x : waveColour)
    x.store(0);
  transition::recipes();
  license.reload();
  authorized.store(license.authorized);
  auto file =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("M7G/Transitions-V1-favourites.txt");
  favourites.addLines(file.loadFileAsString());
  compare[0] = parameterState();
  compare[1] = parameterState();
}
void TransitionsProcessor::setValue(const char *id, float v) {
  if (auto *p = state.getParameter(id))
    p->setValueNotifyingHost(p->convertTo0to1(v));
}
void TransitionsProcessor::setBank(int b) {
  b = juce::jlimit(0, 2, b);
  setCurrentProgram(bankPrograms[size_t(b)].load());
}
void TransitionsProcessor::setCurrentProgram(int index) {
  index = juce::jlimit(0, 149, index);
  program.store(index);
  bankPrograms[size_t(index / 50)].store(index);
  setValue("mode", float(index / 50));
}
bool TransitionsProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
  return (in == juce::AudioChannelSet::mono() ||
          in == juce::AudioChannelSet::stereo()) &&
         (in == out || (in == juce::AudioChannelSet::mono() &&
                        out == juce::AudioChannelSet::stereo()));
}
void TransitionsProcessor::prepareToPlay(double sr, int) {
  rate = sr;
  engine.prepare(sr);
  setLatencySamples(engine.latencySamples());
  for (auto *s : {&amountSmooth, &mixSmooth, &bypassSmooth})
    s->reset(sr, .025);
  amountSmooth.setCurrentAndTargetValue(raw[0]->load() * .01f);
  mixSmooth.setCurrentAndTargetValue(raw[2]->load() * .01f);
  bypassSmooth.setCurrentAndTargetValue(raw[5]->load());
  previousBlock = scopeCount = 0;
  hostPositionKnown = wasPlaying = false;
  scopePeak = 0;
  scopeClass = 0;
  resetRequest.store(false);
}
void TransitionsProcessor::processBlock(juce::AudioBuffer<float> &b,
                                        juce::MidiBuffer &) {
  juce::ScopedNoDenormals noDenormals;
  int samples = b.getNumSamples(),
      channels = juce::jmin(2, getTotalNumOutputChannels());
  if (samples == 0 || channels == 0)
    return;
  if (getChannelCountOfBus(true, 0) == 1 && channels == 2)
    b.copyFrom(1, 0, b, 0, 0, samples);
  int selectedBank = juce::jlimit(0, 2, int(raw[4]->load()));
  int index = bankPrograms[size_t(selectedBank)].load();
  program.store(index);
  double bpm = 120, grid = engine.latencySamples(), hostPpq = 0;
  bool hasPpq = false;
  bool seek = false;
  if (auto *playhead = getPlayHead())
    if (auto pos = playhead->getPosition()) {
      if (auto v = pos->getBpm())
        bpm = juce::jlimit(30.0, 300.0, *v);
      if (auto sample = pos->getTimeInSamples()) {
        seek =
            hostPositionKnown && pos->getIsPlaying() &&
            std::abs(double(*sample) - (previousHostSample + previousBlock)) >
                juce::jmax(8, samples * 2);
        previousHostSample = double(*sample);
        previousBlock = samples;
        hostPositionKnown = true;
      }
      if (wasPlaying && !pos->getIsPlaying())
        stopRequest.store(true);
      wasPlaying = pos->getIsPlaying();
      if (auto ppq = pos->getPpqPosition()) {
        hostPpq = *ppq;
        hasPpq = true;
      }
    }
  if (resetRequest.exchange(false) || seek)
    engine.reset();
  if (hasPpq)
    grid = double(engine.time() + 1) - hostPpq * (rate * 60 / bpm) + engine.latencySamples();
  if (stopRequest.exchange(false))
    engine.stopFX();
  amountSmooth.setTargetValue(raw[0]->load() * .01f);
  mixSmooth.setTargetValue(raw[2]->load() * .01f);
  bypassSmooth.setTargetValue(hostBypass ? 1.0f : raw[5]->load());
  bool enabled =
      authorized.load() &&
      std::max(raw[0]->load() * .01f, amountSmooth.getCurrentValue()) >
          .00001f &&
      std::max(raw[2]->load() * .01f, mixSmooth.getCurrentValue()) > .00001f &&
      bypassSmooth.getCurrentValue() < .9999f;
  engine.setControls(index, raw[1]->load(), raw[3]->load(), bpm, grid, enabled);
  tempo.store(float(bpm));
  double inEnergy = 0, outEnergy = 0;
  int scopeHop = juce::jmax(1, int(rate * .01));
  for (int i = 0; i < samples; ++i) {
    float l = b.getSample(0, i), r = b.getSample(channels - 1, i);
    inEnergy += (l * l + r * r) * .5;
    float amount = amountSmooth.getNextValue();
    engine.process(l, r, amount);
    auto output =
        engine.mix(amount, mixSmooth.getNextValue(),
                   authorized.load() ? bypassSmooth.getNextValue() : 1.0f);
    b.setSample(
        0, i, channels == 1 ? (output.left + output.right) * .5f : output.left);
    if (channels == 2)
      b.setSample(1, i, output.right);
    outEnergy += (output.left * output.left + output.right * output.right) * .5;
    scopePeak = juce::jmax(
        scopePeak, juce::jmax(std::abs(output.left), std::abs(output.right)));
    if (output.activity > .01f)
      scopeClass = output.colour;
    if (++scopeCount >= scopeHop) {
      int head = waveHead.load();
      waveform[size_t(head)].store(scopePeak);
      waveColour[size_t(head)].store(scopeClass);
      waveHead.store((head + 1) % 512);
      scopeCount = 0;
      scopePeak = 0;
      scopeClass = 0;
    }
  }
  inputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(inEnergy / samples)), -100.0f));
  outputDB.store(juce::Decibels::gainToDecibels(
      float(std::sqrt(outEnergy / samples)), -100.0f));
  activity.store(engine.idle() ? 0.0f : 1.0f);
  progress.store(engine.progress());
  idle.store(engine.idle());
  activeVoices.store(engine.voices());
  mode.store(engine.mode());
  gap.store(engine.gapMs());
  phraseCount.store(engine.phraseCount());
  overflows.store(engine.dropped());
  lastEventStart.store(engine.lastTrigger());
  const auto &recipe = transition::recipes()[size_t(index)];
  tailSeconds.store(
      recipe.hold ? 32.0 : juce::jmax(4.0, double(recipe.decay) * 1.5 + 3));
}
juce::ValueTree TransitionsProcessor::parameterState() {
  auto tree = state.copyState();
  tree.setProperty("program", program.load(), nullptr);
  tree.setProperty("creator", "Makkine Seven Golden", nullptr);
  for (int i = 0; i < 3; ++i)
    tree.setProperty("bank" + juce::String(i), bankPrograms[size_t(i)].load(),
                     nullptr);
  return tree;
}
void TransitionsProcessor::selectCompare(int side) {
  const juce::ScopedLock lock(compareLock);
  side = juce::jlimit(0, 1, side);
  if (side == compareIndex.load())
    return;
  compare[size_t(compareIndex.load())] = parameterState();
  auto next = compare[size_t(side)].createCopy();
  state.replaceState(next);
  program.store(int(next.getProperty("program", 0)));
  for (int i = 0; i < 3; ++i)
    bankPrograms[size_t(i)].store(
        juce::jlimit(i * 50, i * 50 + 49,
                     int(next.getProperty("bank" + juce::String(i), i * 50))));
  compareIndex.store(side);
  stopRequest.store(true);
}
void TransitionsProcessor::copyCompare() {
  const juce::ScopedLock lock(compareLock);
  compare[size_t(1 - compareIndex.load())] = parameterState();
}
void TransitionsProcessor::getStateInformation(juce::MemoryBlock &data) {
  const juce::ScopedLock lock(compareLock);
  auto tree = parameterState();
  compare[size_t(compareIndex.load())] = tree.createCopy();
  tree.setProperty("compare", compareIndex.load(), nullptr);
  for (int i = 0; i < 2; ++i) {
    juce::ValueTree child(i == 0 ? "COMPARE_A" : "COMPARE_B");
    child.addChild(compare[size_t(i)].createCopy(), -1, nullptr);
    tree.addChild(child, -1, nullptr);
  }
  copyXmlToBinary(*tree.createXml(), data);
}
void TransitionsProcessor::setStateInformation(const void *data, int size) {
  const juce::ScopedLock lock(compareLock);
  auto xml = getXmlFromBinary(data, size);
  if (!xml || !xml->hasTagName("M7G_TRANSITIONS"))
    return;
  auto tree = juce::ValueTree::fromXml(*xml);
  for (int i = 0; i < 2; ++i) {
    auto child = tree.getChildWithName(i == 0 ? "COMPARE_A" : "COMPARE_B");
    if (child.getNumChildren() > 0)
      compare[size_t(i)] = child.getChild(0).createCopy();
    if (child.isValid())
      tree.removeChild(child, nullptr);
  }
  state.replaceState(tree);
  program.store(juce::jlimit(0, 149, int(tree.getProperty("program", 0))));
  compareIndex.store(juce::jlimit(0, 1, int(tree.getProperty("compare", 0))));
  for (int i = 0; i < 3; ++i)
    bankPrograms[size_t(i)].store(
        juce::jlimit(i * 50, i * 50 + 49,
                     int(tree.getProperty("bank" + juce::String(i), i * 50))));
  resetRequest.store(true);
}
bool TransitionsProcessor::isFavourite(int i) const {
  const juce::ScopedLock lock(favouriteLock);
  return favourites.contains(
      transition::recipes()[size_t(juce::jlimit(0, 149, i))].name);
}
void TransitionsProcessor::toggleFavourite(int i) {
  const juce::ScopedLock lock(favouriteLock);
  auto name =
      juce::String(transition::recipes()[size_t(juce::jlimit(0, 149, i))].name);
  if (favourites.contains(name))
    favourites.removeString(name);
  else
    favourites.add(name);
  auto file =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("M7G/Transitions-V1-favourites.txt");
  file.getParentDirectory().createDirectory();
  file.replaceWithText(favourites.joinIntoString("\n"));
}
juce::String TransitionsProcessor::explanation() const {
  if (!authorized.load())
    return "Importa tu M7G.license para activar todos los efectos";
  if (overflows.load() > 0 && activeVoices.load() >= 8)
    return "Cola de efectos llena: reduce TIEMPO si disparas muchas frases";
  if (activeVoices.load() > 0) {
    const char *messages[]{"Preparando una entrada antes de la voz",
                           "Rellenando la pausa entre frases",
                           "Transformando el final de la voz"};
    return juce::String(messages[juce::jlimit(0, 2, mode.load())]) + " / " +
           juce::String(activeVoices.load()) + " capas";
  }
  return "Escuchando tu voz / anticipacion 2000 ms / BPM " +
         juce::String(tempo.load(), 1);
}
juce::AudioProcessorEditor *TransitionsProcessor::createEditor() {
  return new TransitionsEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new TransitionsProcessor();
}
