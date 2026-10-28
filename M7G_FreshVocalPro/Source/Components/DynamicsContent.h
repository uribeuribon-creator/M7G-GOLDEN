#pragma once
#include "CRMeter.h"
namespace freshui {
class DynamicsContent final : public ModuleContent {
public:
  explicit DynamicsContent(FreshProcessor &);
  void resized() override;

private:
  Knob comp, color, focus;
  Options style;
  CRMeter meter;
};
} // namespace freshui
