#pragma once
#include "Widgets.h"
namespace freshui {
class SidebarComponent final : public juce::Component,
                               public juce::DragAndDropContainer,
                               private juce::Timer {
public:
  explicit SidebarComponent(FreshProcessor &);
  ~SidebarComponent() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  std::function<void(int)> onSelect;

private:
  class Entry;
  void timerCallback() override;
  FreshProcessor &processor;
  std::array<std::unique_ptr<Entry>, 4> entries;
  int lastOrder = -1, lastMode = -1;
};
} // namespace freshui
