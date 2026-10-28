#include "PluginEditor.h"
#include "TuneData.h"
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
  g.setFont(juce::Font(TuneSkin::body()).withHeight(h));
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
juce::Typeface::Ptr TuneSkin::body() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      TuneData::DejaVuSans_ttf, TuneData::DejaVuSans_ttfSize);
  return f;
}
juce::Typeface::Ptr TuneSkin::graffiti() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      TuneData::RubikWetPaintRegular_ttf,
      TuneData::RubikWetPaintRegular_ttfSize);
  return f;
}
TuneSkin::TuneSkin() {
  setColour(juce::ListBox::backgroundColourId, navy);
  setColour(juce::Slider::textBoxTextColourId, white);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::transparentBlack);
  setColour(juce::PopupMenu::backgroundColourId, navy);
  setColour(juce::PopupMenu::textColourId, white);
}
void TuneSkin::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h,
                                float pos, float start, float end,
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
void TuneSkin::drawButtonBackground(juce::Graphics &g, juce::Button &b,
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
void TuneSkin::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool,
                              bool) {
  label(g, b.getButtonText(), b.getLocalBounds().reduced(3),
        b.getHeight() > 40 ? 14.0f : 10.5f, b.getToggleState() ? gold : white);
}
TuneKnob::TuneKnob(TuneProcessor &p, const char *id, const juce::String &name,
                   const juce::String &tip)
    : caption(name) {
  addAndMakeVisible(slider);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                             juce::MathConstants<float>::pi * 2.8f, true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 110, 22);
  slider.setTooltip(tip);
  slider.setTextValueSuffix(juce::String(id) == "retune"    ? " ms"
                            : juce::String(id) == "formant" ? " st"
                                                            : " %");
  attachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.state, id, slider);
}
void TuneKnob::resized() {
  slider.setBounds(0, 23, getWidth(), getHeight() - 23);
}
void TuneKnob::paint(juce::Graphics &g) {
  label(g, caption, getLocalBounds().withHeight(22), 10.5f, gold);
}
TuneEditor::TuneEditor(TuneProcessor &processor)
    : AudioProcessorEditor(processor), p(processor) {
  setLookAndFeel(&skin);
  setOpaque(true);
  for (int i = 0; i < 4; ++i) {
    banks[size_t(i)].setButtonText(tune::banks[size_t(i)]);
    banks[size_t(i)].onClick = [this, i] {
      p.setBank(i);
      filter();
    };
    addAndMakeVisible(banks[size_t(i)]);
  }
  for (auto *c : std::initializer_list<juce::Component *>{
           &list,    &search,   &previous, &next,       &random, &fav,
           &a,       &b,        &copy,     &auth,       &bypass, &harmony,
           &doubles, &autoKey,  &model,    &key,        &scale,  &quality,
           &count,   &interval, &custom,   &doubleHuman})
    addAndMakeVisible(c);
  list.setComponentID("preset-list");
  list.setRowHeight(36);
  search.setComponentID("preset-search");
  search.setTextToShowWhenEmpty("Buscar preset...", muted);
  search.setColour(juce::TextEditor::backgroundColourId, navy);
  search.setColour(juce::TextEditor::textColourId, white);
  search.setFont(juce::Font(TuneSkin::body()).withHeight(12));
  search.onTextChange = [this] { filter(); };
  previous.onClick = [this] { navigate(-1); };
  next.onClick = [this] { navigate(1); };
  random.onClick = [this] {
    if (filtered.empty())
      return;
    int n = juce::Random::getSystemRandom().nextInt(int(filtered.size()));
    if (filtered.size() > 1 && filtered[size_t(n)] == p.getCurrentProgram())
      n = (n + 1) % int(filtered.size());
    p.setCurrentProgram(filtered[size_t(n)]);
    refresh();
  };
  fav.onClick = [this] {
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
  auto button = [this](juce::TextButton &x, const char *id) {
    x.setClickingTogglesState(true);
    buttonAttachments.push_back(std::make_unique<BA>(p.state, id, x));
  };
  button(harmony, "harmonyOn");
  button(doubles, "doublesOn");
  button(autoKey, "autoKey");
  button(bypass, "bypass");
  auto combo = [this](juce::ComboBox &x, const char *id,
                      const juce::StringArray &items) {
    x.addItemList(items, 1);
    comboAttachments.push_back(std::make_unique<CA>(p.state, id, x));
  };
  combo(model, "model",
        {"ALIEN NATURAL", "REGGAETON FLOW", "TRAP AUTO", "POP CRYSTAL",
         "CLASSIC VINTAGE"});
  combo(key, "key",
        {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"});
  combo(scale, "scale", {"CROMATICA", "MAYOR", "MENOR", "MENOR ARMONICA"});
  combo(quality, "quality", {"ESTUDIO", "MONITOR"});
  combo(count, "voices", {"1 VOZ", "2 VOCES", "3 VOCES", "4 VOCES"});
  // Integer parameters require the same one-based UI mapping as choice
  // parameters.
  combo(interval, "interval",
        {"3a + 5a", "5a + OCTAVA", "OCTAVA ARRIBA", "OCTAVA ABAJO",
         "CORO COMPLETO", "PERSONALIZADA"});
  const char *titles[]{"CORRECCION IA", "HUMAN <> ROBOT", "RETUNE SPEED",
                       "HUMANIZE",      "FORMANTE",       "MIX ARMONIAS"};
  const char *tips[]{
      "CUANTO CORRIGE el centro de la nota. La respiracion sigue natural.",
      "CUANTO EFECTO ROBOT. Cero natural; cien pega las notas. HUMANIZE "
      "conserva expresion.",
      "QUE TAN RAPIDO AFINA. Cero es la respuesta mas rapida del motor, no "
      "cero latencia.",
      "CUANTA EXPRESION CONSERVA. Mas valor deja vibrato y cambios humanos.",
      "COLOR DE LA GARGANTA. Conserva el timbre al afinar; mueve para hacerlo "
      "grave o fino.",
      "CUANTO CORO Y DOBLES. Sus efectos no se aplican a la voz principal."};
  for (int i = 0; i < 6; ++i) {
    knobs[size_t(i)] =
        std::make_unique<TuneKnob>(p, tune::ids[size_t(i)], titles[i], tips[i]);
    addAndMakeVisible(*knobs[size_t(i)]);
  }
  for (int i = 0; i < 6; ++i) {
    voices[size_t(i)].setButtonText(i < 4 ? "ARMONIA " + juce::String(i + 1)
                                          : "DOBLE " + juce::String(i - 3));
    voices[size_t(i)].onClick = [this, i] { selectVoice(i); };
    addAndMakeVisible(voices[size_t(i)]);
    fx[size_t(i)].setSliderStyle(juce::Slider::LinearHorizontal);
    fx[size_t(i)].setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 20);
    addAndMakeVisible(fx[size_t(i)]);
  }
  custom.setSliderStyle(juce::Slider::LinearHorizontal);
  custom.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 20);
  custom.setTooltip("Intervalo en semitonos; se ajusta a la escala elegida.");
  customAttachment = std::make_unique<SA>(p.state, "custom", custom);
  doubleHuman.setSliderStyle(juce::Slider::LinearHorizontal);
  doubleHuman.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 20);
  doubleHuman.setTooltip(
      "CUANTO VARIAN LOS DOBLES. Cambia microtiempo, afinacion y vibrato.");
  humanAttachment = std::make_unique<SA>(p.state, "doubleHuman", doubleHuman);
  quality.setTooltip("ESTUDIO: mas resolucion. MONITOR: menos latencia, menor "
                     "detalle en graves. Deten la reproduccion para cambiar.");
  autoKey.setTooltip("Estima una tonalidad a partir de notas recibidas. Puede "
                     "ser ambigua: desactiva para elegirla.");
  harmony.setTooltip(
      "Genera hasta cuatro voces derivadas de tu toma, siguiendo la escala.");
  doubles.setTooltip("Simula dos dobles mediante resintesis y "
                     "microvariaciones. No son otras tomas grabadas.");
  auth.setTooltip("Una licencia M7G desbloquea todo. Pulsa para importar un "
                  "archivo firmado.");
  selectVoice(0);
  setResizable(true, true);
  setResizeLimits(1200, 800, 1900, 1200);
  setSize(1440, 900);
  filter();
  startTimerHz(30);
}
TuneEditor::~TuneEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void TuneEditor::filter() {
  lastBank = p.bank();
  filtered.clear();
  auto term = search.getText().trim();
  for (int i = p.bank() * 25; i < p.bank() * 25 + 25; ++i)
    if (juce::String(tune::presets()[size_t(i)].name).containsIgnoreCase(term))
      filtered.push_back(i);
  selecting = true;
  list.updateContent();
  auto it = std::find(filtered.begin(), filtered.end(), p.getCurrentProgram());
  if (it != filtered.end())
    list.selectRow(int(it - filtered.begin()));
  else
    list.deselectAllRows();
  selecting = false;
  refresh();
}
void TuneEditor::navigate(int d) {
  if (filtered.empty())
    return;
  auto it = std::find(filtered.begin(), filtered.end(), p.getCurrentProgram());
  int index = it == filtered.end() ? 0 : int(it - filtered.begin());
  index = (index + d + int(filtered.size())) % int(filtered.size());
  p.setCurrentProgram(filtered[size_t(index)]);
  refresh();
}
void TuneEditor::selectVoice(int v) {
  selectedVoice = v;
  for (int i = 0; i < 6; ++i) {
    voices[size_t(i)].setToggleState(i == v, juce::dontSendNotification);
    fxAttachments[size_t(i)].reset();
    fxAttachments[size_t(i)] = std::make_unique<SA>(
        p.state, "v" + juce::String(v + 1) + "_" + tune::fxIds[size_t(i)],
        fx[size_t(i)]);
  }
  repaint();
}
void TuneEditor::importLicense() {
  chooser = std::make_unique<juce::FileChooser>("Importar M7G.license",
                                                juce::File{}, "*.license");
  chooser->launchAsync(juce::FileBrowserComponent::openMode |
                           juce::FileBrowserComponent::canSelectFiles,
                       [safe = juce::Component::SafePointer<TuneEditor>(this)](
                           const juce::FileChooser &c) {
                         if (safe && c.getResult().existsAsFile()) {
                           safe->p.importLicense(c.getResult());
                           safe->refresh();
                         }
                       });
}
void TuneEditor::refresh() {
  if (lastBank != p.bank()) {
    filter();
    return;
  }
  selecting = true;
  auto selected =
      std::find(filtered.begin(), filtered.end(), p.getCurrentProgram());
  if (selected != filtered.end())
    list.selectRow(int(selected - filtered.begin()));
  selecting = false;
  for (int i = 0; i < 4; ++i)
    banks[size_t(i)].setToggleState(i == p.bank(), juce::dontSendNotification);
  copy.setButtonText(p.compareSlot() == 0 ? "A > B" : "B > A");
  a.setToggleState(p.compareSlot() == 0, juce::dontSendNotification);
  b.setToggleState(p.compareSlot() == 1, juce::dontSendNotification);
  fav.setToggleState(p.isFavourite(p.getCurrentProgram()),
                     juce::dontSendNotification);
  auth.setButtonText(p.authorized.load() ? "AUTHORIZED: M7G GOLDEN"
                                         : "IMPORTAR M7G.license");
  quality.setEnabled(!p.playing.load());
  key.setEnabled(p.value("autoKey") < .5f);
  scale.setEnabled(p.value("autoKey") < .5f);
  custom.setEnabled(p.value("interval") > 4.5f);
  repaint();
}
void TuneEditor::timerCallback() {
  if (lastProgram != p.getCurrentProgram()) {
    lastProgram = p.getCurrentProgram();
    filter();
  } else
    refresh();
}
void TuneEditor::resized() {
  int w = getWidth(), h = getHeight(), left = 250, right = 310, cx = left + 30,
      cw = w - cx - right - 40, rx = w - right - 20;
  auth.setBounds(w - 315, 18, 295, 28);
  a.setBounds(w - 315, 60, 43, 27);
  b.setBounds(w - 263, 60, 43, 27);
  copy.setBounds(w - 211, 60, 80, 27);
  bypass.setBounds(w - 121, 60, 101, 27);
  for (int i = 0; i < 4; ++i)
    banks[size_t(i)].setBounds(cx + i * (w - cx - 20) / 4, 107,
                               (w - cx - 20) / 4 - 10, 42);
  search.setBounds(18, 119, left - 18, 30);
  previous.setBounds(18, 159, 35, 27);
  next.setBounds(60, 159, 35, 27);
  random.setBounds(102, 159, 78, 27);
  fav.setBounds(187, 159, 48, 27);
  list.setBounds(18, 211, left - 10, h - 243);
  model.setBounds(cx, 168, 210, 28);
  key.setBounds(cx + 220, 168, 65, 28);
  scale.setBounds(cx + 295, 168, 150, 28);
  autoKey.setBounds(cx + 455, 168, 100, 28);
  quality.setBounds(cx + 565, 168, std::max(115, cw - 565), 28);
  waveArea = {cx, 215, cw, 76};
  pianoArea = {cx, 308, cw, std::max(230, h - 576)};
  int ky = h - 221;
  for (int i = 0; i < 6; ++i)
    knobs[size_t(i)]->setBounds(cx + i * cw / 6, ky, cw / 6 - 5, 172);
  harmony.setBounds(rx + 8, 258, 140, 32);
  doubles.setBounds(rx + 158, 258, 144, 32);
  count.setBounds(rx + 8, 307, 90, 28);
  interval.setBounds(rx + 108, 307, 194, 28);
  custom.setBounds(rx + 92, 346, 210, 23);
  for (int i = 0; i < 6; ++i)
    voices[size_t(i)].setBounds(rx + 8 + (i % 2) * 150, 391 + (i / 2) * 60, 142,
                                39);
  for (int i = 0; i < 6; ++i)
    fx[size_t(i)].setBounds(rx + 94, 589 + i * 30, 208, 25);
  doubleHuman.setBounds(rx + 8, h - 70, 294, 25);
}
void TuneEditor::paintListBoxItem(int row, juce::Graphics &g, int width,
                                  int height, bool selected) {
  if (row < 0 || row >= int(filtered.size()))
    return;
  int index = filtered[size_t(row)];
  if (selected) {
    g.setColour(gold.withAlpha(.13f));
    g.fillRoundedRectangle(1, 1, float(width - 3), float(height - 2), 5);
    g.setColour(gold.withAlpha(.6f));
    g.drawRoundedRectangle(1, 1, float(width - 3), float(height - 2), 5, 1);
  }
  alien(g, {6, 8, 14, 19}, selected ? gold : muted);
  label(g, juce::String(index % 25 + 1).paddedLeft('0', 2), {24, 0, 27, height},
        9, muted);
  label(g, tune::presets()[size_t(index)].name, {55, 0, width - 62, height},
        10.5f, selected ? white : muted, juce::Justification::centredLeft);
}
void TuneEditor::selectedRowsChanged(int row) {
  if (!selecting && row >= 0 && row < int(filtered.size())) {
    p.setCurrentProgram(filtered[size_t(row)]);
    refresh();
  }
}
void TuneEditor::paint(juce::Graphics &g) {
  int w = getWidth(), h = getHeight(), cx = 280, cw = w - cx - 350,
      rx = w - 330;
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff162d47), 0, 0, navy,
                                         float(w), float(h), false));
  g.fillAll();
  g.setColour(gold.withAlpha(.22f));
  g.drawLine(18, 100, float(w - 20), 100);
  g.drawLine(260, 110, 260, float(h - 26));
  alien(g, {25, 23, 44, 57}, gold);
  g.setFont(juce::Font(TuneSkin::graffiti()).withHeight(26));
  g.setColour(gold);
  g.drawText("M7G ALIEN TUNE GOD", 91, 18, w - 450, 39,
             juce::Justification::centredLeft);
  label(g, "V1 TUNE GOD / MAKKINE SEVEN GOLDEN", {93, 61, 500, 17}, 10, cyan,
        juce::Justification::centredLeft);
  label(g, "LAS MAKKINAS PRODUCEN SOLAS", {93, 82, 500, 13}, 9, gold,
        juce::Justification::centredLeft);
  label(g, juce::String(tune::banks[size_t(p.bank())]) + " / 25 PRESETS",
        {20, 191, 225, 16}, 9, gold, juce::Justification::centredLeft);
  auto panel = [&](juce::Rectangle<int> r, juce::Colour c) {
    g.setColour(navy);
    g.fillRoundedRectangle(r.toFloat(), 8);
    g.setColour(c.withAlpha(.4f));
    g.drawRoundedRectangle(r.toFloat(), 8, 1);
  };
  panel(waveArea, gold);
  panel(pianoArea, blue);
  panel({rx, 215, 310, h - 251}, gold);
  label(g, juce::String(tune::presets()[size_t(p.getCurrentProgram())].name),
        waveArea.reduced(10).withHeight(20), 12, white,
        juce::Justification::centredLeft);
  juce::Path waveform;
  int wh = p.waveHead.load();
  for (int i = 0; i < 512; ++i) {
    float x = waveArea.getX() + 8 + (waveArea.getWidth() - 16) * i / 511.f,
          y = waveArea.getBottom() - 17 -
              p.wave[size_t((wh + i) & 511)].load() * 35;
    if (!i)
      waveform.startNewSubPath(x, y);
    else
      waveform.lineTo(x, y);
  }
  g.setColour(gold.withAlpha(.8f));
  g.strokePath(waveform, juce::PathStrokeType(1.1f));
  int bottom = 36, top = 96;
  float note = p.inputNote.load();
  if (note > 35) {
    bottom = std::clamp(int(note) - 24, 24, 60);
    top = bottom + 48;
  }
  auto roll = pianoArea.reduced(6);
  int keyboard = 43;
  float sh = float(roll.getHeight()) / (top - bottom);
  for (int n = bottom; n <= top; ++n) {
    float y = roll.getBottom() - (n - bottom + 1) * sh;
    int pc = (n % 12 + 12) % 12;
    bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    g.setColour(black ? juce::Colour(0xff0a1020) : juce::Colour(0xff1c2b43));
    g.fillRect(float(roll.getX()), y, float(roll.getWidth()), sh);
    g.setColour(white.withAlpha(.07f));
    g.drawHorizontalLine(int(y), float(roll.getX()), float(roll.getRight()));
    if (pc == 0)
      label(g, juce::MidiMessage::getMidiNoteName(n, true, true, 4),
            {roll.getX() + 2, int(y), 38, int(sh + 2)}, 8, muted,
            juce::Justification::centredLeft);
  }
  int head = p.noteHead.load();
  float step = (roll.getWidth() - keyboard) / 512.f;
  for (int i = 0; i < 512; ++i) {
    auto &column = p.roll[size_t((head + i) & 511)];
    if (column.strength.load() < .6f)
      continue;
    float x = roll.getX() + keyboard + i * step;
    for (int v = 0; v < 8; ++v) {
      if (v >= 2 && v <= 5 &&
          (p.value("harmonyOn") < .5f || v - 1 > p.value("voices")))
        continue;
      if (v >= 6 && p.value("doublesOn") < .5f)
        continue;
      float n = column.notes[size_t(v)].load();
      if (n < bottom || n > top)
        continue;
      float y = roll.getBottom() - (n - bottom + .5f) * sh;
      auto c = v == 0   ? gold.withAlpha(.22f)
               : v == 1 ? gold
               : v % 2  ? purple
                        : blue;
      g.setColour(c);
      g.fillRoundedRectangle(x, y - 2.5f, std::max(2.f, step + 1),
                             v < 2 ? 5.f : 3.f, 2.f);
    }
  }
  label(g, "VOZ DETECTADA: " + p.profileText(),
        {cx, pianoArea.getBottom() + 10, cw, 19}, 11, cyan,
        juce::Justification::centredLeft);
  label(g,
        p.keyText() + " / CORRIGE " + juce::String(p.cents.load(), 1) +
            " cents",
        {cx, pianoArea.getBottom() + 31, cw, 17}, 10, muted,
        juce::Justification::centredLeft);
  label(g, "ALIEN CHOIR ENGINE", {rx + 8, 225, 294, 22}, 14, gold);
  label(g, "PERSONALIZADA", {rx + 8, 346, 82, 23}, 8, muted);
  for (int i = 0; i < 6; ++i) {
    float energy = std::min(1.f, p.voiceEnergy[size_t(i)].load() * 8);
    g.setColour(i % 2 ? purple : blue);
    g.fillRoundedRectangle(float(rx + 9 + (i % 2) * 150),
                           float(434 + (i / 2) * 60), 140 * energy, 3, 1);
  }
  label(g, "EFECTOS SOLO DE ESTA VOZ", {rx + 8, 564, 294, 18}, 10, gold);
  const char *labels[]{"ANCHO", "COLOR", "AIRE", "REVERB", "DISTORSION", "PAN"};
  for (int i = 0; i < 6; ++i)
    label(g, labels[i], {rx + 10, 589 + i * 30, 80, 25}, 9, muted,
          juce::Justification::centredLeft);
  label(g, "HUMANIZAR DOBLES", {rx + 8, h - 94, 294, 18}, 10, gold);
  auto core = p.activeQuality.load() == 0 ? 2048 : 512;
  juce::String neural =
      p.neural->ready.load() ? "ONNX LOCAL" : "YIN / RED NO DISPONIBLE";
  label(
      g,
      neural + " / LATENCIA " +
          juce::String(1000. * core /
                           (p.getSampleRate() > 0 ? p.getSampleRate() : 48000.),
                       2) +
          " ms / " + (p.activeQuality.load() == 0 ? "ESTUDIO" : "MONITOR"),
      {18, h - 25, w - 36, 18}, 9, muted, juce::Justification::centredLeft);
  label(g,
        "ORO: PRINCIPAL / AZUL Y VIOLETA: VOCES GENERADAS / HISTORIAL DE NOTAS "
        "Y DESTINOS",
        {cx, h - 46, cw, 14}, 8, muted, juce::Justification::centredLeft);
}
