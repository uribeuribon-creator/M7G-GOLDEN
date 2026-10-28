#include "TestSupport.h"
#include <thread>
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  for (bool digital : {false, true})
    for (bool silent : {true, false}) {
      auto p = std::make_unique<ScratchProcessor>();
      p->setCurrentProgram(digital ? 15 : 35);
      p->setRateAndBufferSizeDetails(48000, 256);
      p->prepareToPlay(48000, 256);
      juce::AudioBuffer<float> b(2, 256);
      juce::MidiBuffer midi;
      constexpr int blocks = 750;
      double callbacks = 0, maximum = 0;
      auto start = std::chrono::steady_clock::now();
      for (int k = 0; k < blocks; ++k) {
        for (int i = 0; i < 256; ++i) {
          double t = (k * 256 + i) / 48000.;
          float x = silent ? 0 : scratchVoice(std::fmod(t, 3.8));
          b.setSample(0, i, x);
          b.setSample(1, i, x);
        }
        auto a = std::chrono::steady_clock::now();
        p->processBlock(b, midi);
        double dt =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - a)
                .count();
        callbacks += dt;
        maximum = std::max(maximum, dt);
        std::this_thread::sleep_until(
            start + std::chrono::microseconds(
                        int64_t((k + 1) * 256 / 48000. * 1000000)));
      }
      double seconds = blocks * 256 / 48000.;
      std::cout << "BENCH instances=1 mode=" << (digital ? "DIGITAL" : "VINYL")
                << " signal=" << (silent ? "silence" : "vocal")
                << " rate=48000 block=256 audio_seconds=" << seconds
                << " callback_seconds=" << callbacks
                << " callback_wall_percent=" << 100 * callbacks / seconds
                << " worst_callback_ms=" << maximum * 1000
                << " underflows=" << p->digitalUnderflows.load()
                << " logical_cpus=" << juce::SystemStats::getNumCpus() << "\n";
    }
  std::cout
      << "Realtime-paced callback wall time, including scheduling pauses. "
         "Editors closed; synthetic voice. No laptop CPU guarantee.\n";
}
