#pragma once
#include "Widgets.h"
#include <juce_opengl/juce_opengl.h>
namespace freshui {
class SpaceView final : public juce::Component, private juce::Timer {
public:
  explicit SpaceView(FreshProcessor &);
  ~SpaceView() override;
  void paint(juce::Graphics &) override;
  void visibilityChanged() override;

private:
  void timerCallback() override;
  FreshProcessor &processor;
  juce::OpenGLContext context;
  double angle = 0;
};
} // namespace freshui
