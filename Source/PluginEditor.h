#pragma once
#include "PluginProcessor.h"
class AlienSkin final : public juce::LookAndFeel_V4 {
public:
  AlienSkin();
  static juce::Typeface::Ptr bodyTypeface();
  static juce::Typeface::Ptr titleTypeface();
  static juce::Typeface::Ptr graffitiTypeface();
  juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override {
    return bodyTypeface();
  }
  juce::Font getLabelFont(juce::Label &) override {
    return juce::Font(juce::FontOptions(bodyTypeface()).withHeight(11));
  }
  juce::Font getComboBoxFont(juce::ComboBox &) override {
    return juce::Font(juce::FontOptions(bodyTypeface()).withHeight(11));
  }
  juce::Font getPopupMenuFont() override {
    return juce::Font(juce::FontOptions(bodyTypeface()).withHeight(12));
  }
  void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider &) override;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
  void drawButtonText(juce::Graphics &, juce::TextButton &, bool,
                      bool) override;
  void drawToggleButton(juce::Graphics &, juce::ToggleButton &, bool,
                        bool) override;
  void drawComboBox(juce::Graphics &, int, int, bool, int, int, int, int,
                    juce::ComboBox &) override;
};
class RackKnob final : public juce::Component {
public:
  RackKnob(M7GProcessor &, rack::Param, juce::String);
  void resized() override;
  void paint(juce::Graphics &) override;
  void tick();
  juce::Slider slider;

private:
  juce::String caption;
  M7GProcessor &processor;
  rack::Param parameterID;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attachment;
};
class RackModule final : public juce::Component {
public:
  RackModule(M7GProcessor &, int);
  void resized() override;
  void paint(juce::Graphics &) override;
  void tick();

private:
  M7GProcessor &processor;
  int index;
  rack::Param enabledParameter;
  juce::ToggleButton enable;
  juce::TextButton analyse{"AI"}, learn{"LEARN 2s"};
  float gateDoor = 1;
  std::vector<std::unique_ptr<RackKnob>> knobs;
  std::vector<std::unique_ptr<juce::ToggleButton>> switches;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>
      switchAttachments;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      enableAttachment;
  juce::Slider parallel;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      parallelAttachment;
  std::array<float, 48> history{};
};
class M7GEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit M7GEditor(M7GProcessor &);
  ~M7GEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  M7GProcessor &processor;
  AlienSkin skin;
  std::array<std::unique_ptr<RackModule>, 26> modules;
  juce::ComboBox presetBox, modeBox;
  juce::TextButton classicTab{"CADENA VOCAL"}, engineTab{"LIMPIEZA / ARMONIAS"},
      tripleTab{"ESPACIO / COLOR"};
  int page = 0, bank = 0;
  std::array<juce::TextButton, 6> bankButtons;
  juce::TextEditor search;
  juce::TextButton previous{"<"}, next{">"}, random{"RANDOM"},
      favourite{juce::String::fromUTF8("FAV \xe2\x99\xa5")},
      favouritesOnly{"VER FAV"}, automation{"AUTO"},
      authorization{"AUTORIZACION"};
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      automationAttachment;
  std::unique_ptr<juce::FileChooser> licenseChooser;
  std::vector<int> visiblePresets;
  int lastProgram = -1;
  float radarAngle = 0;
  int animationRate = 60;
  bool onlyFavourites = false;
  void filterPresets();
  void navigate(int);
  void importLicense();
  void showPage(int);
  juce::ToggleButton godSpace{"GOD SPACE"};
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      modeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      godAttachment;
  juce::TextButton analyseAll{"ANALIZAR TODO"}, compareA{"A"}, compareB{"B"},
      menu{"..."};
  juce::ToggleButton bypass{"BYPASS"}, solo{"SOLO / DELTA"},
      autoLevel{"AUTO LEVEL"};
  std::unique_ptr<RackKnob> input, mix, output, producer;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      bypassAttachment, soloAttachment, autoAttachment;
  juce::Label status;
  juce::TooltipWindow tooltip{this, 500};
  void timerCallback() override;
  void showMenu();
};
