#include "PluginEditor.h"
#include "ScratchData.h"
namespace {
const juce::Colour navy{0xff0a0f1e}, gold{0xffffd700}, blue{0xff4a99ff},
    cyan{0xff63d9ec}, purple{0xffba8cff}, white{0xffe7edf7}, muted{0xff91a0b8};
juce::Colour colour(int bank) {
  return bank == 0 ? gold : bank == 1 ? cyan : purple;
}
void label(juce::Graphics &g, const juce::String &s, juce::Rectangle<int> r,
           float h, juce::Colour c = white,
           int align = juce::Justification::centred) {
  g.setColour(c);
  g.setFont(juce::Font(ScratchSkin::body()).withHeight(h));
  g.drawFittedText(s, r, align, 1);
}
void alien(juce::Graphics &g, juce::Rectangle<float> r, juce::Colour c) {
  juce::Graphics::ScopedSaveState save(g);
  g.addTransform(
      juce::AffineTransform::scale(r.getWidth() / 40, r.getHeight() / 48)
          .translated(r.getX(), r.getY()));
  juce::Path p;
  p.startNewSubPath(20, 47);
  p.cubicTo(0, 33, -4, 12, 8, 4);
  p.cubicTo(16, -1, 24, -1, 32, 4);
  p.cubicTo(44, 12, 40, 33, 20, 47);
  g.setColour(c.withAlpha(.09f));
  g.fillPath(p);
  g.setColour(c);
  g.strokePath(p, juce::PathStrokeType(1.1f));
  g.fillEllipse(6, 16, 12, 6);
  g.fillEllipse(22, 16, 12, 6);
  g.drawLine(17, 36, 23, 36, 1.3f);
}
} // namespace
juce::Typeface::Ptr ScratchSkin::body() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      ScratchData::DejaVuSans_ttf, ScratchData::DejaVuSans_ttfSize);
  return f;
}
juce::Typeface::Ptr ScratchSkin::graffiti() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      ScratchData::RubikWetPaintRegular_ttf,
      ScratchData::RubikWetPaintRegular_ttfSize);
  return f;
}
ScratchSkin::ScratchSkin() {
  setColour(juce::ListBox::backgroundColourId, navy);
  setColour(juce::Slider::textBoxTextColourId, white);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::transparentBlack);
  setColour(juce::PopupMenu::backgroundColourId, navy);
  setColour(juce::PopupMenu::textColourId, white);
}
void ScratchSkin::drawRotarySlider(juce::Graphics &g, int x, int y, int w,
                                   int h, float pos, float start, float end,
                                   juce::Slider &slider) {
  if (slider.getProperties().contains("visualPosition"))
    pos = float(slider.getProperties()["visualPosition"]);
  auto r = juce::Rectangle<float>(float(x), float(y), float(w), float(h))
               .withSizeKeepingCentre(float(juce::jmin(w, h) - 16),
                                      float(juce::jmin(w, h) - 16));
  auto c = r.getCentre();
  g.setColour(juce::Colours::black.withAlpha(.7f));
  g.fillEllipse(r.expanded(5).translated(1, 4));
  g.setColour(gold.withAlpha(.35f));
  g.drawEllipse(r.expanded(5), 1.0f);
  juce::Path path;
  path.addCentredArc(c.x, c.y, r.getWidth() * .5f + 5, r.getHeight() * .5f + 5,
                     0, start, start + pos * (end - start), true);
  g.setColour(gold);
  g.strokePath(path, juce::PathStrokeType(2.2f));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe4ca6d), r.getX(),
                                         r.getY(), juce::Colour(0xff152038),
                                         r.getRight(), r.getBottom(), false));
  g.fillEllipse(r);
  g.setColour(juce::Colour(0xff776832));
  g.drawEllipse(r, 1.5f);
  auto inner = r.reduced(5);
  g.setGradientFill(juce::ColourGradient(
      juce::Colour(0xff3b4148), inner.getX(), inner.getY(),
      juce::Colour(0xff0a1020), inner.getRight(), inner.getBottom(), false));
  g.fillEllipse(inner);
  float angle = start + pos * (end - start);
  auto tip = c + juce::Point<float>(std::sin(angle), -std::cos(angle)) *
                     r.getWidth() * .37f;
  auto base = c + juce::Point<float>(std::sin(angle), -std::cos(angle)) *
                      r.getWidth() * .19f;
  g.setColour(gold);
  g.drawLine({base, tip}, 2.3f);
}
void ScratchSkin::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                       const juce::Colour &, bool over,
                                       bool down) {
  auto r = b.getLocalBounds().toFloat().reduced(.5f);
  auto top =
      b.getToggleState() ? juce::Colour(0xff554a1f) : juce::Colour(0xff15243d);
  g.setGradientFill(juce::ColourGradient(top, r.getX(), r.getY(), navy,
                                         r.getX(), r.getBottom(), false));
  g.fillRoundedRectangle(r, 5);
  g.setColour(gold.withAlpha(down || over         ? .95f
                             : b.getToggleState() ? .8f
                                                  : .35f));
  g.drawRoundedRectangle(r, 5, b.getToggleState() ? 1.5f : 1.0f);
}
void ScratchSkin::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool,
                                 bool) {
  label(g, b.getButtonText(), b.getLocalBounds().reduced(3),
        b.getHeight() > 40 ? 14.0f : 10.5f, b.getToggleState() ? gold : white);
}
ScratchKnob::ScratchKnob(ScratchProcessor &p, const char *id, juce::String name,
                         juce::String tip)
    : caption(std::move(name)) {
  addAndMakeVisible(slider);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                             juce::MathConstants<float>::pi * 2.8f, true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 21);
  slider.setTooltip(tip);
  attachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.state, id, slider);
  if (juce::String(id) == "releaseTime" || juce::String(id) == "echoDelay")
    slider.setTextValueSuffix(" ms");
  else if (juce::String(id) == "speed")
    slider.setTextValueSuffix(" x");
  else if (juce::String(id) == "transformerRate")
    slider.setTextValueSuffix(" Hz");
}
void ScratchKnob::resized() {
  slider.setBounds(0, 22, getWidth(), getHeight() - 22);
}
void ScratchKnob::paint(juce::Graphics &g) {
  label(g, caption, getLocalBounds().withHeight(22), 10, gold);
}
void ScratchKnob::animate() {
  float target = float(slider.valueToProportionOfLength(slider.getValue()));
  if (display < 0)
    display = target;
  display += (target - display) * .24f;
  slider.getProperties().set("visualPosition", display);
  slider.repaint();
}
ScratchPlatter::~ScratchPlatter() {
  if (dragging) {
    processor.setValue("touch", 0);
    if (auto *p = processor.state.getParameter("position"))
      p->endChangeGesture();
  }
}
void ScratchPlatter::mouseDown(const juce::MouseEvent &e) {
  if (!processor.hasWord.load())
    return;
  auto c = getLocalBounds().toFloat().getCentre();
  angle = std::atan2(e.position.y - c.y, e.position.x - c.x);
  target = processor.platterPosition.load();
  dragging = true;
  if (auto *p = processor.state.getParameter("position"))
    p->beginChangeGesture();
  processor.setValue("position", target);
  processor.setValue("touch", 1);
}
void ScratchPlatter::mouseDrag(const juce::MouseEvent &e) {
  if (!dragging)
    return;
  auto c = getLocalBounds().toFloat().getCentre();
  float next = std::atan2(e.position.y - c.y, e.position.x - c.x);
  target = std::clamp(
      target + std::remainder(next - angle, juce::MathConstants<float>::twoPi) /
                   juce::MathConstants<float>::twoPi,
      0.f, 1.f);
  angle = next;
  processor.setValue("position", target);
}
void ScratchPlatter::mouseUp(const juce::MouseEvent &) {
  if (!dragging)
    return;
  dragging = false;
  processor.setValue("touch", 0);
  if (auto *p = processor.state.getParameter("position"))
    p->endChangeGesture();
}
void ScratchPlatter::paint(juce::Graphics &g) {
  auto r = getLocalBounds().toFloat().reduced(16);
  float side = std::min(r.getWidth(), r.getHeight());
  r = r.withSizeKeepingCentre(side, side);
  auto c = r.getCentre();
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3f454a), r.getX(),
                                         r.getY(), juce::Colour(0xff05070d),
                                         r.getRight(), r.getBottom(), false));
  g.fillEllipse(r);
  g.setColour(gold.withAlpha(.65f));
  g.drawEllipse(r, 2);
  g.setColour(juce::Colours::black);
  g.fillEllipse(r.reduced(9));
  for (float radius = side * .22f; radius < side * .47f; radius += 3.4f) {
    g.setColour(juce::Colour(0xff273042).withAlpha(.55f));
    g.drawEllipse(c.x - radius, c.y - radius, radius * 2, radius * 2, .55f);
  }
  auto inner = r.withSizeKeepingCentre(side * .39f, side * .39f);
  g.setGradientFill(juce::ColourGradient(
      juce::Colour(0xff776125), inner.getX(), inner.getY(),
      juce::Colour(0xff19243a), inner.getRight(), inner.getBottom(), false));
  g.fillEllipse(inner);
  g.setColour(gold);
  g.drawEllipse(inner, 1);
  float angle =
      processor.platterPosition.load() * juce::MathConstants<float>::twoPi -
      juce::MathConstants<float>::halfPi;
  auto point =
      c + juce::Point<float>(std::cos(angle), std::sin(angle)) * side * .43f;
  g.drawLine(
      {c + juce::Point<float>(std::cos(angle), std::sin(angle)) * side * .25f,
       point},
      3);
  g.fillEllipse(point.x - 5, point.y - 5, 10, 10);
  label(g, "M7G", {int(inner.getX()), int(c.y - 38), int(inner.getWidth()), 32},
        24, gold);
  label(g, "WORD SCRATCHER",
        {int(inner.getX()), int(c.y - 3), int(inner.getWidth()), 18}, 10,
        white);
  label(g, processor.value("vinylMode") > .5f ? "VINYL" : "DIGITAL",
        {int(inner.getX()), int(c.y + 23), int(inner.getWidth()), 18}, 11,
        cyan);
  g.setColour(juce::Colours::black);
  g.fillEllipse(c.x - 4, c.y - 4, 8, 8);
  label(g,
        processor.hasWord.load() ? "ARRASTRA EL DISCO" : "CAPTURA UNA PALABRA",
        {0, getHeight() - 20, getWidth(), 18}, 11, muted);
}
namespace {
class ScratchAdvanced final : public juce::Component {
public:
  ScratchAdvanced(ScratchProcessor &p) {
    const char *ids[]{"transformerRate", "drive",      "depth",  "duration",
                      "output",          "noiseFloor", "variant"};
    const char *names[]{"CORTES / Hz a 120 BPM", "SATURACION",
                        "RECORRIDO %",           "GESTO / ms",
                        "SALIDA / dB",           "RUIDO APRENDIDO / dB",
                        "VARIACION DEL GESTO"};
    for (int i = 0; i < 7; ++i) {
      labels[size_t(i)].setText(names[i], juce::dontSendNotification);
      labels[size_t(i)].setColour(juce::Label::textColourId, gold);
      addAndMakeVisible(labels[size_t(i)]);
      auto &slider = sliders[size_t(i)];
      slider.setSliderStyle(juce::Slider::LinearHorizontal);
      slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 68, 24);
      addAndMakeVisible(slider);
      attachments[size_t(i)] = std::make_unique<
          juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, ids[i],
                                                                slider);
    }
    setSize(440, 7 * 64 + 16);
  }
  void resized() override {
    for (int i = 0; i < 7; ++i) {
      labels[size_t(i)].setBounds(10, 10 + i * 64, 420, 20);
      sliders[size_t(i)].setBounds(10, 31 + i * 64, 420, 28);
    }
  }
  void paint(juce::Graphics &g) override { g.fillAll(navy); }

private:
  std::array<juce::Label, 7> labels;
  std::array<juce::Slider, 7> sliders;
  std::array<
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 7>
      attachments;
};
} // namespace
ScratchEditor::ScratchEditor(ScratchProcessor &processor)
    : AudioProcessorEditor(processor), p(processor), platter(p) {
  setLookAndFeel(&skin);
  setOpaque(true);
  addAndMakeVisible(advanced);
  for (auto *c : std::initializer_list<juce::Component *>{
           &presets,    &search,      &previous, &next,    &random,
           &favourite,  &a,           &b,        &copy,    &auth,
           &bypass,     &capture,     &play,     &stop,    &learn,
           &automatic,  &smart,       &hold,     &cut,     &autoFader,
           &reverse,    &mode,        &curve,    &pattern, &rpm,
           &crossfader, &sensitivity, &gap,      &platter, &description})
    addAndMakeVisible(c);
  presets.setComponentID("preset-combo");
  search.setComponentID("preset-search");
  platter.setComponentID("vinyl-platter");
  crossfader.setComponentID("scratch-fader");
  presets.setTextWhenNothingSelected("Elige entre 300 presets");
  search.setTextToShowWhenEmpty("Buscar scratch...", muted);
  search.setFont(juce::Font(ScratchSkin::body()).withHeight(12));
  search.setColour(juce::TextEditor::backgroundColourId, navy);
  search.setColour(juce::TextEditor::textColourId, white);
  search.onTextChange = [this] { filter(); };
  presets.onChange = [this] {
    if (!filtering && presets.getSelectedId() > 0) {
      p.setCurrentProgram(presets.getSelectedId() - 1);
      refresh();
    }
  };
  previous.onClick = [this] { navigate(-1); };
  next.onClick = [this] { navigate(1); };
  random.onClick = [this] {
    if (filtered.empty())
      return;
    int i = juce::Random::getSystemRandom().nextInt(int(filtered.size()));
    if (filtered.size() > 1 && filtered[size_t(i)] == p.getCurrentProgram())
      i = (i + 1) % int(filtered.size());
    p.setCurrentProgram(filtered[size_t(i)]);
    refresh();
  };
  favourite.onClick = [this] {
    p.toggleFavourite(p.getCurrentProgram());
    refresh();
  };
  a.onClick = [this] {
    p.selectCompare(0);
    refresh();
  };
  b.onClick = [this] {
    p.selectCompare(1);
    refresh();
  };
  copy.onClick = [this] { p.copyCompare(); };
  auth.onClick = [this] { importLicense(); };
  advanced.onClick = [this] {
    if (!advancedPanel) {
      advancedPanel = std::make_unique<ScratchAdvanced>(p);
      addAndMakeVisible(*advancedPanel);
      advancedPanel->setBounds(495, 385, 440, 464);
      advancedPanel->setTransform(juce::AffineTransform::scale(
          getWidth() / 1440.f, getHeight() / 940.f));
      advancedPanel->setVisible(false);
    }
    advancedPanel->setVisible(!advancedPanel->isVisible());
    advanced.setToggleState(advancedPanel->isVisible(),
                            juce::dontSendNotification);
    if (advancedPanel->isVisible())
      advancedPanel->toFront(false);
  };
  capture.onClick = [this] { p.captureWord(); };
  play.onClick = [this] { p.playScratch(); };
  stop.onClick = [this] { p.stopScratch(); };
  learn.onClick = [this] { p.learnNoise(); };
  auto button = [this](juce::TextButton &b, const char *id) {
    b.setClickingTogglesState(true);
    buttonAttachments.push_back(std::make_unique<BA>(p.state, id, b));
  };
  button(automatic, "autoScratch");
  button(smart, "intelligent");
  button(hold, "hold");
  button(cut, "cut");
  button(autoFader, "autoFader");
  button(reverse, "reverse");
  button(bypass, "bypass");
  auto combo = [this](juce::ComboBox &c, const char *id,
                      const juce::StringArray &items) {
    c.addItemList(items, 1);
    comboAttachments.push_back(std::make_unique<CA>(p.state, id, c));
  };
  combo(mode, "vinylMode", {"DIGITAL / TONO FIJO", "VINYL / TONO LIBRE"});
  combo(curve, "crossfaderCurve", {"CUT", "SMOOTH", "HAMSTER"});
  combo(pattern, "pattern",
        {"CHIRP", "TRANSFORMER", "FLARE / CRAB", "WORD CHOP", "BABY",
         "TEAR / ORBIT", "SLOW DRAG", "LOFI SWEEP", "EXTREME", "SIGNATURE"});
  combo(rpm, "rpm", {"33 1/3 RPM", "45 RPM"});
  const char *mainIds[]{"mix", "intensity", "speed", "tension"};
  const char *mainNames[]{"MEZCLA", "INTENSIDAD", "AGITE", "TENSION"};
  const char *tips[]{
      "Cuanto scratch se mezcla. Cero conserva voz limpia con la misma "
      "latencia.",
      "Fuerza audible del efecto. Cien aplica la receta completa.",
      "Que tan rapido mueve el disco el gesto automatico.",
      "Cuanta fuerza tiene el movimiento del disco."};
  for (int i = 0; i < 4; ++i) {
    mainKnobs[size_t(i)] =
        std::make_unique<ScratchKnob>(p, mainIds[i], mainNames[i], tips[i]);
    addAndMakeVisible(*mainKnobs[size_t(i)]);
  }
  const char *physicalIds[]{"inertia",     "friction",   "motorTorque",
                            "releaseTime", "pitchRange", "vinylNoise",
                            "crackle",     "lofi"};
  const char *physicalNames[]{"INERCIA", "FRICCION", "MOTOR", "SOLTAR",
                              "RANGO",   "RUIDO",    "POLVO", "LO-FI"};
  const char *physicalTips[]{
      "Cuanto conserva el disco su movimiento.",
      "Cuanta fuerza empuja hacia tu mano.",
      "Fuerza del motor cuando sueltas el vinilo.",
      "Tiempo de salida al soltar, en milisegundos.",
      "Limite de velocidad del disco. Mas rango permite cambios de tono mas "
      "extremos.",
      "Ruido real de vinilo, solo con una palabra capturada.",
      "Crujidos reales de polvo en el disco.",
      "Reduce banda, bits y frecuencia de muestreo para color vintage."};
  for (int i = 0; i < 8; ++i) {
    physicalKnobs[size_t(i)] = std::make_unique<ScratchKnob>(
        p, physicalIds[i], physicalNames[i], physicalTips[i]);
    addAndMakeVisible(*physicalKnobs[size_t(i)]);
  }
  const char *echoIds[]{"echoMix", "echoDelay", "echoFeedback"};
  const char *echoNames[]{"ECO", "TIEMPO ECO", "REPETICIONES"};
  for (int i = 0; i < 3; ++i) {
    echoKnobs[size_t(i)] = std::make_unique<ScratchKnob>(
        p, echoIds[i], echoNames[i],
        "Eco real del scratch, con feedback limitado.");
    addAndMakeVisible(*echoKnobs[size_t(i)]);
  }
  for (int i = 0; i < 8; ++i) {
    cues[size_t(i)] = std::make_unique<ScratchCue>(p, i);
    addAndMakeVisible(*cues[size_t(i)]);
  }
  auto slider = [&](juce::Slider &s) {
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 21);
  };
  slider(crossfader);
  slider(sensitivity);
  slider(gap);
  crossfader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  faderAttachment = std::make_unique<SA>(p.state, "crossfader", crossfader);
  senseAttachment = std::make_unique<SA>(p.state, "sensitivity", sensitivity);
  gapAttachment = std::make_unique<SA>(p.state, "gap", gap);
  description.setFont(juce::Font(ScratchSkin::body()).withHeight(10));
  description.setColour(juce::Label::textColourId, muted);
  description.setJustificationType(juce::Justification::centredLeft);
  mode.setTooltip("VINYL cambia tono con velocidad. DIGITAL usa Rubber Band "
                  "para conservarlo; cerca de la parada desvanece.");
  curve.setTooltip("CUT: corte rapido. SMOOTH: entrada suave. HAMSTER: "
                   "invierte los extremos del fader.");
  learn.setTooltip("Escucha un segundo de SOLO ruido de cuarto. Guarda ese "
                   "nivel para capturar mejor.");
  sensitivity.setTooltip("Mas sensibilidad detecta palabras suaves; menos "
                         "evita disparos por ruido.");
  gap.setTooltip("Silencio para separar fragmentos. Esto no reconoce palabras "
                 "por significado.");
  smart.setTooltip("Adapta el gesto al tamano del fragmento y baja el scratch "
                   "sobre una nueva voz.");
  automatic.setTooltip(
      "Captura un fragmento al terminar y dispara el scratch del preset.");
  cut.setTooltip("Silencia el bus scratch, incluido el eco. La voz seca "
                 "depende de MEZCLA e INTELIGENTE.");
  hold.setTooltip("Mantiene el gesto hasta STOP, otra palabra o un limite de "
                  "seguridad de 15 segundos.");
  setResizable(true, true);
  setResizeLimits(1120, 730, 1900, 1240);
  setSize(1440, 940);
  filter();
  startTimerHz(30);
}
ScratchEditor::~ScratchEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void ScratchEditor::filter() {
  filtering = true;
  presets.clear(juce::dontSendNotification);
  filtered.clear();
  auto term = search.getText().trim();
  int group = -1;
  for (int i = 0; i < 300; ++i) {
    auto &q = p.manager.presets[size_t(i)];
    if (!q.name.containsIgnoreCase(term) &&
        !q.category.containsIgnoreCase(term))
      continue;
    if (i / 30 != group) {
      group = i / 30;
      presets.addSectionHeading(scratch::categories[size_t(group)]);
    }
    presets.addItem(q.name, i + 1);
    filtered.push_back(i);
  }
  presets.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
  filtering = false;
  refresh();
}
void ScratchEditor::navigate(int delta) {
  if (filtered.empty())
    return;
  auto it = std::find(filtered.begin(), filtered.end(), p.getCurrentProgram());
  int at = it == filtered.end() ? 0 : int(it - filtered.begin());
  at = (at + delta + int(filtered.size())) % int(filtered.size());
  p.setCurrentProgram(filtered[size_t(at)]);
  refresh();
}
void ScratchEditor::importLicense() {
  chooser = std::make_unique<juce::FileChooser>("Importar M7G.license",
                                                juce::File{}, "*.license");
  chooser->launchAsync(juce::FileBrowserComponent::openMode |
                           juce::FileBrowserComponent::canSelectFiles,
                       [safe = juce::Component::SafePointer<ScratchEditor>(
                            this)](const juce::FileChooser &c) {
                         if (safe && c.getResult().existsAsFile()) {
                           safe->p.importLicense(c.getResult());
                           safe->refresh();
                         }
                       });
}
void ScratchEditor::refresh() {
  filtering = true;
  presets.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
  filtering = false;
  auto &q = p.manager.presets[size_t(p.getCurrentProgram())];
  description.setText(q.category + " / " +
                          juce::String(p.getCurrentProgram() % 30 + 1) +
                          " de 30 / " + q.description,
                      juce::dontSendNotification);
  a.setToggleState(p.compareSlot() == 0, juce::dontSendNotification);
  b.setToggleState(p.compareSlot() == 1, juce::dontSendNotification);
  copy.setButtonText(p.compareSlot() == 0 ? "A > B" : "B > A");
  favourite.setToggleState(p.isFavourite(p.getCurrentProgram()),
                           juce::dontSendNotification);
  auth.setButtonText(p.authorized.load() ? "AUTHORIZED: M7G GOLDEN"
                                         : "IMPORTAR M7G.license");
  learn.setToggleState(p.noiseLearning.load(), juce::dontSendNotification);
  for (auto &c : cues)
    c->setEnabled(p.hasWord.load());
  for (auto &k : mainKnobs)
    k->animate();
  for (auto &k : physicalKnobs)
    k->animate();
  for (auto &k : echoKnobs)
    k->animate();
  platter.repaint();
  repaint();
}
void ScratchEditor::timerCallback() {
  if (lastProgram != p.getCurrentProgram())
    lastProgram = p.getCurrentProgram();
  refresh();
  int wanted = p.scratching.load() || p.inputDB.load() > -60 ? 60 : 15;
  if (wanted != fps) {
    fps = wanted;
    startTimerHz(fps);
  }
}
void ScratchEditor::resized() {
  int w = 1440, h = 940;
  int left = 320, right = 330, cx = left + 45, cw = w - left - right - 90,
      rx = w - right - 20;
  auth.setBounds(w - 315, 18, 295, 28);
  a.setBounds(w - 315, 61, 43, 27);
  b.setBounds(w - 263, 61, 43, 27);
  copy.setBounds(w - 210, 61, 80, 27);
  bypass.setBounds(w - 120, 61, 100, 27);
  search.setBounds(20, 114, 250, 35);
  presets.setBounds(282, 114, w - 302, 35);
  previous.setBounds(20, 161, 40, 28);
  next.setBounds(68, 161, 40, 28);
  random.setBounds(116, 161, 84, 28);
  favourite.setBounds(208, 161, 62, 28);
  description.setBounds(282, 157, w - 302, 35);
  for (int i = 0; i < 4; ++i)
    mainKnobs[size_t(i)]->setBounds(20 + i * 78, 223, 76, 142);
  for (int i = 0; i < 8; ++i)
    physicalKnobs[size_t(i)]->setBounds(20 + (i % 4) * 78, 398 + (i / 4) * 143,
                                        76, 136);
  capture.setBounds(20, 724, 150, 35);
  play.setBounds(182, 724, 150, 35);
  stop.setBounds(20, 770, 150, 35);
  hold.setBounds(182, 770, 150, 35);
  learn.setBounds(20, h - 92, 312, 29);
  int platterSide = std::min(cw, h - 510);
  platter.setBounds(cx + (cw - platterSide) / 2, 375, platterSide, platterSide);
  for (int i = 0; i < 8; ++i)
    cues[size_t(i)]->setBounds(cx + i * cw / 8, h - 106, cw / 8 - 8, 36);
  automatic.setBounds(cx, 220, cw / 2 - 6, 32);
  smart.setBounds(cx + cw / 2 + 6, 220, cw / 2 - 6, 32);
  mode.setBounds(rx + 10, 222, 310, 31);
  pattern.setBounds(rx + 10, 269, 198, 29);
  rpm.setBounds(rx + 218, 269, 102, 29);
  autoFader.setBounds(rx + 10, 317, 150, 30);
  reverse.setBounds(rx + 170, 317, 150, 30);
  crossfader.setBounds(rx + 12, 391, 306, 52);
  curve.setBounds(rx + 10, 454, 198, 29);
  cut.setBounds(rx + 218, 454, 102, 29);
  for (int i = 0; i < 3; ++i)
    echoKnobs[size_t(i)]->setBounds(rx + 10 + i * 104, 518, 100, 139);
  sensitivity.setBounds(rx + 10, 704, 310, 26);
  gap.setBounds(rx + 10, 770, 310, 26);
  advanced.setBounds(20, 809, 312, 29);
  for (auto *child : getChildren())
    if (dynamic_cast<juce::ResizableCornerComponent *>(child) == nullptr &&
        dynamic_cast<juce::ResizableBorderComponent *>(child) == nullptr)
      child->setTransform(juce::AffineTransform::scale(getWidth() / 1440.f,
                                                       getHeight() / 940.f));
}
void ScratchEditor::paint(juce::Graphics &g) {
  juce::Graphics::ScopedSaveState scaled(g);
  g.addTransform(
      juce::AffineTransform::scale(getWidth() / 1440.f, getHeight() / 940.f));
  int w = 1440, h = 940, cx = 365, cw = w - 740, rx = w - 350;
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff162c44), 0, 0, navy,
                                         float(w), float(h), false));
  g.fillAll();
  g.setColour(gold.withAlpha(.25f));
  g.drawLine(20, 100, float(w - 20), 100);
  g.drawLine(20, 202, float(w - 20), 202);
  alien(g, {25, 22, 45, 57}, gold);
  g.setColour(gold);
  g.setFont(juce::Font(ScratchSkin::graffiti()).withHeight(29));
  g.drawFittedText("WORD SCRATCHER PRO - 300", {90, 17, w - 430, 43},
                   juce::Justification::centredLeft, 1);
  label(g, "REAL WORD SCRATCH / MAKKINE SEVEN GOLDEN", {93, 61, 650, 18}, 11,
        cyan, juce::Justification::centredLeft);
  label(g, "LAS MAKKINAS PRODUCEN SOLAS", {93, 83, 650, 13}, 9, gold,
        juce::Justification::centredLeft);
  auto panel = [&](juce::Rectangle<int> r) {
    g.setColour(navy.withAlpha(.75f));
    g.fillRoundedRectangle(r.toFloat(), 8);
    g.setColour(gold.withAlpha(.22f));
    g.drawRoundedRectangle(r.toFloat(), 8, 1);
  };
  panel({12, 212, 328, h - 251});
  panel({rx, 212, 330, h - 251});
  label(g, "FISICA Y TEXTURA REALES", {22, 370, 306, 21}, 11, gold);
  label(g, "PALABRA CAPTURADA / 5 s DE MEMORIA", {cx, 264, cw, 18}, 10, muted,
        juce::Justification::centredLeft);
  auto waveform = juce::Rectangle<int>(cx, 289, cw, 72);
  g.setColour(navy);
  g.fillRoundedRectangle(waveform.toFloat(), 5);
  for (int i = 0; i < 512; ++i) {
    float x = waveform.getX() + waveform.getWidth() * i / 512.f,
          y = p.wordWave[size_t(i)].load() * 130;
    g.setColour(gold.withAlpha(.8f));
    g.drawVerticalLine(int(x), waveform.getCentreY() - y,
                       waveform.getCentreY() + y);
  }
  float cursor =
      waveform.getX() + waveform.getWidth() * p.platterPosition.load();
  g.setColour(cyan);
  g.drawVerticalLine(int(cursor), float(waveform.getY()),
                     float(waveform.getBottom()));
  label(g, "CROSSFADER", {rx + 10, 361, 310, 22}, 13, gold);
  float gate = std::clamp(p.gate.load(), 0.f, 1.f);
  g.setColour(gate > .15f ? cyan : purple);
  g.fillRoundedRectangle(float(rx + 12), 490, 306 * gate, 4, 1);
  label(g, gate > .15f ? "ABIERTO" : "CORTE", {rx + 10, 496, 310, 17}, 9,
        gate > .15f ? cyan : purple);
  label(g, "SENSIBILIDAD DE PALABRA", {rx + 10, 677, 310, 21}, 10, gold);
  label(g, "SILENCIO ENTRE FRAGMENTOS (ms)", {rx + 10, 743, 310, 21}, 10, gold);
  label(g, p.statusText(), {cx, h - 62, cw, 20}, 10, cyan);
  label(g, "HOT CUES / SHIFT + CLICK PARA GUARDAR", {cx, h - 133, cw, 19}, 10,
        gold);
  label(g,
        juce::String(p.playbackRate.load(), 2) + " x / " +
            juce::String(int(p.wordMilliseconds.load())) + " ms / " +
            juce::String(p.wordCount.load()) + " fragmentos",
        {cx, 367, cw, 20}, 11, muted);
  label(g,
        "MEMORIA VOCAL: 5000 ms / COMPENSA EN EL DAW / LATENCIA " +
            juce::String(
                1000. *
                    scratch::VinylEngine::latencyFor(
                        p.getSampleRate() > 0 ? p.getSampleRate() : 48000.) /
                    (p.getSampleRate() > 0 ? p.getSampleRate() : 48000.),
                2) +
            " ms",
        {20, h - 29, w - 40, 18}, 9, muted, juce::Justification::centredLeft);
  float input = std::clamp((p.inputDB.load() + 60) / 60, 0.f, 1.f),
        output = std::clamp((p.outputDB.load() + 60) / 60, 0.f, 1.f);
  g.setColour(blue.withAlpha(.4f));
  g.fillRect(float(cx), float(h - 37), cw * .45f * input, 3.f);
  g.setColour(gold.withAlpha(.6f));
  g.fillRect(float(cx + cw * .55f), float(h - 37), cw * .45f * output, 3.f);
}
