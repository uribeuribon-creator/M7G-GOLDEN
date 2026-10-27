#include "../Source/PluginProcessor.h"
#include <chrono>
#include <iostream>
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  constexpr int instances = 10, blocks = 375, size = 256;
  for (int mode : {0, 1}) {
    std::vector<std::unique_ptr<M7GProcessor>> processors;
    for (int i = 0; i < instances; ++i) {
      auto p = std::make_unique<M7GProcessor>();
      p->setValue(rack::EngineMode, float(mode));
      p->setValue(rack::HarmonyOn, 0);
      p->prepareToPlay(48000, size);
      processors.push_back(std::move(p));
    }
    juce::AudioBuffer<float> source(2, size), block(2, size);
    juce::MidiBuffer midi;
    auto start = std::chrono::steady_clock::now();
    for (int k = 0; k < blocks; ++k) {
      for (int c = 0; c < 2; ++c)
        for (int i = 0; i < size; ++i)
          source.setSample(
              c, i,
              .12f *
                  std::sin(float((k * size + i) * 223 *
                                 juce::MathConstants<double>::twoPi / 48000)));
      for (auto &p : processors) {
        for (int c = 0; c < 2; ++c)
          block.copyFrom(c, 0, source, c, 0, size);
        p->processBlock(block, midi);
      }
    }
    double elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
            .count();
    std::cout << "BENCH mode=" << (mode ? "Studio8x" : "Monitor")
              << " instances=" << instances
              << " rate=48000 block=256 audio_seconds=2 wall_seconds="
              << elapsed
              << " realtime_single_core_percent=" << 100 * elapsed / 2 << "\n";
  }
  std::cout << "Reference runner only. This does not establish 15% on a "
               "physical i5 / Studio One.\n";
}
