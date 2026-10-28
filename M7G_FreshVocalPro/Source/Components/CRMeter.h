#pragma once
#include "Widgets.h"
namespace freshui {
class CRMeter final : public juce::Component, private juce::Timer {
public:
  explicit CRMeter(FreshProcessor &p) : processor(p) {
    setComponentID("gain-reduction-meter");
    startTimerHz(30);
  }
  void paint(juce::Graphics &) override;

private:
  void timerCallback() override { repaint(); }
  FreshProcessor &processor;
};
} // namespace freshui
