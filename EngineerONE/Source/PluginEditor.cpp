#include "PluginEditor.h"
#include "OneData.h"
namespace {
const juce::Colour navy{0xff0a0f1e}, gold{0xffffd760}, white{0xffedf2fc},
    dim{0xff94a2ba};
juce::Colour versionColour(int i) {
  static const uint32_t colours[]{0xffffd760, 0xff66e8ba, 0xff65caff,
                                  0xffbf9bff, 0xffff9c9a};
  return juce::Colour(colours[juce::jlimit(0, 4, i)]);
}
void text(juce::Graphics &g, const juce::String &s, juce::Rectangle<int> r,
          float h, juce::Colour c = white,
          int alignment = juce::Justification::centred) {
  g.setFont(juce::Font(OneSkin::body()).withHeight(h));
  g.setColour(c);
  g.drawFittedText(s, r, alignment, 2);
}
void alien(juce::Graphics &g, juce::Rectangle<float> r) {
  juce::Path p;
  p.startNewSubPath(.5f, 1);
  p.cubicTo(-.1f, .6f, -.1f, 0, .5f, 0);
  p.cubicTo(1.1f, 0, 1.1f, .6f, .5f, 1);
  p.applyTransform(juce::AffineTransform::scale(r.getWidth(), r.getHeight())
                       .translated(r.getX(), r.getY()));
  g.setColour(gold);
  g.strokePath(p, juce::PathStrokeType(1.3f));
  g.fillEllipse(r.getX() + r.getWidth() * .15f, r.getY() + r.getHeight() * .32f,
                r.getWidth() * .27f, r.getHeight() * .1f);
  g.fillEllipse(r.getX() + r.getWidth() * .58f, r.getY() + r.getHeight() * .32f,
                r.getWidth() * .27f, r.getHeight() * .1f);
}
} // namespace
juce::Typeface::Ptr OneSkin::body() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      OneData::DejaVuSans_ttf, OneData::DejaVuSans_ttfSize);
  return f;
}
juce::Typeface::Ptr OneSkin::title() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      OneData::RubikWetPaintRegular_ttf, OneData::RubikWetPaintRegular_ttfSize);
  return f;
}
OneSkin::OneSkin() {
  setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff15243a));
  setColour(juce::ComboBox::textColourId, white);
  setColour(juce::PopupMenu::backgroundColourId, navy);
  setColour(juce::PopupMenu::textColourId, white);
  setColour(juce::Slider::thumbColourId, gold);
  setColour(juce::Slider::trackColourId, juce::Colour(0xff425777));
  setColour(juce::Slider::textBoxTextColourId, white);
  setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff516981));
}
void OneSkin::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                   const juce::Colour &, bool over, bool down) {
  auto r = b.getLocalBounds().toFloat().reduced(.5f);
  if (b.getComponentID() == "ia") {
    auto circle = r.withSizeKeepingCentre(100, 100).reduced(3);
    g.setGradientFill(
        juce::ColourGradient(juce::Colour(0xffffe899), circle.getCentreX(),
                             circle.getY(), juce::Colour(0xff91621c),
                             circle.getCentreX(), circle.getBottom(), false));
    g.fillEllipse(circle);
    g.setColour(gold.withAlpha(over ? 1.f : .7f));
    g.drawEllipse(circle.expanded(4), 1.5f);
    g.setColour(navy);
    g.drawEllipse(circle.reduced(5), 1);
    return;
  }
  auto c =
      b.getToggleState() ? juce::Colour(0xff5b5027) : juce::Colour(0xff182943);
  g.setGradientFill(juce::ColourGradient(over ? c.brighter(.2f) : c, r.getX(),
                                         r.getY(), navy, r.getX(),
                                         r.getBottom(), false));
  g.fillRoundedRectangle(r, 5);
  g.setColour(gold.withAlpha(down ? 1.f : b.getToggleState() ? .9f : .4f));
  g.drawRoundedRectangle(r, 5, 1);
}

void OneSkin::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool over,
                             bool down) {
  if (b.getComponentID() == "ia") {
    g.setColour(navy);
    g.setFont(juce::Font(body()).withHeight(36).boldened());
    g.drawText("IA", b.getLocalBounds(), juce::Justification::centred);
    return;
  }
  juce::LookAndFeel_V4::drawButtonText(g, b, over, down);
}
class OneEditor::Piano final : public juce::Component {
public:
  Piano(OneEditor &e) : editor(e) { setComponentID("pitch-roll"); }
  void invalidate() {
    cache = {};
    repaint();
  }
  void resized() override { invalidate(); }
  void mouseDown(const juce::MouseEvent &e) override {
    if (e.mods.isPopupMenu()) {
      editor.contextMenu();
      return;
    }
    hit = -1;
    if (editor.result)
      for (size_t i = 0; i < boxes.size(); ++i)
        if (boxes[i].contains(e.position)) {
          hit = int(i);
          break;
        }
    editor.selectNote(hit);
    down = e.position;
    if (hit >= 0 && editor.result)
      before = editor.result->notes[size_t(hit)];
    if (hit < 0)
      editor.processor.seek(
          editor.first +
          juce::jlimit(0., 1.,
                       double(e.x - 48) / std::max(1, getWidth() - 65)) *
              editor.duration);
  }
  void mouseDrag(const juce::MouseEvent &e) override {
    if (e.getDistanceFromDragStart() < 6 || dragged)
      return;
    if (!editor.editButton.getToggleState() || hit < 0) {
      editor.startDrag(hit);
      dragged = true;
      return;
    }
    if (!editor.result)
      return;
    auto notes = editor.result->notes;
    auto &n = notes[size_t(hit)];
    auto d = e.position - down;
    if (e.mods.isShiftDown())
      n.timeShift =
          juce::jlimit(-n.begin, editor.total - n.end,
                       before.timeShift + d.x / std::max(1, getWidth() - 65) *
                                              editor.duration);
    else if (e.mods.isAltDown())
      n.vibrato = juce::jlimit(0.f, 2.f, before.vibrato - d.y * .01f);
    else if (e.mods.isCtrlDown())
      n.gainDb = juce::jlimit(-24.f, 18.f, before.gainDb - d.y * .1f);
    else
      n.target = juce::jlimit(
          24.f, 100.f, before.target + std::round(-d.y / std::max(1.f, step)));
    n.touched = true;
    pending = std::move(notes);
    repaint();
  }
  void mouseUp(const juce::MouseEvent &) override {
    if (!pending.empty()) {
      editor.processor.retouch(std::move(pending));
      pending.clear();
    }
    dragged = false;
  }
  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &w) override {
    (w.deltaY > 0 ? editor.zoomIn : editor.zoomOut).triggerClick();
  }
  void paint(juce::Graphics &g) override {
    if (!cache.isValid())
      build();
    g.drawImageAt(cache, 0, 0);
    auto head = editor.processor.playhead.load();
    if (head >= editor.first && head <= editor.first + editor.duration) {
      g.setColour(white.withAlpha(.7f));
      g.drawVerticalLine(int(x(head)), 6.f, float(getHeight() - 25));
    }
    if (editor.selected >= 0 && editor.selected < int(boxes.size())) {
      g.setColour(white);
      g.drawRoundedRectangle(boxes[size_t(editor.selected)].expanded(2), 4,
                             1.5f);
    }
  }

private:
  float x(double seconds) const {
    return 48 + float((seconds - editor.first) / editor.duration) *
                    (getWidth() - 65);
  }
  float y(float note) const { return getHeight() - 36 - (note - low) * step; }
  void build() {
    cache = juce::Image(juce::Image::RGB, std::max(1, getWidth()),
                        std::max(1, getHeight()), true);
    juce::Graphics g(cache);
    g.fillAll(juce::Colour(0xff080f1e));
    boxes.clear();
    auto source = editor.input;
    low = 48;
    float high = 76;
    if (source && !source->analysis.notes.empty()) {
      low = 110;
      high = 0;
      for (auto &r : source->analysis.notes) {
        low = std::min(low, r.centre);
        high = std::max(high, r.centre);
      }
      low = std::floor(low) - 3;
      high = std::ceil(high) + 3;
      if (high - low < 10) {
        low -= 3;
        high += 3;
      }
    }
    step = (getHeight() - 137) / (high - low);
    for (int n = int(low); n <= int(high); ++n) {
      bool black = n % 12 == 1 || n % 12 == 3 || n % 12 == 6 || n % 12 == 8 ||
                   n % 12 == 10;
      g.setColour(black ? juce::Colour(0xff111a2c) : juce::Colour(0xff18243a));
      g.fillRect(0, int(y(float(n)) - .5f * step), 43, int(step));
      g.setColour(juce::Colour(0xff23304a));
      g.drawHorizontalLine(int(y(float(n))), 48.f, float(getWidth() - 16));
      text(g, juce::MidiMessage::getMidiNoteName(n, true, true, 3),
           {2, int(y(float(n))) - 8, 38, 16}, 10, dim);
    }
    for (int i = 0; i <= 8; ++i) {
      double t = editor.first + editor.duration * i / 8;
      g.setColour(juce::Colour(0xff26344c));
      g.drawVerticalLine(int(x(t)), 88.f, float(getHeight() - 28));
      text(g, juce::String(t, 2) + "s",
           {int(x(t)) - 30, getHeight() - 23, 60, 18}, 10, dim);
    }
    if (!source) {
      text(g, "CAPTURA TU VOZ  >  IA AFINA SOLA  >  ESCUCHA  >  ARRASTRA",
           getLocalBounds(), 18, gold);
      return;
    }
    const auto &waveform =
        editor.result ? editor.result->waveform : source->waveform;
    for (int px = 48; px < getWidth() - 16; ++px) {
      int index =
          juce::jlimit(0, int(waveform.size()) - 1,
                       int((editor.first + editor.duration * double(px - 48) /
                                               (getWidth() - 65)) /
                           editor.total * waveform.size()));
      if (index >= 0 && index < int(waveform.size())) {
        g.setColour(gold.withAlpha(.65f));
        float h = waveform[size_t(index)] * 29;
        g.drawVerticalLine(px, 41 - h, 41 + h);
      }
    }
    if (editor.result)
      for (auto &n : editor.result->notes) {
        juce::Rectangle<float> r(
            x(n.begin + n.timeShift), y(n.target) - step * .36f,
            std::max(3.f, x(n.end + n.timeShift) - x(n.begin + n.timeShift)),
            step * .72f);
        boxes.push_back(r);
        g.setColour(gold.withAlpha(n.touched ? .5f : .24f));
        g.fillRoundedRectangle(r, 4);
        g.setColour(gold.withAlpha(.7f));
        g.drawRoundedRectangle(r, 4, 1);
      }
    auto draw = [&](const one::Analysis &a, juce::Colour colour, float width) {
      juce::Path path;
      bool connected = false;
      float last = 0;
      for (int px = 48; px < getWidth() - 16; ++px) {
        double t = editor.first +
                   editor.duration * double(px - 48) / (getWidth() - 65);
        int i = int(std::round(t / a.hopSeconds));
        if (i < 0 || i >= int(a.frames.size()) || !a.frames[size_t(i)].voiced) {
          connected = false;
          continue;
        }
        float n = a.frames[size_t(i)].note;
        if (!connected || std::abs(n - last) > 1.7f)
          path.startNewSubPath(float(px), y(n));
        else
          path.lineTo(float(px), y(n));
        connected = true;
        last = n;
      }
      g.setColour(colour);
      g.strokePath(path, juce::PathStrokeType(width));
    };
    draw(source->analysis, juce::Colour(0xff8992a8).withAlpha(.85f), 1.6f);
    if (editor.result)
      draw(editor.result->measured, gold, 2.f);
  }
  OneEditor &editor;
  juce::Image cache;
  float low = 48, step = 15;
  std::vector<juce::Rectangle<float>> boxes;
  int hit = -1;
  juce::Point<float> down;
  one::Note before;
  bool dragged = false;
  std::vector<one::Note> pending;
};
class OneEditor::DragPad final : public juce::Component {
public:
  DragPad(OneEditor &e, bool single) : editor(e), note(single) {
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setComponentID(single ? "drag-note" : "drag-line");
  }
  void mouseDown(const juce::MouseEvent &e) override {
    if (e.mods.isPopupMenu())
      editor.contextMenu();
    dragged = false;
  }
  void mouseDrag(const juce::MouseEvent &e) override {
    if (e.getDistanceFromDragStart() > 5 && !dragged) {
      editor.startDrag(note ? editor.selected : -1);
      dragged = true;
    }
  }
  void paint(juce::Graphics &g) override {
    auto r = getLocalBounds().toFloat().reduced(1);
    bool ready = editor.result && (!note || editor.selected >= 0);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff344265), 0, 0, navy,
                                           0, r.getHeight(), false));
    g.fillRoundedRectangle(r, 10);
    g.setColour(gold.withAlpha(ready ? .85f : .25f));
    g.drawRoundedRectangle(r, 10, 1.4f);
    if (!note) {
      juce::Path hand;
      hand.startNewSubPath(6, 22);
      hand.lineTo(6, 7);
      hand.cubicTo(6, 2, 12, 2, 12, 7);
      hand.lineTo(12, 19);
      hand.lineTo(12, 2);
      hand.cubicTo(12, -2, 18, -2, 18, 2);
      hand.lineTo(18, 18);
      hand.lineTo(18, 5);
      hand.cubicTo(18, 1, 24, 1, 24, 5);
      hand.lineTo(24, 20);
      hand.lineTo(24, 11);
      hand.cubicTo(24, 7, 30, 7, 30, 11);
      hand.lineTo(30, 30);
      hand.cubicTo(30, 46, 14, 48, 9, 38);
      hand.lineTo(0, 25);
      hand.cubicTo(-2, 21, 2, 18, 6, 22);
      g.setColour(ready ? gold : dim);
      g.strokePath(
          hand, juce::PathStrokeType(1.5f),
          juce::AffineTransform::translation(32, float(getHeight() / 2 - 20)));
    }
    text(g, note ? "ARRASTRAR NOTA" : "ARRASTRA ESTA LINEA A TU PROYECTO",
         getLocalBounds().reduced(14).withTrimmedBottom(22), note ? 12.f : 18.f,
         ready ? gold : dim);
    text(g,
         note
             ? "WAV individual + timestamp"
             : "MANO  /  WAV 24-bit - 48 kHz  /  conserva silencios y duracion",
         getLocalBounds().withTrimmedTop(getHeight() - 30), note ? 9.f : 11.f,
         dim);
  }

private:
  OneEditor &editor;
  bool note, dragged = false;
};
OneEditor::OneEditor(OneProcessor &p)
    : AudioProcessorEditor(p), AudioProcessorEditorARAExtension(&p),
      processor(p) {
  setLookAndFeel(&skin);
  piano = std::make_unique<Piano>(*this);
  dragLine = std::make_unique<DragPad>(*this, false);
  dragNote = std::make_unique<DragPad>(*this, true);
  addAndMakeVisible(*piano);
  addAndMakeVisible(*dragLine);
  addAndMakeVisible(*dragNote);
  for (auto *b :
       {&captureButton, &stopButton, &iaButton, &playButton, &abButton,
        &muteButton, &importButton, &editButton, &applyButton, &cancelButton,
        &zoomIn, &zoomOut, &left, &right}) {
    addAndMakeVisible(b);
    b->setColour(juce::TextButton::textColourOffId, white);
  }
  captureButton.setComponentID("capture");
  iaButton.setComponentID("ia");
  playButton.setComponentID("play");
  stopButton.setComponentID("stop");
  editButton.setComponentID("optional-edit");
  iaButton.setColour(juce::TextButton::textColourOffId, gold);
  iaButton.setTooltip("YO AFINO LA VOZ ENTERA. Cada click crea otra "
                      "interpretacion natural. No necesitas mover notas.");
  captureButton.setTooltip("Pulsa y reproduce tu pista. Con ARA, lee el evento "
                           "seleccionado directamente.");
  abButton.setTooltip(
      "Compara la toma sin corregir con la version IA al mismo volumen.");
  captureButton.onClick = [this] { processor.capture(); };
  stopButton.onClick = [this] {
    processor.stop();
    if (processor.recorder.active())
      processor.finishCapture();
  };
  iaButton.onClick = [this] { processor.engineer(); };
  playButton.onClick = [this] { processor.play(false); };
  abButton.setClickingTogglesState(true);
  abButton.onClick = [this] {
    processor.original.store(abButton.getToggleState());
    processor.play(abButton.getToggleState());
  };
  muteButton.setClickingTogglesState(true);
  muteButton.onClick = [this] {
    processor.mute.store(muteButton.getToggleState());
  };
  importButton.onClick = [this] { chooseFile(); };
  cancelButton.onClick = [this] { processor.cancel(); };
  editButton.setClickingTogglesState(true);
  editButton.onClick = [this] {
    resized();
    repaint();
  };
  applyButton.onClick = [this] { processor.applyToEvent(); };
  zoomIn.onClick = [this] {
    duration = std::max(.4, duration * .6);
    first = juce::jlimit(0., std::max(0., total - duration), first);
    piano->invalidate();
  };
  zoomOut.onClick = [this] {
    duration = std::min(total, duration / .6);
    first = juce::jlimit(0., std::max(0., total - duration), first);
    piano->invalidate();
  };
  left.onClick = [this] {
    first = std::max(0., first - duration * .7);
    piano->invalidate();
  };
  right.onClick = [this] {
    first = std::min(std::max(0., total - duration), first + duration * .7);
    piano->invalidate();
  };
  pitch.setRange(-12, 12, .01);
  time.setRange(-500, 500, 1);
  vibrato.setRange(0, 200, 1);
  gain.setRange(-24, 18, .1);
  for (auto *s : {&pitch, &time, &vibrato, &gain}) {
    addAndMakeVisible(s);
    s->setSliderStyle(juce::Slider::LinearHorizontal);
    s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 22);
    s->onDragEnd = [this] { editSelected(); };
    s->onValueChange = [this] {
      if (!juce::ModifierKeys::getCurrentModifiersRealtime()
               .isAnyMouseButtonDown())
        editSelected();
    };
  }
  pitch.setTooltip("RETOQUE OPCIONAL: cambia el centro de la nota seleccionada "
                   "(semitonos).");
  time.setTooltip(
      "RETOQUE OPCIONAL: adelanta o atrasa la nota (milisegundos).");
  vibrato.setTooltip("RETOQUE OPCIONAL: cuanto vibrato natural conservas.");
  gain.setTooltip("RETOQUE OPCIONAL: volumen de esta nota (dB).");
  setResizable(true, true);
  setResizeLimits(1120, 740, 2000, 1400);
  setSize(1400, 920);
  startTimerHz(20);
}
OneEditor::~OneEditor() {
  processor.stop();
  stopTimer();
  setLookAndFeel(nullptr);
}
void OneEditor::chooseFile() {
  chooser = std::make_unique<juce::FileChooser>(
      "Importa una vocal cruda", juce::File(), "*.wav;*.aiff;*.flac");
  juce::Component::SafePointer<OneEditor> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode |
                           juce::FileBrowserComponent::canSelectFiles,
                       [safe](const juce::FileChooser &f) {
                         if (safe && f.getResult().existsAsFile())
                           safe->processor.loadFile(f.getResult());
                       });
}
bool OneEditor::isInterestedInFileDrag(const juce::StringArray &f) {
  return f.size() == 1 &&
         (f[0].endsWithIgnoreCase(".wav") || f[0].endsWithIgnoreCase(".aiff") ||
          f[0].endsWithIgnoreCase(".flac"));
}
void OneEditor::filesDropped(const juce::StringArray &f, int, int) {
  if (isInterestedInFileDrag(f))
    processor.loadFile(juce::File(f[0]));
}
void OneEditor::startDrag(int index) {
  auto p = processor.snapshot();
  if (!p)
    return;
  if (processor.exportStale.load()) {
    processor.setMessage("El evento cambio. CAPTURA y pulsa IA para exportar "
                         "su nueva posicion.");
    return;
  }
  juce::File file = p->lineFile;
  if (index >= 0 && index < int(p->noteFiles.size()))
    file = p->noteFiles[size_t(index)];
  if (!file.existsAsFile()) {
    processor.setMessage(
        "El WAV aun no esta listo. Espera al final del proceso IA.");
    return;
  }
  if (!juce::DragAndDropContainer::performExternalDragDropOfFiles(
          {file.getFullPathName()}, false, this))
    processor.setMessage("Arrastre no disponible. El WAV esta en: " +
                         file.getFullPathName());
}
void OneEditor::contextMenu() {
  juce::PopupMenu menu;
  menu.addItem(1,
               processor.araAvailable()
                   ? "Aplicar version en evento ARA"
                   : "Activar/desactivar correccion en esta pista",
               bool(processor.snapshot()));
  menu.addItem(2, "Mostrar WAV de la linea", bool(processor.snapshot()));
  menu.addItem(3, "Mostrar WAV de la nota", selected >= 0);
  juce::Component::SafePointer<OneEditor> safe(this);
  menu.showMenuAsync({}, [safe](int id) {
    if (!safe)
      return;
    if (id == 1)
      safe->processor.applyToEvent();
    if (auto p = safe->processor.snapshot()) {
      if (id == 2)
        p->lineFile.revealToUser();
      if (id == 3 && safe->selected >= 0 &&
          safe->selected < int(p->noteFiles.size()))
        p->noteFiles[size_t(safe->selected)].revealToUser();
    }
  });
}
void OneEditor::selectNote(int i) {
  selected = i;
  updating = true;
  if (result && i >= 0 && i < int(result->notes.size())) {
    auto &n = result->notes[size_t(i)];
    pitch.setValue(n.target - n.original, juce::dontSendNotification);
    time.setValue(n.timeShift * 1000, juce::dontSendNotification);
    vibrato.setValue(n.vibrato * 100, juce::dontSendNotification);
    gain.setValue(n.gainDb, juce::dontSendNotification);
  }
  updating = false;
  for (auto *s : {&pitch, &time, &vibrato, &gain})
    s->setEnabled(i >= 0);
  piano->repaint();
  dragNote->repaint();
}
void OneEditor::editSelected() {
  if (updating || !result || selected < 0 ||
      selected >= int(result->notes.size()))
    return;
  auto notes = result->notes;
  auto &n = notes[size_t(selected)];
  n.target = n.original + float(pitch.getValue());
  n.timeShift = juce::jlimit(-n.begin, total - n.end, time.getValue() / 1000);
  n.vibrato = float(vibrato.getValue() / 100);
  n.gainDb = float(gain.getValue());
  n.touched = true;
  processor.retouch(std::move(notes));
}
void OneEditor::refresh() {
  auto r = processor.snapshot();
  auto in = processor.inputSnapshot();
  if (r != result || in != input) {
    bool newInput = in != input;
    result = r;
    input = in;
    if (input) {
      total = input->audio.getNumSamples() / input->rate;
      if (newInput) {
        first = 0;
        duration = total;
        selected = -1;
      }
    }
    piano->invalidate();
    dragLine->repaint();
    dragNote->repaint();
    repaint();
  }
  applyButton.setButtonText(processor.araAvailable() ? "APLICAR EN EVENTO ARA"
                                                     : "APLICAR EN PISTA");
  bool active = processor.busy.load();
  iaButton.setEnabled(input && !active && !processor.recorder.active());
  playButton.setEnabled(bool(result));
  applyButton.setEnabled(bool(result));
  cancelButton.setEnabled(active);
}
void OneEditor::timerCallback() {
  refresh();
  piano->repaint();
  repaint(0, 0, getWidth(), 200);
  repaint(0, getHeight() - 125, getWidth(), 125);
}
void OneEditor::resized() {
  int w = getWidth(), h = getHeight();
  captureButton.setBounds(25, 105, 145, 42);
  stopButton.setBounds(180, 105, 85, 42);
  importButton.setBounds(25, 157, 150, 28);
  cancelButton.setBounds(185, 157, 80, 28);
  iaButton.setBounds(w / 2 - 60, 85, 120, 108);
  playButton.setBounds(w - 445, 105, 130, 42);
  abButton.setBounds(w - 305, 105, 165, 42);
  muteButton.setBounds(w - 130, 105, 105, 42);
  applyButton.setBounds(w - 445, 157, 220, 28);
  editButton.setBounds(w - 215, 157, 190, 28);
  bool editing = editButton.getToggleState();
  int bottom = h - (editing ? 222 : 153);
  piano->setBounds(25, 210, w - 50, bottom - 210);
  zoomIn.setBounds(w - 155, 193, 30, 20);
  zoomOut.setBounds(w - 120, 193, 30, 20);
  left.setBounds(w - 85, 193, 25, 20);
  right.setBounds(w - 55, 193, 25, 20);
  for (auto *s : {&pitch, &time, &vibrato, &gain})
    s->setVisible(editing);
  if (editing) {
    int unit = (w - 70) / 4;
    pitch.setBounds(25, bottom + 27, unit, 28);
    time.setBounds(40 + unit, bottom + 27, unit, 28);
    vibrato.setBounds(55 + unit * 2, bottom + 27, unit, 28);
    gain.setBounds(70 + unit * 3, bottom + 27, unit, 28);
  }
  dragLine->setBounds(25, h - 139, w - 310, 78);
  dragNote->setBounds(w - 270, h - 139, 245, 78);
}
void OneEditor::paint(juce::Graphics &g) {
  g.setGradientFill(juce::ColourGradient(navy, 0, 0, juce::Colour(0xff162840),
                                         float(getWidth()), float(getHeight()),
                                         false));
  g.fillAll();
  g.setColour(gold.withAlpha(.4f));
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(9), 13, 1);
  alien(g, {28, 22, 32, 39});
  g.setFont(juce::Font(OneSkin::title()).withHeight(34));
  g.setColour(gold);
  g.drawText("MAKKINE ENGINEER ONE", 75, 20, getWidth() - 430, 40,
             juce::Justification::left);
  text(g, "STUDIO ONE EDITION  /  AUTOMATIC VOCAL ENGINEER", {77, 62, 600, 22},
       12, dim, juce::Justification::left);
  text(g,
       processor.authorized ? "AUTHORIZED: M7G GOLDEN"
                            : "LICENCIA M7G NO ENCONTRADA",
       {getWidth() - 365, 25, 330, 22}, 12, gold, juce::Justification::right);
  text(g,
       processor.araAvailable() ? "VST3 + ARA  /  EVENTO CONECTADO"
                                : "VST3  /  CAPTURA DE PISTA",
       {getWidth() - 365, 52, 330, 22}, 11, dim, juce::Justification::right);
  text(g, "CAPTURA  >  IA AFINA SOLA  >  ESCUCHA  >  ARRASTRA",
       {280, 106, getWidth() / 2 - 350, 50}, 12, dim);
  if (input) {
    text(g, one::keyName(input->key), {25, 188, getWidth() - 580, 20}, 11, gold,
         juce::Justification::left);
    text(g,
         (input->origin.positionKnown
              ? "ORIGEN " + juce::String(input->origin.seconds, 3) + " s"
              : "ORIGEN NO INFORMADO") +
             (input->origin.tempoKnown
                  ? " / " + juce::String(input->origin.bpm, 1) + " BPM"
                  : ""),
         {getWidth() - 550, 188, 360, 20}, 10, dim, juce::Justification::right);
  }
  if (editButton.getToggleState()) {
    int y = getHeight() - 214, unit = (getWidth() - 70) / 4;
    text(g, "PITCH / semitonos", {25, y, unit, 20}, 10, dim);
    text(g, "TIEMPO / ms", {40 + unit, y, unit, 20}, 10, dim);
    text(g, "VIBRATO / %", {55 + 2 * unit, y, unit, 20}, 10, dim);
    text(g, "VOLUMEN / dB", {70 + 3 * unit, y, unit, 20}, 10, dim);
  }
  text(g, processor.statusText(), {25, getHeight() - 55, getWidth() - 50, 25},
       12, white, juce::Justification::left);
  float progressValue = processor.progress.load();
  if (processor.busy.load()) {
    g.setColour(gold);
    g.fillRoundedRectangle(25.f, float(getHeight() - 60),
                           float(getWidth() - 50) * progressValue, 3, 1);
  }
  text(g, "las makkinas producen solas", {25, getHeight() - 29, 370, 20}, 12,
       gold, juce::Justification::left);
  text(g,
       result ? "VERSION IA " + juce::String(result->interpretation) +
                    " / GRIS: ORIGINAL  / ORO: RESULTADO REAL"
              : "SIN AJUSTES OBLIGATORIOS / EDICION MANUAL OPCIONAL",
       {getWidth() - 720, getHeight() - 29, 690, 20}, 10, dim,
       juce::Justification::right);
}
