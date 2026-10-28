#include "SimpleContent.h"
namespace freshui {
SimpleContent::SimpleContent(FreshProcessor &p)
    : processor(p), input(p, fresh::input, "INPUT", "GAIN / dB"),
      reverb(p, fresh::reverb, "REVERB", "AMOUNT"),
      delay(p, fresh::delay, "DELAY", "AMOUNT"),
      output(p, fresh::output, "OUTPUT", "GAIN / dB") {
  for (auto *c : std::array<juce::Component *, 5>{
           {&input, &reverb, &delay, &output, &presets}})
    addAndMakeVisible(*c);
  presets.setRowHeight(26);
  presets.setOutlineThickness(1);
  presets.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff37372e));
  presets.setComponentID("factory-presets");
  startTimerHz(20);
  timerCallback();
}
int SimpleContent::getNumRows() {
  return int(fresh::PresetManager::factory().size());
}
void SimpleContent::paintListBoxItem(int i, juce::Graphics &g, int w, int h,
                                     bool selected) {
  if (i < 0 || i >= getNumRows())
    return;
  if (selected) {
    g.setColour(gold.withAlpha(.15f));
    g.fillAll();
    g.setColour(gold);
    g.fillRect(0, 0, 2, h);
  }
  M7GLookAndFeel::text(g, fresh::PresetManager::factory()[size_t(i)].name,
                       {10, 0, w - 16, h}, 12, selected ? gold : white,
                       juce::Justification::left);
}
void SimpleContent::selectedRowsChanged(int index) {
  if (!selecting && index >= 0)
    processor.choosePreset(index);
}
void SimpleContent::timerCallback() {
  selecting = true;
  int selected = processor.selectedPreset.load();
  if (selected >= 0)
    presets.selectRow(selected, true, true);
  else
    presets.deselectAllRows();
  selecting = false;
  repaint();
}
void SimpleContent::resized() {
  int w = getWidth(), h = getHeight(), list = juce::jlimit(220, 290, w * 2 / 5),
      left = w - list - 26, unit = std::min((left - 24) / 2, (h - 24) / 2);
  int x = 12 + (left - unit * 2) / 2, y = (h - unit * 2) / 2;
  input.setBounds(x, y, unit, unit);
  reverb.setBounds(x + unit, y, unit, unit);
  delay.setBounds(x, y + unit, unit, unit);
  output.setBounds(x + unit, y + unit, unit, unit);
  presets.setBounds(w - list - 12, 22, list, h - 35);
}
void SimpleContent::paint(juce::Graphics &g) {
  g.fillAll(black);
  int list = juce::jlimit(220, 290, getWidth() * 2 / 5);
  M7GLookAndFeel::text(g, "FACTORY PRESETS",
                       {getWidth() - list - 12, 1, list, 18}, 10, dim);
  float in = juce::jlimit(0.f, 1.f, processor.meters().in.load()),
        out = juce::jlimit(0.f, 1.f, processor.meters().out.load());
  g.setColour(gold.withAlpha(.25f));
  g.fillRect(7, 20, 3, getHeight() - 40);
  g.fillRect(getWidth() - list - 23, 20, 3, getHeight() - 40);
  g.setColour(gold);
  g.fillRect(7, getHeight() - 20 - int(in * (getHeight() - 40)), 3,
             int(in * (getHeight() - 40)));
  g.fillRect(getWidth() - list - 23,
             getHeight() - 20 - int(out * (getHeight() - 40)), 3,
             int(out * (getHeight() - 40)));
}
} // namespace freshui
