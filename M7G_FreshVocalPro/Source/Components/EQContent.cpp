#include "EQContent.h"
namespace freshui {
EQContent::EQContent(FreshProcessor &p)
    : ModuleContent(p, 1, "FRESH EQ", fresh::eqOn),
      style(p, fresh::eqStyle, 1) {
  const fresh::Param ids[]{fresh::eqLow,  fresh::eqLowMid, fresh::eqHighMid,
                           fresh::eqHigh, fresh::eq160,    fresh::eq350,
                           fresh::eq1500, fresh::eq6000};
  const char *labels[]{"LOW",      "LOW-MID",  "HIGH-MID", "HIGH",
                       "EXPERT 1", "EXPERT 2", "EXPERT 3", "EXPERT 4"};
  const char *freq[]{"GAIN / 80 Hz",   "GAIN / 700 Hz", "GAIN / 2.5 kHz",
                     "GAIN / 12 kHz",  "GAIN / 160 Hz", "GAIN / 350 Hz",
                     "GAIN / 1.5 kHz", "GAIN / 6 kHz"};
  for (int i = 0; i < 8; ++i) {
    knobs[size_t(i)] = std::make_unique<Knob>(p, ids[i], labels[i], freq[i]);
    addAndMakeVisible(*knobs[size_t(i)]);
  }
  addAndMakeVisible(style);
}
void EQContent::resized() {
  ModuleContent::resized();
  int w = getWidth(), h = getHeight();
  style.setBounds((w - 440) / 2, 93, 440, 34);
  int unit = std::min(expert ? 175 : 185, (w - 120) / 4),
      left = (w - unit * 4 - 60) / 2;
  int top = expert ? 155 : std::max(165, (h - 220) / 2);
  for (int i = 0; i < 8; ++i) {
    auto &knob = *knobs[size_t(i)];
    knob.setVisible(i < 4 || expert);
    knob.setBounds(left + (i % 4) * (unit + 20), top + (i / 4) * (unit + 75),
                   unit, unit + 45);
  }
}
} // namespace freshui
