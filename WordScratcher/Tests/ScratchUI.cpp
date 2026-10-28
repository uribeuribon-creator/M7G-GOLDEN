#include "../Source/PluginEditor.h"
#include "TestSupport.h"
#include <functional>
void visit(juce::Component &c,
           const std::function<void(juce::Component &)> &f) {
  f(c);
  for (auto *x : c.getChildren())
    visit(*x, f);
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    auto p = std::make_unique<ScratchProcessor>();
    p->setCurrentProgram(35);
    p->setRateAndBufferSizeDetails(48000, 256);
    p->prepareToPlay(48000, 256);
    juce::MidiBuffer m;
    for (int k = 0; k < 295; ++k) {
      juce::AudioBuffer<float> b(2, 256);
      for (int i = 0; i < 256; ++i) {
        float x = scratchVoice((k * 256 + i) / 48000.);
        b.setSample(0, i, x);
        b.setSample(1, i, x * .94f);
      }
      p->processBlock(b, m);
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
    editor->setSize(1440, 940);
    juce::ComboBox *combo = nullptr;
    juce::TextEditor *search = nullptr;
    ScratchPlatter *platter = nullptr;
    juce::Slider *fader = nullptr;
    std::array<juce::TextButton *, 8> cues{};
    juce::TextButton *next = nullptr, *previous = nullptr, *fav = nullptr,
                     *random = nullptr, *play = nullptr, *stop = nullptr,
                     *advanced = nullptr;
    int knobs = 0;
    visit(*editor, [&](juce::Component &c) {
      if (dynamic_cast<ScratchKnob *>(&c))
        ++knobs;
      if (auto *v = dynamic_cast<ScratchPlatter *>(&c))
        platter = v;
      if (c.getComponentID() == "preset-combo")
        combo = dynamic_cast<juce::ComboBox *>(&c);
      if (c.getComponentID() == "preset-search")
        search = dynamic_cast<juce::TextEditor *>(&c);
      if (c.getComponentID() == "scratch-fader")
        fader = dynamic_cast<juce::Slider *>(&c);
      if (auto *b = dynamic_cast<juce::TextButton *>(&c)) {
        for (int i = 0; i < 8; ++i)
          if (b->getButtonText() == juce::String(i + 1))
            cues[size_t(i)] = b;
        if (b->getButtonText() == "<")
          previous = b;
        if (b->getButtonText() == ">")
          next = b;
        if (b->getButtonText() == "FAV")
          fav = b;
        if (b->getButtonText() == "RANDOM")
          random = b;
        if (b->getButtonText() == "SCRATCH")
          play = b;
        if (b->getButtonText() == "STOP")
          stop = b;
        if (b->getButtonText() == "MAS CONTROLES")
          advanced = b;
      }
    });
    requireScratch(knobs == 15 && combo && search && platter && fader && next &&
                       previous && fav && random && play && stop && advanced &&
                       advanced->isVisible(),
                   "UI missing real controls");
    requireScratch(combo->getNumItems() == 300,
                   "Grouped ComboBox is not 300 presets");
    for (auto *c : cues)
      requireScratch(c && c->isEnabled(),
                     "Missing/disabled hot cue with captured word");
    combo->setSelectedId(278, juce::sendNotificationSync);
    requireScratch(p->getCurrentProgram() == 277,
                   "Preset ComboBox does not apply host parameters");
    search->setText("Chirp", false);
    search->onTextChange();
    int expected = 0;
    for (auto &q : p->manager.presets)
      if (q.name.containsIgnoreCase("Chirp") ||
          q.category.containsIgnoreCase("Chirp"))
        ++expected;
    requireScratch(combo->getNumItems() == expected,
                   "Search/group filtering failed");
    search->setText("no-preset-__", false);
    search->onTextChange();
    requireScratch(combo->getNumItems() == 0, "Search should have no results");
    search->setText("", false);
    search->onTextChange();
    requireScratch(combo->getNumItems() == 300, "Search reset failed");
    int before = p->getCurrentProgram();
    next->onClick();
    previous->onClick();
    requireScratch(p->getCurrentProgram() == before, "Next/previous failed");
    bool saved = p->isFavourite(before);
    fav->onClick();
    requireScratch(p->isFavourite(before) != saved, "FAV action failed");
    fav->onClick();
    random->onClick();
    requireScratch(p->getCurrentProgram() != before,
                   "Random returned same preset");
    advanced->onClick();
    advanced->onClick();
    cues[3]->onClick();
    play->onClick();
    stop->onClick();
    editor->setSize(1120, 730);
    for (auto *c : editor->getChildren()) {
      auto bounds = editor->getLocalArea(c, c->getLocalBounds());
      requireScratch(bounds.getRight() <= 1122 && bounds.getBottom() <= 732,
                     "Scaled UI control falls outside minimum editor");
    }
    editor->setSize(1440, 940);
    p->setCurrentProgram(35);
    dynamic_cast<ScratchEditor *>(editor.get())->refresh();
    if (argc > 1) {
      juce::File file(juce::String::fromUTF8(argv[1]));
      file.deleteFile();
      auto stream = file.createOutputStream();
      auto image =
          editor->createComponentSnapshot(editor->getLocalBounds(), true);
      requireScratch(
          stream && juce::PNGImageFormat().writeImageToStream(image, *stream),
          "UI screenshot failed");
    }
    std::cout
        << "PASS: grouped searchable 300 presets, 15 animated knobs, platter, "
           "crossfader, 8 cues, navigation/FAV and scaled native screenshot\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
