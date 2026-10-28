#include "PluginEditor.h"
#include "EngineerData.h"
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
  g.setFont(juce::Font(EngineerSkin::body()).withHeight(h));
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
juce::Typeface::Ptr EngineerSkin::body() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      EngineerData::DejaVuSans_ttf, EngineerData::DejaVuSans_ttfSize);
  return f;
}
juce::Typeface::Ptr EngineerSkin::title() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      EngineerData::RubikWetPaintRegular_ttf,
      EngineerData::RubikWetPaintRegular_ttfSize);
  return f;
}
EngineerSkin::EngineerSkin() {
  setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff15243a));
  setColour(juce::ComboBox::textColourId, white);
  setColour(juce::PopupMenu::backgroundColourId, navy);
  setColour(juce::PopupMenu::textColourId, white);
  setColour(juce::Slider::thumbColourId, gold);
  setColour(juce::Slider::trackColourId, juce::Colour(0xff425777));
  setColour(juce::Slider::textBoxTextColourId, white);
  setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff516981));
}
void EngineerSkin::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                        const juce::Colour &, bool over,
                                        bool down) {
  auto r = b.getLocalBounds().toFloat().reduced(.5f);
  auto c =
      b.getToggleState() ? juce::Colour(0xff5b5027) : juce::Colour(0xff182943);
  g.setGradientFill(juce::ColourGradient(over ? c.brighter(.2f) : c, r.getX(),
                                         r.getY(), navy, r.getX(),
                                         r.getBottom(), false));
  g.fillRoundedRectangle(r, 5);
  g.setColour(gold.withAlpha(down ? 1.f : b.getToggleState() ? .9f : .4f));
  g.drawRoundedRectangle(r, 5, 1);
}

class EngineerEditor::PitchPanel final : public juce::Component {
public:
  PitchPanel(EngineerEditor &e, int v) : editor(e), version(v) {}
  void setProject(std::shared_ptr<const engineer::Project> p, double start,
                  double length) {
    project = std::move(p);
    first = start;
    duration = length;
    cache = {};
    repaint();
  }
  void mouseDown(const juce::MouseEvent &e) override {
    editor.processor.seek(
        first +
        juce::jlimit(0., 1., double(e.x - 31) / std::max(1, getWidth() - 38)) *
            duration);
  }
  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &w) override {
    if (w.deltaY > 0)
      editor.zoomIn.triggerClick();
    else
      editor.zoomOut.triggerClick();
  }
  void paint(juce::Graphics &g) override {
    if (!cache.isValid())
      build();
    g.drawImageAt(cache, 0, 0);
    if (project) {
      double head = editor.processor.playhead.load();
      if (head >= first && head <= first + duration) {
        float x = 31 + float((head - first) / duration) * (getWidth() - 38);
        g.setColour(white.withAlpha(.7f));
        g.drawVerticalLine(int(x), 8.f, float(getHeight() - 12));
      }
    }
  }

private:
  void build() {
    cache = juce::Image(juce::Image::RGB, std::max(1, getWidth()),
                        std::max(1, getHeight()), true);
    juce::Graphics g(cache);
    g.fillAll(juce::Colour(0xff090f1c));
    if (!project) {
      text(g, "Carga una vocal\npara ver sus notas", getLocalBounds(), 13, dim);
      return;
    }
    int w = getWidth(), h = getHeight(), left = 31, right = w - 7, top = 83,
        bottom = h - 29;
    float low = 100, high = 0;
    for (const auto &r : project->original.notes) {
      low = std::min(low, r.centre);
      high = std::max(high, r.centre);
    }
    if (high < low) {
      low = 45;
      high = 72;
    }
    low = std::floor(low) - 2;
    high = std::ceil(high) + 2;
    if (high - low < 8) {
      low -= 4;
      high += 4;
    }
    auto y = [&](float note) {
      return bottom - (note - low) / (high - low) * (bottom - top);
    };
    for (int note = int(low); note <= int(high); ++note) {
      g.setColour(note % 12 == 0 ? juce::Colour(0xff374359)
                                 : juce::Colour(0xff19253b));
      g.drawHorizontalLine(int(y(float(note))), float(left), float(right));
      if (note % 2 == 0) {
        static const char *pitch[]{"C",  "C#", "D",  "D#", "E",  "F",
                                   "F#", "G",  "G#", "A",  "A#", "B"};
        text(g,
             juce::String(pitch[(note % 12 + 12) % 12]) +
                 juce::String(note / 12 - 1),
             {0, int(y(float(note))) - 7, 29, 14}, 9, dim);
      }
    }
    for (int mark = 0; mark <= 4; ++mark) {
      float x = left + (right - left) * mark / 4.f;
      g.setColour(juce::Colour(0xff202e44));
      g.drawVerticalLine(int(x), float(top), float(bottom));
      text(g, juce::String(first + duration * mark / 4, 1) + "s",
           {int(x) - 23, h - 22, 46, 16}, 9, dim);
    }
    // Cached overview bins: no full-file scanning or FFT in paint/timer
    // callbacks.
    if (!project->waveform.empty())
      for (int x = left; x < right; ++x) {
        double time = first + duration * (x - left) / std::max(1, right - left);
        int index = juce::jlimit(
            0, int(project->waveform.size() - 1),
            int(time / (project->source.getNumSamples() / project->rate) *
                project->waveform.size()));
        float peak = project->waveform[size_t(index)];
        g.setColour(versionColour(version).withAlpha(.5f));
        g.drawVerticalLine(x, 36 - peak * 24, 36 + peak * 24);
      }
    auto draw = [&](const engineer::Analysis &a, juce::Colour c,
                    float thickness) {
      juce::Path path;
      bool connected = false;
      float last = 0;
      for (int x = left; x <= right; ++x) {
        double time = first + duration * (x - left) / std::max(1, right - left);
        int index = int(std::round(time / a.hopSeconds));
        if (index < 0 || index >= int(a.frames.size()) ||
            !a.frames[size_t(index)].voiced) {
          connected = false;
          continue;
        }
        float note = a.frames[size_t(index)].note;
        if (!connected || std::abs(note - last) > 1.7f)
          path.startNewSubPath(float(x), y(note));
        else
          path.lineTo(float(x), y(note));
        connected = true;
        last = note;
      }
      g.setColour(c);
      g.strokePath(path,
                   juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));
    };
    draw(project->original, juce::Colour(0xffaeb4c0).withAlpha(.75f), 2.8f);
    draw(project->measured[size_t(version)], versionColour(version), 1.4f);
    text(g, "ORIGINAL GRIS / RESULTADO EN COLOR", {8, 65, w - 16, 15}, 8, dim);
  }
  EngineerEditor &editor;
  int version;
  double first = 0, duration = 1;
  std::shared_ptr<const engineer::Project> project;
  juce::Image cache;
};
class EngineerEditor::ExportPad final : public juce::Component,
                                        public juce::SettableTooltipClient {
public:
  ExportPad(EngineerEditor &e, int v) : editor(e), version(v) {
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setTooltip("Arrastra el WAV al mismo inicio del original. Conserva "
               "silencios, duracion, frecuencia y origen BWF si existe.");
  }
  void paint(juce::Graphics &g) override {
    auto r = getLocalBounds().toFloat().reduced(1);
    g.setColour(versionColour(version).withAlpha(isEnabled() ? .10f : .03f));
    g.fillRoundedRectangle(r, 7);
    g.setColour(versionColour(version).withAlpha(isEnabled() ? .8f : .2f));
    g.drawRoundedRectangle(r, 7, 1);
    text(g, "ARRASTRAR WAV A LA DAW", getLocalBounds().reduced(7), 11,
         isEnabled() ? versionColour(version) : dim);
  }
  void mouseDown(const juce::MouseEvent &) override { started = false; }
  void mouseDrag(const juce::MouseEvent &e) override {
    if (started || !isEnabled() || e.getDistanceFromDragStart() < 6)
      return;
    auto p = editor.processor.snapshot();
    if (!p)
      return;
    auto f = p->files[size_t(version)];
    if (!f.existsAsFile())
      return;
    started = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles(
        {f.getFullPathName()}, false, this);
  }

private:
  EngineerEditor &editor;
  int version;
  bool started = false;
};

EngineerEditor::EngineerEditor(EngineerProcessor &p)
    : AudioProcessorEditor(p), processor(p) {
  setLookAndFeel(&skin);
  for (auto *b :
       {&load, &original, &stop, &cancel, &rebuild, &zoomIn, &zoomOut, &folder})
    addAndMakeVisible(*b);
  load.onClick = [this] { chooseFile(); };
  original.onClick = [this] {
    processor.original.store(!processor.original.load());
    processor.playing.store(true);
  };
  stop.onClick = [this] { processor.stop(); };
  cancel.onClick = [this] { processor.cancelRender(); };
  rebuild.onClick = [this] { processor.rebuild(); };
  folder.onClick = [this] {
    if (auto p = processor.snapshot())
      p->files[0].getParentDirectory().revealToUser();
  };
  original.setTooltip("Compara el original con la version seleccionada sin "
                      "mover la posicion. Escucha por la salida del DAW.");
  scale.addItemList({"CROMATICA", "MAYOR", "MENOR"}, 1);
  root.addItemList(
      {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 1);
  addAndMakeVisible(scale);
  addAndMakeVisible(root);
  scale.setTooltip(
      "Cromatica respeta todas las notas. Elige mayor/menor y tonica si "
      "conoces la cancion. Cambiar recalcula las cinco versiones.");
  for (auto *s : {&strength, &volume}) {
    s->setSliderStyle(juce::Slider::LinearHorizontal);
    s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 49, 22);
    addAndMakeVisible(*s);
  }
  strength.textFromValueFunction = [](double v) {
    return juce::String(v * 100, 0) + "%";
  };
  strength.valueFromTextFunction = [](const juce::String &t) {
    return t.getDoubleValue() / 100.;
  };
  strength.setTooltip("Cuanto corrige. 100% aplica la filosofia completa de "
                      "cada version. Cambiar recalcula automaticamente.");
  volume.setTooltip("Volumen de escucha. No cambia el WAV exportado.");
  strengthAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.state, "strength", strength);
  volumeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.state, "volume", volume);
  scaleAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor.state, "scale", scale);
  rootAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor.state, "root", root);
  for (int i = 0; i < 5; ++i) {
    auto n = size_t(i);
    play[n].setButtonText("ESCUCHAR");
    solo[n].setButtonText("SOLO");
    mute[n].setButtonText("MUTE");
    save[n].setButtonText("GUARDAR WAV");
    for (auto *b : {&play[n], &solo[n], &mute[n], &save[n]})
      addAndMakeVisible(*b);
    play[n].onClick = [this, i] { processor.play(i); };
    solo[n].onClick = [this, i] { processor.setSolo(i); };
    mute[n].onClick = [this, i] {
      processor.setMute(i, !processor.isMuted(i));
    };
    save[n].onClick = [this, i] { saveVersion(i); };
    graphs[n] = std::make_unique<PitchPanel>(*this, i);
    drags[n] = std::make_unique<ExportPad>(*this, i);
    addAndMakeVisible(*graphs[n]);
    addAndMakeVisible(*drags[n]);
  }
  addAndMakeVisible(scroll);
  scroll.addListener(this);
  zoomIn.onClick = [this] {
    viewLength = std::max(.5, viewLength * .5);
    viewStart = std::min(viewStart, fullLength - viewLength);
    updateView();
  };
  zoomOut.onClick = [this] {
    viewLength = std::min(fullLength, viewLength * 2);
    viewStart = std::max(0., std::min(viewStart, fullLength - viewLength));
    updateView();
  };
  setResizable(true, true);
  setResizeLimits(1120, 720, 1900, 1222);
  getConstrainer()->setFixedAspectRatio(1400. / 900);
  setSize(1400, 900);
  startTimerHz(20);
  timerCallback();
}
EngineerEditor::~EngineerEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void EngineerEditor::updateView() {
  viewLength = std::min(fullLength, std::max(.5, viewLength));
  viewStart =
      juce::jlimit(0., std::max(0., fullLength - viewLength), viewStart);
  scroll.setRangeLimits(0, fullLength);
  scroll.setCurrentRange(viewStart, viewLength, juce::dontSendNotification);
  for (auto &g : graphs)
    g->setProject(project, viewStart, viewLength);
}
void EngineerEditor::timerCallback() {
  auto next = processor.snapshot();
  if (next != project) {
    project = std::move(next);
    viewStart = 0;
    fullLength = project ? project->source.getNumSamples() / project->rate : 1;
    viewLength = fullLength;
    updateView();
  }
  bool ready = project && !processor.busy.load();
  for (int i = 0; i < 5; ++i) {
    size_t n = size_t(i);
    play[n].setEnabled(ready);
    solo[n].setEnabled(ready);
    mute[n].setEnabled(ready);
    save[n].setEnabled(ready);
    drags[n]->setEnabled(ready);
    solo[n].setToggleState(processor.solo.load() == i,
                           juce::dontSendNotification);
    mute[n].setToggleState(processor.isMuted(i), juce::dontSendNotification);
    play[n].setToggleState(processor.playing.load() &&
                               !processor.original.load() &&
                               processor.selected.load() == i,
                           juce::dontSendNotification);
    graphs[n]->repaint();
  }
  original.setEnabled(ready);
  original.setToggleState(processor.original.load() && processor.playing.load(),
                          juce::dontSendNotification);
  cancel.setEnabled(processor.busy.load());
  folder.setEnabled(ready);
  rebuild.setEnabled(project && !processor.busy.load());
  repaint();
}
void EngineerEditor::resized() {
  uiScale = getWidth() / 1400.f;
  auto set = [](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(x, y, w, h);
  };
  set(load, 24, 103, 176, 34);
  set(original, 210, 103, 160, 34);
  set(stop, 380, 103, 78, 34);
  set(rebuild, 468, 103, 115, 34);
  set(cancel, 593, 103, 95, 34);
  set(scale, 760, 103, 135, 34);
  set(root, 903, 103, 60, 34);
  set(strength, 976, 105, 192, 30);
  set(volume, 1180, 105, 194, 30);
  set(folder, 1170, 190, 204, 22);
  set(zoomOut, 24, 682, 35, 26);
  set(zoomIn, 66, 682, 35, 26);
  set(scroll, 112, 687, 1262, 16);
  for (int i = 0; i < 5; ++i) {
    int x = 24 + i * 272;
    auto n = size_t(i);
    set(*graphs[n], x + 6, 262, 252, 406);
    set(play[n], x + 7, 723, 112, 32);
    set(solo[n], x + 125, 723, 60, 32);
    set(mute[n], x + 191, 723, 60, 32);
    set(*drags[n], x + 7, 765, 244, 42);
    set(save[n], x + 7, 817, 244, 30);
  }
  for (auto *child : getChildren())
    if (dynamic_cast<juce::ResizableCornerComponent *>(child) == nullptr &&
        dynamic_cast<juce::ResizableBorderComponent *>(child) == nullptr)
      child->setTransform(juce::AffineTransform::scale(uiScale));
}
void EngineerEditor::paint(juce::Graphics &real) {
  juce::Graphics::ScopedSaveState saveState(real);
  real.addTransform(juce::AffineTransform::scale(uiScale));
  auto &g = real;
  g.fillAll(navy);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff172d44), 0, 0, navy,
                                         1400, 900, false));
  g.fillRect(0, 0, 1400, 900);
  alien(g, {27, 21, 39, 56});
  g.setColour(gold);
  g.setFont(juce::Font(EngineerSkin::title()).withHeight(29));
  g.drawText("MAKKINE ENGINEER 5X", 85, 17, 860, 39,
             juce::Justification::centredLeft);
  text(g, "AI VOCAL TUNER EVOLUTION  /  CINCO VERSIONES DE UNA MISMA VOZ",
       {88, 61, 860, 19}, 12, dim, juce::Justification::centredLeft);
  text(g,
       processor.authorized ? "AUTHORIZED: M7G GOLDEN"
                            : "LICENCIA GLOBAL NO ENCONTRADA",
       {1090, 25, 283, 26}, 11, gold);
  text(g, "LAS MAKKINAS PRODUCEN SOLAS", {1030, 61, 343, 17}, 10, dim);
  g.setColour(gold.withAlpha(.25f));
  g.drawHorizontalLine(90, 24, 1376);
  text(g, "ESCALA", {761, 83, 135, 15}, 9, dim);
  text(g, "TONICA", {902, 83, 65, 15}, 9, dim);
  text(g, "CUANTO CORRIGE", {977, 83, 192, 15}, 9, dim);
  text(g, "VOLUMEN ESCUCHA", {1180, 83, 194, 15}, 9, dim);
  text(g, processor.statusText(), {24, 148, 1350, 23}, 13, white,
       juce::Justification::centredLeft);
  g.setColour(juce::Colour(0xff26374b));
  g.fillRoundedRectangle(24.f, 181.f, 1350.f, 4.f, 2);
  g.setColour(gold);
  g.fillRoundedRectangle(24.f, 181.f, 1350 * processor.progress.load(), 4.f, 2);
  if (project)
    text(g,
         project->sourceFile.getFileName() + "  /  " +
             juce::String(project->rate / 1000, 1) + " kHz  /  " +
             juce::String(project->source.getNumChannels()) + " canales  /  " +
             juce::String(project->original.phrases.size()) +
             " frases  /  Ruido estimado " +
             juce::String(project->original.noiseDb, 1) + " dB",
         {24, 192, 1136, 18}, 10, dim, juce::Justification::centredLeft);
  for (int i = 0; i < 5; ++i) {
    int x = 24 + i * 272;
    g.setColour(juce::Colour(0xff0d1728));
    g.fillRoundedRectangle(float(x), 220.f, 264.f, 637.f, 9);
    g.setColour(versionColour(i).withAlpha(.5f));
    g.drawRoundedRectangle(float(x), 220.f, 264.f, 637.f, 9, 1);
    text(g, engineer::names[i], {x + 5, 228, 254, 22}, 14, versionColour(i));
    text(g, engineer::descriptions[i], {x + 8, 250, 248, 14}, 9, dim);
  }
  text(g, "CLICK EN LA GRAFICA: MOVER ESCUCHA  /  RUEDA O +/-: ZOOM",
       {112, 704, 1260, 15}, 9, dim, juce::Justification::centredLeft);
  text(g,
       "WAV 32-BIT FLOAT / MISMA DURACION Y SILENCIO INICIAL / COLOCA EN EL "
       "MISMO INICIO DEL ORIGINAL",
       {24, 868, 1352, 17}, 10, gold);
  text(g,
       "Analisis YIN + decisiones por frase / Rubber Band R3 con formantes / "
       "Archivo vocal monofonico",
       {24, 887, 1352, 12}, 8, dim);
}
bool EngineerEditor::isInterestedInFileDrag(const juce::StringArray &files) {
  return files.size() == 1 &&
         juce::File(files[0]).hasFileExtension("wav;aiff;aif;flac");
}
void EngineerEditor::filesDropped(const juce::StringArray &files, int, int) {
  if (isInterestedInFileDrag(files))
    processor.loadFile(juce::File(files[0]));
}
void EngineerEditor::chooseFile() {
  chooser = std::make_unique<juce::FileChooser>(
      "Carga una vocal cruda", juce::File(), "*.wav;*.aiff;*.aif;*.flac");
  juce::Component::SafePointer<EngineerEditor> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode |
                           juce::FileBrowserComponent::canSelectFiles,
                       [safe](const juce::FileChooser &f) {
                         if (safe && f.getResult() != juce::File())
                           safe->processor.loadFile(f.getResult());
                       });
}
void EngineerEditor::saveVersion(int i) {
  auto p = processor.snapshot();
  if (!p)
    return;
  auto source = p->files[size_t(i)];
  chooser = std::make_unique<juce::FileChooser>("Guardar WAV alineado", source,
                                                "*.wav");
  juce::Component::SafePointer<EngineerEditor> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                           juce::FileBrowserComponent::canSelectFiles |
                           juce::FileBrowserComponent::warnAboutOverwriting,
                       [safe, source](const juce::FileChooser &f) {
                         if (safe && f.getResult() != juce::File()) {
                           auto target = f.getResult().withFileExtension("wav");
                           if (target != source && !source.copyFileTo(target))
                             juce::AlertWindow::showMessageBoxAsync(
                                 juce::AlertWindow::WarningIcon,
                                 "No se pudo guardar",
                                 "Elige una carpeta con permiso de escritura.");
                         }
                       });
}
