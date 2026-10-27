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
namespace {
const juce::Colour gold{0xffd5b66d}, bright{0xfff1dca5}, dark{0xff0c1011},
    muted{0xff9b926f};
constexpr const char *titles[]{"GATE",        "DE-ESS AI",  "WARMTH TUBE",
                               "PRESENCE EQ", "AIR",        "OPTO COMPRESS",
                               "LIMITER",     "REVERB",     "DELAY",
                               "STEREO",      "MODULATION", "PITCH"};
constexpr rack::Param enables[]{rack::GateOn,   rack::DeessOn,  rack::WarmOn,
                                rack::EQOn,     rack::AirOn,    rack::CompOn,
                                rack::LimitOn,  rack::ReverbOn, rack::DelayOn,
                                rack::StereoOn, rack::ModOn,    rack::TuneOn};
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
  const auto &spec = specs()[size_t(p)];
  juce::String suffix(spec.suffix);
  if (suffix.contains("Hz") && value >= 1000)
    return juce::String(value / 1000, value < 10000 ? 1 : 0) + " kHz";
  int decimals = (suffix == " dB" || p == EQQ || p == CompRatio ||
                  p == ReverbDecay || p == ModRate)
                     ? 1
                     : 0;
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
  text(g, b.getButtonText(), b.getLocalBounds().reduced(2),
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
                id == rack::PitchKey || id == rack::PitchScale;
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
  analyse.setTooltip("Analizar este módulo durante 3 segundos de reproducción");
  analyse.onClick = [this] { processor.beginAnalysis(index); };
  enableAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, rack::specs()[size_t(enabledParameter)].id, enable);
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
    button(NoiseOn, "NEURAL CLEAN");
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
    break;
  case 4:
    knob(AirAmount, "AIR");
    knob(AirGain, "SHINE");
    knob(AirShimmer, "SHIMMER");
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
    knob(ReverbMix, "SEND");
    knob(ReverbDecay, "DECAY");
    knob(ReverbSize, "SIZE");
    knob(ReverbPredelay, "PRE-DELAY");
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
    break;
  }
}
void RackModule::resized() {
  float sx = float(getWidth()) / 205, sy = float(getHeight()) / 294;
  auto place = [&](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
                juce::roundToInt(w * sx), juce::roundToInt(h * sy));
  };
  place(enable, 8, 5, 151, 23);
  place(analyse, 165, 7, 32, 19);
  for (size_t i = 0; i < knobs.size(); ++i)
    place(*knobs[i], 8 + int(i % 2) * 98, 35 + int(i / 2) * 72, 89, 70);
  for (size_t i = 0; i < switches.size(); ++i) {
    if (index == 6)
      place(*switches[i], 12, 124 + int(i) * 26, 181, 23);
    else
      place(*switches[i], index == 1 ? 102 : 12, index == 1 ? 133 : 269,
            index == 1 ? 95 : 181, 22);
  }
}
void RackModule::tick() {
  for (size_t i = 0; i + 1 < history.size(); ++i)
    history[i] = history[i + 1];
  history.back() = processor.inputDB.load();
  bool on = processor.value(enabledParameter) > .5f;
  bool bus = index == 11 && int(processor.value(rack::VoiceType)) == 5;
  enable.setEnabled(!bus);
  for (auto &knob : knobs)
    knob->setEnabled(on && !bus);
  for (auto &b : switches)
    b->setEnabled(on);
  if (index == 0 && !switches.empty())
    switches[0]->setEnabled(int(processor.value(rack::VoiceType)) != 5);
  analyse.setToggleState(processor.analysisState.load() == 1 &&
                             processor.analysisTarget() == index,
                         juce::dontSendNotification);
  repaint();
}
void RackModule::paint(juce::Graphics &g) {
  juce::Graphics::ScopedSaveState scope(g);
  g.addTransform(juce::AffineTransform::scale(float(getWidth()) / 205,
                                              float(getHeight()) / 294));
  constexpr int W = 205, H = 294;
  auto r = juce::Rectangle<float>(0, 0, float(W), float(H)).reduced(.5f);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff242826), 0, 0,
                                         juce::Colour(0xff0d1112), float(W),
                                         float(H), false));
  g.fillRoundedRectangle(r, 4);
  g.setColour(gold.withAlpha(.65f));
  g.drawRoundedRectangle(r, 4, 1);
  g.setColour(gold.withAlpha(.23f));
  g.drawLine(8, 31, float(W - 8), 31);
  for (int y = 34; y < H; y += 3) {
    g.setColour(juce::Colours::white.withAlpha(.012f));
    g.drawHorizontalLine(y, 4, float(W - 4));
  }
  int y = knobs.size() > 4 ? 258 : 206;
  auto graph = juce::Rectangle<float>(
      11, float(y), float(W - 22),
      knobs.size() > 4 ? 24.0f : (index == 6 ? 65.0f : 49.0f));
  g.setColour(juce::Colour(0xff080c0d));
  g.fillRoundedRectangle(graph, 2);
  g.setColour(gold.withAlpha(.2f));
  g.drawRoundedRectangle(graph, 2, 1);
  if (index == 5) {
    vu(g, graph.reduced(4), processor.reductionDB.load(), true);
    return;
  }
  if (index == 2) {
    auto tube =
        juce::Rectangle<float>(graph.getX() + 15, graph.getY() + 3, 30, 43);
    float glow = juce::jlimit(.1f, 1.0f, (processor.inputDB.load() + 55) / 40) *
                 processor.value(rack::WarmDrive) * .01f;
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xffffb14d).withAlpha(glow * .8f), tube.getCentreX(),
        tube.getCentreY(), juce::Colours::transparentBlack,
        tube.getRight() + 14, tube.getBottom() + 10, true));
    g.fillEllipse(tube.expanded(17, 5));
    g.setColour(gold.withAlpha(.6f));
    g.drawRoundedRectangle(tube, 9, 1);
    g.drawLine(tube.getCentreX() - 4, tube.getY() + 8, tube.getCentreX() - 4,
               tube.getBottom() - 7, 2);
    g.drawLine(tube.getCentreX() + 4, tube.getY() + 8, tube.getCentreX() + 4,
               tube.getBottom() - 7, 2);
    text(g, "EVEN + ODD\n4x COLOUR", graph.withTrimmedLeft(57).toNearestInt(),
         9);
    return;
  }
  if (index == 3) {
    g.setColour(gold.withAlpha(.14f));
    for (int i = 1; i < 4; ++i)
      g.drawHorizontalLine(int(graph.getY() + graph.getHeight() * float(i) / 4),
                           graph.getX(), graph.getRight());
    auto coeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        juce::jmax(8000.0, processor.getSampleRate()),
        processor.value(rack::EQFreq), processor.value(rack::EQQ),
        juce::Decibels::decibelsToGain(processor.value(rack::EQGain)));
    juce::Path line;
    for (int i = 0; i < int(graph.getWidth()); ++i) {
      double f = 120 * std::pow(150.0, double(i) / graph.getWidth());
      float db =
          float(20 * std::log10(coeff->getMagnitudeForFrequency(
                         juce::jmin(f, processor.getSampleRate() * .47),
                         juce::jmax(8000.0, processor.getSampleRate()))));
      float yy = graph.getCentreY() - db * 2;
      if (i == 0)
        line.startNewSubPath(graph.getX() + i, yy);
      else
        line.lineTo(graph.getX() + i, yy);
    }
    g.setColour(gold);
    g.strokePath(line, juce::PathStrokeType(1.6f));
    return;
  }
  if (index == 1 || index == 4) {
    int bars = 24;
    for (int k = 0; k < bars; ++k) {
      float level = processor.spectrum[size_t(k + 8)].load();
      float amount = juce::jlimit(0.0f, 1.0f, (level + 80) / 70);
      auto col = index == 1
                     ? juce::Colour::fromHSV(float(k) / 32, .5f, .8f, .75f)
                     : gold.withAlpha(.75f);
      g.setColour(col);
      float w = graph.getWidth() / bars;
      g.fillRect(graph.getX() + k * w + 1,
                 graph.getBottom() - amount * (graph.getHeight() - 5),
                 juce::jmax(1.0f, w - 2), amount * (graph.getHeight() - 5));
    }
    if (index == 1)
      text(g, "GR " + juce::String(processor.deEssDB.load(), 1) + " dB",
           {12, 260, W - 24, 22}, 9);
    return;
  }
  if (index == 6) {
    float level =
        juce::jlimit(0.0f, 1.0f, (processor.outputDB.load() + 48) / 48);
    for (int i = 0; i < 22; ++i) {
      g.setColour(i < level * 22 ? gold : gold.withAlpha(.12f));
      g.fillRect(graph.getX() + 6 + i * (graph.getWidth() - 12) / 22,
                 graph.getY() + 12, (graph.getWidth() - 12) / 22 - 2, 15.0f);
    }
    text(g, "GR  " + juce::String(processor.limitGR.load(), 1) + " dB",
         graph.withTrimmedTop(31).toNearestInt(), 10);
    return;
  }
  if (index == 7 || index == 8) {
    float duck =
        index == 7 ? processor.revDuckDB.load() : processor.delDuckDB.load();
    float value = juce::jlimit(0.0f, 1.0f, duck / 24);
    g.setColour(gold.withAlpha(.25f));
    g.fillRect(graph.reduced(6, 5));
    g.setColour(gold);
    g.fillRect(graph.reduced(6, 5).withWidth((graph.getWidth() - 12) * value));
    text(g, "DUCK  -" + juce::String(duck, 1) + " dB",
         {W / 2, 190, W / 2 - 8, 48}, 9);
    return;
  }
  if (index == 9) {
    float value = processor.correlation.load();
    g.setColour(gold.withAlpha(.35f));
    g.drawLine(graph.getX() + 8, graph.getCentreY(), graph.getRight() - 8,
               graph.getCentreY(), 2);
    float xx = graph.getX() + 8 + (graph.getWidth() - 16) * (value + 1) * .5f;
    g.setColour(value < 0 ? juce::Colours::orangered : gold);
    g.fillEllipse(xx - 3, graph.getCentreY() - 3, 6, 6);
    text(g, "CORRELATION  " + juce::String(value, 2), {12, 260, W - 24, 22}, 9);
    return;
  }
  if (index == 10) {
    juce::Path line;
    for (int i = 0; i < int(graph.getWidth()); ++i) {
      float v =
          std::sin(juce::MathConstants<float>::twoPi *
                   (float(i) / graph.getWidth() + processor.modPhase.load()));
      float yy = graph.getCentreY() - v * graph.getHeight() * .35f;
      if (i == 0)
        line.startNewSubPath(graph.getX(), yy);
      else
        line.lineTo(graph.getX() + i, yy);
    }
    g.setColour(gold);
    g.strokePath(line, juce::PathStrokeType(1.5f));
    return;
  }
  if (index == 11) {
    float hz = processor.pitchHz.load(), target = processor.targetHz.load();
    bool bus = int(processor.value(rack::VoiceType)) == 5;
    juce::String s = bus ? "BUS SAFE"
                         : (hz > 35 ? juce::String(hz, 1) + " > " +
                                          juce::String(target, 1) + " Hz"
                                    : "WAITING FOR VOICE");
    text(g, s, graph.toNearestInt(), 10);
    text(g, bus ? "TUNE INDIVIDUAL TRACKS" : "MONOPHONIC TRACKING",
         {12, 261, W - 24, 20}, 8);
    return;
  }
  juce::Path line;
  for (size_t i = 0; i < history.size(); ++i) {
    float xx = graph.getX() +
               float(i) * graph.getWidth() / float(history.size() - 1),
          yy = graph.getBottom() -
               juce::jlimit(0.0f, 1.0f, (history[i] + 70) / 70) *
                   (graph.getHeight() - 6) -
               3;
    if (i == 0)
      line.startNewSubPath(xx, yy);
    else
      line.lineTo(xx, yy);
  }
  g.setColour(gold);
  g.strokePath(line, juce::PathStrokeType(1.3f));
}
M7GEditor::M7GEditor(M7GProcessor &p) : AudioProcessorEditor(p), processor(p) {
  setLookAndFeel(&skin);
  for (int i = 0; i < 12; ++i) {
    modules[size_t(i)] = std::make_unique<RackModule>(p, i);
    addAndMakeVisible(*modules[size_t(i)]);
  }
  int lastRole = -1;
  for (size_t i = 0; i < rack::presets().size(); ++i) {
    const auto &preset = rack::presets()[i];
    if (preset.role != lastRole) {
      presetBox.addSectionHeading(M7GProcessor::action(preset.role));
      lastRole = preset.role;
    }
    presetBox.addItem(preset.name, int(i) + 1);
  }
  presetBox.setSelectedId(p.getCurrentProgram() + 1,
                          juce::dontSendNotification);
  presetBox.setTooltip("50 presets organizados por función vocal");
  presetBox.onChange = [this] {
    processor.setCurrentProgram(presetBox.getSelectedId() - 1);
  };
  for (auto *c : std::array<juce::Component *, 9>{
           &presetBox, &analyseAll, &compareA, &compareB, &menu, &bypass, &solo,
           &autoLevel, &status})
    addAndMakeVisible(c);
  analyseAll.onClick = [this] { processor.beginAnalysis(12); };
  analyseAll.setTooltip(
      "Analiza 3 segundos de la señal y ajusta toda la cadena");
  compareA.onClick = [this] { processor.selectCompare(0); };
  compareB.onClick = [this] { processor.selectCompare(1); };
  compareA.setTooltip(
      "Comparación A: guarda y recupera una configuración completa");
  compareB.setTooltip(
      "Comparación B: guarda y recupera una configuración completa");
  menu.onClick = [this] { showMenu(); };
  bypassAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "bypass", bypass);
  soloAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "solo", solo);
  autoAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.apvts, "autogain", autoLevel);
  solo.setTooltip(
      "Escucha la diferencia entre la señal original y la procesada");
  input = std::make_unique<RackKnob>(p, rack::InputTrim, "INPUT");
  mix = std::make_unique<RackKnob>(p, rack::Mix, "MIX");
  output = std::make_unique<RackKnob>(p, rack::OutputTrim, "OUTPUT");
  for (auto *c : {input.get(), mix.get(), output.get()})
    addAndMakeVisible(c);
  status.setColour(juce::Label::textColourId, muted);
  status.setFont(
      juce::Font(juce::FontOptions(AlienSkin::bodyTypeface()).withHeight(10)));
  status.setJustificationType(juce::Justification::centredRight);
  status.setText("50 PRESETS / 12 MODULES / LOCAL PROCESSING",
                 juce::dontSendNotification);
  setResizable(true, false);
  setResizeLimits(1000, 625, 1920, 1200);
  setSize(1280, 800);
  startTimerHz(25);
  timerCallback();
}
M7GEditor::~M7GEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void M7GEditor::resized() {
  float sx = float(getWidth()) / 1280, sy = float(getHeight()) / 800;
  auto place = [&](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
                juce::roundToInt(w * sx), juce::roundToInt(h * sy));
  };
  for (int i = 0; i < 12; ++i)
    place(*modules[size_t(i)], 10 + (i % 6) * 211, 88 + (i / 6) * 300, 205,
          294);
  place(presetBox, 520, 20, 310, 29);
  place(compareA, 844, 20, 30, 29);
  place(compareB, 877, 20, 30, 29);
  place(analyseAll, 920, 20, 185, 29);
  place(menu, 1118, 20, 40, 29);
  place(autoLevel, 1168, 20, 102, 29);
  place(status, 510, 54, 758, 22);
  place(*input, 16, 697, 85, 80);
  place(*mix, 390, 697, 85, 80);
  place(*output, 854, 697, 85, 80);
  place(bypass, 1095, 706, 100, 25);
  place(solo, 1095, 739, 150, 25);
}
void M7GEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff0b1012));
  g.saveState();
  g.addTransform(juce::AffineTransform::scale(float(getWidth()) / 1280,
                                              float(getHeight()) / 800));
  g.setGradientFill(juce::ColourGradient(
      juce::Colour(0xff272a28), 0, 0, juce::Colour(0xff090d10), 0, 800, false));
  g.fillRect(0, 0, 1280, 800);
  for (int y = 0; y < 800; y += 3) {
    g.setColour(juce::Colours::white.withAlpha(.012f));
    g.drawHorizontalLine(y, 0, 1280);
  }
  g.setColour(gold.withAlpha(.7f));
  g.drawHorizontalLine(81, 10, 1270);
  g.drawHorizontalLine(686, 10, 1270);
  g.drawHorizontalLine(781, 10, 1270);
  juce::Path shield;
  shield.startNewSubPath(23, 18);
  shield.lineTo(75, 18);
  shield.lineTo(79, 47);
  shield.lineTo(49, 72);
  shield.lineTo(19, 47);
  shield.closeSubPath();
  g.setColour(gold.withAlpha(.1f));
  g.fillPath(shield);
  g.setColour(gold);
  g.strokePath(shield, juce::PathStrokeType(1.6f));
  text(g, "M7G", {22, 26, 55, 25}, 21, bright);
  text(g, "ALIEN", {23, 50, 52, 12}, 7, gold);
  g.setFont(
      juce::Font(juce::FontOptions(AlienSkin::titleTypeface()).withHeight(32)));
  g.setColour(bright);
  g.drawText("M7G ALIEN VOICE", 95, 14, 414, 40,
             juce::Justification::centredLeft);
  text(g, "VOCAL RACK / NEURAL CLEAN + ADAPTIVE DSP", {97, 55, 400, 17}, 9,
       muted, juce::Justification::centredLeft);
  auto meters = [&](float x, float y, float db, int width) {
    float amount = juce::jlimit(0.0f, 1.0f, (db + 60) / 60);
    for (int i = 0; i < 42; ++i) {
      g.setColour(i < amount * 42 ? (i > 35 ? juce::Colour(0xffe7b557) : gold)
                                  : gold.withAlpha(.13f));
      g.fillRect(x + i * float(width) / 42, y, float(width) / 42 - 2, 9.0f);
    }
    text(g, juce::String(db, 1) + " dBFS", {int(x), int(y) + 12, width, 18}, 9,
         muted, juce::Justification::centredRight);
  };
  meters(112, 724, processor.inputDB.load(), 260);
  meters(944, 724, processor.outputDB.load(), 135);
  vu(g, {495, 701, 159, 65}, processor.outputChannels[0].load());
  vu(g, {667, 701, 159, 65}, processor.outputChannels[1].load());
  text(g, "OUTPUT / 0 VU = -18 dBFS", {495, 768, 331, 12}, 8, muted);
  text(g, "4x OVERSAMPLE", {1125, 761, 130, 16}, 8, gold);
  double sr = processor.getSampleRate();
  double latency = sr > 0 ? 1000 * processor.getLatencySamples() / sr : 0;
  text(g,
       "M7G AUDIO  /  v0.3.0  /  " + juce::String(sr / 1000, 1) + " kHz  /  " +
           juce::String(latency, 1) + " ms",
       {12, 783, 650, 15}, 8, muted, juce::Justification::centredLeft);
  text(g,
       "MODE: " +
           juce::String(
               M7GProcessor::action(int(processor.value(rack::VoiceType)))) +
           "  /  VOCAL ENGINE",
       {760, 783, 505, 15}, 8, muted, juce::Justification::centredRight);
  g.restoreState();
}
void M7GEditor::timerCallback() {
  processor.applyAnalysis();
  int state = processor.analysisState.load();
  if (state == 1) {
    int target = processor.analysisTarget();
    status.setText(
        juce::String("ANALIZANDO ") +
            (target == 12 ? "TODA LA CADENA" : titles[target]) + " / " +
            juce::String(int(processor.analysisProgress.load() * 100)) +
            "% / REPRODUCE LA VOZ",
        juce::dontSendNotification);
  } else if (state < 0)
    status.setText(
        "SIN SEÑAL SUFICIENTE / REPRODUCE UNA VOZ Y VUELVE A ANALIZAR",
        juce::dontSendNotification);
  else
    status.setText("50 PRESETS / 12 MODULES / " +
                       juce::String(M7GProcessor::action(
                           int(processor.value(rack::VoiceType)))),
                   juce::dontSendNotification);
  presetBox.setSelectedId(processor.getCurrentProgram() + 1,
                          juce::dontSendNotification);
  compareA.setToggleState(processor.compareSlot() == 0,
                          juce::dontSendNotification);
  compareB.setToggleState(processor.compareSlot() == 1,
                          juce::dontSendNotification);
  analyseAll.setToggleState(state == 1 && processor.analysisTarget() == 12,
                            juce::dontSendNotification);
  for (auto &m : modules)
    m->tick();
  repaint();
}
void M7GEditor::showMenu() {
  juce::PopupMenu menuItems;
  menuItems.addItem(1, "Restaurar el preset seleccionado");
  menuItems.addItem(2, "Copiar configuración al otro slot A/B");
  menuItems.addSeparator();
  menuItems.addItem(3, "Acerca de M7G ALIEN VOICE");
  juce::Component::SafePointer<M7GEditor> safe(this);
  menuItems.showMenuAsync(
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
              juce::MessageBoxIconType::InfoIcon, "M7G ALIEN VOICE",
              "12 módulos / 50 presets\nIA local RNNoise + análisis DSP "
              "adaptativo.\nLos botones AI miden 3 segundos de señal "
              "reproducida.\nLa afinación trabaja con voces "
              "monofónicas.\nDiseño original; no es una emulación certificada "
              "de hardware.");
      });
}
