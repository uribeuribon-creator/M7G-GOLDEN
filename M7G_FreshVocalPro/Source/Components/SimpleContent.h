#pragma once
#include "Widgets.h"
namespace freshui {
class SimpleContent final : public juce::Component,
                            private juce::ListBoxModel,
                            private juce::Timer {
public:
  explicit SimpleContent(FreshProcessor &);
  void resized() override;
  void paint(juce::Graphics &) override;

private:
  int getNumRows() override;
  void paintListBoxItem(int, juce::Graphics &, int, int, bool) override;
  void selectedRowsChanged(int) override;
  void timerCallback() override;
  FreshProcessor &processor;
  Knob input, reverb, delay, output;
  juce::ListBox presets{"Factory presets", this};
  bool selecting = false;
};
} // namespace freshui
