#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
int main(int argc, char **argv) {
  using namespace Steinberg;
  if (argc != 2)
    return 1;
  std::string modulePath = argv[1];
  if (!modulePath.empty() && modulePath[0] == '@') {
    std::ifstream args(modulePath.substr(1));
    if (!std::getline(args, modulePath))
      return 16;
  }
  HMODULE module = LoadLibraryA(modulePath.c_str());
  if (!module) {
    std::printf("LoadLibrary failed: %lu\n", GetLastError());
    return 2;
  }
  auto init = reinterpret_cast<bool (*)()>(GetProcAddress(module, "InitDll"));
  auto exit = reinterpret_cast<bool (*)()>(GetProcAddress(module, "ExitDll"));
  auto get = reinterpret_cast<IPluginFactory *(PLUGIN_API *)()>(
      GetProcAddress(module, "GetPluginFactory"));
  if (!init || !exit || !get || !init())
    return 3;
  auto *factory = get();
  if (!factory)
    return 4;
  PFactoryInfo info{};
  if (factory->getFactoryInfo(&info) != kResultOk)
    return 5;
  std::printf("VST3 vendor: %s; classes: %d\n", info.vendor,
              factory->countClasses());
  Vst::IComponent *component = nullptr;
  for (int i = 0; i < factory->countClasses(); ++i) {
    PClassInfo cls{};
    factory->getClassInfo(i, &cls);
    if (std::strcmp(cls.category, "Audio Module Class") == 0) {
      const unsigned char expected[]{0x01, 0xEF, 0xCD, 0xAB, 0x82, 0x91,
                                     0xEB, 0xFA, 0x4D, 0x37, 0x47, 0x37,
                                     0x46, 0x72, 0x42, 0x31};
      if (std::strcmp(cls.name, "M7G Fresh Vocal Pro Beast") != 0 ||
          std::memcmp(cls.cid, expected, 16) != 0)
        return 17;
      std::printf("VST3 name: %s; class ID: ", cls.name);
      for (auto byte : cls.cid)
        std::printf("%02X", static_cast<unsigned char>(byte));
      std::puts("");
      if (factory->createInstance(cls.cid, Vst::IComponent_iid,
                                  reinterpret_cast<void **>(&component)) !=
          kResultOk)
        return 6;
      break;
    }
  }
  if (!component)
    return 7;
  if (component->initialize(nullptr) != kResultOk)
    return 8;
  Vst::IAudioProcessor *audio = nullptr;
  if (component->queryInterface(Vst::IAudioProcessor_iid,
                                reinterpret_cast<void **>(&audio)) != kResultOk)
    return 9;
  Vst::SpeakerArrangement stereo = Vst::SpeakerArr::kStereo;
  const int inputBusCount = component->getBusCount(Vst::kAudio, Vst::kInput);
  std::vector<Vst::SpeakerArrangement> arrangements(size_t(inputBusCount),
                                                    Vst::SpeakerArr::kEmpty);
  arrangements[0] = stereo;
  if (audio->setBusArrangements(arrangements.data(), inputBusCount, &stereo,
                                1) != kResultOk)
    return 10;
  Vst::ProcessSetup setup{};
  setup.processMode = Vst::kRealtime;
  setup.symbolicSampleSize = Vst::kSample32;
  setup.maxSamplesPerBlock = 256;
  setup.sampleRate = 48000;
  if (audio->setupProcessing(setup) != kResultOk)
    return 11;
  component->activateBus(Vst::kAudio, Vst::kInput, 0, true);
  component->activateBus(Vst::kAudio, Vst::kOutput, 0, true);
  if (component->setActive(true) != kResultOk ||
      audio->setProcessing(true) != kResultOk)
    return 12;
  float in[2][256]{}, out[2][256]{};
  float *input[]{in[0], in[1]};
  float *output[]{out[0], out[1]};
  std::vector<Vst::AudioBusBuffers> inputBuses(
      static_cast<size_t>(inputBusCount));
  auto &ib = inputBuses[0];
  Vst::AudioBusBuffers ob{};
  ib.numChannels = ob.numChannels = 2;
  ib.channelBuffers32 = input;
  ob.channelBuffers32 = output;
  Vst::ProcessData data{};
  data.processMode = Vst::kRealtime;
  data.symbolicSampleSize = Vst::kSample32;
  data.numSamples = 256;
  data.numInputs = inputBusCount;
  data.numOutputs = 1;
  data.inputs = inputBuses.data();
  data.outputs = &ob;
  double energy = 0;
  for (int block = 0; block < 1100; ++block) {
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < 256; ++i)
        in[c][i] = .15f * std::sin(float(6.283185307179586 * 223 *
                                         (block * 256 + i) / 48000));
    if (audio->process(data) != kResultOk)
      return 13;
    for (auto &channel : out)
      for (auto x : channel) {
        if (!std::isfinite(x) || std::abs(x) > 1.001f)
          return 14;
        energy += x * x;
      }
  }
  std::printf("VST3 audio: energy %.3f; host latency %u samples\n", energy,
              audio->getLatencySamples());
  if (energy < 1)
    return 15;
  audio->setProcessing(false);
  component->setActive(false);
  audio->release();
  component->terminate();
  component->release();
  factory->release();
  exit();
  FreeLibrary(module);
  std::puts(
      "PASS: Windows VST3 factory, stereo buses, audio processing and unload");
  return 0;
}
