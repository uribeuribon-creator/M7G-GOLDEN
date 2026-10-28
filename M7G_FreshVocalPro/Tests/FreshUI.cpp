#include "../Source/PluginEditor.h"
#include "Support.h"
static juce::Component *find(juce::Component &c, const juce::String &id) {
  if (c.getComponentID() == id)
    return &c;
  for (auto *child : c.getChildren())
    if (auto *result = find(*child, id))
      return result;
  return nullptr;
}
static juce::Button &button(juce::Component &c, juce::String id) {
  auto *b = dynamic_cast<juce::Button *>(find(c, id));
  check(b, juce::String("Missing button " + id).toRawUTF8());
  return *b;
}
static void click(juce::Component &c, juce::String id, int wait = 220) {
  button(c, id).triggerClick();
  dispatch(wait);
}
static void shot(juce::Component &c, const juce::File &root,
                 juce::String name) {
  if (root == juce::File())
    return;
  auto out = root.getChildFile(name + ".png").createOutputStream();
  juce::PNGImageFormat png;
  check(out && png.writeImageToStream(
                   c.createComponentSnapshot(c.getLocalBounds()), *out),
        "Actual UI screenshot failed");
}
static juce::String imageHash(juce::Component &c) {
  juce::MemoryOutputStream out;
  juce::PNGImageFormat png;
  png.writeImageToStream(c.createComponentSnapshot(c.getLocalBounds()), out);
  return juce::SHA256(out.getData(), out.getDataSize()).toHexString();
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    juce::File images;
    if (argc == 2) {
      images = juce::File(juce::String::fromUTF8(argv[1]));
      images.createDirectory();
    }
    FreshProcessor p;
    p.prepareToPlay(48000, 256);
    FreshEditor e(p);
    e.addToDesktop(0);
    e.setVisible(true);
    dispatch(100);
    check(e.getWidth() == 600 && e.getHeight() == 400, "Simple is not 600x400");
    shot(e, images, "Fresh-Simple");
    auto *list = dynamic_cast<juce::ListBox *>(find(e, "factory-presets"));
    check(list && list->getModel()->getNumRows() == 35,
          "Factory preset list incomplete");
    list->selectRow(8);
    dispatch(380);
    check(p.selectedPreset.load() == 8 && !p.morphing.load(),
          "Preset click/morph failed");
    check(std::abs(p.settings()[fresh::comp] -
                   fresh::PresetManager::factory()[8].values[fresh::comp]) <
              .02,
          "Preset knobs do not reach actual target values");
    click(e, "mode-1", 300);
    check(e.getWidth() == 1200 && e.getHeight() == 750,
          "Advanced resize animation missed target");
    click(e, "section-0");
    click(e, "dynStyle-2");
    check(p.settings()[fresh::dynStyle] == 2,
          "Dynamics style button is cosmetic");
    auto *comp = dynamic_cast<juce::Slider *>(find(e, "comp"));
    check(comp, "COMP knob missing");
    p.undo.clearUndoHistory();
    float previous = p.settings()[fresh::comp];
    p.undo.beginNewTransaction("Manual compression");
    comp->setValue(42, juce::sendNotificationSync);
    dispatch(120);
    check(std::abs(p.settings()[fresh::comp] - 42) < .01,
          "Knob does not update DSP parameter");
    e.keyPressed(juce::KeyPress('z', juce::ModifierKeys::ctrlModifier, 0));
    dispatch(60);
    check(std::abs(p.settings()[fresh::comp] - previous) < .02,
          "Ctrl+Z did not restore parameter");
    e.keyPressed(juce::KeyPress('y', juce::ModifierKeys::ctrlModifier, 0));
    dispatch(60);
    check(std::abs(p.settings()[fresh::comp] - 42) < .02,
          "Ctrl+Y did not redo parameter");
    click(e, "dynOn", 80);
    check(p.settings()[fresh::dynOn] == 0,
          "Module title bypass does not disable DSP");
    click(e, "dynOn", 80);
    auto sourceAudio = vocal();
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    for (int k = 0; k < 80; ++k) {
      for (int c = 0; c < 2; ++c)
        audio.copyFrom(c, 0, sourceAudio, c, 28000 + k * 256, 256);
      p.processBlock(audio, midi);
    }
    check(p.meters().compression.load() > .1f,
          "Real-time compression meter never responds to vocal input");
    shot(e, images, "Fresh-Advanced-Dynamics");
    click(e, "section-1");
    click(e, "eqStyle-3");
    check(p.settings()[fresh::eqHigh] == 6 && p.settings()[fresh::eqLow] == 0,
          "Airy mode does not set actual EQ curve");
    shot(e, images, "Fresh-Advanced-EQ");
    click(e, "section-2");
    auto *radar = find(e, "space-radar");
    check(radar, "Space radar missing");
    auto before = imageHash(*radar);
    dispatch(100);
    check(imageHash(*radar) != before, "Radar animation is static");
    click(e, "division-0", 80);
    check(p.settings()[fresh::division] == 0,
          "Delay subdivision button failed");
    shot(e, images, "Fresh-Advanced-Space");
    click(e, "section-3");
    auto *curve = find(e, "character-curve");
    check(curve, "Character curve missing");
    before = imageHash(*curve);
    p.set(fresh::character, 80);
    dispatch(60);
    check(imageHash(*curve) != before,
          "Character transfer curve does not react to DSP parameter");
    shot(e, images, "Fresh-Advanced-Character");
    click(e, "mode-2", 300);
    check(e.getWidth() == 1400 && e.getHeight() == 900,
          "Expert resize animation missed target");
    click(e, "section-1");
    check(find(e, "eq160")->isShowing(), "Expert EQ bands not visible");
    shot(e, images, "Fresh-Expert-EQ");
    auto *source = find(e, "section-2");
    auto *destination =
        dynamic_cast<juce::DragAndDropTarget *>(find(e, "section-0"));
    check(source && destination, "Expert drag targets missing");
    juce::DragAndDropTarget::SourceDetails detail(juce::var("fresh-module:2"),
                                                  source, {0, 0});
    check(destination->isInterestedInDragSource(detail),
          "Expert drag/drop disabled");
    destination->itemDropped(detail);
    check(fresh::orders()[size_t(int(p.settings()[fresh::order]))][0] == 2,
          "Module drag did not reorder DSP chain");
    p.set(fresh::comp, 58);
    auto a = p.settings();
    click(e, "ab-state", 60);
    p.set(fresh::comp, 13);
    auto b = p.settings();
    click(e, "ab-state", 60);
    check(p.settings() == a, "A/B button loses A state");
    click(e, "ab-state", 60);
    check(p.settings() == b, "A/B button loses B state");
    auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory)
                      .getChildFile("Fresh-UI-" + juce::Uuid().toString());
    folder.createDirectory();
    auto file = folder.getChildFile("saved.m7g.xml");
    check(p.savePreset(file), "Preset save failed");
    auto saved = p.settings();
    p.choosePreset(0, false);
    check(p.loadPreset(file), "Preset load failed");
    dispatch(380);
    check(p.settings() == saved,
          "Saved preset does not restore complete chain");
    folder.deleteRecursively();
    click(e, "global-bypass", 80);
    check(p.settings()[fresh::bypass] == 1, "Global bypass button failed");
    p.releaseResources();
    std::cout << "PASS: three exact mode sizes, animated resize, 35-preset "
                 "selection/morph, actual knobs, style curves, module/global "
                 "bypass, undo/redo, A/B, XML load/save, expert drag DSP "
                 "order, radar and transfer-curve animation\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "FAIL: " << ex.what() << "\n";
    return 1;
  }
}
