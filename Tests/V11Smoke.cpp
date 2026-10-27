#include "../Source/PluginProcessor.h"
#include <iostream>
#include <stdexcept>
using namespace rack;
namespace {
void check(bool ok, const char *what) {
  if (!ok)
    throw std::runtime_error(what);
}
void render(M7GProcessor &p, int blocks, float amplitude, bool noise = false) {
  juce::AudioBuffer<float> b(2, 256);
  juce::MidiBuffer midi;
  uint32_t rng = 17;
  for (int k = 0; k < blocks; ++k) {
    for (int i = 0; i < 256; ++i) {
      rng = rng * 1664525u + 1013904223u;
      float x =
          amplitude *
          (noise ? (float(rng >> 8) / 8388608.0f - 1)
                 : std::sin(float((k * 256 + i) * 223 *
                                  juce::MathConstants<double>::twoPi / 48000)));
      for (int c = 0; c < 2; ++c)
        b.setSample(c, i, x);
    }
    p.processBlock(b, midi);
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < 256; ++i)
        check(std::isfinite(b.getSample(c, i)) &&
                  std::abs(b.getSample(c, i)) < 3,
              "Non-finite or unbounded audio");
  }
}
} // namespace
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    V11License licence;
    licence.reload();
    check(licence.authorized, "Signed distribution licence not loaded");
    auto file = juce::File::getSpecialLocation(
                    juce::File::commonApplicationDataDirectory)
                    .getChildFile("M7G/M7G.license");
    if (!file.existsAsFile())
      file = V11License::userFile();
    auto doc = file.loadFileAsString();
    check(V11License::validate(doc), "Valid RSA licence rejected");
    check(!V11License::validate(doc.replace("PERPETUAL", "TRIAL")),
          "Licence rights tampering accepted");
    check(!V11License::validate(doc.replace("Makkine", "Hacked")),
          "Licence owner tampering accepted");
    check(!V11License::validate("{}"), "Empty licence accepted");
    std::cout
        << "PASS: RSA signature, perpetual ALL scope and tamper rejection\n";
    V11Pitch pitch;
    for (float cents : {-12.0f, 0.0f, 12.0f}) {
      pitch.reset();
      for (int k = 0; k < 60; ++k)
        check(std::abs(pitch.ratio(220 * std::pow(2.0f, cents / 1200), 220, 1,
                                   .023f, 100, 100, false) -
                       1) < 1e-6f,
              "15 cent dead band changed pitch");
    }
    pitch.reset();
    float r = 1;
    for (int k = 0; k < 80; ++k)
      r = pitch.ratio(220 * std::pow(2.0f, 35.0f / 1200), 220, 1, .023f, 100,
                      100, false);
    check(std::abs(1200 * std::log2(r) + 35) < .2f,
          "Sustained wrong note was not corrected");
    pitch.reset();
    double sum = 0, energy = 0;
    int count = 0;
    for (int k = 0; k < 300; ++k) {
      float cents =
          35 + 22 * std::sin(float(k * .023 * 6 *
                                   juce::MathConstants<double>::twoPi));
      float rr = pitch.ratio(220 * std::pow(2.0f, cents / 1200), 220, 1, .023f,
                             100, 100, false);
      if (k > 100) {
        float corrected = cents + 1200 * std::log2(rr);
        sum += corrected;
        energy += corrected * corrected;
        ++count;
      }
    }
    double mean = sum / count, rms = std::sqrt(energy / count - mean * mean);
    check(std::abs(mean) < 2 && rms > 13,
          "Natural vibrato was flattened or centre was not corrected");
    std::cout << "PASS: 15-cent tolerance; 35-cent correction; vibrato RMS "
              << rms << " cents; centre " << mean << "\n";
    auto p = std::make_unique<M7GProcessor>();
    p->setValue(EngineMode, 0);
    p->setValue(AlienAutomation, 1);
    p->setRateAndBufferSizeDetails(48000, 256);
    p->prepareToPlay(48000, 256);
    render(*p, 250, .03f);
    float initial = p->effectiveValue(InputTrim);
    render(*p, 250, .25f);
    check(p->automationUpdates.load() > 400 &&
              std::abs(initial - p->effectiveValue(InputTrim)) > .01f,
          "AUTO was not active");
    check(p->value(CompRatio) != p->effectiveValue(CompRatio),
          "AUTO wrote host base parameters");
    p->setValue(AlienAutomation, 0);
    render(*p, 1, .12f);
    p->controlTick();
    Values snapshot{};
    for (size_t i = 0; i < Count; ++i)
      snapshot[i] = p->effectiveValue(static_cast<Param>(i));
    render(*p, 400, .007f);
    for (size_t i = 0; i < Count; ++i) {
      if (std::abs(p->effectiveValue(static_cast<Param>(i)) - snapshot[i]) >=
          .11f)
        std::cerr << "Drift: " << specs()[i].id << " " << snapshot[i] << " -> "
                  << p->effectiveValue(static_cast<Param>(i)) << "\n";
      check(std::abs(p->effectiveValue(static_cast<Param>(i)) - snapshot[i]) <
                .11f,
            "MANUAL parameter drifted");
    }
    check(p->value(Bypass) == 0 && p->outputDB.load() > -95,
          "MANUAL became bypass or silence");
    p->setValue(EQGain, -4);
    render(*p, 2, .1f);
    check(p->effectiveValue(EQGain) == -4, "MANUAL edit ignored");
    std::cout << "PASS: live AUTO, internal values, MANUAL latch, no drift, "
                 "manual edit, audio retained\n";
    auto stopped = std::make_unique<M7GProcessor>();
    stopped->setValue(EngineMode, 0);
    stopped->prepareToPlay(48000, 256);
    render(*stopped, 200, .025f);
    float savedTrim = stopped->effectiveValue(InputTrim);
    stopped->setValue(AlienAutomation, 0);
    juce::MemoryBlock instant;
    stopped->getStateInformation(instant);
    auto instantRestore = std::make_unique<M7GProcessor>();
    instantRestore->setStateInformation(instant.getData(),
                                        int(instant.getSize()));
    check(std::abs(instantRestore->value(InputTrim) - savedTrim) < .06f,
          "Immediate stopped-transport state lost effective values");
    stopped->controlTick();
    check(std::abs(stopped->value(InputTrim) - savedTrim) < .06f,
          "Stopped transport did not latch MANUAL");
    std::cout << "PASS: stopped-transport MANUAL capture and immediate session "
                 "save\n";
    p->learnNoise();
    render(*p, 376, .003f, true);
    p->controlTick();
    check(p->noiseLearnState.load() == 0 && p->value(NoiseProfile) > -59 &&
              p->value(NoiseProfile) < -51,
          "Two-second noise profile failed");
    juce::MemoryBlock state;
    p->getStateInformation(state);
    float learned = p->value(NoiseProfile);
    auto restored = std::make_unique<M7GProcessor>();
    restored->setStateInformation(state.getData(), int(state.getSize()));
    check(restored->value(NoiseProfile) == learned && !restored->automatic(),
          "Noise profile / MANUAL state lost");
    std::cout << "PASS: two-second noise learning, measured floor " << learned
              << " dB, session persistence\n";
    p->learnNoise();
    render(*p, 376, 0);
    p->controlTick();
    check(p->noiseLearnState.load() < 0, "Silent noise lesson accepted");
    p->learnNoise();
    render(*p, 376, .015f);
    p->controlTick();
    check(p->noiseLearnState.load() < 0,
          "Voiced noise lesson accepted in Monitor");
    auto idle = std::make_unique<M7GProcessor>();
    idle->setValue(EngineMode, 0);
    idle->setValue(AlienAutomation, 0);
    idle->setValue(ReverbOn, 0);
    idle->setValue(DelayOn, 0);
    idle->prepareToPlay(48000, 256);
    render(*idle, 180, 0);
    check(idle->sleeping.load(), "Silence did not suspend DSP");
    render(*idle, 50, .12f);
    check(!idle->sleeping.load() && idle->outputDB.load() > -80,
          "DSP did not wake on voice");
    auto large = std::make_unique<M7GProcessor>();
    large->setValue(EngineMode, 0);
    large->setValue(AlienAutomation, 0);
    large->prepareToPlay(48000, 256);
    juce::AudioBuffer<float> oversized(2, 65536);
    juce::MidiBuffer emptyMidi;
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < oversized.getNumSamples(); ++i)
        oversized.setSample(
            c, i,
            .08f * std::sin(float(i * 220 * juce::MathConstants<double>::twoPi /
                                  48000)));
    large->processBlock(oversized, emptyMidi);
    check(!large->referencePresent.load() &&
              std::isfinite(oversized.getRMSLevel(0, 0, 65536)) &&
              oversized.getRMSLevel(0, 0, 65536) > .001f,
          "Oversized host block read an unavailable sidechain or lost audio");
    std::cout << "PASS: 65536-sample host block after 256-sample prepare\n";
    std::array<int, 6> banks{};
    for (int i = 0; i < presetCount; ++i)
      ++banks[size_t(workflowBank(i))];
    for (int n : banks)
      check(n > 40, "Empty or thin preset workflow bank");
    std::cout << "PASS: silence suspension and wake; 360 presets across six "
                 "populated banks\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
