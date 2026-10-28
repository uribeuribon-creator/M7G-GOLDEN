#include "CharacterContent.h"
namespace freshui {
CharacterContent::CharacterContent(FreshProcessor &p)
    : ModuleContent(p, 3, "FRESH CHARACTER", fresh::charOn),
      character(p, fresh::character, "CHARACTER", "AMOUNT", accent(3)),
      mode(p, fresh::charMode, 3), harmonics(p, fresh::harmonics, 3),
      tape(p, fresh::tape, 3), width(p, fresh::width, 3), curve(p) {
  for (auto *c : std::array<juce::Component *, 6>{
           {&character, &mode, &harmonics, &tape, &width, &curve}})
    addAndMakeVisible(*c);
}
void CharacterContent::resized() {
  ModuleContent::resized();
  int w = getWidth(), h = getHeight(), side = std::min(430, w / 2 - 50),
      knob = std::min(230, w / 3);
  character.setBounds((w / 2 - knob) / 2, 115, knob, knob + 50);
  curve.setBounds(w - side - 40, 125, side, 260);
  int top = std::min(h - 210, knob + 205), left = 55, width_ = w - 110;
  mode.setBounds(left, top, width_, 33);
  harmonics.setBounds(left, top + 46, width_ / 2 - 20, 33);
  width.setBounds(left + width_ / 2 + 20, top + 46, width_ / 2 - 20, 33);
  tape.setBounds(left, top + 92, width_, 33);
}
} // namespace freshui
