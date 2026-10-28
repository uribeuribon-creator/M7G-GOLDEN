#pragma once
#include "SpaceView.h"
namespace freshui {
class CharacterView final : public juce::Component, private juce::Timer {
public:
  explicit CharacterView(FreshProcessor &);
  ~CharacterView() override;
  void paint(juce::Graphics &) override;
  void visibilityChanged() override;

private:
  void timerCallback() override {
    if (isShowing())
      repaint();
  }
  FreshProcessor &processor;
  juce::OpenGLContext context;
};
} // namespace freshui
