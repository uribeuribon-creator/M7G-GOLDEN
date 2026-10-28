#pragma once
#include "Components/CharacterContent.h"
#include "Components/DynamicsContent.h"
#include "Components/EQContent.h"
#include "Components/HeaderComponent.h"
#include "Components/SidebarComponent.h"
#include "Components/SimpleContent.h"
#include "Components/SpaceContent.h"
class FreshEditor final : public juce::AudioProcessorEditor,
                          public juce::AudioProcessorEditorARAExtension,
                          private juce::Timer {
public:
  explicit FreshEditor(FreshProcessor &);
  ~FreshEditor() override;
  void resized() override;
  void paint(juce::Graphics &) override;
  bool keyPressed(const juce::KeyPress &) override;
  void setMode(int, bool animate = true);
  void setTab(int, bool animate = true);

private:
  void timerCallback() override;
  FreshProcessor &processor;
  freshui::M7GLookAndFeel skin;
  freshui::HeaderComponent header;
  freshui::SidebarComponent sidebar;
  freshui::SimpleContent simple;
  std::array<std::unique_ptr<freshui::ModuleContent>, 4> modules;
  juce::TooltipWindow tips{this, 600};
  juce::TextButton bypass{"BYPASS"};
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      bypassAttachment;
  double resizeStart = 0, fadeStart = 0;
  juce::Point<int> oldSize, targetSize;
  int oldTab = -1, currentTab = 0;
  bool resizing = false;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreshEditor)
};
