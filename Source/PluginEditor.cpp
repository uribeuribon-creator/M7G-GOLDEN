#include "PluginEditor.h"
#include "AlienFonts.h"
juce::Typeface::Ptr AlienSkin::bodyTypeface() {
  static auto face = juce::Typeface::createSystemTypefaceFor(
      AlienFonts::DejaVuSans_ttf, AlienFonts::DejaVuSans_ttfSize);
  return face;
}
juce::Typeface::Ptr AlienSkin::titleTypeface() {
  static auto face = juce::Typeface::createSystemTypefaceFor(
      AlienFonts::LiberationSerifRegular_ttf,
      AlienFonts::LiberationSerifRegular_ttfSize);
  return face;
}
juce::Typeface::Ptr AlienSkin::graffitiTypeface() {
  static auto face = juce::Typeface::createSystemTypefaceFor(
      AlienFonts::RubikWetPaintRegular_ttf,
      AlienFonts::RubikWetPaintRegular_ttfSize);
  return face;
}
namespace {
const juce::Colour gold{0xffd5b66d}, bright{0xfff1dca5}, dark{0xff0c1011},
    muted{0xff9b926f};
constexpr const char *titles[]{"GATE",
                               "DE-ESS AI",
                               "UNISON COLOUR",
                               "PRESENCE EQ",
                               "AIR",
                               "OPTO COMPRESS",
                               "LIMITER",
                               "TRIPLE VERB",
                               "DELAY",
                               "STEREO",
                               "MODULATION",
                               "PITCH / THROAT",
                               "DE-REVERB",
                               "RESONANCE GUARD",
                               "MULTIBAND COLOUR",
                               "HARMONY GOD",
                               "FET COMPRESS",
                               "NEURAL CLEAN",
                               "UNISON DRIVE",
                               "PARALLEL LAB",
                               "VERB A / CHAMBER",
                               "VERB B / HALL",
                               "VERB C / CATHEDRAL",
                               "SMART DELAY",
                               "EQ SCULPTOR",
                               "VOCAL ALIGN"};
constexpr rack::Param enables[]{
    rack::GateOn,     rack::DeessOn,     rack::WarmOn,       rack::EQOn,
    rack::AirOn,      rack::CompOn,      rack::LimitOn,      rack::ReverbOn,
    rack::DelayOn,    rack::StereoOn,    rack::ModOn,        rack::TuneOn,
    rack::DereverbOn, rack::ResonanceOn, rack::SaturationOn, rack::HarmonyOn,
    rack::FetOn,      rack::NoiseOn,     rack::WarmOn,       rack::CompOn,
    rack::ReverbOn,   rack::ReverbOn,    rack::ReverbOn,     rack::DelayOn,
    rack::SculptorOn, rack::AlignOn};
constexpr rack::Param parallels[]{
    rack::GateParallel,     rack::DeessParallel,
    rack::WarmParallel,     rack::EQParallel,
    rack::AirParallel,      rack::CompParallel,
    rack::LimitParallel,    rack::ReverbMix,
    rack::DelayMix,         rack::StereoParallel,
    rack::ModMix,           rack::PitchParallel,
    rack::RoomParallel,     rack::ResonanceParallel,
    rack::SatParallel,      rack::HarmonyMix,
    rack::FetParallel,      rack::NoiseAmount,
    rack::WarmParallel,     rack::CompParallel,
    rack::VerbAMix,         rack::VerbBMix,
    rack::VerbCMix,         rack::DelayMix,
    rack::SculptorParallel, rack::AlignParallel};
const juce::Colour cyan{0xff6de3d5}, violet{0xff9c76e7};
void alien(juce::Graphics &g, juce::Rectangle<float> r, juce::Colour colour) {
  juce::Graphics::ScopedSaveState save(g);
  g.addTransform(
      juce::AffineTransform::scale(r.getWidth() / 40, r.getHeight() / 48)
          .translated(r.getX(), r.getY()));
  juce::Path p;
  p.startNewSubPath(20, 47);
  p.cubicTo(2, 33, -5, 12, 8, 4);
  p.cubicTo(15, -1, 25, -1, 32, 4);
  p.cubicTo(45, 12, 38, 33, 20, 47);
  g.setColour(colour.withAlpha(.12f));
  g.fillPath(p);
  g.setColour(colour);
  g.strokePath(p, juce::PathStrokeType(1.4f));
  juce::Path eyes;
  eyes.addEllipse(6, 16, 12, 6);
  eyes.addEllipse(22, 16, 12, 6);
  g.fillPath(eyes);
  g.drawLine(17, 36, 23, 36, 1.4f);
}
void text(juce::Graphics &g, const juce::String &s, juce::Rectangle<int> bounds,
          float size, juce::Colour colour = gold,
          int alignment = juce::Justification::centred) {
  g.setColour(colour);
  g.setFont(juce::Font(
      juce::FontOptions(AlienSkin::bodyTypeface()).withHeight(size)));
  g.drawFittedText(s, bounds, alignment, 1);
}
void vu(juce::Graphics &g, juce::Rectangle<float> r, float db,
        bool reduction = false) {
  g.setColour(juce::Colour(0xff050708));
  g.fillRoundedRectangle(r.expanded(3), 4);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe8cb8e), r.getX(),
                                         r.getY(), juce::Colour(0xffa97d3b),
                                         r.getX(), r.getBottom(), false));
  g.fillRoundedRectangle(r, 3);
  g.setColour(juce::Colour(0xff704615));
  g.drawRoundedRectangle(r, 3, 1);
  auto centre = juce::Point<float>(r.getCentreX(), r.getBottom() + 9);
  float radius = r.getHeight() * .88f;
  float start = -juce::MathConstants<float>::pi * .34f, end = -start;
  for (int i = 0; i <= 10; ++i) {
    float a = start + (end - start) * float(i) / 10;
    float len = i % 2 ? 4.0f : 7.0f;
    auto p = centre +
             juce::Point<float>(std::sin(a) * radius, -std::cos(a) * radius);
    auto q = centre + juce::Point<float>(std::sin(a) * (radius - len),
                                         -std::cos(a) * (radius - len));
    g.setColour(i > 7 ? juce::Colour(0xff983a25) : juce::Colour(0xff332b1a));
    g.drawLine({p, q}, i % 2 ? 1 : 1.5f);
  }
  float norm = reduction ? juce::jlimit(0.0f, 1.0f, db / 18)
                         : juce::jlimit(0.0f, 1.0f, (db + 42) / 30);
  float angle = start + (end - start) * norm;
  auto tip = centre + juce::Point<float>(std::sin(angle) * (radius - 3),
                                         -std::cos(angle) * (radius - 3));
  g.setColour(juce::Colour(0xff3a1d16));
  {
    juce::Graphics::ScopedSaveState needleClip(g);
    g.reduceClipRegion(r.toNearestInt());
    g.drawLine({centre, tip}, 1.5f);
  }
  text(g, reduction ? "GAIN REDUCTION" : "VU",
       r.withHeight(r.getHeight() * .28f)
           .translated(0, r.getHeight() * .46f)
           .toNearestInt(),
       10, juce::Colour(0xff3b2d1b));
}
juce::String formatValue(rack::Param p, double value) {
  using namespace rack;
  int i = juce::jlimit(0, 12, int(std::round(value)));
  if (p == DelayNote) {
    const char *a[]{"1/16", "1/8", "1/8 D", "1/4", "1/4 D", "1/2"};
    return a[juce::jlimit(0, 5, i)];
  }
  if (p == ModType) {
    const char *a[]{"CHORUS", "FLANGER", "VIBRATO"};
    return a[juce::jlimit(0, 2, i)];
  }
  if (p == PitchKey) {
    const char *a[]{"AUTO", "C", "C#", "D", "D#", "E", "F",
                    "F#",   "G", "G#", "A", "A#", "B"};
    return a[i];
  }
  if (p == PitchScale) {
    const char *a[]{"AUTO", "CHROMATIC", "MAJOR", "MINOR"};
    return a[juce::jlimit(0, 3, i)];
  }
  if (p == PreampStyle) {
    const char *a[]{"IRON 73", "PUNCH 512",  "EDGE 4K",
                    "AIR 737", "VELVET BOX", "DIRTY TG"};
    return a[juce::jlimit(0, 5, i)];
  }
  if (p == PitchStyle)
    return i ? "ROBOT" : "NATURAL";
  if (p == HarmonyPattern) {
    const char *a[]{"OCTAVES", "FIFTHS", "CHOIR", "ANGELIC"};
    return a[juce::jlimit(0, 3, i)];
  }
  if (p == SatStyle) {
    const char *a[]{"TUBE", "TAPE", "TRANSISTOR", "TRANSFORMER", "CRUSH"};
    return a[juce::jlimit(0, 4, i)];
  }
  if (p == DelayStyle) {
    const char *a[]{"SLAP", "THROW", "PING-PONG", "TAPE DUCK", "PING + VERB"};
    return a[juce::jlimit(0, 4, i)];
  }
  if (p == ReverbStyle) {
    const char *a[]{"CHAMBER", "HALL", "PLATE"};
    return a[juce::jlimit(0, 2, i)];
  }
  const auto &spec = specs()[size_t(p)];
  juce::String suffix(spec.suffix);
  if (suffix.contains("Hz") && value >= 1000)
    return juce::String(value / 1000, value < 10000 ? 1 : 0) + " kHz";
  int decimals = (suffix == " dB" || p == EQQ || p == CompRatio ||
                  p == ReverbDecay || p == ModRate)
                     ? 1
                     : 0;
  if (spec.step < 1)
    decimals = spec.step < .1f ? 2 : 1;
  return juce::String(value, decimals) + suffix;
}
} // namespace
AlienSkin::AlienSkin() {
  setColour(juce::ComboBox::backgroundColourId, dark);
  setColour(juce::ComboBox::textColourId, bright);
  setColour(juce::ComboBox::outlineColourId, gold.withAlpha(.65f));
  setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff141718));
  setColour(juce::PopupMenu::textColourId, bright);
  setColour(juce::PopupMenu::highlightedBackgroundColourId,
            juce::Colour(0xff514327));
  setColour(juce::Slider::textBoxTextColourId, bright);
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
}
void AlienSkin::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h,
                                 float pos, float start, float end,
                                 juce::Slider &slider) {
  auto r = juce::Rectangle<float>(float(x), float(y), float(w), float(h))
               .withSizeKeepingCentre(float(juce::jmin(w, h)),
                                      float(juce::jmin(w, h)))
               .reduced(5);
  auto c = r.getCentre();
  float radius = r.getWidth() * .5f;
  g.setColour(juce::Colours::black.withAlpha(.8f));
  g.fillEllipse(r.translated(1, 3).expanded(3));
  g.setColour(gold.withAlpha(slider.isEnabled() ? .45f : .15f));
  g.drawEllipse(r.expanded(3), 1);
  juce::Path arc;
  arc.addCentredArc(c.x, c.y, radius + 5, radius + 5, 0, start, end, true);
  g.setColour(gold.withAlpha(.15f));
  g.strokePath(arc, juce::PathStrokeType(2));
  juce::Path active;
  active.addCentredArc(c.x, c.y, radius + 5, radius + 5, 0, start,
                       start + pos * (end - start), true);
  g.setColour(gold.withAlpha(slider.isEnabled() ? .75f : .18f));
  g.strokePath(active, juce::PathStrokeType(2));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffbea46c), r.getX(),
                                         r.getY(), juce::Colour(0xff171b1b),
                                         r.getRight(), r.getBottom(), false));
  g.fillEllipse(r);
  g.setColour(juce::Colour(0xff514833));
  g.drawEllipse(r, 1);
  auto inner = r.reduced(4);
  g.setGradientFill(juce::ColourGradient(
      juce::Colour(0xff202322), inner.getX(), inner.getY(),
      juce::Colour(0xff605239), inner.getRight(), inner.getBottom(), false));
  g.fillEllipse(inner);
  float a = start + pos * (end - start);
  g.setColour(bright.withAlpha(slider.isEnabled() ? 1.0f : .3f));
  g.drawLine(
      c.x + std::sin(a) * radius * .28f, c.y - std::cos(a) * radius * .28f,
      c.x + std::sin(a) * radius * .82f, c.y - std::cos(a) * radius * .82f, 2);
  g.setColour(juce::Colours::white.withAlpha(.12f));
  g.fillEllipse(r.getX() + radius * .4f, r.getY() + radius * .22f, radius * .5f,
                radius * .2f);
}
void AlienSkin::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                     const juce::Colour &, bool over,
                                     bool down) {
  auto r = b.getLocalBounds().toFloat().reduced(.5f);
  auto top =
      b.getToggleState() ? juce::Colour(0xff775e2c) : juce::Colour(0xff252828);
  auto bottom = down ? juce::Colour(0xff9f7c36) : juce::Colour(0xff0b0e0f);
  g.setGradientFill(juce::ColourGradient(top, r.getX(), r.getY(), bottom,
                                         r.getX(), r.getBottom(), false));
  g.fillRoundedRectangle(r, 3);
  g.setColour(gold.withAlpha(over ? .9f : .55f));
  g.drawRoundedRectangle(r, 3, 1);
}
void AlienSkin::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool,
                               bool) {
  bool ai = b.getButtonText() == "AI" || b.getButtonText() == "ANALIZAR TODO";
  if (ai)
    alien(g, {4, 3, 12.0f, float(b.getHeight() - 6)}, cyan);
  text(g, b.getButtonText(),
       b.getLocalBounds().reduced(2).withTrimmedLeft(ai ? 15 : 0),
       b.getHeight() < 24 ? 9.0f : 11.0f, b.isEnabled() ? bright : muted);
}
void AlienSkin::drawToggleButton(juce::Graphics &g, juce::ToggleButton &b, bool,
                                 bool) {
  auto r = b.getLocalBounds();
  float cy = float(r.getCentreY());
  g.setColour(juce::Colours::black);
  g.fillEllipse(1, cy - 4, 8, 8);
  g.setColour((b.getToggleState() ? gold : juce::Colour(0xff4e4b3b))
                  .withAlpha(b.isEnabled() ? 1.0f : .3f));
  g.fillEllipse(2, cy - 3, 6, 6);
  text(g, b.getButtonText(), r.withTrimmedLeft(13),
       r.getHeight() < 23 ? 9.0f : 11.0f, b.isEnabled() ? gold : muted,
       juce::Justification::centredLeft);
}
void AlienSkin::drawComboBox(juce::Graphics &g, int w, int h, bool, int, int,
                             int, int, juce::ComboBox &) {
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff242623), 0, 0, dark,
                                         0, float(h), false));
  g.fillRoundedRectangle(0, 0, float(w), float(h), 3);
  g.setColour(gold.withAlpha(.6f));
  g.drawRoundedRectangle(.5f, .5f, float(w - 1), float(h - 1), 3, 1);
  juce::Path p;
  p.addTriangle(float(w - 15), float(h) * .42f, float(w - 7), float(h) * .42f,
                float(w - 11), float(h) * .62f);
  g.fillPath(p);
}
RackKnob::RackKnob(M7GProcessor &p, rack::Param id, juce::String name)
    : caption(std::move(name)) {
  addAndMakeVisible(slider);
  slider.setColour(juce::Slider::textBoxOutlineColourId,
                   juce::Colours::transparentBlack);
  slider.setColour(juce::Slider::textBoxBackgroundColourId,
                   juce::Colours::transparentBlack);
  slider.setColour(juce::Slider::textBoxTextColourId, bright);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                             juce::MathConstants<float>::pi * 2.75f, true);
  bool choice = id == rack::DelayNote || id == rack::ModType ||
                id == rack::PitchKey || id == rack::PitchScale ||
                id == rack::PreampStyle || id == rack::PitchStyle ||
                id == rack::HarmonyPattern || id == rack::SatStyle ||
                id == rack::DelayStyle || id == rack::ReverbStyle;
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, choice, 85, 17);
  slider.setDoubleClickReturnValue(true, rack::specs()[size_t(id)].def);
  slider.setTooltip(juce::String(rack::specs()[size_t(id)].name) +
                    " • Doble clic: valor predeterminado");
  attachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.apvts, rack::specs()[size_t(id)].id, slider);
  slider.textFromValueFunction = [id](double v) { return formatValue(id, v); };
  slider.valueFromTextFunction = [id](const juce::String &s) {
    double v = s.getDoubleValue();
    if (juce::String(rack::specs()[size_t(id)].suffix).contains("Hz") &&
        s.containsIgnoreCase("k"))
      v *= 1000;
    return v;
  };
  slider.updateText();
}
void RackKnob::resized() {
  slider.setBounds(0, 12, getWidth(), getHeight() - 12);
}
void RackKnob::paint(juce::Graphics &g) {
  text(g, caption, getLocalBounds().withHeight(12), 8.5f,
       isEnabled() ? muted : muted.withAlpha(.4f));
}
RackModule::RackModule(M7GProcessor &p, int module)
    : processor(p), index(module), enabledParameter(enables[module]) {
  history.fill(-80);
  enable.setButtonText(titles[index]);
  addAndMakeVisible(enable);
  addAndMakeVisible(analyse);
  analyse.setTooltip("Mide 3 segundos de audio y ajusta este modulo");
  analyse.onClick = [this] { processor.beginAnalysis(index); };
  enableAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, rack::specs()[size_t(enabledParameter)].id, enable);
  parallel.setSliderStyle(juce::Slider::LinearHorizontal);
  parallel.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 16);
  parallel.setColour(juce::Slider::trackColourId, cyan);
  parallel.setColour(juce::Slider::thumbColourId, gold);
  parallel.setColour(juce::Slider::textBoxOutlineColourId,
                     juce::Colours::transparentBlack);
  parallel.setColour(juce::Slider::textBoxTextColourId, bright);
  parallel.setTextValueSuffix(" %");
  parallel.setTooltip("Rama paralela alineada. Los efectos de espacio usan su "
                      "envio; el techo de seguridad se conserva.");
  addAndMakeVisible(parallel);
  parallelAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.apvts, rack::specs()[size_t(parallels[index])].id, parallel);
  auto knob = [&](rack::Param id, const char *caption) {
    auto k = std::make_unique<RackKnob>(p, id, caption);
    addAndMakeVisible(*k);
    knobs.push_back(std::move(k));
  };
  auto button = [&](rack::Param id, const char *name) {
    auto b = std::make_unique<juce::ToggleButton>(name);
    addAndMakeVisible(*b);
    switchAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            p.apvts, rack::specs()[size_t(id)].id, *b));
    switches.push_back(std::move(b));
  };
  using namespace rack;
  switch (index) {
  case 0:
    knob(GateThreshold, "THRESHOLD");
    knob(GateRange, "RANGE");
    knob(GateAttack, "ATTACK");
    knob(GateRelease, "RELEASE");
    knob(GateLowcut, "LOW CUT");
    break;
  case 1:
    knob(DeessFreq, "FREQUENCY");
    knob(DeessThreshold, "THRESHOLD");
    knob(DeessAmount, "AMOUNT");
    button(DeessListen, "LISTEN");
    break;
  case 2:
    knob(WarmDrive, "DRIVE");
    knob(WarmBias, "BIAS");
    knob(WarmTone, "TONE");
    knob(WarmOutput, "OUTPUT");
    break;
  case 3:
    knob(EQFreq, "FREQUENCY");
    knob(EQGain, "GAIN");
    knob(EQQ, "Q");
    knob(EQDirty, "DIRTY BLEND");
    break;
  case 4:
    knob(AirAmount, "AIR");
    knob(AirGain, "SHINE");
    knob(AirShimmer, "HARMONICS");
    knob(AirFilter, "FILTER");
    break;
  case 5:
    knob(CompReduction, "PEAK REDUCTION");
    knob(CompGain, "GAIN");
    knob(CompRatio, "RATIO");
    knob(CompRelease, "RECOVERY");
    break;
  case 6:
    knob(LimitCeiling, "CEILING");
    knob(LimitRelease, "RELEASE");
    button(LimitFast, "FAST");
    button(LimitLookahead, "LOOKAHEAD");
    break;
  case 7:
    knob(ReverbMix, "MASTER SEND");
    knob(ReverbDecay, "TAIL SCALE");
    knob(ReverbSize, "SIZE");
    knob(ReverbShimmer, "SHIMMER");
    knob(ReverbDuck, "DUCKING");
    break;
  case 8:
    knob(DelayMix, "SEND");
    knob(DelayNote, "SYNC NOTE");
    knob(DelayFeedback, "FEEDBACK");
    knob(DelayTone, "TONE");
    knob(DelayDuck, "DUCKING");
    break;
  case 9:
    knob(Width, "WIDTH");
    knob(Pan, "PAN");
    knob(StereoLow, "MONO BASS");
    knob(Spread, "SPREAD");
    break;
  case 10:
    knob(ModRate, "RATE");
    knob(ModDepth, "DEPTH");
    knob(ModMix, "MIX");
    knob(ModType, "TYPE");
    break;
  case 11:
    knob(TuneAmount, "AMOUNT");
    knob(TuneSpeed, "RETUNE");
    knob(PitchKey, "KEY");
    knob(PitchScale, "SCALE");
    knob(PitchStyle, "CHARACTER");
    knob(PitchFormant, "THROAT / FORMANT");
    break;
  case 12:
    knob(DereverbAmount, "AMOUNT");
    knob(DereverbTail, "ROOM RECOVERY");
    knob(TransientProtect, "TRANSIENTS");
    break;
  case 13:
    knob(ResonanceAmount, "REDUCTION");
    knob(ResonanceFocus, "FOCUS");
    knob(HarmonicProtect, "HARMONIC PROTECT");
    knob(TransientProtect, "TRANSIENTS");
    break;
  case 14:
    knob(SatLow, "LOW DRIVE");
    knob(SatMid, "MID DRIVE");
    knob(SatHigh, "HIGH DRIVE");
    knob(SatStyle, "SATURATION LAW");
    knob(SatCrossLow, "LOW CROSSOVER");
    knob(SatCrossHigh, "HIGH CROSSOVER");
    break;
  case 15:
    knob(HarmonyMix, "LEVEL");
    knob(HarmonyPattern, "VOICING");
    knob(HarmonyDetune, "HUMANIZE");
    knob(HarmonyFormant, "FORMANTS");
    break;
  case 16:
    knob(FetThreshold, "THRESHOLD");
    knob(FetRatio, "RATIO");
    knob(FetAttack, "ATTACK");
    knob(FetRelease, "RELEASE");
    knob(FetGain, "MAKEUP");
    break;
  case 17:
    knob(NoiseAmount, "NEURAL BLEND");
    break;
  case 18:
    knob(PreampStyle, "PRE MODEL");
    knob(PreampGain, "GAIN");
    knob(PreampImpedance, "VIRTUAL IMPEDANCE");
    button(PreampPad, "PAD -20 dB");
    button(PhantomModel, "48V / VIRTUAL BIAS");
    switches.back()->setTooltip(
        "Cambia el rail virtual del modelo. No activa phantom ni cambia la "
        "impedancia fisica de tu interfaz.");
    break;
  case 19:
    knob(CompParallel, "NEW YORK MIX");
    knob(SatParallel, "SATURATION MIX");
    knob(EQParallel, "EQ MIX");
    knob(EQDirty, "CLEAN / DIRTY");
    button(Punch, "PUNCH");
    break;
  case 20:
    knob(VerbAMix, "LAYER MIX");
    knob(VerbADecay, "DECAY");
    knob(VerbAPredelay, "PRE-DELAY");
    knob(VerbAFilter, "FILTER");
    knob(VerbAMod, "MODULATION");
    break;
  case 21:
    knob(VerbBMix, "LAYER MIX");
    knob(VerbBDecay, "DECAY");
    knob(VerbBPredelay, "PRE-DELAY");
    knob(VerbBFilter, "FILTER");
    knob(VerbBMod, "MODULATION");
    break;
  case 22:
    knob(VerbCMix, "LAYER MIX");
    knob(VerbCDecay, "DECAY");
    knob(VerbCPredelay, "PRE-DELAY");
    knob(VerbCFilter, "FILTER");
    knob(VerbCMod, "MODULATION");
    break;
  case 23:
    knob(DelayStyle, "MODE");
    knob(SlapTime, "SLAP TIME");
    knob(DelayTime, "MANUAL TIME");
    knob(DelayDrive, "SATURATION");
    knob(ThrowSensitivity, "PHRASE GAP");
    button(DelaySync, "TEMPO SYNC");
    break;
  case 24:
    knob(SculptorAmount, "AMOUNT");
    knob(SculptorTilt, "TILT");
    knob(SculptorBody, "BODY");
    knob(SculptorPresence, "PRESENCE");
    knob(SculptorAir, "AIR");
    knob(SculptorProtect, "TRANSIENT PROTECT");
    break;
  case 25:
    knob(AlignAmount, "ALIGN STRENGTH");
    knob(AlignRange, "SEARCH RANGE");
    knob(AlignSpeed, "CROSSFADE");
    analyse.setTooltip("Conecta la voz principal al sidechain Lead reference y "
                       "reproduce ambas pistas");
    break;
  }
}
void RackModule::resized() {
  float sx = float(getWidth()) / 205, sy = float(getHeight()) / 318;
  auto place = [&](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
                juce::roundToInt(w * sx), juce::roundToInt(h * sy));
  };
  place(enable, 8, 5, 146, 23);
  place(analyse, 157, 7, 41, 19);
  for (size_t i = 0; i < knobs.size(); ++i)
    place(*knobs[i], 8 + int(i % 2) * 98, 35 + int(i / 2) * 72, 89, 70);
  for (size_t i = 0; i < switches.size(); ++i) {
    if (index == 18)
      place(*switches[i], 12, 194 + int(i) * 27, 181, 23);
    else if (index == 6)
      place(*switches[i], 12, 124 + int(i) * 26, 181, 23);
    else if (index == 1)
      place(*switches[i], 102, 133, 95, 22);
    else
      place(*switches[i], 12, 255 + int(i) * 24, 181, 22);
  }
  place(parallel, 10, 294, 186, 20);
}
void RackModule::tick() {
  for (size_t i = 0; i + 1 < history.size(); ++i)
    history[i] = history[i + 1];
  history.back() = processor.inputDB.load();
  bool on = processor.value(enabledParameter) > .5f;
  bool bus = (index == 11 || index == 15) &&
             int(processor.value(rack::VoiceType)) == 5;
  bool windowed = index == 11 || index == 12 || index == 13 || index == 15 ||
                  index == 17 || index == 24 || index == 25;
  bool available = !(windowed && processor.engineMode() == 0) && !bus;
  enable.setEnabled(available);
  for (auto &k : knobs)
    k->setEnabled(on && available);
  for (auto &b : switches)
    b->setEnabled(on && available);
  parallel.setEnabled(on && available);
  analyse.setEnabled(available);
  analyse.setToggleState(processor.analysisState.load() == 1 &&
                             processor.analysisTarget() == index,
                         juce::dontSendNotification);
  repaint();
}
void RackModule::paint(juce::Graphics &g) {
  juce::Graphics::ScopedSaveState state(g);
  g.addTransform(juce::AffineTransform::scale(float(getWidth()) / 205,
                                              float(getHeight()) / 318));
  juce::Path hull;
  hull.startNewSubPath(8, 1);
  hull.lineTo(197, 1);
  hull.lineTo(204, 8);
  hull.lineTo(204, 309);
  hull.lineTo(196, 317);
  hull.lineTo(8, 317);
  hull.lineTo(1, 309);
  hull.lineTo(1, 8);
  hull.closeSubPath();
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff172632), 0, 0,
                                         juce::Colour(0xff080e17), 205, 318,
                                         false));
  g.fillPath(hull);
  g.setColour(gold.withAlpha(.7f));
  g.strokePath(hull, juce::PathStrokeType(1));
  g.setColour(cyan.withAlpha(.35f));
  g.drawLine(9, 31, 195, 31);
  g.drawLine(9, 281, 195, 281);
  text(g,
       "PARALLEL / " + juce::String(index == 7 || index == 8 || index == 15 ||
                                            (index >= 20 && index <= 23)
                                        ? "SEND"
                                        : "DRY - WET"),
       {10, 282, 185, 11}, 7.5f, cyan);
  if (knobs.size() > 4) {
    if (index == 20 || index == 21 || index == 22)
      text(g, juce::String(processor.verbLayerDB(index - 20), 1) + " dBFS",
           {104, 214, 90, 40}, 9, cyan);
    return;
  }
  auto graph = juce::Rectangle<float>(11, knobs.size() > 2 ? 211.0f : 194.0f,
                                      183, knobs.size() > 2 ? 49.0f : 65.0f);
  g.setColour(juce::Colour(0xff050b13));
  g.fillRoundedRectangle(graph, 3);
  g.setColour(cyan.withAlpha(.2f));
  g.drawRoundedRectangle(graph, 3, 1);
  if (index == 5 || index == 16) {
    vu(g, graph.reduced(4),
       index == 5 ? processor.reductionDB.load() : processor.fetGR.load(),
       true);
    return;
  }
  if (index == 11) {
    text(g,
         juce::String(processor.pitchHz.load(), 1) + " > " +
             juce::String(processor.targetHz.load(), 1) + " Hz",
         graph.toNearestInt(), 10, cyan);
    return;
  }
  if (index == 15) {
    for (int i = 0; i < 8; ++i) {
      g.setColour(i < processor.harmonyVoices.load() ? cyan
                                                     : cyan.withAlpha(.1f));
      g.fillRoundedRectangle(18.0f + i * 21, graph.getY() + 10, 13, 28, 3);
    }
    text(g, "8 VOICES / CEPSTRAL FORMANTS", {12, 262, 181, 15}, 8, muted);
    return;
  }
  if (index == 25) {
    text(g,
         processor.referencePresent.load()
             ? juce::String(processor.alignOffset.load(), 1) + " ms / " +
                   juce::String(int(processor.alignConfidence.load() * 100)) +
                   "%"
             : "CONNECT LEAD REFERENCE",
         graph.toNearestInt(), 9,
         processor.referencePresent.load() ? cyan : gold);
    return;
  }
  if (index == 18) {
    alien(g, {22, 244, 18, 23}, cyan);
    text(g, "DIGITAL LOAD + MAGNETIC STATE", {43, 244, 151, 25}, 7.5f, muted);
    return;
  }
  if (index == 12 || index == 13 || index == 6 || index == 19 || index == 7 ||
      index == 8) {
    float db =
        index == 12
            ? processor.dereverbGR.load()
            : (index == 13
                   ? processor.resonanceGR.load()
                   : (index == 6
                          ? processor.limitGR.load()
                          : (index == 19
                                 ? processor.reductionDB.load() +
                                       processor.fetGR.load()
                                 : (index == 7 ? processor.revDuckDB.load()
                                               : processor.delDuckDB.load()))));
    g.setColour(cyan.withAlpha(.2f));
    g.fillRect(graph.reduced(7, 12));
    g.setColour(cyan);
    g.fillRect(graph.reduced(7, 12).withWidth(
        juce::jlimit(0.0f, 1.0f, db / 24) * (graph.getWidth() - 14)));
    text(g, "REDUCTION " + juce::String(db, 1) + " dB", {12, 262, 181, 15}, 8,
         muted);
    return;
  }
  if (index == 2) {
    alien(g, graph.reduced(68, 2), gold);
    text(g, "M7G UNISON DRIVE", {12, 262, 181, 15}, 8, muted);
    return;
  }
  for (int k = 0; k < 24; ++k) {
    float level = processor.spectrum[size_t(k + 4)].load();
    float value = juce::jlimit(0.0f, 1.0f, (level + 85) / 75);
    g.setColour((k % 3 ? gold : cyan).withAlpha(.75f));
    g.fillRect(graph.getX() + k * graph.getWidth() / 24 + 1,
               graph.getBottom() - value * (graph.getHeight() - 6),
               graph.getWidth() / 24 - 2, value * (graph.getHeight() - 6));
  }
}
M7GEditor::M7GEditor(M7GProcessor &p) : AudioProcessorEditor(p), processor(p) {
  setLookAndFeel(&skin);
  for (int i = 0; i < 26; ++i) {
    modules[size_t(i)] = std::make_unique<RackModule>(p, i);
    addAndMakeVisible(*modules[size_t(i)]);
  }
  for (int i = 0; i < rack::presetCount; ++i) {
    if (i == 0)
      presetBox.addSectionHeading("TRIPLE GOD SIGNATURE");
    if (i == 14)
      presetBox.addSectionHeading("GENRE + VOCAL ROLE");
    if (i == 230)
      presetBox.addSectionHeading("50 CLASSIC / FUTURE VOICE FX");
    presetBox.addItem(rack::presets()[size_t(i)].name, i + 1);
  }
  presetBox.setSelectedId(p.getCurrentProgram() + 1,
                          juce::dontSendNotification);
  presetBox.onChange = [this] {
    processor.setCurrentProgram(presetBox.getSelectedId() - 1);
  };
  presetBox.setTooltip(
      "280 presets hechos por Makkine Seven Golden. Referencias de estilo; sin "
      "participacion de los artistas.");
  for (auto *c : std::array<juce::Component *, 14>{
           &presetBox, &analyseAll, &compareA, &compareB, &menu, &bypass, &solo,
           &autoLevel, &status, &classicTab, &engineTab, &tripleTab, &modeBox,
           &godSpace})
    addAndMakeVisible(c);
  analyseAll.onClick = [this] {
    processor.beginAnalysis(M7GProcessor::analysisAll);
  };
  compareA.onClick = [this] { processor.selectCompare(0); };
  compareB.onClick = [this] { processor.selectCompare(1); };
  menu.onClick = [this] { showMenu(); };
  classicTab.onClick = [this] { showPage(0); };
  engineTab.onClick = [this] { showPage(1); };
  tripleTab.onClick = [this] { showPage(2); };
  modeBox.addItemList(
      {"MONITOR / ZERO LATENCY", "STUDIO / 8x", "LINEAR / FIR 8x"}, 1);
  modeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          p.apvts, "enginemode", modeBox);
  modeBox.setTooltip(
      "Deten el transporte al cambiar modo. Monitor omite neural, espectro, "
      "pitch, armonias y alineacion; Linear usa reconstruccion FIR 8x.");
  bypassAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "bypass", bypass);
  soloAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "solo", solo);
  autoAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "autogain", autoLevel);
  godAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "godspace", godSpace);
  input = std::make_unique<RackKnob>(p, rack::InputTrim, "INPUT");
  mix = std::make_unique<RackKnob>(p, rack::Mix, "MIX");
  output = std::make_unique<RackKnob>(p, rack::OutputTrim, "OUTPUT");
  sino = std::make_unique<RackKnob>(p, rack::Sino, "SINO / ESPECTACULAR");
  for (auto *c : {input.get(), mix.get(), output.get(), sino.get()})
    addAndMakeVisible(c);
  sino->slider.setTooltip(
      "Macro neuronal local. Analiza continuamente y mezcla objetivos de "
      "nivel, color, espacio y armonias; los controles manuales son su punto "
      "de partida.");
  status.setFont(
      juce::Font(juce::FontOptions(AlienSkin::bodyTypeface()).withHeight(9)));
  status.setColour(juce::Label::textColourId, cyan);
  status.setJustificationType(juce::Justification::centredRight);
  setResizable(true, false);
  setResizeLimits(1100, 775, 1920, 1350);
  setSize(1280, 900);
  showPage(2);
  startTimerHz(25);
  timerCallback();
}
M7GEditor::~M7GEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void M7GEditor::showPage(int selected) {
  page = selected;
  for (int i = 0; i < 26; ++i)
    modules[size_t(i)]->setVisible(
        page == 0 ? i < 12
                  : (page == 1 ? (i >= 12 && i < 18) || i >= 24
                               : (i >= 18 && i < 24)));
  classicTab.setToggleState(page == 0, juce::dontSendNotification);
  engineTab.setToggleState(page == 1, juce::dontSendNotification);
  tripleTab.setToggleState(page == 2, juce::dontSendNotification);
  resized();
  repaint();
}
void M7GEditor::resized() {
  float sx = float(getWidth()) / 1280, sy = float(getHeight()) / 900;
  auto place = [&](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
                juce::roundToInt(w * sx), juce::roundToInt(h * sy));
  };
  for (int i = 0; i < 26; ++i) {
    int slot = i < 12 ? i : (i < 18 ? i - 12 : (i < 24 ? i - 18 : i - 18));
    place(*modules[size_t(i)], 10 + (slot % 6) * 211, 120 + (slot / 6) * 324,
          205, 318);
  }
  place(presetBox, 520, 14, 430, 29);
  place(compareA, 963, 14, 30, 29);
  place(compareB, 998, 14, 30, 29);
  place(analyseAll, 1040, 14, 160, 29);
  place(menu, 1208, 14, 60, 29);
  place(autoLevel, 1038, 50, 108, 25);
  place(godSpace, 1155, 50, 113, 25);
  place(classicTab, 520, 80, 151, 26);
  place(engineTab, 681, 80, 166, 26);
  place(tripleTab, 857, 80, 126, 26);
  place(status, 986, 78, 282, 29);
  place(*input, 14, 787, 85, 83);
  place(*mix, 320, 787, 85, 83);
  place(*sino, 429, 768, 119, 109);
  place(*output, 855, 787, 85, 83);
  place(bypass, 1095, 780, 155, 25);
  place(solo, 1095, 813, 155, 25);
  place(modeBox, 1095, 851, 173, 26);
}
void M7GEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff060b15));
  juce::Graphics::ScopedSaveState save(g);
  g.addTransform(juce::AffineTransform::scale(float(getWidth()) / 1280,
                                              float(getHeight()) / 900));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff183345), 640, 70,
                                         juce::Colour(0xff080b16), 640, 900,
                                         false));
  g.fillRect(0, 0, 1280, 900);
  for (int y = 120; y < 760; y += 28) {
    g.setColour(cyan.withAlpha(.035f));
    g.drawLine(0, float(y), 1280, float(y));
  }
  for (int x = 0; x < 1280; x += 64) {
    g.setColour(violet.withAlpha(.04f));
    g.drawLine(640, 80, float(x), 780);
  }
  g.setColour(gold.withAlpha(.65f));
  g.drawLine(12, 112, 1268, 112);
  g.drawLine(12, 764, 1268, 764);
  g.setColour(cyan.withAlpha(.3f));
  g.drawLine(12, 889, 1268, 889);
  alien(g, {24, 17, 48, 62}, gold);
  g.setColour(bright);
  g.setFont(juce::Font(
      juce::FontOptions(AlienSkin::graffitiTypeface()).withHeight(27)));
  g.drawFittedText("M7G ALIEN VOICE V10", {92, 12, 414, 38},
                   juce::Justification::centredLeft, 1);
  text(g, "TRIPLE GOD ORIGIN", {95, 48, 410, 19}, 12, cyan,
       juce::Justification::centredLeft);
  g.setFont(juce::Font(
      juce::FontOptions(AlienSkin::graffitiTypeface()).withHeight(17)));
  g.setColour(gold);
  g.drawFittedText("LAS MAKKINAS PRODUCEN SOLAS", {95, 72, 412, 30},
                   juce::Justification::centredLeft, 1);
  text(g, "HECHOS POR MAKKINE SEVEN GOLDEN", {520, 47, 430, 23}, 9, muted,
       juce::Justification::centredLeft);
  if (page == 2) {
    auto r = juce::Rectangle<float>(35, 470, 1210, 260);
    juce::Path ship;
    ship.startNewSubPath(50, 670);
    ship.lineTo(280, 495);
    ship.lineTo(640, 460);
    ship.lineTo(1000, 495);
    ship.lineTo(1230, 670);
    ship.lineTo(980, 635);
    ship.lineTo(800, 730);
    ship.lineTo(480, 730);
    ship.lineTo(300, 635);
    ship.closeSubPath();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff142d3a), 640, 470,
                                           juce::Colour(0xff080b18), 640, 730,
                                           false));
    g.fillPath(ship);
    g.setColour(cyan.withAlpha(.3f));
    g.strokePath(ship, juce::PathStrokeType(1.4f));
    alien(g, {615, 670, 40, 52}, cyan);
    text(g, "TRIPLE GOD / FLIGHT SYSTEMS", {400, 636, 480, 26}, 16, gold);
    for (int i = 0; i < 3; ++i) {
      float x = 130 + i * 260;
      float db = processor.verbLayerDB(i);
      text(g,
           juce::String("VERB ") +
               juce::String::charToString(juce::juce_wchar('A' + i)),
           {int(x), 527, 170, 24}, 11, cyan);
      g.setColour(cyan.withAlpha(.12f));
      g.fillRect(x, 566.0f, 170.0f, 9.0f);
      g.setColour(cyan);
      g.fillRect(x, 566.0f, 170 * juce::jlimit(0.0f, 1.0f, (db + 70) / 60),
                 9.0f);
      text(g, juce::String(db, 1) + " dBFS", {int(x), 584, 170, 20}, 10, muted);
    }
    text(g,
         "SMART DELAY / DUCK -" + juce::String(processor.delDuckDB.load(), 1) +
             " dB",
         {905, 527, 275, 40}, 12, gold);
    text(g,
         "SINO AUTO LEVEL " + juce::String(processor.sinoTrim.load(), 1) +
             " dB",
         {905, 578, 275, 35}, 11, cyan);
  }
  auto meter = [&](int x, int y, int w, float db) {
    for (int i = 0; i < 36; ++i) {
      g.setColour(i < (db + 60) * .6f ? cyan : cyan.withAlpha(.10f));
      g.fillRect(float(x) + i * float(w) / 36, float(y), float(w) / 36 - 2,
                 8.0f);
    }
    text(g, juce::String(db, 1) + " dBFS", {x, y + 12, w, 18}, 9, muted,
         juce::Justification::centredRight);
  };
  meter(112, 815, 190, processor.inputDB.load());
  meter(951, 815, 125, processor.outputDB.load());
  vu(g, {568, 800, 121, 58}, processor.outputChannels[0].load());
  vu(g, {703, 800, 121, 58}, processor.outputChannels[1].load());
  text(g, "OUTPUT / 0 VU = -18 dBFS", {568, 863, 256, 13}, 8, muted);
  double sr = processor.getSampleRate(),
         latency = sr > 0 ? processor.getLatencySamples() * 1000 / sr : 0;
  text(g,
       "M7G / V10.0.0 / " + juce::String(sr / 1000, 1) + " kHz / " +
           juce::String(latency, 1) + " ms",
       {12, 890, 620, 10}, 7.5f, muted, juce::Justification::centredLeft);
  text(g, "280 PRESETS / 26 AI MODULES / LOCAL PROCESSING", {720, 890, 548, 10},
       7.5f, muted, juce::Justification::centredRight);
}
void M7GEditor::timerCallback() {
  processor.applyAnalysis();
  int state = processor.analysisState.load(),
      target = processor.analysisTarget();
  juce::String label;
  if (state == 1)
    label = "ANALIZANDO " +
            juce::String(int(processor.analysisProgress.load() * 100)) +
            "% / REPRODUCE VOZ";
  else if (state == -2)
    label = "CONECTA LEAD REFERENCE AL SIDECHAIN";
  else if (state < 0)
    label = "SIN SENAL / REPRODUCE LA VOZ";
  else
    label = processor.engineMode() == 0
                ? "MONITOR: PITCH / NEURAL / ALIGN OFF"
                : "280 PRESETS / " + juce::String(M7GProcessor::action(int(
                                         processor.value(rack::VoiceType))));
  status.setText(label, juce::dontSendNotification);
  presetBox.setSelectedId(processor.getCurrentProgram() + 1,
                          juce::dontSendNotification);
  compareA.setToggleState(processor.compareSlot() == 0,
                          juce::dontSendNotification);
  compareB.setToggleState(processor.compareSlot() == 1,
                          juce::dontSendNotification);
  analyseAll.setToggleState(state == 1 && target == M7GProcessor::analysisAll,
                            juce::dontSendNotification);
  for (auto &m : modules)
    m->tick();
  repaint();
}
void M7GEditor::showMenu() {
  juce::PopupMenu m;
  m.addItem(1, "Restaurar preset");
  m.addItem(2, "Copiar al otro slot A/B");
  m.addSeparator();
  m.addItem(3, "Acerca de M7G TRIPLE GOD");
  juce::Component::SafePointer<M7GEditor> safe(this);
  m.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(&menu),
      [safe](int result) {
        if (!safe)
          return;
        if (result == 1)
          safe->processor.setCurrentProgram(
              safe->processor.getCurrentProgram());
        if (result == 2)
          safe->processor.copyCompare();
        if (result == 3)
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::InfoIcon,
              "M7G ALIEN VOICE V10 TRIPLE GOD ORIGIN",
              "Hecho por Makkine Seven Golden.\nRNNoise local + MLP de "
              "autoajuste y CNN de candidatos de pitch entrenadas con senales "
              "sinteticas.\nDSP original; sin motores propietarios ni "
              "certificacion de equivalencia.\n48V e impedancia son modelos "
              "digitales, no control de tu interfaz.\nMonitor omite "
              "operaciones con ventana. Alineacion requiere "
              "sidechain.\nEvaluacion de voces reales y Studio One 7 "
              "pendientes.");
      });
}
