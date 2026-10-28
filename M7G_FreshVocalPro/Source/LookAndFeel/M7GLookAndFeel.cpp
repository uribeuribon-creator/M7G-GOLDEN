#include "M7GLookAndFeel.h"
#if __has_include("FreshData.h")
#include "FreshData.h"
#else
#include "BinaryData.h"
namespace FreshData = BinaryData;
#endif
namespace freshui {
juce::Typeface::Ptr M7GLookAndFeel::regular() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      FreshData::InterRegular_ttf, FreshData::InterRegular_ttfSize);
  return f;
}
juce::Typeface::Ptr M7GLookAndFeel::heavy() {
  static auto f = juce::Typeface::createSystemTypefaceFor(
      FreshData::InterBlack_ttf, FreshData::InterBlack_ttfSize);
  return f;
}
M7GLookAndFeel::M7GLookAndFeel() {
  setColour(juce::TextButton::textColourOffId, white);
  setColour(juce::TextButton::textColourOnId, white);
  setColour(juce::Label::textColourId, white);
  setColour(juce::Slider::textBoxTextColourId, white);
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
  setColour(juce::PopupMenu::backgroundColourId, panel);
  setColour(juce::PopupMenu::textColourId, white);
  setColour(juce::TooltipWindow::backgroundColourId, black);
  setColour(juce::TooltipWindow::textColourId, gold);
  setColour(juce::TooltipWindow::outlineColourId, gold);
  setColour(juce::ListBox::backgroundColourId, black);
  setColour(juce::ScrollBar::thumbColourId, gold.withAlpha(.5f));
}
void M7GLookAndFeel::text(juce::Graphics &g, juce::String s,
                          juce::Rectangle<int> r, float size,
                          juce::Colour color, int align, bool bold) {
  g.setFont(juce::Font(bold ? heavy() : regular()).withHeight(size));
  g.setColour(color);
  g.drawFittedText(s, r, align, 1);
}
juce::Font M7GLookAndFeel::getTextButtonFont(juce::TextButton &, int h) {
  return juce::Font(regular()).withHeight(float(juce::jlimit(10, 14, h / 2)));
}
juce::Font M7GLookAndFeel::getLabelFont(juce::Label &) {
  return juce::Font(regular()).withHeight(14);
}
juce::Font M7GLookAndFeel::getSliderPopupFont(juce::Slider &) {
  return juce::Font(heavy()).withHeight(15);
}
void M7GLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int w,
                                      int h, float position, float start,
                                      float end, juce::Slider &slider) {
  auto r = juce::Rectangle<float>(float(x), float(y), float(w), float(h))
               .withSizeKeepingCentre(float(std::min(w, h) - 12),
                                      float(std::min(w, h) - 12));
  auto a = slider.getProperties().contains("accent")
               ? juce::Colour(
                     juce::uint32(int64_t(slider.getProperties()["accent"])))
               : gold;
  bool over = slider.isMouseOverOrDragging();
  for (int i = 4; i > 0; --i) {
    g.setColour(a.withAlpha((over ? .045f : .012f) * float(5 - i)));
    g.drawEllipse(r.expanded(float(i * 2)), 2);
  }
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff363636),
                                         r.getCentreX(), r.getY(), black,
                                         r.getCentreX(), r.getBottom(), false));
  g.fillEllipse(r);
  g.setColour(juce::Colour(0xff474744));
  g.drawEllipse(r, 1);
  auto inner = r.reduced(9);
  g.setColour(black);
  g.fillEllipse(inner);
  float radius = r.getWidth() * .5f - 3;
  juce::Path rail, arc;
  rail.addCentredArc(r.getCentreX(), r.getCentreY(), radius, radius, 0, start,
                     end, true);
  arc.addCentredArc(r.getCentreX(), r.getCentreY(), radius, radius, 0, start,
                    start + position * (end - start), true);
  g.setColour(juce::Colour(0xff39372f));
  g.strokePath(rail, juce::PathStrokeType(3));
  g.setColour(a);
  g.strokePath(arc, juce::PathStrokeType(over ? 4 : 3));
  float angle = start + position * (end - start);
  auto centre = r.getCentre();
  g.drawLine(centre.x + std::sin(angle) * radius * .52f,
             centre.y - std::cos(angle) * radius * .52f,
             centre.x + std::sin(angle) * radius * .82f,
             centre.y - std::cos(angle) * radius * .82f, 2.5f);
}
void M7GLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                          const juce::Colour &, bool over,
                                          bool down) {
  if (bool(b.getProperties()["module-title"]))
    return;
  auto r = b.getLocalBounds().toFloat().reduced(1);
  auto c =
      b.getProperties().contains("accent")
          ? juce::Colour(juce::uint32(int64_t(b.getProperties()["accent"])))
          : gold;
  bool on = b.getToggleState();
  g.setGradientFill(
      juce::ColourGradient(on ? c.withAlpha(.23f) : juce::Colour(0xff262626), 0,
                           0, black, 0, r.getHeight(), false));
  g.fillRoundedRectangle(r, 5);
  g.setColour((on || over ? c : juce::Colour(0xff45453e))
                  .withAlpha(b.isEnabled() ? 1.f : .35f));
  g.drawRoundedRectangle(r, 5, down ? 2.f : 1.f);
  if (on) {
    g.setColour(c.withAlpha(.15f));
    g.drawRoundedRectangle(r.reduced(2), 3, 3);
  }
}
void M7GLookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &b,
                                    bool over, bool down) {
  if (bool(b.getProperties()["module-title"])) {
    auto color = b.getToggleState() ? white : dim;
    g.setColour(b.getToggleState() ? gold : dim);
    g.fillEllipse(0.f, float(b.getHeight() / 2 - 3), 6, 6);
    text(g, b.getButtonText(), b.getLocalBounds().withTrimmedLeft(15), 18,
         color, juce::Justification::left, true);
    return;
  }
  juce::LookAndFeel_V4::drawButtonText(g, b, over, down);
}
} // namespace freshui
