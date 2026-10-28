#include "../Source/PluginEditor.h"
#include "Support.h"
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    auto p = std::make_unique<OneProcessor>();
    p->prepareToPlay(48000, 256);
    auto folder =
        juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("Makkine-ONE-UI-" + juce::Uuid().toString());
    folder.createDirectory();
    auto file = folder.getChildFile("vocal.wav");
    one::write24(file, vocal(), 12.5, true);
    p->loadFile(file);
    auto wait = [&] {
      auto start = juce::Time::getMillisecondCounter();
      while (p->busy.load() &&
             juce::Time::getMillisecondCounter() - start < 120000)
        dispatch(20);
      check(!p->busy.load(), "Worker timed out");
    };
    wait();
    std::unique_ptr<juce::AudioProcessorEditor> e(p->createEditor());
    e->setSize(1400, 920);
    e->addToDesktop(0);
    e->setVisible(true);
    dispatch(100);
    auto button = [&](juce::String id) {
      for (auto *c : e->getChildren())
        if (c->getComponentID() == id)
          if (auto *b = dynamic_cast<juce::TextButton *>(c))
            return b;
      return (juce::TextButton *)nullptr;
    };
    check(button("ia") && button("capture") && button("play"),
          "Core flow controls missing");
    check(!p->snapshot(),
          "Manual adjustments should never be needed before IA");
    button("ia")->onClick();
    wait();
    dispatch(100);
    check(bool(p->snapshot()), p->statusText().toRawUTF8());
    button("play")->onClick();
    p->seek(.8);
    juce::AudioBuffer<float> b(2, 256);
    juce::MidiBuffer midi;
    for (int k = 0; k < 10; ++k) {
      b.clear();
      p->processBlock(b, midi);
    }
    check(b.getRMSLevel(0, 0, 256) > .01, "Play button silent");
    check(p->snapshot()->lineFile.existsAsFile(),
          "Drag line has no actual WAV");
    check(!button("optional-edit")->getToggleState(),
          "Manual editing should be collapsed by default");
    if (argc == 2) {
      juce::PNGImageFormat png;
      auto stream =
          juce::File(juce::String::fromUTF8(argv[1])).createOutputStream();
      check(stream &&
                png.writeImageToStream(
                    e->createComponentSnapshot(e->getLocalBounds()), *stream),
            "Screenshot failed");
    }
    for (auto size :
         {juce::Point<int>(1120, 740), juce::Point<int>(1800, 1200)}) {
      e->setSize(size.x, size.y);
      dispatch(25);
      for (auto *c : e->getChildren())
        if (c->isVisible())
          check(e->getLocalBounds().contains(c->getBounds()),
                "UI control escapes bounds on resize");
    }
    e.reset();
    p->releaseResources();
    p.reset();
    folder.deleteRecursively();
    std::cout << "PASS: actual IA button automatic tuning, preview, real drag "
                 "WAVs, optional manual edits hidden, resizable native GUI\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "FAIL: " << ex.what() << "\n";
    return 1;
  }
}
