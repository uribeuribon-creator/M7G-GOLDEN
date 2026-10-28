#include "CRMeter.h"
namespace freshui {
void CRMeter::paint(juce::Graphics &g) {
  auto r = getLocalBounds().toFloat();
  g.setColour(black);
  g.fillRoundedRectangle(r, 6);
  float value = processor.meters().compression.load();
  M7GLookAndFeel::text(g, "CR  " + juce::String(value, 1),
                       {0, 4, getWidth(), 32}, 22, gold,
                       juce::Justification::centred, true);
  M7GLookAndFeel::text(g, "GAIN REDUCTION", {0, 38, getWidth(), 17}, 10, dim);
  int width = (getWidth() - 24) / 24;
  for (int i = 0; i < 24; ++i) {
    g.setColour(value > i ? (i > 16 ? accent(0) : gold)
                          : juce::Colour(0xff30302c));
    g.fillRoundedRectangle(float(12 + i * width), float(getHeight() - 20),
                           float(width - 2), 9, 1);
  }
}
} // namespace freshui
