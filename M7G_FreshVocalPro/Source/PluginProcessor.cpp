#include "PluginProcessor.h"
// APVTS mantiene automatizacion del host, undo y presets; el audio lee atomicos
// sin copiar ValueTree.
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout FreshProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout result;
  for (int i = 0; i < fresh::count; ++i) {
    auto &s = fresh::specs[size_t(i)];
    juce::ParameterID id{s.id, 1};
    if (s.kind == 2)
      result.add(std::make_unique<juce::AudioParameterBool>(id, s.name,
                                                            s.initial > .5));
    else if (s.kind == 1)
      result.add(std::make_unique<juce::AudioParameterChoice>(
          id, s.name, fresh::choices(fresh::Param(i)), int(s.initial)));
    else {
      juce::NormalisableRange<float> range(s.low, s.high, .01f);
      if (i == fresh::focus)
        range.setSkewForCentre(5000);
      result.add(std::make_unique<juce::AudioParameterFloat>(id, s.name, range,
                                                             s.initial));
    }
  }
  return result;
}
FreshProcessor::FreshProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Vocal", juce::AudioChannelSet::stereo(), true)
              .withOutput("Fresh Vocal", juce::AudioChannelSet::stereo(),
                          true)),
      undo(1000000), state(*this, &undo, "FRESH_VOCAL_BEAST", layout()) {
  for (int i = 0; i < fresh::count; ++i) {
    raw[size_t(i)] = state.getRawParameterValue(fresh::specs[size_t(i)].id);
    state.addParameterListener(fresh::specs[size_t(i)].id, this);
  }
  a = b = settings();
  juce::ignoreUnused(fresh::orders());
  license.reload();
  authorized = license.authorized;
  startTimerHz(50);
}
FreshProcessor::~FreshProcessor() {
  stopTimer();
  for (auto &s : fresh::specs)
    state.removeParameterListener(s.id, this);
}
bool FreshProcessor::isBusesLayoutSupported(const BusesLayout &l) const {
  auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
  return (in == juce::AudioChannelSet::mono() ||
          in == juce::AudioChannelSet::stereo()) &&
         in == out;
}
void FreshProcessor::prepareToPlay(double sr, int block) {
  rate = sr;
  maximum = std::max(32, block);
  chain.prepare(sr, getTotalNumOutputChannels(), maximum);
  setLatencySamples(chain.latency());
}
fresh::Values FreshProcessor::settings() const {
  fresh::Values values{};
  for (int i = 0; i < fresh::count; ++i)
    values[size_t(i)] = raw[size_t(i)]->load();
  return values;
}
void FreshProcessor::set(fresh::Param p, float value) {
  auto *parameter = state.getParameter(fresh::specs[size_t(p)].id);
  parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
void FreshProcessor::parameterChanged(const juce::String &, float) {
  if (morphing.load() && !morphWriting.load())
    morphing.store(false);
}
void FreshProcessor::process(juce::AudioBuffer<float> &buffer,
                             juce::MidiBuffer &midi, bool hostBypass) {
  juce::ScopedNoDenormals suppress;
  midi.clear();
  juce::AudioPlayHead::PositionInfo pos;
  if (auto *head = getPlayHead())
    if (auto p = head->getPosition())
      pos = *p;
  chain.process(buffer, settings(), pos.getBpm().orFallback(120.), hostBypass);
}
void FreshProcessor::processBlock(juce::AudioBuffer<float> &b,
                                  juce::MidiBuffer &m) {
  process(b, m, false);
}
void FreshProcessor::processBlockBypassed(juce::AudioBuffer<float> &b,
                                          juce::MidiBuffer &m) {
  process(b, m, true);
}
void FreshProcessor::apply(const fresh::Values &values, juce::String name,
                           bool morph) {
  morphing.store(false);
  undo.beginNewTransaction("Preset " + name);
  morphFrom = settings();
  morphTo = fresh::sanitise(values);
  {
    juce::ScopedLock guard(nameLock);
    currentName = std::move(name);
  }
  if (morph) {
    morphStart = juce::Time::getMillisecondCounterHiRes();
    for (int i = 0; i < fresh::count; ++i)
      if (fresh::specs[size_t(i)].kind)
        set(fresh::Param(i), morphTo[size_t(i)]);
    morphing.store(true);
  } else
    for (int i = 0; i < fresh::count; ++i)
      set(fresh::Param(i), morphTo[size_t(i)]);
}
void FreshProcessor::choosePreset(int index, bool morph) {
  auto &presets = fresh::PresetManager::factory();
  index = juce::jlimit(0, int(presets.size()) - 1, index);
  selectedPreset.store(index);
  apply(presets[size_t(index)].values, presets[size_t(index)].name, morph);
}
void FreshProcessor::setCurrentProgram(int index) {
  choosePreset(index, false);
}
juce::String FreshProcessor::presetName() const {
  juce::ScopedLock guard(const_cast<juce::CriticalSection &>(nameLock));
  return currentName;
}
void FreshProcessor::switchAB() {
  morphing.store(false);
  if (bankB.load()) {
    b = settings();
    apply(a, "A", false);
    bankB.store(false);
  } else {
    a = settings();
    apply(b, "B", false);
    bankB.store(true);
  }
}
bool FreshProcessor::loadPreset(const juce::File &file) {
  auto xml = juce::XmlDocument::parse(file);
  if (!xml)
    return false;
  fresh::Values v;
  juce::String name;
  if (!fresh::PresetManager::fromTree(juce::ValueTree::fromXml(*xml), v, name))
    return false;
  apply(v, name, true);
  return true;
}
bool FreshProcessor::savePreset(const juce::File &file) {
  auto tree = fresh::PresetManager::toTree(settings(), presetName());
  if (auto xml = tree.createXml())
    return file.replaceWithText(xml->toString());
  return false;
}
void FreshProcessor::setEQStyle(int style) {
  morphing.store(false);
  undo.beginNewTransaction("EQ Style");
  set(fresh::eqStyle, float(style));
  for (auto p : {fresh::eqLow, fresh::eqLowMid, fresh::eqHighMid, fresh::eqHigh,
                 fresh::eq160, fresh::eq350, fresh::eq1500, fresh::eq6000})
    set(p, 0);
  if (style == 0)
    set(fresh::eqLow, 2);
  if (style == 2)
    set(fresh::eqHighMid, 2);
  if (style == 3)
    set(fresh::eqHigh, 6);
}
void FreshProcessor::reorder(int from, int to) {
  auto sequence = fresh::orders()[size_t(
      juce::jlimit(0, 23, int(settings()[fresh::order])))];
  auto begin = std::find(sequence.begin(), sequence.end(), from),
       end = std::find(sequence.begin(), sequence.end(), to);
  if (begin == sequence.end() || end == sequence.end())
    return;
  int value = *begin;
  int aIndex = int(begin - sequence.begin()),
      bIndex = int(end - sequence.begin());
  if (aIndex < bIndex)
    std::rotate(begin, begin + 1, end + 1);
  else if (aIndex > bIndex)
    std::rotate(end, begin, begin + 1);
  (void)value;
  auto &all = fresh::orders();
  int i = int(std::find(all.begin(), all.end(), sequence) - all.begin());
  undo.beginNewTransaction("Module order");
  set(fresh::order, float(i));
}

fresh::Meters &FreshProcessor::meters() { return chain.meters; }

void FreshProcessor::timerCallback() {
  if (morphing.load()) {
    morphWriting.store(true);
    double t = juce::jlimit(
        0., 1., (juce::Time::getMillisecondCounterHiRes() - morphStart) / 300.);
    double ease = t * t * (3 - 2 * t);
    for (int i = 0; i < fresh::count; ++i)
      if (!fresh::specs[size_t(i)].kind)
        set(fresh::Param(i),
            float(morphFrom[size_t(i)] +
                  (morphTo[size_t(i)] - morphFrom[size_t(i)]) * ease));
    if (t >= 1)
      morphing.store(false);
    morphWriting.store(false);
  }
}
void FreshProcessor::getStateInformation(juce::MemoryBlock &out) {
  auto copy = state.copyState();
  copy.setProperty("name", presetName(), nullptr);
  copy.setProperty("view", viewMode.load(), nullptr);
  copy.setProperty("tab", selectedTab.load(), nullptr);
  if (auto xml = copy.createXml())
    copyXmlToBinary(*xml, out);
}
void FreshProcessor::setStateInformation(const void *data, int size) {
  if (auto xml = getXmlFromBinary(data, size)) {
    auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.hasType("FRESH_VOCAL_BEAST"))
      return;
    morphing.store(false);
    state.replaceState(tree);
    viewMode.store(juce::jlimit(0, 2, int(tree.getProperty("view", 0))));
    selectedTab.store(juce::jlimit(0, 3, int(tree.getProperty("tab", 0))));
    {
      juce::ScopedLock guard(nameLock);
      currentName = tree.getProperty("name", "Default").toString();
    }
    a = b = settings();
    undo.clearUndoHistory();
  }
}
juce::AudioProcessorEditor *FreshProcessor::createEditor() {
  return new FreshEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new FreshProcessor();
}
