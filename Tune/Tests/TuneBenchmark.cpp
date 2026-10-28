#include "TestSupport.h"
#include <thread>
#include <windows.h>
double processCPU() {
  FILETIME create, exit, kernel, user;
  GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user);
  ULARGE_INTEGER k, u;
  k.LowPart = kernel.dwLowDateTime;
  k.HighPart = kernel.dwHighDateTime;
  u.LowPart = user.dwLowDateTime;
  u.HighPart = user.dwHighDateTime;
  return (k.QuadPart + u.QuadPart) * 1e-7;
}
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  for (int quality : {0, 1})
    for (bool harmonies : {false, true}) {
      auto p = std::make_unique<TuneProcessor>();
      p->setValue("quality", float(quality));
      p->setValue("harmonyOn", harmonies ? 1.f : 0.f);
      p->setValue("voices", 4);
      p->setRateAndBufferSizeDetails(48000, 256);
      p->prepareToPlay(48000, 256);
      juce::AudioBuffer<float> b(2, 256);
      juce::MidiBuffer m;
      constexpr int blocks = 563;
      double cpuStart = processCPU(), callback = 0;
      auto start = std::chrono::steady_clock::now();
      for (int k = 0; k < blocks; ++k) {
        for (int i = 0; i < 256; ++i) {
          float x = vowel((k * 256 + i) / 48000., 224.7);
          b.setSample(0, i, x);
          b.setSample(1, i, x);
        }
        auto before = std::chrono::steady_clock::now();
        p->processBlock(b, m);
        callback += std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - before)
                        .count();
        std::this_thread::sleep_until(
            start + std::chrono::microseconds(
                        int64_t((k + 1) * 256 / 48000. * 1000000)));
      }
      double cpu = processCPU() - cpuStart, seconds = blocks * 256 / 48000.;
      std::cout << "BENCH instance=1 mode=" << (quality ? "Monitor" : "Studio")
                << " harmonies=" << (harmonies ? 4 : 0)
                << " rate=48000 block=256 audio_seconds=" << seconds
                << " callback_seconds=" << callback
                << " total_process_cpu_seconds=" << cpu
                << " one_core_equivalent_percent=" << 100 * cpu / seconds
                << " neural_batches=" << p->neural->runs.load()
                << " logical_cpus=" << juce::SystemStats::getNumCpus() << "\n";
    }
  std::cout << "Realtime-paced process-CPU measurement includes ONNX worker; "
               "editors closed, synthetic vowel. Machine-specific result, not "
               "a laptop guarantee.\n";
}
