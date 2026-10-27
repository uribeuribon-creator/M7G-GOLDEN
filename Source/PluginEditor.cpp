#include "PluginEditor.h"
namespace {
const juce::Colour gold{0xffd4af37};
}
M7GEditor::M7GEditor(M7GProcessor &p)
    : AudioProcessorEditor(p), processor(p),
      voiceAttachment(p.apvts, "voicetype", voiceBox),
      intensityAttachment(p.apvts, "intensity", intensity) {
  voiceBox.addItemList({"VOZ PRINCIPAL", "DOBLES / DOUBLES", "REFUERZOS / HARM",
                        "ADLIBS", "ARMONIAS / COROS", "BUS VOCAL"},
                       1);
  voiceBox.setSelectedId(1 + int(*p.apvts.getRawParameterValue("voicetype")),
                         juce::dontSendNotification);
  voiceBox.setJustificationType(juce::Justification::centred);
  voiceBox.setColour(juce::ComboBox::backgroundColourId,
                     juce::Colour(0xff151515));
  voiceBox.setColour(juce::ComboBox::textColourId, gold);
  voiceBox.setColour(juce::ComboBox::outlineColourId, gold);
  intensity.setSliderStyle(juce::Slider::LinearHorizontal);
  intensity.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
  intensity.setColour(juce::Slider::thumbColourId, gold);
  intensity.setColour(juce::Slider::trackColourId, gold);
  intensity.setTextValueSuffix(" %");
  for (auto *l :
       {&title, &subtitle, &action, &input, &output, &intensityLabel}) {
    addAndMakeVisible(l);
    l->setJustificationType(juce::Justification::centred);
    l->setColour(juce::Label::textColourId, juce::Colours::white);
  }
  title.setText("M7G GOLDEN VOICE AI", juce::dontSendNotification);
  title.setFont(juce::Font(juce::FontOptions(30, juce::Font::bold)));
  title.setColour(juce::Label::textColourId, gold);
  subtitle.setText(
      "NEURAL CLEAN  /  TUNE  /  DE-ESS  /  DYNAMIC EQ  /  COMP  /  SPACE",
      juce::dontSendNotification);
  subtitle.setFont(juce::Font(juce::FontOptions(12)));
  intensityLabel.setText("PROCESSING INTENSITY", juce::dontSendNotification);
  intensityLabel.setColour(juce::Label::textColourId, gold);
  action.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
  action.setColour(juce::Label::textColourId, juce::Colours::lime);
  addAndMakeVisible(voiceBox);
  addAndMakeVisible(intensity);
  setSize(700, 520);
  startTimerHz(30);
  timerCallback();
}
void M7GEditor::resized() {
  int w = getWidth();
  title.setBounds(0, 15, w, 36);
  subtitle.setBounds(0, 53, w, 20);
  voiceBox.setBounds(100, 120, w - 200, 48);
  intensityLabel.setBounds(100, 183, w - 200, 20);
  intensity.setBounds(100, 210, w - 200, 60);
  action.setBounds(20, 285, w - 40, 25);
  input.setBounds(0, 348, w, 25);
  output.setBounds(0, 430, w, 25);
}
void M7GEditor::paint(juce::Graphics &g) {
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff080808), 0, 0,
                                         juce::Colour(0xff1e1e1e), 0,
                                         float(getHeight()), false));
  g.fillAll();
  g.setColour(gold);
  g.fillRect(0, 0, getWidth(), 4);
  g.setColour(juce::Colour(0xff121212));
  g.fillRoundedRectangle(20, 90, float(getWidth() - 40), 340, 12);
  g.setColour(gold.withAlpha(.4f));
  g.drawRoundedRectangle(20, 90, float(getWidth() - 40), 340, 12, 1);
  auto meter = [&](int y, float db, juce::Colour col) {
    float norm = juce::jlimit(0.0f, 1.0f, (db + 40) / 46);
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(60, float(y), 580, 10, 5);
    g.setColour(col);
    g.fillRoundedRectangle(60, float(y), 580 * norm, 10, 5);
  };
  meter(380, processor.inputDB.load(), juce::Colours::lime);
  meter(410, processor.outputDB.load(), gold);
  g.setColour(gold.withAlpha(.6f));
  g.setFont(juce::Font(juce::FontOptions(11)));
  g.drawText(
      "M7G  /  GOLDEN SERIES                                   VOCAL ENGINE 02",
      40, 475, 620, 20, juce::Justification::centred);
}
void M7GEditor::timerCallback() {
  action.setText(M7GProcessor::action(
                     int(*processor.apvts.getRawParameterValue("voicetype"))),
                 juce::dontSendNotification);
  input.setText("INPUT  " + juce::String(processor.inputDB.load(), 1) + " dBFS",
                juce::dontSendNotification);
  output.setText("OUTPUT  " + juce::String(processor.outputDB.load(), 1) +
                     " dBFS    /    GR  " +
                     juce::String(processor.reductionDB.load(), 1) + " dB",
                 juce::dontSendNotification);
  repaint();
}
