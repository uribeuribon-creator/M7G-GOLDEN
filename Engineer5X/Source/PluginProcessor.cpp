#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout
EngineerProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout p;
  p.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"strength", 1}, "Cantidad de correccion",
      juce::NormalisableRange<float>(0.f, 1.f), 1.f,
      juce::AudioParameterFloatAttributes()
          .withStringFromValueFunction(
              [](float v, int) { return juce::String(v * 100, 0) + "%"; })
          .withValueFromStringFunction([](const juce::String &text) {
            return text.getFloatValue() / 100;
          })));
  p.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"scale", 1}, "Escala",
      juce::StringArray{"Cromatica", "Mayor", "Menor"}, 0));
  p.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"root", 1}, "Tonica",
      juce::StringArray{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A",
                        "A#", "B"},
      0));
  p.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"volume", 1}, "Volumen de escucha dB", -24.f, 6.f,
      -3.f));
  return p;
}
EngineerProcessor::EngineerProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Vocal", juce::AudioChannelSet::stereo(), true)
              .withOutput("Escucha", juce::AudioChannelSet::stereo(), true)),
      Thread("Makkine five-version engineer"),
      state(*this, nullptr, "MAKKINE_ENGINEER_5X", layout()) {
  license.reload();
  authorized = license.authorized;
  for (auto id : {"strength", "scale", "root"})
    state.addParameterListener(id, this);
  startThread(juce::Thread::Priority::low);
  startTimerHz(5);
}
EngineerProcessor::~EngineerProcessor() {
  stopTimer();
  for (auto id : {"strength", "scale", "root"})
    state.removeParameterListener(id, this);
  signalThreadShouldExit();
  generation.fetch_add(1);
  notify();
  waitForThreadToExit(-1);
  current.store(nullptr);
}
bool EngineerProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
  return (in == juce::AudioChannelSet::mono() ||
          in == juce::AudioChannelSet::stereo()) &&
         (out == in || (in == juce::AudioChannelSet::mono() &&
                        out == juce::AudioChannelSet::stereo()));
}
void EngineerProcessor::prepareToPlay(double sr, int) {
  hostRate = sr;
  cursor = 0;
  fade = 0;
  audioProject = nullptr;
  appliedSeek = 0;
  setLatencySamples(0);
}
engineer::Settings EngineerProcessor::settings() const {
  return {state.getRawParameterValue("strength")->load(),
          int(state.getRawParameterValue("scale")->load()),
          int(state.getRawParameterValue("root")->load())};
}
void EngineerProcessor::loadFile(const juce::File &f) {
  playing.store(false);
  original.store(false);
  seek(0);
  auto s = settings();
  {
    juce::ScopedLock guard(lock);
    generation.fetch_add(1);
    pendingFile = f;
    lastFile = f;
    pendingSettings = s;
    request = true;
    message = "Preparando vocal...";
    busy.store(true);
    progress.store(0);
  }
  parameterDirty.store(false);
  notify();
}
void EngineerProcessor::rebuild() {
  juce::File f;
  {
    juce::ScopedLock guard(lock);
    f = lastFile;
  }
  if (f != juce::File())
    loadFile(f);
}
void EngineerProcessor::cancelRender() {
  {
    juce::ScopedLock guard(lock);
    generation.fetch_add(1);
    request = false;
    message = "Analisis cancelado. La version anterior sigue disponible.";
    busy.store(false);
    progress.store(0);
  }
}
void EngineerProcessor::parameterChanged(const juce::String &, float) {
  parameterTime.store(juce::Time::getMillisecondCounter());
  parameterDirty.store(true);
}
void EngineerProcessor::timerCallback() {
  if (parameterDirty.load() &&
      juce::Time::getMillisecondCounter() - parameterTime.load() > 600) {
    parameterDirty.store(false);
    rebuild();
  }
}
juce::String EngineerProcessor::statusText() const {
  juce::ScopedLock guard(lock);
  return message;
}
juce::String EngineerProcessor::sourcePath() const {
  juce::ScopedLock guard(lock);
  return lastFile.getFullPathName();
}
std::shared_ptr<const engineer::Project> EngineerProcessor::snapshot() const {
  juce::ScopedLock guard(lock);
  return owner;
}
void EngineerProcessor::publish(std::shared_ptr<engineer::Project> p) {
  juce::ScopedLock guard(lock);
  if (owner)
    retired.push_back(owner);
  owner = std::move(p);
  publishedSerial.fetch_add(1);
  current.store(owner.get());
  retired.erase(std::remove_if(retired.begin(), retired.end(),
                               [&](const auto &old) {
                                 return old.get() != hazard.load();
                               }),
                retired.end());
}
void EngineerProcessor::run() {
  while (!threadShouldExit()) {
    juce::File file;
    engineer::Settings s;
    uint64_t token;
    {
      juce::ScopedLock guard(lock);
      if (!request) {
        file = juce::File();
        token = 0;
      } else {
        file = pendingFile;
        s = pendingSettings;
        token = generation.load();
        request = false;
      }
    }
    if (file == juce::File()) {
      wait(100);
      continue;
    }
    auto folder =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("M7G/Engineer5X/Exports")
            .getChildFile(juce::Uuid().toString());
    auto cancelled = [&] {
      return threadShouldExit() || generation.load() != token;
    };
    try {
      auto p =
          engineer::buildProject(file, s, folder, cancelled,
                                 [&](float value, const juce::String &text) {
                                   if (!cancelled()) {
                                     juce::ScopedLock guard(lock);
                                     if (!cancelled()) {
                                       progress.store(value);
                                       message = text;
                                     }
                                   }
                                 });
      if (cancelled()) {
        folder.deleteRecursively();
        continue;
      }
      bool accepted = false;
      {
        juce::ScopedLock guard(lock);
        if (!cancelled()) {
          publish(std::move(p));
          seek(0);
          busy.store(false);
          accepted = true;
        }
      }
      if (!accepted)
        folder.deleteRecursively();
    } catch (const std::exception &e) {
      folder.deleteRecursively();
      juce::ScopedLock guard(lock);
      if (!cancelled()) {
        message = "No se completo: " + juce::String::fromUTF8(e.what());
        busy.store(false);
        progress.store(0);
      }
    }
  }
}
void EngineerProcessor::play(int v) {
  selected.store(juce::jlimit(0, 4, v));
  original.store(false);
  solo.store(v);
  auto p = snapshot();
  if (p && playhead.load() >= p->source.getNumSamples() / p->rate)
    seek(0);
  playing.store(true);
}
void EngineerProcessor::stop() { playing.store(false); }
void EngineerProcessor::seek(double seconds) {
  requestedSeek.store(std::max(0., seconds));
  seekSerial.fetch_add(1);
  playhead.store(std::max(0., seconds));
}
void EngineerProcessor::setSolo(int v) {
  solo.store(solo.load() == v ? -1 : v);
  selected.store(v);
}
void EngineerProcessor::setMute(int v, bool on) { mute[size_t(v)].store(on); }
namespace {
float cubic(const juce::AudioBuffer<float> &b, int channel, double position) {
  int i = int(position);
  float f = float(position - i);
  auto sample = [&](int at) {
    return b.getSample(channel, juce::jlimit(0, b.getNumSamples() - 1, at));
  };
  float a = sample(i - 1), c = sample(i), d = sample(i + 1), e = sample(i + 2);
  return a * (-f * (f - 1) * (f - 2) / 6) +
         c * ((f + 1) * (f - 1) * (f - 2) / 2) +
         d * (-(f + 1) * f * (f - 2) / 2) + e * ((f + 1) * f * (f - 1) / 6);
}
} // namespace
void EngineerProcessor::processBlock(juce::AudioBuffer<float> &b,
                                     juce::MidiBuffer &midi) {
  juce::ScopedNoDenormals noDenormals;
  midi.clear();
  const engineer::Project *p;
  do {
    p = current.load();
    hazard.store(p);
  } while (p != current.load());
  auto publication = publishedSerial.load();
  if (p != audioProject || publication != audioSerial) {
    audioSerial = publication;
    audioProject = p;
    cursor = 0;
    fade = 0;
    modeFade = 1;
    audioVersion = selected.load();
    audioRaw = original.load();
  }
  auto serial = seekSerial.load();
  if (serial != appliedSeek) {
    appliedSeek = serial;
    cursor = requestedSeek.load() * (p ? p->rate : hostRate);
    fade = 0;
  }
  int choice = solo.load();
  if (choice < 0)
    choice = selected.load();
  choice = juce::jlimit(0, 4, choice);
  bool raw = original.load();
  if (choice != audioVersion || raw != audioRaw) {
    oldVersion = audioVersion;
    oldRaw = audioRaw;
    audioVersion = choice;
    audioRaw = raw;
    modeFade = 0;
  }
  float gain = juce::Decibels::decibelsToGain(
      state.getRawParameterValue("volume")->load());
  float peak = 0, coefficient = float(1 - std::exp(-1 / (hostRate * .004)));
  if (p && (playing.load() || fade > .00001f) && authorized) {
    const auto &source = raw ? p->source : p->versions[size_t(choice)];
    const auto &previous =
        oldRaw ? p->source
               : p->versions[size_t(juce::jlimit(0, 4, oldVersion))];
    double increment = p->rate / hostRate;
    auto sampleForOutput = [&](const juce::AudioBuffer<float> &data, int c) {
      if (b.getNumChannels() == 1 && data.getNumChannels() == 2)
        return .5f * (cubic(data, 0, cursor) + cubic(data, 1, cursor));
      return cubic(data, std::min(c, data.getNumChannels() - 1), cursor);
    };
    for (int i = 0; i < b.getNumSamples(); ++i) {
      bool end = cursor >= source.getNumSamples();
      if (end)
        playing.store(false);
      float target = playing.load() && !end ? 1.f : 0.f;
      fade += (target - fade) * coefficient;
      float muteTarget = (!raw && mute[size_t(choice)].load()) ? 0.f : 1.f;
      muteFade += (muteTarget - muteFade) * coefficient;
      modeFade = std::min(1.f, modeFade + float(1 / (hostRate * .008)));
      for (int c = 0; c < b.getNumChannels(); ++c) {
        float dry = c < getTotalNumInputChannels() ? b.getSample(c, i) : 0;
        if (!std::isfinite(dry))
          dry = 0;
        float wet = end ? 0 : sampleForOutput(source, c);
        if (!end && modeFade < 1)
          wet = sampleForOutput(previous, c) * (1 - modeFade) + wet * modeFade;
        float x = dry * (1 - fade) + wet * gain * fade * muteFade;
        b.setSample(c, i, x);
        peak = std::max(peak, std::abs(x));
      }
      if (!end && (playing.load() || fade > .00001f))
        cursor += increment;
    }
    playhead.store(
        std::min(cursor / p->rate, p->source.getNumSamples() / p->rate));
  } else {
    for (int c = getTotalNumInputChannels(); c < b.getNumChannels(); ++c)
      b.clear(c, 0, b.getNumSamples());
    for (int c = 0; c < b.getNumChannels(); ++c)
      for (int i = 0; i < b.getNumSamples(); ++i) {
        float x = b.getSample(c, i);
        if (!std::isfinite(x)) {
          x = 0;
          b.setSample(c, i, 0);
        }
        peak = std::max(peak, std::abs(x));
      }
    fade = 0;
  }
  meter.store(std::max(peak, meter.load() * .85f));
  hazard.store(nullptr);
}
void EngineerProcessor::getStateInformation(juce::MemoryBlock &dest) {
  auto copy = state.copyState();
  copy.setProperty("source", sourcePath(), nullptr);
  copy.setProperty("selected", selected.load(), nullptr);
  for (int i = 0; i < 5; ++i)
    copy.setProperty("mute" + juce::String(i), isMuted(i), nullptr);
  if (auto xml = copy.createXml())
    copyXmlToBinary(*xml, dest);
}
void EngineerProcessor::setStateInformation(const void *data, int bytes) {
  if (auto xml = getXmlFromBinary(data, bytes)) {
    auto copy = juce::ValueTree::fromXml(*xml);
    if (!copy.hasType("MAKKINE_ENGINEER_5X"))
      return;
    state.replaceState(copy);
    selected.store(juce::jlimit(0, 4, int(copy.getProperty("selected", 0))));
    for (int i = 0; i < 5; ++i)
      mute[size_t(i)].store(
          bool(copy.getProperty("mute" + juce::String(i), false)));
    auto file = copy.getProperty("source").toString();
    if (file.isNotEmpty())
      loadFile(juce::File(file));
  }
}
juce::AudioProcessorEditor *EngineerProcessor::createEditor() {
  return new EngineerEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new EngineerProcessor();
}
