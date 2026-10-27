#include "../Source/PluginEditor.h"
#include "TestSupport.h"
#include <functional>
void visit(juce::Component &c,
           const std::function<void(juce::Component &)> &f) {
  f(c);
  for (auto *child : c.getChildren())
    visit(*child, f);
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    auto p = std::make_unique<TransitionsProcessor>();
    p->setRateAndBufferSizeDetails(48000, 256);
    p->prepareToPlay(48000, 256);
    juce::MidiBuffer midi;
    for (int k = 0; k < 1100; ++k) {
      juce::AudioBuffer<float> b(2, 256);
      for (int i = 0; i < 256; ++i) {
        float x = testVoice((k * 256 + i) / 48000.0);
        b.setSample(0, i, x);
        b.setSample(1, i, x);
      }
      p->processBlock(b, midi);
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
    editor->setSize(1280, 800);
    int knobs = 0;
    juce::ListBox *list = nullptr;
    juce::TextEditor *search = nullptr;
    juce::TextButton *back = nullptr, *forward = nullptr, *random = nullptr,
                     *fav = nullptr, *stop = nullptr, *bypass = nullptr;
    std::array<juce::TextButton *, 3> banks{};
    visit(*editor, [&](juce::Component &c) {
      if (dynamic_cast<TransitionKnob *>(&c))
        ++knobs;
      if (c.getComponentID() == "preset-list")
        list = dynamic_cast<juce::ListBox *>(&c);
      if (c.getComponentID() == "preset-search")
        search = dynamic_cast<juce::TextEditor *>(&c);
      if (auto *b = dynamic_cast<juce::TextButton *>(&c)) {
        for (int i = 0; i < 3; ++i)
          if (b->getButtonText() == transition::bankName(i))
            banks[size_t(i)] = b;
        if (b->getButtonText() == "<")
          back = b;
        if (b->getButtonText() == ">")
          forward = b;
        if (b->getButtonText() == "RANDOM")
          random = b;
        if (b->getButtonText().startsWith("FAV"))
          fav = b;
        if (b->getButtonText() == "STOP FX")
          stop = b;
        if (b->getButtonText() == "BYPASS")
          bypass = b;
      }
    });
    requireTransition(knobs == 4 && list && search && back && forward &&
                          random && fav && stop && bypass,
                      "UI missing controls or not exactly four knobs");
    for (int i = 0; i < 3; ++i) {
      requireTransition(banks[size_t(i)] != nullptr, "Mode selector missing");
      banks[size_t(i)]->onClick();
      requireTransition(list->getListBoxModel()->getNumRows() == 50 &&
                            p->bank() == i,
                        "Bank does not contain 50 effects");
      if (argc > 1) {
        p->prepareToPlay(48000, 256);
        for (int k = 0; k < 1200; ++k) {
          juce::AudioBuffer<float> b(2, 256);
          for (int n = 0; n < 256; ++n) {
            float x = testVoice((k * 256 + n) / 48000.0);
            b.setSample(0, n, x);
            b.setSample(1, n, x);
          }
          p->processBlock(b, midi);
        }
        auto *ui = dynamic_cast<TransitionsEditor *>(editor.get());
        ui->refresh();
        auto image =
            editor->createComponentSnapshot(editor->getLocalBounds(), true);
        juce::File base(juce::String::fromUTF8(argv[1]));
        auto file = base.getSiblingFile(base.getFileNameWithoutExtension() +
                                        (i == 0   ? ""
                                         : i == 1 ? "-cambio"
                                                  : "-salida") +
                                        ".png");
        file.deleteFile();
        auto stream = file.createOutputStream();
        requireTransition(
            stream && juce::PNGImageFormat().writeImageToStream(image, *stream),
            "UI PNG render failed");
      }
    }
    search->setText("no-preset-___", false);
    search->onTextChange();
    requireTransition(list->getListBoxModel()->getNumRows() == 0,
                      "Preset search did not filter");
    search->setText("", false);
    search->onTextChange();
    requireTransition(list->getListBoxModel()->getNumRows() == 50,
                      "Search reset failed");
    int initial = p->getCurrentProgram();
    back->onClick();
    forward->onClick();
    requireTransition(p->getCurrentProgram() == initial,
                      "Previous/next navigation failed");
    random->onClick();
    requireTransition(p->getCurrentProgram() != initial,
                      "Random selected same preset");
    bool wasFav = p->isFavourite(p->getCurrentProgram());
    fav->onClick();
    requireTransition(p->isFavourite(p->getCurrentProgram()) != wasFav,
                      "FAV callback failed");
    fav->onClick();
    bypass->setToggleState(true, juce::sendNotificationSync);
    requireTransition(p->value("bypass") > .5f, "Bypass attachment failed");
    stop->onClick();
    std::cout << "PASS: spaceship editor, exactly 4 knobs, 3 banks x 50 FX, "
                 "search/navigation/random/FAV/A-B controls, bypass and PNG "
                 "rendering\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
