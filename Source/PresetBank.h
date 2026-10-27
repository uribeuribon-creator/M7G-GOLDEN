#pragma once
#include "RackParameters.h"
namespace rack {
struct Preset {
  const char *name;
  int role;
  float drive, air, reduction, ratio, tune, verb, delay, duck, feedback;
};
inline const std::array<Preset, 50> &presets() {
  static const std::array<Preset, 50> bank{
      {{"Commercial Rap Vocal", 0, 66, 48, 48, 4.5f, 78, 8, 6, 78, 24},
       {"Lead - Studio Gold", 0, 60, 42, 43, 4, 70, 9, 5, 75, 22},
       {"Lead - Modern Pop", 0, 42, 60, 40, 3.5f, 72, 12, 7, 72, 26},
       {"Lead - Velvet RnB", 0, 74, 36, 38, 3, 68, 14, 9, 68, 32},
       {"Lead - Trap Focus", 0, 68, 55, 55, 5.5f, 95, 6, 11, 86, 28},
       {"Lead - Reggaeton Front", 0, 62, 50, 52, 5, 85, 7, 8, 82, 24},
       {"Lead - Acoustic Intimate", 0, 32, 28, 26, 2.2f, 42, 10, 3, 60, 15},
       {"Lead - Power Ballad", 0, 58, 52, 39, 3.4f, 60, 17, 8, 68, 30},
       {"Lead - Rock Edge", 0, 85, 30, 54, 5, 35, 8, 5, 80, 20},
       {"Lead - Deep Baritone", 0, 72, 35, 40, 3.5f, 62, 9, 4, 75, 20},
       {"Lead - Silky Soprano", 0, 36, 46, 32, 2.8f, 58, 13, 6, 70, 26},
       {"Lead - Voiceover Clean", 0, 25, 25, 34, 3, 0, 0, 0, 100, 0},
       {"Doubles - Tight Rap", 1, 52, 24, 62, 6, 35, 5, 5, 82, 18},
       {"Doubles - Wide Pop", 1, 42, 36, 49, 4.5f, 40, 11, 10, 72, 26},
       {"Doubles - Dark Glue", 1, 70, 12, 59, 5.5f, 25, 8, 4, 78, 20},
       {"Doubles - Velvet Stereo", 1, 63, 28, 45, 4, 32, 12, 9, 68, 30},
       {"Doubles - Air Layer", 1, 30, 62, 46, 4.2f, 45, 13, 8, 72, 24},
       {"Doubles - Urban Stack", 1, 65, 28, 64, 6.5f, 42, 7, 7, 82, 22},
       {"Doubles - Soft Whisper", 1, 28, 40, 52, 4.8f, 18, 15, 6, 65, 24},
       {"Doubles - Vintage Pair", 1, 82, 16, 50, 4.5f, 25, 10, 8, 70, 28},
       {"Support - Low Anchor", 2, 62, 12, 68, 7, 20, 5, 3, 82, 16},
       {"Support - Rap Emphasis", 2, 70, 25, 65, 6.5f, 25, 6, 6, 86, 20},
       {"Support - Soft Bed", 2, 40, 20, 48, 4, 15, 12, 4, 72, 20},
       {"Support - Warm Body", 2, 84, 18, 56, 5, 22, 8, 5, 78, 22},
       {"Support - Bright Accent", 2, 42, 55, 52, 4.5f, 32, 9, 7, 80, 24},
       {"Support - Rhythmic Glue", 2, 58, 30, 67, 7.5f, 28, 5, 9, 86, 30},
       {"Support - Intimate Texture", 2, 32, 24, 44, 3.5f, 12, 10, 3, 65, 16},
       {"Support - Dense Stack", 2, 75, 22, 72, 8, 20, 9, 6, 82, 24},
       {"Adlibs - Trap Hard", 3, 80, 70, 42, 4, 100, 17, 18, 88, 38},
       {"Adlibs - Air Throw", 3, 58, 82, 35, 3.2f, 90, 22, 24, 90, 50},
       {"Adlibs - Dark Echo", 3, 78, 20, 46, 4.5f, 80, 14, 25, 82, 58},
       {"Adlibs - Wide Orbit", 3, 64, 58, 38, 3.5f, 88, 26, 22, 75, 46},
       {"Adlibs - Vintage Radio", 3, 90, 15, 60, 6, 55, 9, 16, 72, 42},
       {"Adlibs - RnB Answer", 3, 66, 48, 34, 3, 72, 21, 16, 78, 36},
       {"Adlibs - Urban Spark", 3, 72, 72, 44, 4.2f, 95, 15, 20, 86, 40},
       {"Adlibs - Long Tail", 3, 48, 60, 30, 2.8f, 78, 30, 26, 88, 60},
       {"Harmony - Silk Choir", 4, 55, 42, 43, 4, 55, 20, 8, 70, 24},
       {"Harmony - Wide Gospel", 4, 64, 50, 48, 4.5f, 50, 24, 10, 72, 30},
       {"Harmony - Pop Halo", 4, 35, 68, 38, 3.4f, 62, 22, 12, 75, 32},
       {"Harmony - Warm Ensemble", 4, 78, 28, 46, 4.2f, 40, 18, 6, 68, 22},
       {"Harmony - Acoustic Blend", 4, 30, 32, 30, 2.5f, 35, 17, 4, 65, 18},
       {"Harmony - Urban Chords", 4, 62, 42, 53, 5, 58, 16, 9, 78, 28},
       {"Harmony - Angel Air", 4, 25, 82, 35, 3, 65, 28, 10, 72, 35},
       {"Harmony - Dark Velvet", 4, 82, 18, 45, 4, 40, 21, 7, 68, 26},
       {"Bus - Transparent Glue", 5, 20, 20, 28, 2, 0, 3, 0, 80, 0},
       {"Bus - Rap Console", 5, 42, 32, 38, 2.5f, 0, 4, 2, 85, 16},
       {"Bus - Pop Polish", 5, 28, 48, 30, 2.2f, 0, 5, 3, 80, 18},
       {"Bus - Warm Tape", 5, 68, 16, 35, 2.4f, 0, 4, 2, 78, 18},
       {"Bus - Choir Space", 5, 32, 38, 25, 1.8f, 0, 12, 4, 75, 20},
       {"Bus - Dry Master", 5, 18, 18, 24, 1.6f, 0, 0, 0, 100, 0}}};
  return bank;
}
inline Values presetValues(int index) {
  const auto &bank = presets();
  index = juce::jlimit(0, 49, index);
  const auto &p = bank[static_cast<size_t>(index)];
  auto v = defaults();
  v[VoiceType] = float(p.role);
  v[WarmDrive] = p.drive;
  v[WarmBias] = p.drive * .6f;
  v[AirAmount] = p.air;
  v[CompReduction] = p.reduction;
  v[CompRatio] = p.ratio;
  v[TuneAmount] = p.tune;
  v[TuneOn] = p.tune > 0 ? 1.0f : 0.0f;
  v[ReverbMix] = p.verb;
  v[DelayMix] = p.delay;
  v[ReverbDuck] = p.duck;
  v[DelayDuck] = juce::jmin(100.0f, p.duck + 5);
  v[DelayFeedback] = p.feedback;
  v[EQGain] =
      p.role == 0 ? 2.5f : (p.role == 3 ? 3.2f : (p.role == 5 ? .8f : 1.3f));
  v[GateThreshold] = p.role == 0 ? -48.0f : -52.0f;
  v[CompGain] = p.role == 5 ? .5f : 2;
  v[TuneSpeed] = p.role == 3 ? 10.0f : (p.role == 0 ? 40.0f : 65.0f);
  v[ReverbDecay] = p.role == 3 ? 2.6f : (p.role == 4 ? 2.2f : 1.4f);
  v[ReverbSize] = p.role == 3 ? 72.0f : (p.role == 4 ? 66.0f : 48.0f);
  v[DelayNote] = p.role == 3 ? 3.0f : 1.0f;
  v[DelayTone] = p.air < 30 ? 3500.0f : 6500.0f;
  v[Width] = (p.role == 1 || p.role == 4) ? 125.0f : 100.0f;
  v[NoiseOn] = p.role == 5 ? 0.0f : 1.0f;
  v[AirShimmer] = p.role == 3 ? 35.0f : 15.0f;
  v[WarmTone] = (p.air - 45) * .035f;
  v[EQFreq] = p.role == 2 ? 2100.0f : 3200.0f;
  v[Spread] = p.role == 1 ? 25.0f : (p.role == 4 ? 20.0f : 8.0f);
  v[ModMix] =
      (p.role == 1 || p.role == 4) ? 9.0f : (p.role == 3 ? 14.0f : 0.0f);
  v[ModRate] = .35f + float(index % 7) * .13f;
  v[ModDepth] = 15.0f + float(index % 6) * 5;
  v[ReverbOn] = p.verb > 0 ? 1.0f : 0.0f;
  v[DelayOn] = p.delay > 0 ? 1.0f : 0.0f;
  v[ModOn] = v[ModMix] > 0 ? 1.0f : 0.0f;
  if (p.role == 3 && index % 3 == 0)
    v[ModType] = 1;
  if (index == 35)
    v[ModType] = 2;
  if (index == 11) {
    v[ReverbOn] = 0;
    v[DelayOn] = 0;
    v[NoiseAmount] = 80;
  }
  if (index == 32) {
    v[AirFilter] = 10500;
    v[EQGain] = -2;
    v[WarmTone] = -3;
  }
  if (index == 35) {
    v[ReverbDecay] = 4.2f;
    v[DelayNote] = 4;
  }
  if (p.role == 5) {
    v[NoiseOn] = 0;
    v[TuneOn] = 0;
    v[CompRelease] = 220;
    v[GateRange] = 12;
  }
  return v;
}
} // namespace rack
