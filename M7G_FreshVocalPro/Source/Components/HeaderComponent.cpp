#include "HeaderComponent.h"
namespace freshui {
HeaderComponent::HeaderComponent(FreshProcessor &p) : processor(p) {
  for (auto *b : {&load, &save, &previous, &next, &ab})
    addAndMakeVisible(b);
  load.setComponentID("load-preset");
  save.setComponentID("save-preset");
  previous.setComponentID("previous-preset");
  next.setComponentID("next-preset");
  ab.setComponentID("ab-state");
  load.onClick = [this] { file(false); };
  save.onClick = [this] { file(true); };
  previous.onClick = [this] {
    int i = processor.selectedPreset.load();
    processor.choosePreset((i - 1 + processor.getNumPrograms()) %
                           processor.getNumPrograms());
  };
  next.onClick = [this] {
    int i = processor.selectedPreset.load();
    processor.choosePreset((i + 1) % processor.getNumPrograms());
  };
  ab.onClick = [this] { processor.switchAB(); };
  const char *names[]{"Simple", "Advanced", "Expert"};
  for (int i = 0; i < 3; ++i) {
    auto &b = modes[size_t(i)];
    b.setButtonText(names[i]);
    b.setComponentID("mode-" + juce::String(i));
    b.onClick = [this, i] {
      if (onMode)
        onMode(i);
    };
    addAndMakeVisible(b);
  }
  preset.setJustificationType(juce::Justification::centred);
  preset.setColour(juce::Label::backgroundColourId, black);
  preset.setColour(juce::Label::outlineColourId, juce::Colour(0xff3a392e));
  addAndMakeVisible(preset);
  startTimerHz(20);
  timerCallback();
}
void HeaderComponent::file(bool saving) {
  chooser = std::make_unique<juce::FileChooser>(
      saving ? "Guardar preset de cadena completa"
             : "Cargar preset de cadena completa",
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
          .getChildFile("M7G Fresh Vocal.m7g.xml"),
      "*.xml");
  juce::Component::SafePointer<HeaderComponent> safe(this);
  int flags = saving ? juce::FileBrowserComponent::saveMode |
                           juce::FileBrowserComponent::warnAboutOverwriting
                     : juce::FileBrowserComponent::openMode |
                           juce::FileBrowserComponent::canSelectFiles;
  chooser->launchAsync(flags, [safe, saving](const juce::FileChooser &choice) {
    if (!safe || choice.getResult() == juce::File())
      return;
    bool ok = saving ? safe->processor.savePreset(choice.getResult())
                     : safe->processor.loadPreset(choice.getResult());
    if (!ok)
      juce::NativeMessageBox::showMessageBoxAsync(
          juce::MessageBoxIconType::WarningIcon, "M7G Fresh Vocal",
          saving ? "No pude guardar el preset."
                 : "El archivo no es un preset completo valido de M7G Fresh "
                   "Vocal.");
  });
}
void HeaderComponent::timerCallback() {
  preset.setText(processor.presetName(), juce::dontSendNotification);
  for (int i = 0; i < 3; ++i)
    modes[size_t(i)].setToggleState(processor.viewMode.load() == i,
                                    juce::dontSendNotification);
  ab.setToggleState(processor.bankB.load(), juce::dontSendNotification);
}
void HeaderComponent::resized() {
  int w = getWidth();
  bool small = w < 900;
  if (small) {
    for (int i = 0; i < 3; ++i)
      modes[size_t(i)].setBounds(w - 204 + i * 64, 8, 62, 28);
    load.setBounds(12, 43, 48, 25);
    previous.setBounds(66, 43, 25, 25);
    preset.setBounds(96, 43, w - 320, 25);
    next.setBounds(w - 218, 43, 25, 25);
    save.setBounds(w - 185, 43, 50, 25);
    ab.setBounds(w - 125, 43, 110, 25);
  } else {
    int modeWidth = 255;
    for (int i = 0; i < 3; ++i)
      modes[size_t(i)].setBounds(w - modeWidth - 12 + i * 85, 13, 81, 34);
    ab.setBounds(w - modeWidth - 75, 15, 54, 30);
    int start = std::max(316, w / 3 - 50),
        available = w - modeWidth - 95 - start;
    load.setBounds(start, 15, 53, 30);
    previous.setBounds(start + 60, 15, 25, 30);
    preset.setBounds(start + 91, 15, std::max(70, available - 180), 30);
    next.setBounds(start + available - 81, 15, 25, 30);
    save.setBounds(start + available - 48, 15, 48, 30);
  }
}
void HeaderComponent::paint(juce::Graphics &g) {
  g.setGradientFill(juce::ColourGradient(black, 0, 0, juce::Colour(0xff1a1a1a),
                                         0, float(getHeight()), false));
  g.fillAll();
  bool small = getWidth() < 900;
  float font = small ? 17.f : 23.f;
  M7GLookAndFeel::text(g, "M7G", {13, small ? 7 : 15, small ? 43 : 58, 30},
                       font, gold, juce::Justification::left, true);
  M7GLookAndFeel::text(g, "FRESH VOCAL PRO",
                       {small ? 61 : 78, small ? 7 : 15, small ? 240 : 245, 30},
                       font, white, juce::Justification::left, true);
  g.setColour(gold.withAlpha(.23f));
  g.drawHorizontalLine(getHeight() - 1, 0.f, float(getWidth()));
}
} // namespace freshui
