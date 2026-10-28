#include "../Source/PluginEditor.h"
#include "Support.h"
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    auto p = std::make_unique<EngineerProcessor>();
    p->prepareToPlay(44100, 256);
    auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory)
                      .getChildFile("Engineer-UI-" + juce::Uuid().toString());
    folder.createDirectory();
    auto input = folder.getChildFile("Test-vocal.wav");
    engineer::writeWav(input, vocal(), 48000, {});
    p->loadFile(input);
    auto start = juce::Time::getMillisecondCounter();
    while (p->busy.load() &&
           juce::Time::getMillisecondCounter() - start < 120000)
      dispatch(25);
    check(!p->busy.load() && p->snapshot(),
          "Background analysis did not finish with the editor closed");
    std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
    editor->setSize(1400, 900);
    editor->addToDesktop(0);
    editor->setVisible(true);
    dispatch(80);
    std::array<std::vector<juce::TextButton *>, 4> buttons;
    for (auto *child : editor->getChildren())
      if (auto *b = dynamic_cast<juce::TextButton *>(child)) {
        if (b->getButtonText() == "ESCUCHAR")
          buttons[0].push_back(b);
        if (b->getButtonText() == "SOLO")
          buttons[1].push_back(b);
        if (b->getButtonText() == "MUTE")
          buttons[2].push_back(b);
        if (b->getButtonText() == "GUARDAR WAV")
          buttons[3].push_back(b);
      }
    for (const auto &set : buttons)
      check(set.size() == 5,
            "Five-column play/solo/mute/save controls missing");
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> audio(2, 256);
    for (int version = 0; version < 5; ++version) {
      buttons[0][size_t(version)]->onClick();
      p->seek(.8);
      double energy = 0;
      for (int k = 0; k < 10; ++k) {
        audio.clear();
        p->processBlock(audio, midi);
        energy += audio.getRMSLevel(0, 0, 256);
      }
      check(energy > .01 && p->selected.load() == version,
            "Version preview is silent or selected wrong version");
      buttons[2][size_t(version)]->onClick();
      for (int k = 0; k < 10; ++k) {
        audio.clear();
        p->processBlock(audio, midi);
      }
      check(p->isMuted(version) && audio.getMagnitude(0, 256) < 1e-5,
            "Mute failed");
      buttons[2][size_t(version)]->onClick();
      buttons[1][size_t(version)]->onClick();
      check(p->solo.load() == -1, "Solo toggle failed");
    }
    // A stereo file previewed through a mono host must sum both channels.
    p->play(0);
    p->seek(.8);
    for (int k = 0; k < 10; ++k) {
      audio.clear();
      p->processBlock(audio, midi);
    }
    auto stereo = audio;
    juce::AudioBuffer<float> mono(1, 256);
    p->seek(.8);
    for (int k = 0; k < 10; ++k) {
      mono.clear();
      p->processBlock(mono, midi);
    }
    double error = 0;
    for (int i = 0; i < 256; ++i)
      error += std::abs(mono.getSample(0, i) - .5f * (stereo.getSample(0, i) +
                                                      stereo.getSample(1, i)));
    check(error < .001,
          "Mono preview drops a stereo channel instead of downmixing");
    p->play(3);
    p->seek(1.1);
    audio.clear();
    p->processBlock(audio, midi);
    dispatch(60);
    if (argc == 2) {
      juce::PNGImageFormat png;
      auto path = juce::File(juce::String::fromUTF8(argv[1]));
      auto out = path.createOutputStream();
      check(out && png.writeImageToStream(editor->createComponentSnapshot(
                                              editor->getLocalBounds()),
                                          *out),
            "Screenshot export failed");
    }
    editor->setSize(1120, 720);
    dispatch(30);
    for (auto *child : editor->getChildren()) {
      auto bounds = editor->getLocalArea(child, child->getLocalBounds());
      check(bounds.getRight() <= 1122 && bounds.getBottom() <= 722,
            "Control falls outside minimum editor");
    }
    auto project = p->snapshot();
    for (auto file : project->files)
      check(file.existsAsFile(), "Drag target has no real exported WAV");
    p->stop();
    editor.reset();
    p.reset();
    folder.deleteRecursively();
    std::cout << "PASS: editor-closed analysis, five real play/solo/mute/save "
                 "columns, exported WAV drag sources, 44.1/48k preview, native "
                 "screenshot and scaled UI\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
