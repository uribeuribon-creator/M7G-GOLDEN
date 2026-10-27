#pragma once
#include "PluginProcessor.h"
class M7GEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit M7GEditor(M7GProcessor &);
  ~M7GEditor() override { stopTimer(); }
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  M7GProcessor &processor;
  juce::ComboBox voiceBox;
  juce::Slider intensity;
  juce::Label title, subtitle, action, input, output, intensityLabel;
  juce::AudioProcessorValueTreeState::ComboBoxAttachment voiceAttachment;
  juce::AudioProcessorValueTreeState::SliderAttachment intensityAttachment;
  void timerCallback() override;
};
