#pragma once
#include "Widgets.h"
namespace freshui {
class HeaderComponent final : public juce::Component, private juce::Timer {
public:
  explicit HeaderComponent(FreshProcessor &);
  void paint(juce::Graphics &) override;
  void resized() override;
  std::function<void(int)> onMode;

private:
  void timerCallback() override;
  void file(bool save);
  FreshProcessor &processor;
  juce::TextButton load{"Load"}, save{"Save"}, previous{"<"}, next{">"},
      ab{"A / B"};
  std::array<juce::TextButton, 3> modes;
  juce::Label preset;
  std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace freshui
