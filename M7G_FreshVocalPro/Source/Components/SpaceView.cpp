#include "SpaceView.h"
namespace freshui {
SpaceView::SpaceView(FreshProcessor &p) : processor(p) {
  setComponentID("space-radar");
  if (juce::SystemStats::getEnvironmentVariable("M7G_DISABLE_OPENGL", "") !=
      "1") {
    context.setComponentPaintingEnabled(true);
    context.setContinuousRepainting(false);
    context.attachTo(*this);
  }
  startTimerHz(60);
}
SpaceView::~SpaceView() {
  stopTimer();
  context.detach();
}
void SpaceView::visibilityChanged() { startTimerHz(60); }
void SpaceView::timerCallback() {
  if (isShowing()) {
    angle += .012;
    repaint();
  }
}
void SpaceView::paint(juce::Graphics &g) {
  auto r = getLocalBounds().toFloat().reduced(12);
  g.setColour(black);
  g.fillRoundedRectangle(r, 12);
  auto centre = r.getCentre();
  float radius = std::min(r.getWidth(), r.getHeight()) * .42f;
  g.setColour(gold.withAlpha(.12f));
  for (int i = 1; i <= 4; ++i)
    g.drawEllipse(centre.x - radius * i / 4, centre.y - radius * i / 4,
                  radius * i / 2, radius * i / 2, 1);
  g.drawLine(centre.x - radius, centre.y, centre.x + radius, centre.y, 1);
  g.drawLine(centre.x, centre.y - radius, centre.x, centre.y + radius, 1);
  auto p = processor.settings();
  float scale = .15f + .85f * p[fresh::time] / 10,
        energy = juce::jlimit(0.f, 1.f, processor.meters().out.load() * 3);
  g.setColour(gold.withAlpha(.35f));
  float activeRadius = radius * scale;
  g.drawEllipse(centre.x - activeRadius, centre.y - activeRadius,
                activeRadius * 2, activeRadius * 2, 1.5f);
  for (int i = 0; i < 20; ++i) {
    double a = angle * (.4 + i * .035) + i * 2.399963;
    float distance = activeRadius * (.25f + .75f * float((i * 7) % 20) / 20);
    float x = centre.x + float(std::sin(a)) * distance,
          y = centre.y + float(std::cos(a)) * distance;
    float size = 2.5f + energy * 2;
    g.setColour(gold.withAlpha(.35f + .5f * energy));
    g.fillEllipse(x - size / 2, y - size / 2, size, size);
  }
  g.setColour(gold.withAlpha(.6f));
  g.drawLine(centre.x, centre.y,
             centre.x + float(std::sin(angle)) * activeRadius,
             centre.y + float(std::cos(angle)) * activeRadius, 1);
  M7GLookAndFeel::text(g, "SPACE FIELD", {0, getHeight() - 35, getWidth(), 20},
                       11, gold);
}
} // namespace freshui
