#include "PluginEditor.h"
#include "TransitionsFonts.h"
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
  g.setFont(juce::Font(TransitionsSkin::body()).withHeight(h));
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
juce::Typeface::Ptr TransitionsSkin::body() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      TransitionsFonts::DejaVuSans_ttf, TransitionsFonts::DejaVuSans_ttfSize);
  return f;
}
juce::Typeface::Ptr TransitionsSkin::graffiti() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      TransitionsFonts::RubikWetPaintRegular_ttf,
      TransitionsFonts::RubikWetPaintRegular_ttfSize);
  return f;
}
TransitionsSkin::TransitionsSkin() {
  setColour(juce::ListBox::backgroundColourId, navy);
  setColour(juce::Slider::textBoxTextColourId, white);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::transparentBlack);
  setColour(juce::PopupMenu::backgroundColourId, navy);
  setColour(juce::PopupMenu::textColourId, white);
}
void TransitionsSkin::drawRotarySlider(juce::Graphics &g, int x, int y, int w,
                                       int h, float pos, float start, float end,
                                       juce::Slider &) {
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
void TransitionsSkin::drawButtonBackground(juce::Graphics &g, juce::Button &b,
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
void TransitionsSkin::drawButtonText(juce::Graphics &g, juce::TextButton &b,
                                     bool, bool) {
  label(g, b.getButtonText(), b.getLocalBounds().reduced(3),
        b.getHeight() > 40 ? 14.0f : 10.5f, b.getToggleState() ? gold : white);
}
TransitionKnob::TransitionKnob(TransitionsProcessor &p, const char *id,
                               juce::String name, juce::String tip)
    : caption(std::move(name)) {
  addAndMakeVisible(slider);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                             juce::MathConstants<float>::pi * 2.8f, true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 120, 23);
  slider.setTooltip(tip);
  slider.setTextValueSuffix(" %");
  attachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.state, id, slider);
  if (juce::String(id) == "time") {
    slider.setTextValueSuffix("");
    slider.textFromValueFunction = [&p](double v) {
      return juce::String(int(transition::durationMs(p.bank(), float(v)))) +
             " ms";
    };
    slider.valueFromTextFunction = [&p](const juce::String &s) {
      float ms = s.getFloatValue();
      return double(juce::jlimit(0.0f, 100.0f,
                                 (ms - (p.bank() == 0 ? 200 : 400)) /
                                     (p.bank() == 0 ? 13.0f : 11.0f)));
    };
    slider.updateText();
  }
}
void TransitionKnob::resized() {
  slider.setBounds(0, 25, getWidth(), getHeight() - 25);
}
void TransitionKnob::paint(juce::Graphics &g) {
  label(g, caption, getLocalBounds().withHeight(24), 11.5f, gold);
}
TransitionsEditor::TransitionsEditor(TransitionsProcessor &p)
    : AudioProcessorEditor(p), processor(p) {
  setLookAndFeel(&skin);
  setOpaque(true);
  for (int i = 0; i < 3; ++i) {
    banks[size_t(i)].setButtonText(transition::bankName(i));
    banks[size_t(i)].onClick = [this, i] {
      processor.setBank(i);
      filter();
    };
    addAndMakeVisible(banks[size_t(i)]);
  }
  for (auto *c : std::initializer_list<juce::Component *>{
           &presets, &search, &previous, &next, &random, &favourite, &compareA,
           &compareB, &stop, &bypass, &authorization, &menu})
    addAndMakeVisible(c);
  presets.setComponentID("preset-list");
  presets.setRowHeight(40);
  presets.setOutlineThickness(0);
  search.setComponentID("preset-search");
  search.setTextToShowWhenEmpty("Buscar en este banco...", muted);
  search.setFont(juce::Font(TransitionsSkin::body()).withHeight(12));
  search.setColour(juce::TextEditor::backgroundColourId, navy);
  search.setColour(juce::TextEditor::textColourId, white);
  search.setColour(juce::TextEditor::outlineColourId, gold.withAlpha(.4f));
  search.onTextChange = [this] { filter(); };
  previous.onClick = [this] { navigate(-1); };
  next.onClick = [this] { navigate(1); };
  random.onClick = [this] {
    if (filtered.empty())
      return;
    int i = juce::Random::getSystemRandom().nextInt(int(filtered.size()));
    if (filtered.size() > 1 &&
        filtered[size_t(i)] == processor.getCurrentProgram())
      i = (i + 1) % int(filtered.size());
    processor.setCurrentProgram(filtered[size_t(i)]);
    refresh();
  };
  favourite.onClick = [this] {
    processor.toggleFavourite(processor.getCurrentProgram());
    filter();
  };
  favourite.setTooltip("Guarda este efecto entre tus favoritos. El menu "
                       "permite ver solo favoritos.");
  compareA.onClick = [this] {
    processor.selectCompare(0);
    refresh();
  };
  compareB.onClick = [this] {
    processor.selectCompare(1);
    refresh();
  };
  compareB.setTooltip(
      "Compara B con A. El menu permite copiar la mezcla al otro lado.");
  stop.onClick = [this] { processor.stopEffects(); };
  stop.setTooltip("Corta las colas e infinitos con una salida suave. La voz "
                  "limpia sigue sonando.");
  bypass.setClickingTogglesState(true);
  bypassAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.state, "bypass", bypass);
  bypass.setTooltip(
      "Escucha la voz limpia conservando la compensacion de dos segundos.");
  authorization.onClick = [this] { importLicense(); };
  menu.onClick = [this] { showMenu(); };
  knobs[0] = std::make_unique<TransitionKnob>(
      p, "amount", "CANTIDAD",
      "CUANTO EFECTO: cambia la intensidad de la transformacion. Cero conserva "
      "la voz limpia.");
  knobs[1] = std::make_unique<TransitionKnob>(
      p, "time", "TIEMPO",
      "LARGO DEL EFECTO: entradas de 200 a 1500 ms; puentes y salidas de 400 a "
      "1500 ms. Se aplica a los proximos disparos.");
  knobs[2] =
      std::make_unique<TransitionKnob>(p, "mix", "MIX",
                                       "MEZCLA: combina la voz limpia con el "
                                       "efecto. Cero deja solo la voz limpia.");
  knobs[3] = std::make_unique<TransitionKnob>(
      p, "sensitivity", "SENSIBILIDAD DE PALABRA",
      "QUE TAN FACIL DETECTA LA VOZ: sube para voces suaves. Baja si el ruido "
      "dispara efectos. Separa frases con una pausa de 250 ms.");
  for (auto &k : knobs)
    addAndMakeVisible(*k);
  setResizable(true, false);
  setResizeLimits(1024, 640, 1920, 1200);
  setSize(1280, 800);
  filter();
  refresh();
  startTimerHz(30);
}
TransitionsEditor::~TransitionsEditor() {
  stopTimer();
  presets.setModel(nullptr);
  setLookAndFeel(nullptr);
}
void TransitionsEditor::filter() {
  filtered.clear();
  int bank = processor.bank();
  for (int i = bank * 50; i < bank * 50 + 50; ++i)
    if (juce::String(transition::recipes()[size_t(i)].name)
            .containsIgnoreCase(search.getText()) &&
        (!onlyFavourites || processor.isFavourite(i)))
      filtered.push_back(i);
  selecting = true;
  presets.updateContent();
  auto found = std::find(filtered.begin(), filtered.end(),
                         processor.getCurrentProgram());
  if (found != filtered.end())
    presets.selectRow(int(found - filtered.begin()), false, true);
  else
    presets.deselectAllRows();
  selecting = false;
  repaint();
}
void TransitionsEditor::selectedRowsChanged(int row) {
  if (!selecting && row >= 0 && row < int(filtered.size())) {
    processor.setCurrentProgram(filtered[size_t(row)]);
    refresh();
  }
}
void TransitionsEditor::paintListBoxItem(int row, juce::Graphics &g, int w,
                                         int h, bool selected) {
  if (row < 0 || row >= int(filtered.size()))
    return;
  int index = filtered[size_t(row)];
  auto accent = colour(index / 50);
  if (selected) {
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(.2f), 0, 0, navy,
                                           float(w), 0, false));
    g.fillRoundedRectangle(2, 1, float(w - 4), float(h - 2), 4);
    g.setColour(accent.withAlpha(.65f));
    g.drawRoundedRectangle(2, 1, float(w - 4), float(h - 2), 4, 1);
  }
  alien(g, {8, 10, 15, 20}, accent.withAlpha(selected ? 1.0f : .45f));
  label(g, juce::String(index % 50 + 1).paddedLeft('0', 2), {26, 8, 25, h - 12},
        9, muted);
  g.setColour(selected ? white : muted);
  g.setFont(juce::Font(TransitionsSkin::body()).withHeight(11.7f));
  g.drawFittedText(transition::recipes()[size_t(index)].name, 58, 2, w - 66,
                   h - 4, juce::Justification::centredLeft, 2);
}
void TransitionsEditor::navigate(int direction) {
  if (filtered.empty())
    return;
  auto found = std::find(filtered.begin(), filtered.end(),
                         processor.getCurrentProgram());
  int pos = found == filtered.end() ? (direction > 0 ? -1 : 0)
                                    : int(found - filtered.begin());
  processor.setCurrentProgram(filtered[size_t(
      (pos + direction + int(filtered.size())) % int(filtered.size()))]);
  refresh();
}
void TransitionsEditor::refresh() {
  int program = processor.getCurrentProgram();
  if (program != lastProgram) {
    lastProgram = program;
    filter();
    knobs[1]->slider.updateText();
  }
  for (int i = 0; i < 3; ++i)
    banks[size_t(i)].setToggleState(i == processor.bank(),
                                    juce::dontSendNotification);
  favourite.setToggleState(processor.isFavourite(program),
                           juce::dontSendNotification);
  compareA.setToggleState(processor.compareSlot() == 0,
                          juce::dontSendNotification);
  compareB.setToggleState(processor.compareSlot() == 1,
                          juce::dontSendNotification);
  authorization.setButtonText(processor.authorized.load()
                                  ? "AUTHORIZED: M7G GOLDEN"
                                  : "IMPORTAR M7G.license");
  repaint();
}
void TransitionsEditor::resized() {
  float sx = float(getWidth()) / 1280, sy = float(getHeight()) / 800;
  auto place = [&](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
                juce::roundToInt(w * sx), juce::roundToInt(h * sy));
  };
  place(search, 18, 119, 270, 29);
  place(previous, 18, 157, 33, 28);
  place(next, 56, 157, 33, 28);
  place(random, 96, 157, 89, 28);
  place(favourite, 192, 157, 96, 28);
  place(presets, 18, 225, 270, 545);
  for (int i = 0; i < 3; ++i)
    place(banks[size_t(i)], 326 + i * 310, 112, 294, 57);
  place(compareA, 1090, 67, 37, 27);
  place(compareB, 1135, 67, 37, 27);
  place(menu, 1181, 67, 79, 27);
  place(authorization, 1001, 19, 259, 28);
  place(stop, 1013, 741, 115, 34);
  place(bypass, 1138, 741, 122, 34);
  for (int i = 0; i < 4; ++i)
    place(*knobs[size_t(i)], 351 + i * 221, 520, 185, 193);
}
void TransitionsEditor::paint(juce::Graphics &g) {
  g.fillAll(navy);
  juce::Graphics::ScopedSaveState state(g);
  g.addTransform(juce::AffineTransform::scale(float(getWidth()) / 1280,
                                              float(getHeight()) / 800));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff162d48), 640, 0, navy,
                                         640, 800, false));
  g.fillRect(0, 0, 1280, 800);
  g.setColour(gold.withAlpha(.35f));
  g.drawLine(307, 113, 307, 778, 1);
  g.drawLine(18, 103, 1260, 103, 1);
  g.drawLine(325, 726, 1260, 726, 1);
  g.drawLine(18, 788, 1260, 788, 1);
  g.setColour(cyan.withAlpha(.1f));
  g.drawEllipse(15, 16, 70, 70, 1);
  g.drawLine(50, 51, 50 + std::cos(radar) * 35, 51 + std::sin(radar) * 35,
             1.0f);
  alien(g, {28, 25, 44, 54}, gold);
  g.setColour(gold);
  g.setFont(juce::Font(TransitionsSkin::graffiti()).withHeight(28));
  g.drawFittedText("M7G ALIEN TRANSITIONS GOD", {103, 12, 872, 39},
                   juce::Justification::centredLeft, 1);
  label(g, "V1 TRANSITIONS / 150 FX / MAKKINE SEVEN GOLDEN", {106, 56, 850, 21},
        11, cyan, juce::Justification::centredLeft);
  label(g, "LAS MAKKINAS PRODUCEN SOLAS", {106, 80, 820, 16}, 9, gold,
        juce::Justification::centredLeft);
  label(g,
        juce::String(transition::bankName(processor.bank())) + " / " +
            juce::String(filtered.size()) + " FX",
        {18, 194, 270, 25}, 11, gold, juce::Justification::centredLeft);
  label(g, transition::recipes()[size_t(processor.getCurrentProgram())].name,
        {337, 182, 907, 30}, 19, white, juce::Justification::centredLeft);
  auto wave = juce::Rectangle<float>(330, 225, 920, 215);
  g.setColour(juce::Colour(0xff060c19));
  g.fillRoundedRectangle(wave, 9);
  g.setColour(colour(processor.bank()).withAlpha(.4f));
  g.drawRoundedRectangle(wave, 9, 1);
  for (int i = 1; i < 8; ++i) {
    g.setColour(blue.withAlpha(.1f));
    g.drawVerticalLine(int(wave.getX() + wave.getWidth() * i / 8),
                       wave.getY() + 24, wave.getBottom() - 17);
  }
  g.setColour(blue.withAlpha(.2f));
  g.drawHorizontalLine(int(wave.getCentreY()), wave.getX() + 10,
                       wave.getRight() - 10);
  int head = processor.waveHead.load();
  for (int i = 0; i < 512; ++i) {
    int at = (head + i) % 512;
    float peak = processor.waveform[size_t(at)].load();
    int mode = processor.waveColour[size_t(at)].load();
    auto c = mode == 1 ? gold : mode == 3 ? purple : mode == 2 ? cyan : blue;
    g.setColour(c.withAlpha(.85f));
    float height = juce::jlimit(1.0f, 80.0f, peak * 230);
    float x = wave.getX() + 10 + i * (wave.getWidth() - 20) / 512;
    g.drawLine(x, wave.getCentreY() - height, x, wave.getCentreY() + height,
               1.1f);
  }
  label(g, "FORMA DE ONDA / ORO: ENTRADA / AZUL: VOZ LIMPIA / VIOLETA: SALIDA",
        {345, 237, 883, 20}, 9, muted, juce::Justification::centredLeft);
  label(g, processor.explanation(), {343, 406, 888, 23}, 11,
        colour(processor.bank()));
  auto pathBounds = juce::Rectangle<float>(335, 454, 910, 42);
  g.setColour(cyan.withAlpha(.12f));
  for (int i = 0; i < 9; ++i)
    g.drawVerticalLine(int(pathBounds.getX() + pathBounds.getWidth() * i / 8),
                       pathBounds.getY(), pathBounds.getBottom());
  juce::Path trajectory;
  const auto &r = transition::recipes()[size_t(processor.getCurrentProgram())];
  for (int i = 0; i <= 128; ++i) {
    float t = float(i) / 128, y = t;
    using F = transition::Family;
    if (r.family == F::Reverse)
      y = 1 - t;
    else if (r.family == F::Tape || r.family == F::Vinyl)
      y = juce::jlimit(0.0f, 1.0f,
                       r.speed0 * t + .5f * (r.speed1 - r.speed0) * t * t);
    else if (r.family == F::Stutter || r.family == F::Chops ||
             r.family == F::Repeat)
      y = std::fmod(t * (r.family == F::Chops ? 4.0f : float(r.repeats)), 1.0f);
    else if (r.family == F::Hall || r.family == F::Shimmer ||
             r.family == F::ReverseVerb)
      y = t * t;
    float x = pathBounds.getX() + t * pathBounds.getWidth(),
          yy = pathBounds.getBottom() - y * pathBounds.getHeight();
    if (i == 0)
      trajectory.startNewSubPath(x, yy);
    else
      trajectory.lineTo(x, yy);
  }
  g.setColour(colour(processor.bank()).withAlpha(.15f));
  g.strokePath(trajectory, juce::PathStrokeType(5));
  g.setColour(colour(processor.bank()));
  g.strokePath(trajectory, juce::PathStrokeType(1.5f));
  if (processor.activity.load() > 0) {
    float x =
        pathBounds.getX() + processor.progress.load() * pathBounds.getWidth();
    g.setColour(white);
    g.drawLine(x, pathBounds.getY(), x, pathBounds.getBottom(), 1);
  }
  label(g, "LECTURA / ENVOLVENTE", {336, 499, 350, 16}, 8, muted,
        juce::Justification::centredLeft);
  label(g,
        "BPM " + juce::String(processor.tempo.load(), 1) +
            " / ANTICIPACION 2000 ms",
        {820, 499, 422, 16}, 9, muted, juce::Justification::centredRight);
  auto meter = [&](int x, float db, const char *name) {
    label(g, name, {x, 741, 57, 18}, 9, muted,
          juce::Justification::centredLeft);
    g.setColour(blue.withAlpha(.15f));
    g.fillRoundedRectangle(float(x), 765, 195, 5, 2);
    g.setColour(cyan);
    g.fillRoundedRectangle(
        float(x), 765, 195 * juce::jlimit(0.0f, 1.0f, (db + 60) / 60), 5, 2);
    label(g, juce::String(db, 1) + " dBFS", {x + 58, 741, 137, 18}, 9, white,
          juce::Justification::centredRight);
  };
  meter(337, processor.inputDB.load(), "ENTRADA");
  meter(590, processor.outputDB.load(), "SALIDA");
  label(g,
        "V1 TRANSITIONS / COMPENSACION DE LATENCIA PARA MEZCLA / PROCESAMIENTO "
        "LOCAL",
        {17, 789, 980, 10}, 7.5f, muted, juce::Justification::centredLeft);
  label(g, "JUCE 7 / WINDOWS x64", {1025, 789, 235, 10}, 7.5f, gold,
        juce::Justification::centredRight);
}
void TransitionsEditor::timerCallback() {
  int wanted = processor.inputDB.load() > -60 || processor.outputDB.load() > -65
                   ? 60
                   : 15;
  if (wanted != animationRate) {
    animationRate = wanted;
    startTimerHz(wanted);
  }
  if (processor.inputDB.load() > -60)
    radar += 1.4f / animationRate;
  refresh();
}
void TransitionsEditor::importLicense() {
  chooser = std::make_unique<juce::FileChooser>(
      "Selecciona tu licencia global M7G.license", juce::File(), "*.license");
  juce::Component::SafePointer<TransitionsEditor> safe(this);
  chooser->launchAsync(
      juce::FileBrowserComponent::openMode |
          juce::FileBrowserComponent::canSelectFiles,
      [safe](const juce::FileChooser &c) {
        if (!safe || c.getResult() == juce::File())
          return;
        bool ok = safe->processor.importLicense(c.getResult());
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, "M7G GOLDEN",
            ok ? "Todos los efectos autorizados para siempre."
               : "Archivo no valido. Selecciona un M7G.license firmado.");
        safe->refresh();
      });
}
void TransitionsEditor::showMenu() {
  juce::PopupMenu m;
  m.addItem(1, "Copiar al otro lado A/B");
  m.addItem(2, "Ver solo favoritos", true, onlyFavourites);
  m.addItem(3, "Ayuda / latencia");
  juce::Component::SafePointer<TransitionsEditor> safe(this);
  m.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(&menu),
      [safe](int result) {
        if (!safe)
          return;
        if (result == 1)
          safe->processor.copyCompare();
        if (result == 2) {
          safe->onlyFavourites = !safe->onlyFavourites;
          safe->filter();
        }
        if (result == 3)
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::InfoIcon, "V1 TRANSITIONS",
              "150 efectos originales creados por Makkine Seven Golden.\nPara "
              "sonar antes de tu voz se usan dos segundos de anticipacion, "
              "compensados por el DAW durante reproduccion.\nINTRO: entrada; "
              "CAMBIO: pausa mayor de 300 ms; SALIDA: ultimo tramo.\nEl "
              "detector separa frases con 250 ms de pausa. Baja sensibilidad "
              "si dispara con ruido.\nLos ecos y stutters siguen el BPM del "
              "DAW; 120 BPM si no esta disponible.\nLos infinitos se cortan "
              "con la siguiente frase, STOP FX o el limite de seguridad de 30 "
              "s.\nLas pruebas de escucha con voces reales y Studio One 7 "
              "necesitan evaluacion directa.");
      });
}
