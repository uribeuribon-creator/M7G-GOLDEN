#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace freshui {
inline const juce::Colour black{0xff0a0a0a}, panel{0xff141414},
    gold{0xffd4af37}, white{0xfff2f0e9}, dim{0xff898982};
inline juce::Colour accent(int module) {
  const juce::Colour c[]{juce::Colour(0xffec725f), gold,
                         juce::Colour(0xffb08af2), juce::Colour(0xff65aaff)};
  return c[juce::jlimit(0, 3, module)];
}
class M7GLookAndFeel final : public juce::LookAndFeel_V4 {
public:
  M7GLookAndFeel();
  static juce::Typeface::Ptr regular();
  static juce::Typeface::Ptr heavy();
  static void text(juce::Graphics &, juce::String, juce::Rectangle<int>, float,
                   juce::Colour = white,
                   int align = juce::Justification::centred, bool bold = false);
  void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider &) override;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
  void drawButtonText(juce::Graphics &, juce::TextButton &, bool,
                      bool) override;
  juce::Font getTextButtonFont(juce::TextButton &, int) override;
  juce::Font getLabelFont(juce::Label &) override;
  juce::Font getSliderPopupFont(juce::Slider &) override;
};
} // namespace freshui
