#pragma once
#include "CharacterView.h"
namespace freshui {
class CharacterContent final : public ModuleContent {
public:
  explicit CharacterContent(FreshProcessor &);
  void resized() override;

private:
  Knob character;
  Options mode, harmonics, tape, width;
  CharacterView curve;
};
} // namespace freshui
