#pragma once
#include "Widgets.h"
namespace freshui {
class EQContent final : public ModuleContent {
public:
  explicit EQContent(FreshProcessor &);
  void resized() override;

private:
  std::array<std::unique_ptr<Knob>, 8> knobs;
  Options style;
};
} // namespace freshui
