#pragma once
#include "PluginProcessor.h"
class EngineerSkin final : public juce::LookAndFeel_V4 {
public:
  EngineerSkin();
  static juce::Typeface::Ptr body();
  static juce::Typeface::Ptr title();
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
};
class EngineerEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer,
                             private juce::ScrollBar::Listener,
                             public juce::FileDragAndDropTarget {
public:
  explicit EngineerEditor(EngineerProcessor &);
  ~EngineerEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  bool isInterestedInFileDrag(const juce::StringArray &) override;
  void filesDropped(const juce::StringArray &, int, int) override;
  void chooseFile();
  void saveVersion(int);
  EngineerProcessor &processor;

private:
  class PitchPanel;
  class ExportPad;
  void timerCallback() override;
  void updateView();
  void scrollBarMoved(juce::ScrollBar *, double start) override {
    viewStart = start;
    updateView();
  }
  EngineerSkin skin;
  juce::TooltipWindow tips{this, 550};
  juce::TextButton load{"CARGAR VOCAL"}, original{"ORIGINAL / A-B"},
      stop{"PAUSA"}, cancel{"CANCELAR"}, rebuild{"RECALCULAR"}, zoomIn{"+"},
      zoomOut{"-"}, folder{"CARPETA WAV"};
  juce::ComboBox scale, root;
  juce::Slider strength, volume;
  juce::ScrollBar scroll{false};
  std::array<juce::TextButton, 5> play, solo, mute, save;
  std::array<std::unique_ptr<PitchPanel>, 5> graphs;
  std::array<std::unique_ptr<ExportPad>, 5> drags;
  std::unique_ptr<juce::FileChooser> chooser;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      strengthAttachment, volumeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      scaleAttachment, rootAttachment;
  std::shared_ptr<const engineer::Project> project;
  double viewStart = 0, viewLength = 1, fullLength = 1;
  float uiScale = 1;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerEditor)
};
