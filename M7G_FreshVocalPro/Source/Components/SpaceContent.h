#pragma once
#include "SpaceView.h"
namespace freshui {
class SpaceContent final : public ModuleContent {
public:
  explicit SpaceContent(FreshProcessor &);
  void resized() override;

private:
  Knob reverb, time, delay;
  Options size, type, division;
  SpaceView radar;
};
} // namespace freshui
