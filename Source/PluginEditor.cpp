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
const juce::Colour gold{0xffffd700}, bright{0xfff1dca5}, dark{0xff0a0f1e},
    muted{0xff9b926f};
constexpr const char *titles[]{"ALIEN NOISE GATE GOD",
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
                               "ALIEN PITCH GOD",
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
    : caption(std::move(name)), processor(p), parameterID(id) {
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
  slider.setTooltip(caption +
                    ". Arrastra para cambiar. Doble clic: valor inicial. En "
                    "AUTO el alien lo ajusta; MANUAL conserva tu ajuste.");
  if (id == rack::TuneAmount)
    slider.setTooltip("CUANTO CORRIGE: sube para acercar mas la nota al "
                      "centro. Dentro de 15 cents no corrige.");
  if (id == rack::KeepHuman)
    slider.setTooltip(
        "CONSERVAR HUMANO: sube para mantener vibrato, deslizamientos y "
        "expresion. Baja para un efecto mas firme.");
  if (id == rack::GateThreshold)
    slider.setTooltip("CUANDO CIERRA: el ruido por debajo de este nivel se "
                      "reduce. Pulsa LEARN sin cantar durante dos segundos.");
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
void RackKnob::tick() {
  if (processor.automatic() && !slider.isMouseButtonDown() &&
      !slider.hasKeyboardFocus(true))
    slider.setValue(processor.effectiveValue(parameterID),
                    juce::dontSendNotification);
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
    knob(GateThreshold, "CUANDO CIERRA");
    addAndMakeVisible(learn);
    learn.onClick = [this] { processor.learnNoise(); };
    learn.setTooltip("APRENDER RUIDO: deja sonar solo el ruido de tu "
                     "habitacion durante dos segundos, sin cantar.");
    break;
  case 1:
    knob(DeessAmount, "QUITAR ESES");
    knob(DeessFreq, "ZONA DE ESES");
    button(DeessListen, "ESCUCHAR ESES");
    break;
  case 2:
    knob(WarmDrive, "CALOR");
    knob(WarmTone, "COLOR");
    break;
  case 3:
    knob(EQGain, "PRESENCIA");
    knob(EQFreq, "ZONA DE VOZ");
    break;
  case 4:
    knob(AirAmount, "AIRE");
    knob(AirGain, "BRILLO");
    break;
  case 5:
    knob(CompReduction, "CONTROLAR PICOS");
    knob(CompGain, "RECUPERAR NIVEL");
    break;
  case 6:
    knob(LimitCeiling, "NIVEL MAXIMO");
    knob(LimitRelease, "RECUPERACION");
    break;
  case 7:
    knob(ReverbDecay, "DURACION");
    knob(ReverbDuck, "ESPACIO AL CANTAR");
    break;
  case 8:
    knob(DelayNote, "RITMO DEL ECO");
    knob(DelayFeedback, "REPETICIONES");
    break;
  case 9:
    knob(Width, "ANCHURA");
    knob(Pan, "IZQUIERDA / DERECHA");
    break;
  case 10:
    knob(ModDepth, "MOVIMIENTO");
    knob(ModRate, "VELOCIDAD");
    break;
  case 11:
    knob(TuneAmount, "CUANTO CORRIGE");
    knob(KeepHuman, "CONSERVAR HUMANO");
    break;
  case 12:
    knob(DereverbAmount, "QUITAR HABITACION");
    knob(TransientProtect, "CUIDAR ATAQUES");
    break;
  case 13:
    knob(ResonanceAmount, "SUAVIZAR");
    knob(HarmonicProtect, "CUIDAR TIMBRE");
    break;
  case 14:
    knob(SatMid, "ARMONICOS");
    knob(SatStyle, "TIPO DE COLOR");
    break;
  case 15:
    knob(HarmonyPattern, "TIPO DE CORO");
    knob(HarmonyDetune, "HUMANIZAR CORO");
    break;
  case 16:
    knob(FetThreshold, "CUANTO APRIETA");
    knob(FetGain, "RECUPERAR NIVEL");
    break;
  case 17:
    knob(NoiseAmount, "QUITAR RUIDO");
    knob(TransientProtect, "CUIDAR VOZ");
    break;
  case 18:
    knob(PreampStyle, "TIPO DE PREVIO");
    knob(PreampGain, "EMPUJE");
    button(PreampPad, "BAJAR ENTRADA");
    button(PhantomModel, "48V VIRTUAL");
    switches.back()->setTooltip(
        "Cambia el color digital. No activa el phantom de tu interfaz.");
    break;
  case 19:
    knob(SatParallel, "COLOR EN PARALELO");
    knob(EQParallel, "EQ EN PARALELO");
    button(Punch, "MAS PEGADA");
    break;
  case 20:
    knob(VerbADecay, "DURACION CORTA");
    knob(VerbAPredelay, "SEPARAR DE VOZ");
    break;
  case 21:
    knob(VerbBDecay, "DURACION MEDIA");
    knob(VerbBPredelay, "SEPARAR DE VOZ");
    break;
  case 22:
    knob(VerbCDecay, "DURACION LARGA");
    knob(VerbCPredelay, "SEPARAR DE VOZ");
    break;
  case 23:
    knob(DelayStyle, "TIPO DE ECO");
    knob(DelayDuck, "BAJAR AL CANTAR");
    button(DelaySync, "SEGUIR TEMPO DAW");
    break;
  case 24:
    knob(SculptorAmount, "EQUILIBRAR VOZ");
    knob(SculptorTilt, "OSCURO / BRILLANTE");
    break;
  case 25:
    knob(AlignAmount, "CUANTO ALINEA");
    knob(AlignRange, "MARGEN DE TIEMPO");
    analyse.setTooltip("Conecta tu voz principal a Lead reference del "
                       "sidechain y reproduce ambas voces.");
    break;
  }
}
void RackModule::resized() {
  float sx = float(getWidth()) / 205, sy = float(getHeight()) / 318;
  auto place = [&](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
                juce::roundToInt(w * sx), juce::roundToInt(h * sy));
  };
  place(enable, 6, 5, 151, 23);
  place(analyse, 163, 7, 34, 19);
  for (size_t i = 0; i < knobs.size(); ++i)
    place(*knobs[i], 8 + int(i) * 98, 47, 89, 111);
  if (index == 0)
    place(learn, 109, 77, 85, 36);
  for (size_t i = 0; i < switches.size(); ++i)
    place(*switches[i], 10 + int(i) * 98, 168, switches.size() == 1 ? 183 : 98,
          22);
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
  for (auto &k : knobs) {
    k->setEnabled(on && available);
    k->tick();
  }
  if (processor.automatic() && !parallel.isMouseButtonDown())
    parallel.setValue(processor.effectiveValue(parallels[index]),
                      juce::dontSendNotification);
  gateDoor += .2f * ((processor.gateGR.load() < 3 ? 1.0f : 0.0f) - gateDoor);
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
       "MEZCLA PARALELA / " + juce::String(index == 7 || index == 8 ||
                                                   index == 15 ||
                                                   (index >= 20 && index <= 23)
                                               ? "ENVIO"
                                               : "SECO - EFECTO"),
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
    float reduction =
        index == 5 ? processor.reductionDB.load() : processor.fetGR.load();
    vu(g, graph.reduced(4).withTrimmedBottom(10), reduction, true);
    g.setColour(cyan.withAlpha(.18f));
    g.fillRect(graph.getX() + 5, graph.getBottom() - 8, graph.getWidth() - 10,
               4);
    g.setColour(cyan);
    g.fillRect(
        graph.getX() + 5, graph.getBottom() - 8,
        (graph.getWidth() - 10) * juce::jlimit(0.0f, 1.0f, reduction / 18), 4);
    return;
  }
  if (index == 0) {
    g.setColour(gold.withAlpha(.5f));
    float door = (1 - gateDoor) * graph.getWidth() * .48f;
    g.fillRect(graph.withWidth(door));
    g.fillRect(graph.withX(graph.getRight() - door).withWidth(door));
    text(
        g,
        processor.noiseLearnState.load() == 1
            ? "APRENDIENDO " +
                  juce::String(int(processor.noiseLearnProgress.load() * 100)) +
                  "%"
        : gateDoor > .5f ? "PUERTA ABIERTA"
                         : "RUIDO REDUCIDO",
        graph.toNearestInt(), 9, cyan);
    return;
  }
  if (index == 11) {
    float hz = processor.pitchHz.load(), target = processor.targetHz.load();
    for (int n = 0; n < 12; ++n) {
      float x = graph.getX() + 9 + n * 14;
      g.setColour(cyan.withAlpha(.25f));
      g.fillEllipse(x, graph.getY() + 16, 3, 3);
    }
    if (hz > 35 && target > 35) {
      float note = std::fmod(69 + 12 * std::log2(target / 440) + 120, 12.0f);
      float x = graph.getX() + 9 + note * 14;
      g.setColour(gold);
      g.fillEllipse(x - 3, graph.getY() + 13, 9, 9);
      float drift =
          juce::jlimit(-14.0f, 14.0f, processor.pitchCorrection.load() * .18f);
      g.setColour(cyan);
      g.drawLine(x + 1, graph.getY() + 17, x + 1, graph.getY() + 17 + drift, 2);
    }
    text(g,
         juce::String(processor.pitchCorrection.load(), 0) +
             " cents / EXPRESION VIVA",
         {12, 262, 181, 15}, 8, muted);
    return;
  }
  if (index == 3 || index == 24 || index == 13) {
    juce::Path curve;
    for (int k = 0; k < 32; ++k) {
      float x = graph.getX() + k * graph.getWidth() / 31;
      float y = graph.getBottom() - 6 -
                juce::jlimit(0.0f, 1.0f,
                             (processor.spectrum[size_t(k)].load() + 85) / 75) *
                    (graph.getHeight() - 12);
      if (k == 0)
        curve.startNewSubPath(x, y);
      else
        curve.lineTo(x, y);
    }
    g.setColour(cyan.withAlpha(.15f));
    g.strokePath(curve, juce::PathStrokeType(6));
    g.setColour(cyan);
    g.strokePath(curve, juce::PathStrokeType(1.6f));
    text(g, "ESPECTRO REAL / VOZ EN EQUILIBRIO", {12, 262, 181, 15}, 7.8f,
         muted);
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
  for (int i = 0; i < 6; ++i) {
    auto &button = bankButtons[size_t(i)];
    button.setButtonText(rack::bankName(i));
    addAndMakeVisible(button);
    button.onClick = [this, i] {
      bank = i;
      onlyFavourites = false;
      filterPresets();
    };
  }
  for (auto *c : std::initializer_list<juce::Component *>{
           &presetBox,    &analyseAll, &compareA,  &compareB,       &menu,
           &bypass,       &solo,       &status,    &classicTab,     &engineTab,
           &tripleTab,    &modeBox,    &godSpace,  &search,         &previous,
           &next,         &random,     &favourite, &favouritesOnly, &automation,
           &authorization})
    addAndMakeVisible(c);
  presetBox.setComponentID("workflow-presets");
  automation.setComponentID("alien-automation");
  search.setComponentID("preset-search");
  presetBox.onChange = [this] {
    if (presetBox.getSelectedId() > 0)
      processor.setCurrentProgram(presetBox.getSelectedId() - 1);
  };
  presetBox.setTooltip("Elige un punto de partida. Todos los presets fueron "
                       "creados por Makkine Seven Golden.");
  search.setTextToShowWhenEmpty("Buscar preset en este banco...", muted);
  search.setColour(juce::TextEditor::backgroundColourId, dark);
  search.setColour(juce::TextEditor::textColourId, bright);
  search.setColour(juce::TextEditor::outlineColourId, gold.withAlpha(.4f));
  search.setFont(
      juce::Font(juce::FontOptions(AlienSkin::bodyTypeface()).withHeight(12)));
  search.onTextChange = [this] { filterPresets(); };
  search.setTooltip(
      "Escribe una palabra para encontrar un preset en el banco seleccionado.");
  previous.onClick = [this] { navigate(-1); };
  next.onClick = [this] { navigate(1); };
  random.onClick = [this] {
    if (!visiblePresets.empty()) {
      int index =
          juce::Random::getSystemRandom().nextInt(int(visiblePresets.size()));
      if (visiblePresets.size() > 1 &&
          visiblePresets[size_t(index)] == processor.getCurrentProgram())
        index = (index + 1) % int(visiblePresets.size());
      processor.setCurrentProgram(visiblePresets[size_t(index)]);
    }
  };
  favourite.onClick = [this] {
    processor.toggleFavourite(processor.getCurrentProgram());
    filterPresets();
  };
  favourite.setTooltip("Guarda o quita este preset de tus favoritos. Se "
                       "conserva al cerrar el DAW.");
  favouritesOnly.onClick = [this] {
    onlyFavourites = !onlyFavourites;
    filterPresets();
  };
  favouritesOnly.setTooltip("Muestra solo tus favoritos de este banco. Pulsa "
                            "otra vez para ver todos.");
  automation.setClickingTogglesState(true);
  automationAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "alienautomation", automation);
  automation.setTooltip(
      "AUTO escucha y mueve los controles internos. MANUAL congela los ajustes "
      "y te deja cambiarlos. El audio sigue procesandose en ambos modos.");
  authorization.onClick = [this] { importLicense(); };
  analyseAll.onClick = [this] {
    processor.beginAnalysis(M7GProcessor::analysisAll);
  };
  analyseAll.setTooltip("Mide tres segundos de voz y propone una mezcla. AUTO "
                        "sigue adaptandola; MANUAL permite un ajuste puntual.");
  compareA.onClick = [this] { processor.selectCompare(0); };
  compareB.onClick = [this] { processor.selectCompare(1); };
  compareA.setTooltip("Comparar mezcla A");
  compareB.setTooltip("Comparar mezcla B. El menu permite copiar la mezcla "
                      "actual al otro lado.");
  menu.onClick = [this] { showMenu(); };
  classicTab.onClick = [this] { showPage(0); };
  engineTab.onClick = [this] { showPage(1); };
  tripleTab.onClick = [this] { showPage(2); };
  modeBox.addItemList(
      {"MONITOR / SIN VENTANAS", "STUDIO / 8x", "LINEAR / FIR 8x"}, 1);
  modeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          p.apvts, "enginemode", modeBox);
  modeBox.setTooltip("Deten el transporte antes de cambiar. Monitor ahorra CPU "
                     "y retardo; omite afinacion, armonias y limpieza "
                     "espectral. Studio ofrece la cadena completa.");
  bypassAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "bypass", bypass);
  soloAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "solo", solo);
  godAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "godspace", godSpace);
  input = std::make_unique<RackKnob>(p, rack::InputTrim, "ENTRADA");
  mix = std::make_unique<RackKnob>(p, rack::Mix, "VOZ / MEZCLA");
  output = std::make_unique<RackKnob>(p, rack::OutputTrim, "SALIDA");
  producer =
      std::make_unique<RackKnob>(p, rack::AutoProducer, "INTENCION ALIEN");
  for (auto *c : {input.get(), mix.get(), output.get(), producer.get()})
    addAndMakeVisible(c);
  producer->slider.setTooltip(
      "CUANTO AYUDA EL ALIEN: intensidad del autoajuste en AUTO. En MANUAL "
      "este control no cambia tus ajustes congelados.");
  status.setFont(
      juce::Font(juce::FontOptions(AlienSkin::bodyTypeface()).withHeight(11)));
  status.setColour(juce::Label::textColourId, cyan);
  status.setJustificationType(juce::Justification::centred);
  setResizable(true, false);
  setResizeLimits(1100, 775, 1920, 1350);
  setSize(1280, 900);
  filterPresets();
  showPage(0);
  startTimerHz(60);
  timerCallback();
}
M7GEditor::~M7GEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void M7GEditor::filterPresets() {
  visiblePresets.clear();
  presetBox.clear(juce::dontSendNotification);
  for (int i = 0; i < rack::presetCount; ++i) {
    auto name = rack::presets()[size_t(i)].name;
    if (rack::workflowBank(i) == bank &&
        name.containsIgnoreCase(search.getText()) &&
        (!onlyFavourites || processor.isFavourite(i))) {
      visiblePresets.push_back(i);
      presetBox.addItem(name, i + 1);
    }
  }
  presetBox.setTextWhenNothingSelected(visiblePresets.empty()
                                           ? "Sin coincidencias en este banco"
                                           : "Selecciona un preset");
  if (std::find(visiblePresets.begin(), visiblePresets.end(),
                processor.getCurrentProgram()) != visiblePresets.end())
    presetBox.setSelectedId(processor.getCurrentProgram() + 1,
                            juce::dontSendNotification);
  for (int i = 0; i < 6; ++i)
    bankButtons[size_t(i)].setToggleState(i == bank,
                                          juce::dontSendNotification);
  favouritesOnly.setToggleState(onlyFavourites, juce::dontSendNotification);
}
void M7GEditor::navigate(int direction) {
  if (visiblePresets.empty())
    return;
  auto it = std::find(visiblePresets.begin(), visiblePresets.end(),
                      processor.getCurrentProgram());
  int at = it == visiblePresets.end() ? (direction > 0 ? -1 : 0)
                                      : int(it - visiblePresets.begin());
  int count = int(visiblePresets.size());
  processor.setCurrentProgram(
      visiblePresets[size_t((at + direction + count) % count)]);
}
void M7GEditor::showPage(int selected) {
  page = selected;
  for (int i = 0; i < 26; ++i)
    modules[size_t(i)]->setVisible(
        page == 0
            ? i < 12
            : (page == 1 ? (i >= 12 && i < 18) || i >= 24 : i >= 18 && i < 24));
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
    place(*modules[size_t(i)], 10 + (slot % 6) * 211, 192 + (slot / 6) * 282,
          205, 274);
  }
  place(automation, 542, 27, 226, 34);
  place(status, 510, 64, 754, 31);
  place(analyseAll, 1050, 20, 210, 34);
  place(menu, 988, 20, 48, 34);
  place(authorization, 1018, 91, 243, 17);
  for (int i = 0; i < 6; ++i)
    place(bankButtons[size_t(i)], 10 + i * 211, 116, 205, 25);
  place(presetBox, 10, 150, 357, 29);
  place(previous, 374, 150, 27, 29);
  place(next, 406, 150, 27, 29);
  place(random, 440, 150, 68, 29);
  place(favourite, 514, 150, 64, 29);
  place(favouritesOnly, 584, 150, 76, 29);
  place(search, 667, 150, 200, 29);
  place(compareA, 880, 150, 31, 29);
  place(compareB, 915, 150, 31, 29);
  place(classicTab, 960, 148, 102, 33);
  place(engineTab, 1069, 148, 101, 33);
  place(tripleTab, 1177, 148, 91, 33);
  place(godSpace, 800, 25, 176, 34);
  place(*input, 14, 787, 85, 83);
  place(*mix, 320, 787, 85, 83);
  place(*producer, 429, 768, 119, 109);
  place(*output, 855, 787, 85, 83);
  place(bypass, 1095, 780, 155, 25);
  place(solo, 1095, 813, 155, 25);
  place(modeBox, 1095, 851, 173, 26);
}
void M7GEditor::paint(juce::Graphics &g) {
  g.fillAll(dark);
  juce::Graphics::ScopedSaveState save(g);
  g.addTransform(juce::AffineTransform::scale(float(getWidth()) / 1280,
                                              float(getHeight()) / 900));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff182b47), 640, 0, dark,
                                         640, 900, false));
  g.fillRect(0, 0, 1280, 900);
  // Static cockpit struts and instrumentation; animations reuse scalar meters.
  for (int x = 0; x < 1280; x += 80) {
    g.setColour(cyan.withAlpha(.035f));
    g.drawLine(640, 175, float(x), 760);
  }
  g.setColour(gold.withAlpha(.55f));
  g.drawLine(10, 184, 1268, 184);
  g.drawLine(10, 764, 1268, 764);
  g.setColour(cyan.withAlpha(.22f));
  g.drawEllipse(12, 11, 66, 66, 1);
  g.drawEllipse(21, 20, 48, 48, 1);
  g.drawLine(45, 44, 45 + std::cos(radarAngle) * 33,
             44 + std::sin(radarAngle) * 33, 1.5f);
  alien(g, {21, 15, 48, 62}, gold);
  g.setFont(juce::Font(
      juce::FontOptions(AlienSkin::graffitiTypeface()).withHeight(27)));
  g.setColour(gold);
  g.drawFittedText("M7G ALIEN VOICE V11", {90, 10, 411, 37},
                   juce::Justification::centredLeft, 1);
  text(g, "ALIEN INTELLIGENCE / GOLDEN EDITION", {94, 48, 408, 19}, 10, cyan,
       juce::Justification::centredLeft);
  g.setFont(juce::Font(
      juce::FontOptions(AlienSkin::graffitiTypeface()).withHeight(16)));
  g.setColour(gold);
  g.drawFittedText("LAS MAKKINAS PRODUCEN SOLAS", {93, 67, 410, 26},
                   juce::Justification::centredLeft, 1);
  text(g, "PRESETS HECHOS POR MAKKINE SEVEN GOLDEN", {94, 93, 409, 15}, 7.8f,
       muted, juce::Justification::centredLeft);
  text(g, "ALIEN AUTOMATION", {542, 7, 226, 18}, 10, gold);
  if (page != 0) {
    // Voice-driven radar in the free flight deck of the extended pages.
    float cx = page == 2 ? 640.0f : 1058.0f, cy = 607;
    float radius = page == 2 ? 108.0f : 87.0f;
    g.setColour(cyan.withAlpha(.10f));
    for (int i = 1; i <= 4; ++i)
      g.drawEllipse(cx - radius * i / 4, cy - radius * i / 4, radius * i / 2,
                    radius * i / 2, 1);
    g.drawLine(cx - radius, cy, cx + radius, cy);
    g.drawLine(cx, cy - radius, cx, cy + radius);
    float signal =
        juce::jlimit(0.0f, 1.0f, (processor.rawInputDB.load() + 60) / 45);
    g.setColour(cyan.withAlpha(.2f + signal * .7f));
    g.drawLine(cx, cy, cx + std::cos(radarAngle) * radius,
               cy + std::sin(radarAngle) * radius, 2);
    alien(g, {cx - 16, cy - 20, 32, 41}, gold.withAlpha(.5f + signal * .5f));
    text(
        g, "VOCAL RADAR / ESCUCHANDO",
        {int(cx - radius - 40), int(cy + radius + 8), int(radius * 2 + 80), 19},
        9, cyan);
    if (page == 2) {
      for (int i = 0; i < 3; ++i) {
        float y = 540 + i * 53;
        float db = processor.verbLayerDB(i);
        text(g,
             juce::String("ESPACIO ") +
                 juce::String::charToString(juce::juce_wchar('A' + i)),
             {160, int(y) - 16, 210, 18}, 10, gold);
        g.setColour(cyan.withAlpha(.12f));
        g.fillRect(150.0f, y + 7, 230.0f, 8.0f);
        g.setColour(cyan);
        g.fillRect(150.0f, y + 7,
                   230 * juce::jlimit(0.0f, 1.0f, (db + 80) / 65), 8.0f);
      }
      text(g, "ECO INTELIGENTE", {860, 537, 310, 25}, 15, gold);
      text(g,
           "Bajando al cantar: " + juce::String(processor.delDuckDB.load(), 1) +
               " dB",
           {860, 575, 310, 25}, 12, cyan);
      text(g, "360 PRESETS / UNA SOLA LICENCIA", {860, 630, 310, 25}, 10,
           muted);
    }
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
  text(g, "SALIDA / 0 VU = -18 dBFS", {568, 863, 256, 13}, 8, muted);
  double sr = processor.getSampleRate(),
         latency = sr > 0 ? processor.getLatencySamples() * 1000 / sr : 0;
  text(g,
       "V11.0.0 / " + juce::String(sr / 1000, 1) + " kHz / " +
           juce::String(latency, 1) + " ms",
       {12, 889, 620, 11}, 8, muted, juce::Justification::centredLeft);
  text(g,
       processor.licensed.load() ? "AUTHORIZED: M7G GOLDEN / TODOS LOS MODULOS"
                                 : "SIN LICENCIA / PASO DIRECTO",
       {710, 889, 558, 11}, 8, gold, juce::Justification::centredRight);
}
void M7GEditor::timerCallback() {
  // The processor owns the 20 Hz control timer, including when this editor is
  // closed.
  float level = processor.rawInputDB.load();
  int wantedRate = level > -60 || processor.outputDB.load() > -75 ? 60 : 15;
  if (wantedRate != animationRate) {
    animationRate = wantedRate;
    startTimerHz(animationRate);
  }
  if (level > -60)
    radarAngle += (0.5f + juce::jlimit(0.0f, 1.0f, (level + 60) / 45)) /
                  float(animationRate);
  status.setText(
      processor.analysisState.load() == 1
          ? "ANALIZANDO VOZ " +
                juce::String(int(processor.analysisProgress.load() * 100)) + "%"
          : processor.intelligenceText(),
      juce::dontSendNotification);
  automation.setButtonText(processor.automatic() ? "AUTO  |  manual"
                                                 : "auto  |  MANUAL");
  authorization.setButtonText(processor.licensed.load()
                                  ? "AUTHORIZED: M7G GOLDEN"
                                  : "IMPORTAR M7G.license");
  int program = processor.getCurrentProgram();
  if (program != lastProgram) {
    if (rack::workflowBank(program) != bank) {
      bank = rack::workflowBank(program);
      filterPresets();
    }
    presetBox.setSelectedId(program + 1, juce::dontSendNotification);
    lastProgram = program;
  }
  favourite.setToggleState(processor.isFavourite(program),
                           juce::dontSendNotification);
  compareA.setToggleState(processor.compareSlot() == 0,
                          juce::dontSendNotification);
  compareB.setToggleState(processor.compareSlot() == 1,
                          juce::dontSendNotification);
  for (auto &m : modules)
    if (m->isVisible())
      m->tick();
  for (auto *knob : {input.get(), mix.get(), output.get(), producer.get()})
    knob->tick();
  repaint();
}
void M7GEditor::importLicense() {
  licenseChooser = std::make_unique<juce::FileChooser>(
      "Selecciona tu licencia global M7G.license", juce::File(), "*.license");
  juce::Component::SafePointer<M7GEditor> safe(this);
  licenseChooser->launchAsync(
      juce::FileBrowserComponent::openMode |
          juce::FileBrowserComponent::canSelectFiles,
      [safe](const juce::FileChooser &chooser) {
        if (!safe || chooser.getResult() == juce::File())
          return;
        bool ok = safe->processor.importLicense(chooser.getResult());
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, "M7G GOLDEN",
            ok ? "Licencia valida. Todos los modulos autorizados para siempre."
               : "Archivo no valido. Selecciona el M7G.license firmado que "
                 "recibiste.");
      });
}
void M7GEditor::showMenu() {
  juce::PopupMenu m;
  m.addItem(1, "Restaurar preset");
  m.addItem(2, "Copiar al otro lado A/B");
  m.addItem(3, "Acerca de V11 / ayuda");
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
              "M7G ALIEN VOICE V11 - ALIEN INTELLIGENCE",
              "Hecho por Makkine Seven Golden.\nAUTO adapta; MANUAL congela "
              "los ajustes sin desactivar el audio.\nLEARN: dos segundos de "
              "ruido sin cantar.\nAlineacion: conecta la principal a Lead "
              "reference.\nProcesamiento local, sin subir tu voz.\n48V es un "
              "modelo digital.\nModelos entrenados con senales sinteticas; sin "
              "equivalencia certificada con motores propietarios.\nPruebas de "
              "escucha y Studio One 7 pendientes.");
      });
}
