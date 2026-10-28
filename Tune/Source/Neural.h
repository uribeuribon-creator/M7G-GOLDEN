#pragma once
#include <TuneData.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <mutex>
#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define ORT_API_MANUAL_INIT
#include <onnxruntime_cxx_api.h>
#if JUCE_WINDOWS
#include <windows.h>
#endif
namespace tune {
struct NeuralPacket {
  std::array<float, 192> pitch{};
  std::array<float, 12> profile{};
  float hz = 0;
  uint64_t serial = 0;
};
class NeuralWorker final : public juce::Thread {
public:
  NeuralWorker() : Thread("M7G TUNE neural analysis") {
    try {
#if JUCE_WINDOWS
      static std::once_flag loaded;
      std::call_once(loaded, [] {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&moduleAnchor), &self);
        wchar_t path[32768]{};
        GetModuleFileNameW(self, path, 32768);
        auto dll =
            juce::File(juce::String(path)).getSiblingFile("onnxruntime.dll");
        auto handle =
            LoadLibraryExW(dll.getFullPathName().toWideCharPointer(), nullptr,
                           LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                               LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!handle)
          throw std::runtime_error("Bundled ONNX Runtime cannot load");
        auto api = reinterpret_cast<const OrtApiBase *(ORT_API_CALL *)()>(
            GetProcAddress(handle, "OrtGetApiBase"));
        if (!api)
          throw std::runtime_error("OrtGetApiBase unavailable");
        Ort::InitApi(api()->GetApi(ORT_API_VERSION));
      });
#else
      throw std::runtime_error("Windows x64 runtime required");
#endif
      env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_ERROR, "M7G TUNE");
      env->DisableTelemetryEvents();
      Ort::SessionOptions options;
      options.SetIntraOpNumThreads(1);
      options.SetInterOpNumThreads(1);
      options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
      options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
      options.DisableProfiling();
      pitch = std::make_unique<Ort::Session>(
          *env, TuneData::AlienPitchCandidate_onnx,
          size_t(TuneData::AlienPitchCandidate_onnxSize), options);
      profile = std::make_unique<Ort::Session>(
          *env, TuneData::AlienVoiceProfile_onnx,
          size_t(TuneData::AlienVoiceProfile_onnxSize), options);
      ready.store(true);
      startThread();
    } catch (const std::exception &e) {
      error = juce::String(e.what());
      ready.store(false);
    }
  }
  ~NeuralWorker() override {
    signalThreadShouldExit();
    notify();
    stopThread(5000);
  }
  bool submit(const NeuralPacket &p) {
    int s1, n1, s2, n2;
    fifo.prepareToWrite(1, s1, n1, s2, n2);
    if (!n1)
      return false;
    packets[size_t(s1)] = p;
    fifo.finishedWrite(1);
    return true;
  }
  std::atomic<bool> ready{false};
  std::atomic<uint64_t> runs{0}, sourceSerial{0};
  std::atomic<float> sourceHz{0};
  std::array<std::atomic<float>, 3> scores{};
  std::array<std::atomic<float>, 4> acoustic{};
  juce::String error;
  void run() override {
    auto memory =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    const char *inputs[]{"features"};
    const char *po[]{"probability"};
    const char *ao[]{"profile"};
    const std::array<int64_t, 3> ps{3, 2, 32};
    const std::array<int64_t, 2> as{1, 12};
    while (!threadShouldExit()) {
      int s1, n1, s2, n2;
      int count = fifo.getNumReady();
      if (!count) {
        wait(5);
        continue;
      }
      fifo.prepareToRead(count, s1, n1, s2, n2);
      NeuralPacket p = packets[size_t(n2 ? s2 + n2 - 1 : s1 + n1 - 1)];
      fifo.finishedRead(count);
      try {
        auto x = Ort::Value::CreateTensor<float>(
            memory, p.pitch.data(), p.pitch.size(), ps.data(), ps.size());
        auto a = Ort::Value::CreateTensor<float>(
            memory, p.profile.data(), p.profile.size(), as.data(), as.size());
        auto py = pitch->Run(Ort::RunOptions{nullptr}, inputs, &x, 1, po, 1);
        auto ay = profile->Run(Ort::RunOptions{nullptr}, inputs, &a, 1, ao, 1);
        for (int i = 0; i < 3; ++i)
          scores[size_t(i)].store(py[0].GetTensorData<float>()[i]);
        for (int i = 0; i < 4; ++i)
          acoustic[size_t(i)].store(
              std::clamp(ay[0].GetTensorData<float>()[i], 0.f, 1.f));
        sourceHz.store(p.hz);
        sourceSerial.store(p.serial);
        runs.fetch_add(1);
      } catch (...) {
        ready.store(false);
        return;
      }
    }
  }

private:
  static void moduleAnchor() {}
  juce::AbstractFifo fifo{8};
  std::array<NeuralPacket, 8> packets{};
  std::unique_ptr<Ort::Env> env;
  std::unique_ptr<Ort::Session> pitch, profile;
};
} // namespace tune
