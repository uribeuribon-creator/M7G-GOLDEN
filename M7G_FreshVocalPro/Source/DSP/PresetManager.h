#pragma once
#include "Parameters.h"
namespace fresh {
struct FactoryPreset {
  juce::String name;
  Values values;
};
class PresetManager {
public:
  static const std::vector<FactoryPreset> &factory();
  static juce::ValueTree toTree(const Values &, const juce::String &);
  static bool fromTree(const juce::ValueTree &, Values &, juce::String &);
};
} // namespace fresh
