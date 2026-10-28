#include "CharacterView.h"
namespace freshui {
CharacterView::CharacterView(FreshProcessor &p) : processor(p) {
  setComponentID("character-curve");
  if (juce::SystemStats::getEnvironmentVariable("M7G_DISABLE_OPENGL", "") !=
      "1") {
    context.setComponentPaintingEnabled(true);
    context.setContinuousRepainting(false);
    context.attachTo(*this);
  }
  startTimerHz(60);
}
CharacterView::~CharacterView() {
  stopTimer();
  context.detach();
}
void CharacterView::visibilityChanged() { startTimerHz(60); }
void CharacterView::paint(juce::Graphics &g) {
  auto r = getLocalBounds().toFloat().reduced(6);
  g.setColour(black);
  g.fillRoundedRectangle(r, 10);
  g.setColour(accent(3).withAlpha(.12f));
  for (int i = 1; i < 8; ++i) {
    float x = r.getX() + r.getWidth() * i / 8;
    g.drawLine(x, r.getY() + 15, x, r.getBottom() - 35, 1);
  }
  for (int i = 1; i < 6; ++i) {
    float y = r.getY() + (r.getHeight() - 35) * i / 6;
    g.drawLine(r.getX() + 10, y, r.getRight() - 10, y, 1);
  }
  auto p = processor.settings();
  juce::Path curve;
  float mid = (r.getY() + r.getBottom() - 35) * .5f;
  for (int i = 0; i <= 300; ++i) {
    float input = -1 + 2.f * i / 300;
    float delta =
        fresh::CharacterSat::transfer(
            input, p[fresh::character] * .01f, int(p[fresh::charMode]),
            int(p[fresh::harmonics]), int(p[fresh::tape])) -
        input;
    float x = r.getX() + 10 + (r.getWidth() - 20) * i / 300,
          y = mid - delta * (r.getHeight() - 50) * .45f;
    if (i == 0)
      curve.startNewSubPath(x, y);
    else
      curve.lineTo(x, y);
  }
  g.setColour(accent(3).withAlpha(.1f));
  g.strokePath(curve, juce::PathStrokeType(8));
  g.setColour(accent(3));
  g.strokePath(curve, juce::PathStrokeType(2));
  M7GLookAndFeel::text(g, "HARMONIC SIGNATURE  /  output - input",
                       {0, getHeight() - 30, getWidth(), 20}, 10, accent(3));
}
} // namespace freshui
