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
    if (auto *module = dynamic_cast<RackModule *>(&c)) {
      ++modules;
      int count = 0;
      for (auto *child : module->getChildren())
        if (dynamic_cast<RackKnob *>(child))
          ++count;
      if (count > 2)
        throw std::runtime_error("Module has more than two knobs");
    }
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
  if (modules != 26 || ai != 26 || knobs != 55 || presetItems < 40 || !global ||
      !bypass || parallelBars != 26)
    return 1;
  int bankCount = 0, totalPresets = 0;
  for (int bank = 0; bank < 6; ++bank) {
    visit(*editor, [&](juce::Component &c) {
      if (auto *button = dynamic_cast<juce::TextButton *>(&c))
        if (button->getButtonText() == rack::bankName(bank)) {
          button->onClick();
          ++bankCount;
        }
    });
    visit(*editor, [&](juce::Component &c) {
      if (auto *combo = dynamic_cast<juce::ComboBox *>(&c))
        if (combo->getNumItems() > 30)
          totalPresets += combo->getNumItems();
    });
  }
  if (bankCount != 6 || totalPresets != 360)
    return 7;
  visit(*editor, [&](juce::Component &c) {
    if (auto *button = dynamic_cast<juce::TextButton *>(&c))
      if (button->getButtonText() == rack::bankName(0))
        button->onClick();
  });
  if (argc > 1) {
    auto image =
        editor->createComponentSnapshot(editor->getLocalBounds(), true);
    juce::File out(juce::String::fromUTF8(argv[1]));
    out.deleteFile();
    auto stream = out.createOutputStream();
    if (!stream || !juce::PNGImageFormat().writeImageToStream(image, *stream))
      return 2;
    for (const auto &page :
         std::array<std::pair<juce::String, juce::String>, 3>{
             {{"CADENA VOCAL", "-cabina"},
              {"LIMPIEZA / ARMONIAS", "-intelligence"},
              {"ESPACIO / COLOR", "-space"}}}) {
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
  juce::ComboBox *workflow = nullptr;
  juce::TextEditor *search = nullptr;
  juce::TextButton *back = nullptr, *forward = nullptr, *random = nullptr,
                   *fav = nullptr, *autoMode = nullptr;
  visit(*editor, [&](juce::Component &c) {
    if (c.getComponentID() == "workflow-presets")
      workflow = dynamic_cast<juce::ComboBox *>(&c);
    if (c.getComponentID() == "preset-search")
      search = dynamic_cast<juce::TextEditor *>(&c);
    if (auto *b = dynamic_cast<juce::TextButton *>(&c)) {
      if (b->getButtonText() == "<")
        back = b;
      if (b->getButtonText() == ">")
        forward = b;
      if (b->getButtonText() == "RANDOM")
        random = b;
      if (b->getButtonText().startsWith("FAV"))
        fav = b;
      if (b->getComponentID() == "alien-automation")
        autoMode = b;
    }
  });
  if (!workflow || !search || !back || !forward || !random || !fav || !autoMode)
    return 8;
  search->setText("no-preset-__", true);
  if (workflow->getNumItems() != 0)
    return 9;
  search->setText("", true);
  if (workflow->getNumItems() < 40)
    return 10;
  int start = p->getCurrentProgram();
  back->onClick();
  forward->onClick();
  if (p->getCurrentProgram() != start)
    return 11;
  random->onClick();
  if (p->getCurrentProgram() == start)
    return 12;
  bool wasFav = p->isFavourite(p->getCurrentProgram());
  fav->onClick();
  if (p->isFavourite(p->getCurrentProgram()) == wasFav)
    return 13;
  fav->onClick();
  autoMode->setToggleState(false, juce::sendNotificationSync);
  if (p->automatic())
    return 14;
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
            << knobs
            << " attached knobs, 360 workflow presets and bypass attachment\n";
}
