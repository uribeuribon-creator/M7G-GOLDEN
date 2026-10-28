#include "PresetManager.h"
namespace fresh {
const std::vector<FactoryPreset> &PresetManager::factory() {
  static const auto presets = [] {
    std::vector<FactoryPreset> all;
    auto add = [&](juce::String name, Values v) {
      all.push_back({std::move(name), sanitise(v)});
    };
    auto v = defaults();
    v[comp] = 70;
    v[dynStyle] = 2;
    v[character] = 44;
    add("Aggressive Vocal", v);
    v = defaults();
    v[comp] = 28;
    v[reverb] = 34;
    v[delay] = 12;
    v[character] = 15;
    v[width] = 1;
    add("backing vocal", v);
    v[reverb] = 40;
    v[eqLow] = -5;
    v[output] = -2;
    add("Backing Vocal 2", v);
    add("Default", defaults());
    for (int i = 0; i < 3; ++i) {
      v = defaults();
      v[eqLowMid] = -3 - i;
      v[eqHighMid] = i;
      v[comp] = 24 + 5.f * i;
      v[reverb] = 15;
      add(i == 0 ? "Mid Control" : "Mid Control " + juce::String(i + 1), v);
    }
    auto reg = [&](int i) {
      auto r = defaults();
      if (i > 1) {
        r[comp] = 18.9f + 1.15f * (i - 1);
        r[color] = 50 + 2.5f * (i - 1);
        r[focus] = 5000 + float((i % 4) - 1) * 200;
        r[reverb] = 24.1f + float((i * 3) % 9);
        r[time] = 4.6f + float((i % 4) - 1) * .3f;
        r[delay] = 6 + float(i % 4);
      }
      r[eqStyle] = 2;
      return r;
    };
    add("Reggaeton 1", reg(1));
    add("Reggaeton 10", reg(10));
    for (int i = 2; i <= 9; ++i)
      add("Reggaeton " + juce::String(i), reg(i));
    v = reg(1);
    v[reverb] = 45;
    v[time] = 7;
    v[delay] = 10;
    add("Reggaeton big Reverb", v);
    v = defaults();
    v[comp] = 25;
    v[color] = 60;
    v[focus] = 4800;
    v[reverb] = 20;
    v[time] = 3.5;
    v[delay] = 4;
    v[eqLow] = 2;
    v[eqHigh] = 4;
    v[dynStyle] = 2;
    v[charMode] = 2;
    v[character] = 35;
    add("TRAP PESADO 1", v);
    v[comp] = 30;
    v[color] = 65;
    v[reverb] = 22;
    v[time] = 4;
    v[delay] = 5;
    v[width] = 1;
    v[character] = 45;
    add("TRAP PESADO 2", v);
    v = defaults();
    v[comp] = 15;
    v[color] = 40;
    v[focus] = 5500;
    v[reverb] = 26;
    v[time] = 4.8;
    v[delay] = 6;
    v[eqLow] = -1;
    v[dynStyle] = 0;
    v[verbType] = 0;
    v[division] = 1;
    add("Voz Femenina 1", v);
    v[eqStyle] = 0;
    v[eqHigh] = 2;
    v[character] = 18;
    add("Voz Femenina 2", v);
    v[eqStyle] = 2;
    v[dynStyle] = 1;
    v[eqHigh] = 5;
    add("Voz Femenina 3", v);
    v = defaults();
    v[character] = 23;
    v[eqLow] = 2;
    v[eqHigh] = 2;
    v[dynStyle] = 0;
    v[verbType] = 1;
    v[reverb] = 15;
    v[time] = 1.8;
    add("Warm Vocal", v);
    v = defaults();
    v[comp] = 14;
    v[reverb] = 10;
    v[delay] = 2;
    v[time] = 1.5;
    v[eqLow] = -1;
    v[eqHigh] = 3;
    add("Lead Vocal Clean", v);
    v = defaults();
    for (auto p : {comp, color, reverb, delay, character})
      v[size_t(p)] = 50;
    v[time] = 5;
    add("M7G Golden Perfect", v);
    v = defaults();
    v[comp] = 38;
    v[focus] = 6200;
    v[reverb] = 14;
    v[time] = 2;
    v[delay] = 5;
    v[eqLow] = -3;
    v[eqHighMid] = -1;
    add("IA Control Vocal", v);
    v = defaults();
    v[reverb] = 46;
    v[time] = 6.6;
    v[delay] = 18;
    v[character] = 19;
    v[division] = 1;
    add("Kid Laroi Syrup", v);
    v = defaults();
    v[comp] = 52;
    v[character] = 65;
    v[charMode] = 1;
    v[reverb] = 17;
    add("M7G Beast Punch", v);
    v = defaults();
    v[eqHigh] = 9;
    v[focus] = 6800;
    v[reverb] = 30;
    v[character] = 12;
    add("M7G Diamond Air", v);
    v = defaults();
    v[character] = 55;
    v[tape] = 1;
    v[eqHigh] = 1;
    v[eqLow] = 3;
    v[dynStyle] = 0;
    add("M7G Velvet Tape", v);
    v = defaults();
    v[reverb] = 65;
    v[time] = 8;
    v[delay] = 24;
    v[spaceMode] = 2;
    add("M7G Cathedral Gold", v);
    v = defaults();
    v[delay] = 35;
    v[reverb] = 10;
    v[time] = 1;
    v[division] = 0;
    v[character] = 18;
    add("M7G Dembow Echo", v);
    v = defaults();
    v[comp] = 45;
    v[reverb] = 8;
    v[delay] = 0;
    v[character] = 25;
    v[eqLow] = -4;
    add("M7G Radio Forward", v);
    v = defaults();
    v[reverb] = 36;
    v[delay] = 15;
    v[character] = 40;
    v[width] = 1;
    v[output] = -2;
    add("M7G Midnight Doubles", v);
    return all;
  }();
  return presets;
}
juce::ValueTree PresetManager::toTree(const Values &values,
                                      const juce::String &name) {
  juce::ValueTree tree("M7GPresets");
  tree.setProperty("name", name, nullptr);
  tree.setProperty("format", 1, nullptr);
  for (int i = 0; i < count; ++i)
    tree.setProperty(specs[size_t(i)].id, values[size_t(i)], nullptr);
  return tree;
}
bool PresetManager::fromTree(const juce::ValueTree &tree, Values &values,
                             juce::String &name) {
  if (!tree.hasType("M7GPresets") || int(tree["format"]) != 1)
    return false;
  auto next = defaults();
  for (int i = 0; i < count; ++i) {
    auto key = specs[size_t(i)].id;
    if (!tree.hasProperty(key))
      return false;
    float x = float(tree[key]);
    if (!std::isfinite(x))
      return false;
    next[size_t(i)] = x;
  }
  values = sanitise(next);
  name = tree["name"].toString().substring(0, 100);
  return true;
}
} // namespace fresh
