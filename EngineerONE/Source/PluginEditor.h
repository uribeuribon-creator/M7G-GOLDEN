#pragma once
#include "PluginProcessor.h"
class OneSkin final : public juce::LookAndFeel_V4 {
public:
  OneSkin();
  static juce::Typeface::Ptr body();
  static juce::Typeface::Ptr title();
  void drawButtonText(juce::Graphics &, juce::TextButton &, bool,
                      bool) override;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
};
class OneEditor final : public juce::AudioProcessorEditor,
                        public juce::AudioProcessorEditorARAExtension,
                        private juce::Timer,
                        public juce::FileDragAndDropTarget {
public:
  explicit OneEditor(OneProcessor &);
  ~OneEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  bool isInterestedInFileDrag(const juce::StringArray &) override;
  void filesDropped(const juce::StringArray &, int, int) override;
  void chooseFile();
  void contextMenu();
  void startDrag(int note = -1);
  OneProcessor &processor;

private:
  class Piano;
  class DragPad;
  void timerCallback() override;
  void refresh();
  void selectNote(int);
  void editSelected();
  OneSkin skin;
  juce::TooltipWindow tips{this, 500};
  juce::TextButton captureButton{"CAPTURAR"}, stopButton{"STOP"},
      iaButton{"IA"}, playButton{"ESCUCHAR"}, abButton{"ORIGINAL / A-B"},
      muteButton{"MUTE"}, importButton{"IMPORTAR WAV"},
      editButton{"RETOQUES OPCIONALES"}, applyButton{"APLICAR EN EVENTO"},
      cancelButton{"CANCELAR"}, zoomIn{"+"}, zoomOut{"-"}, left{"<"},
      right{">"};
  juce::Slider pitch, time, vibrato, gain;
  std::unique_ptr<Piano> piano;
  std::unique_ptr<DragPad> dragLine, dragNote;
  std::unique_ptr<juce::FileChooser> chooser;
  std::shared_ptr<const one::Result> result;
  std::shared_ptr<const one::Input> input;
  int selected = -1;
  bool updating = false;
  double first = 0, duration = 1, total = 1;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OneEditor)
};
