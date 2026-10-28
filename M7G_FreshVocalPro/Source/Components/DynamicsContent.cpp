#include "DynamicsContent.h"
namespace freshui {
DynamicsContent::DynamicsContent(FreshProcessor &p)
    : ModuleContent(p, 0, "FRESH DYNAMICS", fresh::dynOn),
      comp(p, fresh::comp, "COMP", "AMOUNT"),
      color(p, fresh::color, "COLOR", "AMOUNT"),
      focus(p, fresh::focus, "DE-ESS FOCUS", "Hz"),
      style(p, fresh::dynStyle, 0), meter(p) {
  for (auto *c : std::array<juce::Component *, 5>{
           {&comp, &color, &focus, &style, &meter}})
    addAndMakeVisible(*c);
}
void DynamicsContent::resized() {
  ModuleContent::resized();
  int w = getWidth(), h = getHeight();
  style.setBounds((w - 390) / 2, 93, 390, 34);
  int unit = std::min(190, (w - 110) / 3), start = (w - unit * 3 - 60) / 2,
      y = std::max(154, (h - 240) / 2);
  comp.setBounds(start, y, unit, unit + 50);
  color.setBounds(start + unit + 30, y, unit, unit + 50);
  focus.setBounds(start + unit * 2 + 60, y, unit, unit + 50);
  meter.setBounds(w / 2 - 145, std::min(h - 110, y + unit + 90), 290, 80);
}
} // namespace freshui
