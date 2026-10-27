#include "TestSupport.h"
#include <chrono>
#include <fstream>
#include <map>
#include <set>
namespace {
double energy(const std::vector<float> &x, int from, int count) {
  double sum = 0;
  for (int i = std::max(0, from); i < std::min(int(x.size()), from + count);
       ++i)
    sum += x[size_t(i)] * x[size_t(i)];
  return sum;
}
void timing() {
  using namespace transition;
  for (int bank = 0; bank < 3; ++bank) {
    auto engine = std::make_unique<Engine>();
    engine->prepare(48000);
    engine->setControls(bank * 50, 60, 65, 120, 0, true);
    int first = -1, last = -1;
    double before = 0, after = 0;
    for (int i = 0; i < 48000 * 6; ++i) {
      float input = testVoice(double(i) / 48000);
      engine->process(input, input);
      auto result = engine->mix(1, 1, 0);
      if (result.activity > .01f && first < 0)
        first = i;
      if (result.activity > .01f)
        last = i;
      if (i > 48000 && i < int(48000 * 2.45))
        before += result.left * result.left;
      if (i > int(48000 * 3.85))
        after += result.left * result.left;
    }
    std::cout << "Timing bank=" << bank << " first=" << double(first) / 48000
              << " last=" << double(last) / 48000 << " pre-energy=" << before
              << " post-energy=" << after << "\n";
    if (bank == 0)
      requireTransition(first > int(.45 * 48000) && first < int(2.25 * 48000) &&
                            before > .001,
                        "INTRO failed lookahead / pre-vocal timing");
    if (bank == 1)
      requireTransition(first > int(3.85 * 48000) && first < int(4.4 * 48000),
                        "CAMBIO did not wait for the actual phrase gap");
    if (bank == 2)
      requireTransition(first > int(2.45 * 48000) && first < int(3.85 * 48000),
                        "SALIDA did not process the final part of the phrase");
  }
}
void dryAndState() {
  for (double rate : {44100.0, 48000.0, 96000.0})
    for (int mode = 0; mode < 3; ++mode) {
      auto p = std::make_unique<TransitionsProcessor>();
      p->setCurrentProgram(100);
      p->setValue(mode == 0   ? "amount"
                  : mode == 1 ? "mix"
                              : "bypass",
                  mode == 2 ? 1 : 0);
      p->setRateAndBufferSizeDetails(rate, 127);
      p->prepareToPlay(rate, 127);
      juce::MidiBuffer midi;
      int latency = p->getLatencySamples();
      requireTransition(latency == int(rate * 2), "Incorrect declared latency");
      int peak = -1;
      float maximum = 0;
      double error = 0;
      for (int start = 0; start < latency + 500; start += 127) {
        int n = std::min(127, latency + 500 - start);
        juce::AudioBuffer<float> b(2, n);
        b.clear();
        if (start == 0) {
          b.setSample(0, 0, .3f);
          b.setSample(1, 0, .3f);
        }
        p->processBlock(b, midi);
        for (int i = 0; i < n; ++i) {
          float x = b.getSample(0, i);
          float expected = start + i == latency ? .3f : 0;
          error += std::abs(x - expected);
          if (std::abs(x) > maximum) {
            maximum = std::abs(x);
            peak = start + i;
          }
        }
      }
      requireTransition(peak == latency && error < 1e-6,
                        "Zero amount/mix/bypass changed aligned dry audio");
    }
  auto p = std::make_unique<TransitionsProcessor>();
  p->setCurrentProgram(143);
  p->setValue("amount", 83);
  p->setValue("time", 61);
  p->setValue("mix", 42);
  p->copyCompare();
  p->selectCompare(1);
  p->setValue("mix", 17);
  p->selectCompare(0);
  requireTransition(std::abs(p->value("mix") - 42) < .01f, "A/B recall failed");
  juce::MemoryBlock state;
  p->getStateInformation(state);
  auto copy = std::make_unique<TransitionsProcessor>();
  copy->setStateInformation(state.getData(), int(state.getSize()));
  requireTransition(copy->getCurrentProgram() == 143 &&
                        std::abs(copy->value("amount") - 83) < .01f,
                    "Preset/state recall failed");
  copy->setBank(0);
  copy->setCurrentProgram(22);
  copy->setBank(2);
  requireTransition(copy->getCurrentProgram() == 143,
                    "Bank-local preset memory failed");
  bool favourite = copy->isFavourite(143);
  copy->toggleFavourite(143);
  auto recalled = std::make_unique<TransitionsProcessor>();
  requireTransition(recalled->isFavourite(143) != favourite,
                    "Favourite did not persist");
  copy->toggleFavourite(143);
  requireTransition(p->authorized.load(),
                    "Shared M7G global licence not loaded");
  auto invalid = juce::File::getSpecialLocation(juce::File::tempDirectory)
                     .getChildFile("M7G-transition-invalid.license");
  invalid.replaceWithText("{}");
  requireTransition(!p->importLicense(invalid) && p->authorized.load(),
                    "Invalid import removed existing authorisation");
  invalid.deleteFile();
  std::cout << "PASS: exact 2-second dry/bypass latency at three rates, A/B, "
               "state, bank memory, favourites, global licence\n";
}
void ratesSeekAndIdle() {
  for (double sr : {44100.0, 96000.0})
    for (int block : {64, 1024})
      for (int preset : {0, 9, 50, 89, 100, 123}) {
        auto out = renderTransition(preset, sr, block, 80, 70, 93, 6.1);
        requireTransition(out.energy > .001 && out.delta > .00001,
                          "Rate/block render silent or did not apply FX");
      }
  auto p = std::make_unique<TransitionsProcessor>();
  p->setRateAndBufferSizeDetails(48000, 256);
  p->prepareToPlay(48000, 256);
  juce::MidiBuffer midi;
  for (int i = 0; i < 500; ++i) {
    juce::AudioBuffer<float> b(2, 256);
    b.clear();
    p->processBlock(b, midi);
  }
  requireTransition(p->idle.load() && p->activeVoices.load() == 0,
                    "Silent effect engine did not suspend");
  juce::AudioBuffer<float> empty(2, 0);
  p->processBlock(empty, midi);
  juce::AudioBuffer<float> large(2, 65536);
  large.clear();
  p->processBlock(large, midi);
  requireTransition(large.getMagnitude(0, 65536) == 0,
                    "Oversized silence block produced audio");
  TestPlayHead head;
  p->setPlayHead(&head);
  for (int k = 0; k < 500; ++k) {
    head.sample = k * 256;
    juce::AudioBuffer<float> b(2, 256);
    for (int i = 0; i < 256; ++i) {
      float x = testVoice((k * 256 + i) / 48000.0);
      b.setSample(0, i, x);
      b.setSample(1, i, x);
    }
    p->processBlock(b, midi);
  }
  head.sample = 48000 * 20;
  juce::AudioBuffer<float> seek(2, 256);
  seek.clear();
  p->processBlock(seek, midi);
  requireTransition(seek.getMagnitude(0, 256) == 0 && p->idle.load(),
                    "Seek retained stale delayed audio");
  p->setPlayHead(nullptr);
  auto layout = p->getBusesLayout();
  layout.inputBuses.set(0, juce::AudioChannelSet::mono());
  layout.outputBuses.set(0, juce::AudioChannelSet::stereo());
  requireTransition(p->setBusesLayout(layout),
                    "Mono-to-stereo layout rejected");
  p->prepareToPlay(48000, 256);
  juce::AudioBuffer<float> mono(2, 256);
  mono.clear();
  p->processBlock(mono, midi);
  std::cout << "PASS: alternate rates/blocks, silence suspension, "
               "zero/oversized callbacks, seek flush and mono/stereo\n";
}
void renderDemo(const juce::File &directory) {
  directory.createDirectory();
  auto file = directory.getChildFile("M7G-Transitions-V1-Demo.wav");
  file.deleteFile();
  auto stream = file.createOutputStream();
  juce::WavAudioFormat wav;
  std::unique_ptr<juce::AudioFormatWriter> writer(
      wav.createWriterFor(stream.release(), 48000, 2, 16, {}, 0));
  requireTransition(bool(writer), "Demo writer failed");
  juce::String manifest =
      "Synthetic vowel signal, NOT a real singer. Lookahead-rendered FX.\n";
  const int presets[]{0, 5, 8, 51, 52, 58, 100, 107, 109, 122};
  double cursor = 0;
  for (int preset : presets) {
    auto output = renderTransition(preset, 48000, 256, 100, 100, 120, 6.0);
    int from = int(48000 * 1.25), count = int(48000 * 4.6);
    juce::AudioBuffer<float> b(2, count);
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < count; ++i)
        b.setSample(c, i, output.audio[size_t(from + i)] * .8f);
    requireTransition(writer->writeFromAudioSampleBuffer(b, 0, count),
                      "Demo write failed");
    manifest += juce::String(cursor, 2) + "s | " +
                juce::String(transition::bankName(preset / 50)) + " | " +
                transition::recipes()[size_t(preset)].name + "\n";
    cursor += count / 48000.0;
  }
  writer.reset();
  directory.getChildFile("M7G-Transitions-V1-Demo.txt")
      .replaceWithText(manifest);
  std::cout << "PASS: rendered ten-effect synthetic-vowel listening reel\n";
}
} // namespace
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    if (argc == 3 && juce::String(argv[1]) == "--render") {
      renderDemo(juce::File(juce::String::fromUTF8(argv[2])));
      return 0;
    }
    auto start = std::chrono::steady_clock::now();
    dryAndState();
    timing();
    std::set<juce::String> names;
    std::set<uint64_t> fingerprints;
    std::map<uint64_t, int> seen;
    for (int i = 0; i < 150; ++i) {
      auto out = renderTransition(i);
      names.insert(transition::recipes()[size_t(i)].name);
      if (seen.count(out.hash))
        std::cerr << "DUPLICATE AUDIO " << seen[out.hash] << " and " << i
                  << " / " << transition::recipes()[size_t(i)].name << "\n";
      seen[out.hash] = i;
      fingerprints.insert(out.hash);
      if (out.delta < .00001) {
        std::cerr << "Ineffective preset " << i << " "
                  << transition::recipes()[size_t(i)].name << "\n";
        throw std::runtime_error("Factory effect did not change audio");
      }
      requireTransition(out.phrases == 2,
                        "Phrase detector missed/repeated a phrase");
    }
    requireTransition(names.size() == 150 && fingerprints.size() == 150,
                      "Factory names or audio recipes are duplicates");
    std::cout << "PASS: 150 distinct named factory recipes produce 150 "
                 "distinct audible renders\n";
    auto slow = renderTransition(52, 48000, 256, 100, 100, 75);
    auto fast = renderTransition(52, 48000, 256, 100, 100, 150);
    requireTransition(slow.hash != fast.hash,
                      "Host BPM did not change rhythmic effect");
    auto amount = renderTransition(2, 48000, 256, 35, 100);
    auto mix = renderTransition(2, 48000, 256, 100, 35);
    requireTransition(amount.hash != mix.hash,
                      "AMOUNT and MIX have identical functions");
    std::cout
        << "PASS: actual host BPM changes rhythm; AMOUNT differs from MIX\n";
    ratesSeekAndIdle();
    std::cout << "PASS: transition DSP in "
              << std::chrono::duration<double>(
                     std::chrono::steady_clock::now() - start)
                     .count()
              << " seconds\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
