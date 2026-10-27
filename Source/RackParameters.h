#pragma once
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
namespace rack {
enum Param {
  VoiceType,
  Mix,
  InputTrim,
  OutputTrim,
  AutoGain,
  GateOn,
  GateThreshold,
  GateRange,
  GateAttack,
  GateRelease,
  DeessOn,
  DeessFreq,
  DeessThreshold,
  DeessAmount,
  DeessListen,
  WarmOn,
  WarmDrive,
  WarmBias,
  WarmTone,
  WarmOutput,
  EQOn,
  EQFreq,
  EQGain,
  EQQ,
  AirOn,
  AirAmount,
  AirGain,
  AirShimmer,
  AirFilter,
  CompOn,
  CompReduction,
  CompRatio,
  CompGain,
  CompRelease,
  LimitOn,
  LimitCeiling,
  LimitRelease,
  LimitFast,
  LimitLookahead,
  TuneOn,
  TuneAmount,
  TuneSpeed,
  NoiseOn,
  NoiseAmount,
  ReverbOn,
  ReverbMix,
  ReverbSize,
  ReverbPredelay,
  ReverbDecay,
  ReverbDuck,
  DelayOn,
  DelayMix,
  DelayNote,
  DelayFeedback,
  DelayTone,
  DelayDuck,
  Width,
  StereoOn,
  Pan,
  StereoLow,
  Spread,
  ModOn,
  ModRate,
  ModDepth,
  ModMix,
  ModType,
  PitchKey,
  PitchScale,
  Bypass,
  Solo,
  Count
};
struct Spec {
  const char *id;
  const char *name;
  float min, max, def, step;
  const char *suffix;
  bool toggle = false;
  float skew = 1;
};
inline const std::array<Spec, Count> &specs() {
  static const std::array<Spec, Count> s{
      {{"voicetype", "Voice role", 0, 5, 0, 1, ""},
       {"intensity", "Mix", 0, 100, 100, .1f, " %"},
       {"inputtrim", "Input", -24, 24, 0, .1f, " dB"},
       {"outputtrim", "Output", -24, 12, -1, .1f, " dB"},
       {"autogain", "Auto level", 0, 1, 1, 1, "", true},
       {"gateon", "Gate", 0, 1, 1, 1, "", true},
       {"gatethreshold", "Gate threshold", -80, -15, -48, .1f, " dB"},
       {"gaterange", "Gate range", 0, 48, 24, .1f, " dB"},
       {"gateattack", "Gate attack", .1f, 30, 2, .1f, " ms", false, .4f},
       {"gaterelease", "Gate release", 20, 500, 120, 1, " ms", false, .5f},
       {"deesson", "De-esser", 0, 1, 1, 1, "", true},
       {"deessfreq", "De-ess frequency", 3000, 11000, 6500, 1, " Hz", false,
        .7f},
       {"deessthreshold", "De-ess threshold", -60, -5, -32, .1f, " dB"},
       {"deessamount", "De-ess amount", 0, 100, 60, .1f, " %"},
       {"deesslisten", "De-ess listen", 0, 1, 0, 1, "", true},
       {"warmon", "Tube colour", 0, 1, 1, 1, "", true},
       {"warmdrive", "Tube drive", 0, 100, 60, .1f, " %"},
       {"warmbias", "Tube bias", 0, 100, 40, .1f, " %"},
       {"warmtone", "Tube tone", -6, 6, 0, .1f, " dB"},
       {"warmoutput", "Tube output", -12, 12, 0, .1f, " dB"},
       {"eqon", "Presence EQ", 0, 1, 1, 1, "", true},
       {"eqfreq", "Presence frequency", 700, 6500, 3200, 1, " Hz", false, .55f},
       {"eqgain", "Presence gain", -8, 8, 2.5f, .1f, " dB"},
       {"eqq", "Presence Q", .3f, 4, 1.1f, .01f, "", false, .5f},
       {"airon", "Air", 0, 1, 1, 1, "", true},
       {"airamount", "Air amount", 0, 100, 40, .1f, " %"},
       {"airgain", "Air shine", -3, 6, 1.5f, .1f, " dB"},
       {"airshimmer", "Air harmonics", 0, 100, 20, .1f, " %"},
       {"airfilter", "Air filter", 8000, 20000, 18000, 1, " Hz", false, .7f},
       {"compon", "Optical compressor", 0, 1, 1, 1, "", true},
       {"compreduction", "Peak reduction", 0, 100, 45, .1f, " %"},
       {"compratio", "Compression ratio", 1.2f, 10, 4.5f, .1f, ":1", false,
        .6f},
       {"compgain", "Compressor gain", -12, 12, 2, .1f, " dB"},
       {"comprelease", "Optical recovery", 40, 600, 110, 1, " ms", false, .5f},
       {"limiton", "Limiter", 0, 1, 1, 1, "", true},
       {"limitceiling", "Limiter ceiling", -12, -.1f, -1, .1f, " dB"},
       {"limitrelease", "Limiter release", 20, 400, 85, 1, " ms", false, .5f},
       {"limitfast", "Fast limiting", 0, 1, 1, 1, "", true},
       {"limitlookahead", "Lookahead", 0, 1, 1, 1, "", true},
       {"tuneon", "Automatic tuning", 0, 1, 1, 1, "", true},
       {"tuneamount", "Tuning amount", 0, 100, 75, .1f, " %"},
       {"tunespeed", "Retune speed", 5, 150, 40, 1, " ms", false, .5f},
       {"noiseon", "Neural denoise", 0, 1, 1, 1, "", true},
       {"noiseamount", "Neural blend", 0, 100, 65, .1f, " %"},
       {"reverbon", "Reverb", 0, 1, 1, 1, "", true},
       {"reverbmix", "Reverb send", 0, 40, 9, .1f, " %"},
       {"reverbsize", "Reverb size", 0, 100, 50, .1f, " %"},
       {"reverbpredelay", "Reverb pre-delay", 0, 120, 25, 1, " ms"},
       {"reverbdecay", "Reverb decay", .3f, 5, 1.5f, .1f, " s", false, .6f},
       {"reverbduck", "Reverb ducking", 0, 100, 65, .1f, " %"},
       {"delayon", "Delay", 0, 1, 1, 1, "", true},
       {"delaymix", "Delay send", 0, 40, 6, .1f, " %"},
       {"delaynote", "Delay note", 0, 5, 1, 1, ""},
       {"delayfeedback", "Delay feedback", 0, 85, 24, .1f, " %"},
       {"delaytone", "Delay tone", 1000, 12000, 5500, 1, " Hz", false, .5f},
       {"delayduck", "Delay ducking", 0, 100, 75, .1f, " %"},
       {"width", "Space width", 0, 150, 100, .1f, " %"},
       {"stereoon", "Stereo", 0, 1, 1, 1, "", true},
       {"pan", "Pan", -100, 100, 0, .1f, " %"},
       {"stereolow", "Mono bass", 60, 500, 140, 1, " Hz", false, .6f},
       {"spread", "Stereo spread", 0, 100, 10, .1f, " %"},
       {"modon", "Modulation", 0, 1, 1, 1, "", true},
       {"modrate", "Modulation rate", .05f, 8, .7f, .01f, " Hz", false, .4f},
       {"moddepth", "Modulation depth", 0, 100, 25, .1f, " %"},
       {"modmix", "Modulation mix", 0, 100, 0, .1f, " %"},
       {"modtype", "Modulation type", 0, 2, 0, 1, ""},
       {"pitchkey", "Pitch key", 0, 12, 0, 1, ""},
       {"pitchscale", "Pitch scale", 0, 3, 0, 1, ""},
       {"bypass", "Bypass", 0, 1, 0, 1, "", true},
       {"solo", "Delta listen", 0, 1, 0, 1, "", true}}};
  return s;
}
using Values = std::array<float, Count>;
inline Values defaults() {
  Values v{};
  for (size_t i = 0; i < v.size(); ++i)
    v[i] = specs()[i].def;
  return v;
}
inline juce::AudioProcessorValueTreeState::ParameterLayout layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout p;
  for (size_t i = 0; i < Count; ++i) {
    const auto &s = specs()[i];
    juce::ParameterID id{s.id, 1};
    if (i == VoiceType)
      p.add(std::make_unique<juce::AudioParameterChoice>(
          id, s.name,
          juce::StringArray{"PRINCIPAL", "DOBLES", "REFUERZOS", "ADLIBS",
                            "ARMONIAS", "BUS VOCAL"},
          0));
    else if (i == DelayNote)
      p.add(std::make_unique<juce::AudioParameterChoice>(
          id, s.name,
          juce::StringArray{"1/16", "1/8", "1/8 D", "1/4", "1/4 D", "1/2"}, 1));
    else if (i == ModType)
      p.add(std::make_unique<juce::AudioParameterChoice>(
          id, s.name, juce::StringArray{"Chorus", "Flanger", "Vibrato"}, 0));
    else if (i == PitchKey)
      p.add(std::make_unique<juce::AudioParameterChoice>(
          id, s.name,
          juce::StringArray{"Auto", "C", "C#", "D", "D#", "E", "F", "F#", "G",
                            "G#", "A", "A#", "B"},
          0));
    else if (i == PitchScale)
      p.add(std::make_unique<juce::AudioParameterChoice>(
          id, s.name, juce::StringArray{"Auto", "Chromatic", "Major", "Minor"},
          0));
    else if (s.toggle)
      p.add(
          std::make_unique<juce::AudioParameterBool>(id, s.name, s.def > .5f));
    else
      p.add(std::make_unique<juce::AudioParameterFloat>(
          id, s.name,
          juce::NormalisableRange<float>(s.min, s.max, s.step, s.skew), s.def));
  }
  return p;
}
} // namespace rack
