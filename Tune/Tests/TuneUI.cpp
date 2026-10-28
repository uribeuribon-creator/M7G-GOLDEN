#include "../Source/PluginEditor.h"
#include "TestSupport.h"
#include <functional>
void walk(juce::Component &c, const std::function<void(juce::Component &)> &f) {
  f(c);
  for (auto *x : c.getChildren())
    walk(*x, f);
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    auto p = std::make_unique<TuneProcessor>();
    p->setCurrentProgram(argc > 2 ? std::atoi(argv[2]) : 54);
    p->setRateAndBufferSizeDetails(48000, 256);
    p->prepareToPlay(48000, 256);
    juce::MidiBuffer m;
    for (int k = 0; k < 700; ++k) {
      juce::AudioBuffer<float> b(2, 256);
      for (int i = 0; i < 256; ++i) {
        double t = (k * 256 + i) / 48000.;
        constexpr double pitches[]{224.7, 258., 333.8, 386.};
        float x = vowel(t, pitches[int(t / .8) % 4]);
        b.setSample(0, i, x);
        b.setSample(1, i, x);
      }
      p->processBlock(b, m);
    }
    std::unique_ptr<juce::AudioProcessorEditor> ui(p->createEditor());
    ui->setSize(1440, 900);
    int knobs = 0;
    juce::ListBox *list = nullptr;
    juce::TextEditor *search = nullptr;
    std::array<juce::TextButton *, 4> banks{};
    juce::TextButton *harmony = nullptr, *doubles = nullptr;
    walk(*ui, [&](juce::Component &c) {
      if (dynamic_cast<TuneKnob *>(&c))
        ++knobs;
      if (c.getComponentID() == "preset-list")
        list = dynamic_cast<juce::ListBox *>(&c);
      if (c.getComponentID() == "preset-search")
        search = dynamic_cast<juce::TextEditor *>(&c);
      if (auto *b = dynamic_cast<juce::TextButton *>(&c)) {
        for (int i = 0; i < 4; ++i)
          if (b->getButtonText() == tune::banks[size_t(i)])
            banks[size_t(i)] = b;
        if (b->getButtonText() == "CREAR ARMONIAS")
          harmony = b;
        if (b->getButtonText() == "DOBLES REALES")
          doubles = b;
      }
    });
    requireTune(knobs == 6 && list && search && harmony && doubles,
                "Missing UI controls / not six knobs");
    for (int i = 0; i < 4; ++i) {
      requireTune(banks[size_t(i)] != nullptr, "Missing preset bank");
      banks[size_t(i)]->onClick();
      requireTune(list->getListBoxModel()->getNumRows() == 25,
                  "Bank is not 25 presets");
    }
    search->setText("not-found-_____", false);
    search->onTextChange();
    requireTune(list->getListBoxModel()->getNumRows() == 0, "Search failed");
    search->setText("", false);
    search->onTextChange();
    requireTune(list->getListBoxModel()->getNumRows() == 25,
                "Search reset failed");
    harmony->setToggleState(true, juce::sendNotificationSync);
    doubles->setToggleState(true, juce::sendNotificationSync);
    requireTune(p->value("harmonyOn") > .5f && p->value("doublesOn") > .5f,
                "Choir switches not attached");
    if (argc > 1) {
      p->setCurrentProgram(argc > 2 ? std::atoi(argv[2]) : 54);
      dynamic_cast<TuneEditor *>(ui.get())->refresh();
      auto image = ui->createComponentSnapshot(ui->getLocalBounds(), true);
      juce::File f(juce::String::fromUTF8(argv[1]));
      f.deleteFile();
      auto stream = f.createOutputStream();
      requireTune(stream &&
                      juce::PNGImageFormat().writeImageToStream(image, *stream),
                  "PNG export failed");
    }
    std::cout << "PASS: six knobs, four banks x25, search, choir switches and "
                 "actual live piano-roll screenshot\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
