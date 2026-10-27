#include "TestSupport.h"
#include <chrono>
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  for (bool silent : {true, false}) {
    std::vector<std::unique_ptr<TransitionsProcessor>> processors;
    for (int i = 0; i < 10; ++i) {
      auto p = std::make_unique<TransitionsProcessor>();
      p->setCurrentProgram((i * 17) % 150);
      p->setRateAndBufferSizeDetails(48000, 256);
      p->prepareToPlay(48000, 256);
      processors.push_back(std::move(p));
    }
    juce::AudioBuffer<float> source(2, 256), audio(2, 256);
    juce::MidiBuffer midi;
    auto start = std::chrono::steady_clock::now();
    for (int k = 0; k < 938; ++k) {
      for (int i = 0; i < 256; ++i) {
        float x = silent ? 0 : testVoice((k * 256 + i) / 48000.0);
        source.setSample(0, i, x);
        source.setSample(1, i, x);
      }
      for (auto &p : processors) {
        for (int c = 0; c < 2; ++c)
          audio.copyFrom(c, 0, source, c, 0, 256);
        p->processBlock(audio, midi);
      }
    }
    double elapsed = std::chrono::duration<double>(
                         std::chrono::steady_clock::now() - start)
                         .count(),
           seconds = 938 * 256 / 48000.0;
    std::cout << "BENCH instances=10 rate=48000 block=256 signal="
              << (silent ? "silence" : "vocal-phrases")
              << " audio_seconds=" << seconds << " wall_seconds=" << elapsed
              << " single_core_equivalent_percent=" << 100 * elapsed / seconds
              << " logical_cpus=" << juce::SystemStats::getNumCpus() << "\n";
  }
  std::cout << "Callback-only runner measurement; no hardware-specific CPU "
               "guarantee. Fixed lookahead: 2000 ms.\n";
}
