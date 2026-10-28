#include "PluginEditor.h"
using freshui::M7GLookAndFeel;
FreshEditor::FreshEditor(FreshProcessor &p)
    : AudioProcessorEditor(p), AudioProcessorEditorARAExtension(&p),
      processor(p), header(p), sidebar(p), simple(p) {
  setLookAndFeel(&skin);
  modules[0] = std::make_unique<freshui::DynamicsContent>(p);
  modules[1] = std::make_unique<freshui::EQContent>(p);
  modules[2] = std::make_unique<freshui::SpaceContent>(p);
  modules[3] = std::make_unique<freshui::CharacterContent>(p);
  for (auto &m : modules)
    addChildComponent(*m);
  addAndMakeVisible(header);
  addAndMakeVisible(sidebar);
  addAndMakeVisible(simple);
  addAndMakeVisible(bypass);
  bypass.setClickingTogglesState(true);
  bypass.setComponentID("global-bypass");
  bypassAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.state, "bypass", bypass);
  header.onMode = [this](int m) { setMode(m); };
  sidebar.onSelect = [this](int tab) { setTab(tab); };
  setWantsKeyboardFocus(true);
  setResizable(true, true);
  setResizeLimits(600, 400, 1800, 1200);
  currentTab = p.selectedTab.load();
  setMode(p.viewMode.load(), false);
  setTab(currentTab, false);
  startTimerHz(60);
}
FreshEditor::~FreshEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void FreshEditor::setMode(int mode, bool animate) {
  mode = juce::jlimit(0, 2, mode);
  processor.viewMode.store(mode);
  const juce::Point<int> sizes[]{{600, 400}, {1200, 750}, {1400, 900}};
  targetSize = sizes[mode];
  oldSize = {getWidth(), getHeight()};
  resizeStart = juce::Time::getMillisecondCounterHiRes();
  resizing = animate;
  simple.setVisible(mode == 0);
  sidebar.setVisible(mode != 0);
  for (int i = 0; i < 4; ++i) {
    modules[size_t(i)]->setExpert(mode == 2);
    modules[size_t(i)]->setVisible(mode != 0 && i == currentTab);
    modules[size_t(i)]->setAlpha(1);
  }
  oldTab = -1;
  if (!animate)
    setSize(targetSize.x, targetSize.y);
  resized();
}
void FreshEditor::setTab(int tab, bool animate) {
  tab = juce::jlimit(0, 3, tab);
  processor.selectedTab.store(tab);
  int previous = currentTab;
  currentTab = tab;
  if (processor.viewMode.load() == 0)
    return;
  for (int i = 0; i < 4; ++i)
    modules[size_t(i)]->setVisible(i == currentTab ||
                                   (animate && i == previous && i != tab));
  if (animate && previous != tab) {
    oldTab = previous;
    fadeStart = juce::Time::getMillisecondCounterHiRes();
    modules[size_t(tab)]->setAlpha(0);
    modules[size_t(tab)]->toFront(false);
  } else {
    oldTab = -1;
    modules[size_t(tab)]->setAlpha(1);
  }
  repaint();
}
void FreshEditor::resized() {
  int headerHeight = getWidth() < 900 ? 76 : 60;
  header.setBounds(0, 0, getWidth(), headerHeight);
  auto body = juce::Rectangle<int>(0, headerHeight, getWidth(),
                                   getHeight() - headerHeight - 24);
  simple.setBounds(body);
  sidebar.setBounds(body.removeFromLeft(180));
  for (auto &m : modules)
    m->setBounds(body);
  bypass.setBounds(getWidth() - 90, getHeight() - 22, 80, 19);
}
void FreshEditor::timerCallback() {
  double now = juce::Time::getMillisecondCounterHiRes();
  if (resizing) {
    double t = juce::jlimit(0., 1., (now - resizeStart) / 200.);
    double e = t * t * (3 - 2 * t);
    setSize(int(std::round(oldSize.x + (targetSize.x - oldSize.x) * e)),
            int(std::round(oldSize.y + (targetSize.y - oldSize.y) * e)));
    if (t >= 1)
      resizing = false;
  }
  if (oldTab >= 0) {
    float t = float(juce::jlimit(0., 1., (now - fadeStart) / 150.));
    modules[size_t(currentTab)]->setAlpha(t);
    modules[size_t(oldTab)]->setAlpha(1 - t);
    if (t >= 1) {
      modules[size_t(oldTab)]->setVisible(false);
      oldTab = -1;
    }
  }
  repaint(0, getHeight() - 24, getWidth(), 24);
}
bool FreshEditor::keyPressed(const juce::KeyPress &key) {
  if (!key.getModifiers().isCommandDown() && !key.getModifiers().isCtrlDown())
    return false;
  int code = key.getKeyCode();
  if (code == 'z' || code == 'Z') {
    processor.morphing.store(false);
    if (key.getModifiers().isShiftDown())
      processor.undo.redo();
    else
      processor.undo.undo();
    return true;
  }
  if (code == 'y' || code == 'Y') {
    processor.morphing.store(false);
    processor.undo.redo();
    return true;
  }
  return false;
}
void FreshEditor::paint(juce::Graphics &g) {
  g.fillAll(freshui::black);
  int y = getHeight() - 24;
  g.setColour(juce::Colour(0xff25251f));
  g.drawHorizontalLine(y, 0.f, float(getWidth()));
  M7GLookAndFeel::text(g,
                       processor.authorized
                           ? "M7G GOLDEN  /  FRESH VOCAL PRO BEAST"
                           : "M7G FRESH VOCAL PRO / LICENCIA NO ENCONTRADA",
                       {12, y + 3, getWidth() - 290, 18}, 9, freshui::dim,
                       juce::Justification::left);
  M7GLookAndFeel::text(g,
                       processor.ara() ? "ARA EVENT FX" : "VST3 / 35 PRESETS",
                       {getWidth() - 270, y + 3, 165, 18}, 9, freshui::gold,
                       juce::Justification::right);
}
