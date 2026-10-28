#pragma once
#include "PluginProcessor.h"
class TuneSkin final : public juce::LookAndFeel_V4 {
public:
  TuneSkin();
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
class TuneKnob final : public juce::Component {
public:
  TuneKnob(TuneProcessor &, const char *, const juce::String &,
           const juce::String &);
  void resized() override;
  void paint(juce::Graphics &) override;
  juce::Slider slider;

private:
  juce::String caption;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attachment;
};
class TuneEditor final : public juce::AudioProcessorEditor,
                         private juce::Timer,
                         private juce::ListBoxModel {
public:
  explicit TuneEditor(TuneProcessor &);
  ~TuneEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  void refresh();

private:
  TuneProcessor &p;
  TuneSkin skin;
  juce::TooltipWindow tooltip{this, 450};
  std::array<juce::TextButton, 4> banks;
  juce::ListBox list{"presets", this};
  juce::TextEditor search;
  juce::TextButton previous{"<"}, next{">"}, random{"RANDOM"}, fav{"FAV"},
      a{"A"}, b{"B"}, copy{"A > B"}, auth{"AUTHORIZED: M7G GOLDEN"},
      bypass{"BYPASS"};
  juce::TextButton harmony{"CREAR ARMONIAS"}, doubles{"DOBLES REALES"},
      autoKey{"AUTO TONO"};
  juce::ComboBox model, key, scale, quality, count, interval;
  std::array<juce::TextButton, 6> voices;
  std::array<juce::Slider, 6> fx;
  juce::Slider custom, doubleHuman;
  std::array<std::unique_ptr<TuneKnob>, 6> knobs;
  std::vector<int> filtered;
  int selectedVoice = 0, lastProgram = -1, lastBank = -1;
  bool selecting = false;
  using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
  using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  std::vector<std::unique_ptr<BA>> buttonAttachments;
  std::vector<std::unique_ptr<CA>> comboAttachments;
  std::array<std::unique_ptr<SA>, 6> fxAttachments;
  std::unique_ptr<SA> customAttachment, humanAttachment;
  std::unique_ptr<juce::FileChooser> chooser;
  juce::Rectangle<int> pianoArea, waveArea;
  void timerCallback() override;
  void filter();
  void navigate(int);
  void selectVoice(int);
  void importLicense();
  int getNumRows() override { return int(filtered.size()); }
  void paintListBoxItem(int, juce::Graphics &, int, int, bool) override;
  void selectedRowsChanged(int) override;
};
