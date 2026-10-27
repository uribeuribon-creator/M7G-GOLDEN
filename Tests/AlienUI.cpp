#include "../Source/PluginEditor.h"
#include <iostream>
namespace {
void visit(juce::Component &c,
           const std::function<void(juce::Component &)> &f) {
  f(c);
  for (auto *child : c.getChildren())
    visit(*child, f);
}
} // namespace
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  auto p = std::make_unique<M7GProcessor>();
  p->setRateAndBufferSizeDetails(48000, 256);
  p->prepareToPlay(48000, 256);
  juce::MidiBuffer midi;
  for (int k = 0; k < 200; ++k) {
    juce::AudioBuffer<float> block(2, 256);
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < 256; ++i) {
        double phase =
            juce::MathConstants<double>::twoPi * 220 * (k * 256 + i) / 48000;
        block.setSample(c, i,
                        .12f *
                            float(std::sin(phase) + .3 * std::sin(phase * 3) +
                                  .12 * std::sin(phase * 9)));
      }
    p->processBlock(block, midi);
  }
  std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
  editor->setSize(1280, 800);
  int modules = 0, ai = 0, knobs = 0, presetItems = 0;
  juce::TextButton *global = nullptr;
  juce::ToggleButton *bypass = nullptr;
  std::vector<juce::TextButton *> analyse;
  visit(*editor, [&](juce::Component &c) {
    if (dynamic_cast<RackModule *>(&c))
      ++modules;
    if (dynamic_cast<RackKnob *>(&c))
      ++knobs;
    if (auto *combo = dynamic_cast<juce::ComboBox *>(&c))
      presetItems = juce::jmax(presetItems, combo->getNumItems());
    if (auto *b = dynamic_cast<juce::TextButton *>(&c)) {
      if (b->getButtonText() == "AI") {
        ++ai;
        analyse.push_back(b);
      }
      if (b->getButtonText() == "ANALIZAR TODO")
        global = b;
    }
    if (auto *b = dynamic_cast<juce::ToggleButton *>(&c))
      if (b->getButtonText() == "BYPASS")
        bypass = b;
  });
  std::cout << "UI inventory: " << modules << " modules, " << ai
            << " analysis buttons, " << knobs << " knobs, " << presetItems
            << " presets\n";
  if (modules != 12 || ai != 12 || knobs != 49 || presetItems != 50 ||
      !global || !bypass)
    return 1;
  if (argc > 1) {
    auto image =
        editor->createComponentSnapshot(editor->getLocalBounds(), true);
    juce::File out(juce::String::fromUTF8(argv[1]));
    out.deleteFile();
    auto stream = out.createOutputStream();
    if (!stream || !juce::PNGImageFormat().writeImageToStream(image, *stream))
      return 2;
    std::cout << "UI image: " << out.getFullPathName() << "\n";
  }
  for (size_t i = 0; i < analyse.size(); ++i) {
    analyse[i]->onClick();
    if (p->analysisTarget() != int(i) || p->analysisState.load() != 1)
      return 3;
  }
  global->onClick();
  if (p->analysisTarget() != 12)
    return 4;
  bypass->setToggleState(true, juce::sendNotificationSync);
  if (p->value(rack::Bypass) < .5f)
    return 5;
  std::cout << "PASS: 12 rack modules, 12 module-analysis callbacks, global "
               "analysis, "
            << knobs << " attached knobs, 50 presets and bypass attachment\n";
}
