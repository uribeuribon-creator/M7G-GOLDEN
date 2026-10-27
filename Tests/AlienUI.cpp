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
  editor->setSize(1280, 900);
  int modules = 0, ai = 0, knobs = 0, presetItems = 0, parallelBars = 0;
  juce::TextButton *global = nullptr;
  juce::ToggleButton *bypass = nullptr;
  std::vector<juce::TextButton *> analyse;
  visit(*editor, [&](juce::Component &c) {
    if (dynamic_cast<RackModule *>(&c))
      ++modules;
    if (auto *slider = dynamic_cast<juce::Slider *>(&c))
      if (slider->getSliderStyle() == juce::Slider::LinearHorizontal)
        ++parallelBars;
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
  if (modules != 26 || ai != 26 || knobs != 113 || presetItems != 280 ||
      !global || !bypass || parallelBars != 26)
    return 1;
  if (argc > 1) {
    auto image =
        editor->createComponentSnapshot(editor->getLocalBounds(), true);
    juce::File out(juce::String::fromUTF8(argv[1]));
    out.deleteFile();
    auto stream = out.createOutputStream();
    if (!stream || !juce::PNGImageFormat().writeImageToStream(image, *stream))
      return 2;
    for (const auto &page :
         std::array<std::pair<juce::String, juce::String>, 2>{
             {{"CABINA VOCAL", "-cabina"},
              {"ALIEN INTELLIGENCE", "-intelligence"}}}) {
      visit(*editor, [&](juce::Component &child) {
        if (auto *button = dynamic_cast<juce::TextButton *>(&child))
          if (button->getButtonText() == page.first)
            button->onClick();
      });
      auto pageImage =
          editor->createComponentSnapshot(editor->getLocalBounds(), true);
      auto pageFile = out.getSiblingFile(out.getFileNameWithoutExtension() +
                                         page.second + ".png");
      pageFile.deleteFile();
      auto pageStream = pageFile.createOutputStream();
      if (!pageStream ||
          !juce::PNGImageFormat().writeImageToStream(pageImage, *pageStream))
        return 6;
    }
    std::cout << "UI image: " << out.getFullPathName() << "\n";
  }
  for (size_t i = 0; i < analyse.size(); ++i) {
    analyse[i]->onClick();
    if (p->analysisTarget() != int(i) || p->analysisState.load() != 1)
      return 3;
  }
  global->onClick();
  if (p->analysisTarget() != M7GProcessor::analysisAll)
    return 4;
  bypass->setToggleState(true, juce::sendNotificationSync);
  if (p->value(rack::Bypass) < .5f)
    return 5;
  std::cout << "PASS: 26 rack modules, 26 module-analysis callbacks, global "
               "analysis, "
            << knobs << " attached knobs, 280 presets and bypass attachment\n";
}
