#pragma once
#include "PluginProcessor.h"
class TransitionsSkin final : public juce::LookAndFeel_V4 {
public:
  TransitionsSkin();
  static juce::Typeface::Ptr body();
  static juce::Typeface::Ptr graffiti();
  juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override {
    return body();
  }
  juce::Font getLabelFont(juce::Label &) override {
    return juce::Font(body()).withHeight(12);
  }
  juce::Font getTextButtonFont(juce::TextButton &, int) override {
    return juce::Font(body()).withHeight(12);
  }
  void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider &) override;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
  void drawButtonText(juce::Graphics &, juce::TextButton &, bool,
                      bool) override;
};
class TransitionKnob final : public juce::Component {
public:
  TransitionKnob(TransitionsProcessor &, const char *, juce::String,
                 juce::String);
  void resized() override;
  void paint(juce::Graphics &) override;
  juce::Slider slider;

private:
  juce::String caption;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attachment;
};
class TransitionsEditor final : public juce::AudioProcessorEditor,
                                private juce::Timer,
                                private juce::ListBoxModel {
public:
  explicit TransitionsEditor(TransitionsProcessor &);
  ~TransitionsEditor() override;
  void resized() override;
  void paint(juce::Graphics &) override;
  void refresh();

private:
  TransitionsProcessor &processor;
  TransitionsSkin skin;
  std::array<juce::TextButton, 3> banks;
  juce::ListBox presets{"presets", this};
  juce::TextEditor search;
  juce::TextButton previous{"<"}, next{">"}, random{"RANDOM"},
      favourite{juce::String::fromUTF8("FAV \xe2\x99\xa5")}, compareA{"A"},
      compareB{"B"}, stop{"STOP FX"}, bypass{"BYPASS"},
      authorization{"AUTHORIZED: M7G GOLDEN"}, menu{"..."};
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      bypassAttachment;
  std::array<std::unique_ptr<TransitionKnob>, 4> knobs;
  std::vector<int> filtered;
  bool selecting = false, onlyFavourites = false;
  int lastProgram = -1, animationRate = 30;
  float radar = 0;
  juce::TooltipWindow tooltip{this, 500};
  std::unique_ptr<juce::FileChooser> chooser;
  void timerCallback() override;
  void filter();
  void navigate(int);
  void importLicense();
  void showMenu();
  int getNumRows() override { return int(filtered.size()); }
  void paintListBoxItem(int, juce::Graphics &, int, int, bool) override;
  void selectedRowsChanged(int row) override;
};
