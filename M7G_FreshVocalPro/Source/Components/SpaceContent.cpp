#include "SpaceContent.h"
namespace freshui {
SpaceContent::SpaceContent(FreshProcessor &p)
    : ModuleContent(p, 2, "FRESH SPACE", fresh::spaceOn),
      reverb(p, fresh::reverb, "REVERB", "SPACE AMOUNT"),
      time(p, fresh::time, "TIME", "SPACE SIZE"),
      delay(p, fresh::delay, "DELAY", "ECHO AMOUNT"),
      size(p, fresh::spaceMode, 2), type(p, fresh::verbType, 2),
      division(p, fresh::division, 2), radar(p) {
  for (auto *c : std::array<juce::Component *, 7>{
           {&reverb, &time, &delay, &size, &type, &division, &radar}})
    addAndMakeVisible(*c);
}
void SpaceContent::resized() {
  ModuleContent::resized();
  int w = getWidth(), h = getHeight(), graph = std::min(330, w / 3),
      leftWidth = w - graph - 55;
  size.setBounds(40, 91, leftWidth - 50, 31);
  type.setBounds(40, 132, leftWidth - 50, 31);
  division.setBounds(40, 173, leftWidth - 50, 31);
  int unit = std::min(150, (leftWidth - 70) / 3),
      top = std::max(240, (h - 100 - unit) / 2),
      left = (leftWidth - unit * 3 - 30) / 2 + 20;
  reverb.setBounds(left, top, unit, unit + 55);
  time.setBounds(left + unit + 15, top, unit, unit + 55);
  delay.setBounds(left + unit * 2 + 30, top, unit, unit + 55);
  radar.setBounds(w - graph - 25, std::max(120, (h - graph) / 2), graph, graph);
}
} // namespace freshui
