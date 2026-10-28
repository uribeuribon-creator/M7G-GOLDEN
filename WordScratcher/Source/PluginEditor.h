#pragma once
#include "PluginProcessor.h"
class ScratchSkin final : public juce::LookAndFeel_V4 {
public:
  ScratchSkin();
  static juce::Typeface::Ptr body();
  static juce::Typeface::Ptr graffiti();
  juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override {
    return body();
  }
  juce::Font getLabelFont(juce::Label &) override {
    return juce::Font(body()).withHeight(11);
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
class ScratchKnob final : public juce::Component {
public:
  ScratchKnob(ScratchProcessor &, const char *, juce::String, juce::String);
  void resized() override;
  void paint(juce::Graphics &) override;
  void animate();
  juce::Slider slider;

private:
  juce::String caption;
  float display = -1;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attachment;
};
class ScratchPlatter final : public juce::Component,
                             public juce::SettableTooltipClient {
public:
  explicit ScratchPlatter(ScratchProcessor &p) : processor(p) {
    setTooltip("Arrastra el vinilo: adelante acelera, atras invierte. Suelta "
               "para el motor.");
  }
  ~ScratchPlatter() override;
  void paint(juce::Graphics &) override;
  void mouseDown(const juce::MouseEvent &) override;
  void mouseDrag(const juce::MouseEvent &) override;
  void mouseUp(const juce::MouseEvent &) override;

private:
  ScratchProcessor &processor;
  float angle = 0, target = .5f;
  bool dragging = false;
};
class ScratchCue final : public juce::TextButton {
public:
  ScratchCue(ScratchProcessor &p, int i)
      : juce::TextButton(juce::String(i + 1)), processor(p), index(i) {
    setTooltip(
        "Pulsa para ir al cue. Shift + click guarda la posicion actual.");
    onClick = [this] { processor.cue(index); };
  }
  void mouseDown(const juce::MouseEvent &e) override {
    if (e.mods.isShiftDown() || e.mods.isPopupMenu()) {
      processor.setValue("cue" + juce::String(index + 1),
                         processor.platterPosition.load());
      return;
    }
    juce::TextButton::mouseDown(e);
  }

private:
  ScratchProcessor &processor;
  int index = 0;
};
class ScratchEditor final : public juce::AudioProcessorEditor,
                            private juce::Timer {
public:
  explicit ScratchEditor(ScratchProcessor &);
  ~ScratchEditor() override;
  void resized() override;
  void paint(juce::Graphics &) override;
  void refresh();

private:
  ScratchProcessor &p;
  ScratchSkin skin;
  ScratchPlatter platter;
  juce::TooltipWindow tooltip{this, 450};
  juce::ComboBox presets, mode, curve, pattern, rpm;
  juce::TextEditor search;
  juce::TextButton previous{"<"}, next{">"}, random{"RANDOM"}, favourite{"FAV"},
      a{"A"}, b{"B"}, copy{"A > B"}, auth{"AUTHORIZED: M7G GOLDEN"},
      bypass{"BYPASS"};
  juce::TextButton capture{"CAPTURAR"}, play{"SCRATCH"}, stop{"STOP"},
      learn{"APRENDER RUIDO"}, advanced{"MAS CONTROLES"},
      automatic{"AUTO SCRATCH"}, smart{"INTELIGENTE"}, hold{"HOLD / 15s"},
      cut{"CORTE"}, autoFader{"AUTO FADER"}, reverse{"REVERSE"};
  std::array<std::unique_ptr<ScratchKnob>, 4> mainKnobs;
  std::array<std::unique_ptr<ScratchKnob>, 8> physicalKnobs;
  std::array<std::unique_ptr<ScratchKnob>, 3> echoKnobs;
  std::array<std::unique_ptr<ScratchCue>, 8> cues;
  juce::Slider crossfader, sensitivity, gap;
  juce::Label description;
  std::vector<int> filtered;
  int lastProgram = -1, fps = 30;
  bool filtering = false;
  using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
  using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  std::vector<std::unique_ptr<BA>> buttonAttachments;
  std::vector<std::unique_ptr<CA>> comboAttachments;
  std::unique_ptr<SA> faderAttachment, senseAttachment, gapAttachment;
  std::unique_ptr<juce::FileChooser> chooser;
  std::unique_ptr<juce::Component> advancedPanel;
  void filter();
  void navigate(int);
  void importLicense();
  void timerCallback() override;
};
