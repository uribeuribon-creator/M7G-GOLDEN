#pragma once
#include "RackParameters.h"
namespace v11 {
inline bool isIntent(rack::Param k) {
  using namespace rack;
  const auto &s = specs()[size_t(k)];
  return s.toggle || (s.step == 1 && s.max <= 12) || k == Mix ||
         k == OutputTrim || k == AutoProducer || k == KeepHuman ||
         k == NoiseProfile || k == PreampImpedance;
}
// Bounded proposals; the audio callback smooths them before applying them.
// Musical intent and routing are never guessed from a single isolated voice.
inline void propose(rack::Values &p, const rack::Values &base, float db,
                    float crest, float brightness, float corr,
                    const std::array<float, 8> &nn) {
  using namespace rack;
  float s = base[AutoProducer] * .01f;
  auto set = [&](Param k, float x) {
    p[k] = std::clamp(base[k] + s * (x - base[k]), specs()[size_t(k)].min,
                      specs()[size_t(k)].max);
  };
  float level = db + p[InputTrim];
  float dark = std::clamp(.35f - brightness, -.4f, .4f);
  float profile = base[NoiseProfile];
  set(GateThreshold, profile > -99 ? profile + p[InputTrim] + 8
                                   : std::clamp(level - 29, -65.0f, -35.0f));
  set(NeuralProtect, 80);
  set(GateRange, 22 + nn[3] * 16);
  set(GateAttack, 1.5f + crest * .04f);
  set(GateRelease, 110 + crest * 4);
  set(GateLowcut, 65 + nn[4] * 30);
  set(DeessFreq, 5800 + brightness * 2400);
  set(DeessThreshold, level - 13);
  set(DeessAmount, 28 + brightness * 55);
  set(WarmBias, 35 + nn[0] * 15);
  set(WarmTone, dark * 4);
  set(WarmOutput, -nn[0]);
  set(EQFreq, 2500 + brightness * 1000);
  set(EQGain, 1 + dark * 5);
  set(EQQ, .85f + nn[5] * .7f);
  set(EQDirty, nn[0] * 14);
  set(AirGain, .6f + dark * 3);
  set(AirShimmer, 8 + nn[1] * 15);
  set(AirFilter, 15000 + (1 - brightness) * 3000);
  set(CompRatio, 2.2f + std::clamp(crest, 0.0f, 20.0f) * .12f);
  set(CompGain, 1);
  set(CompRelease, 100 + crest * 5);
  set(LimitCeiling, -1);
  set(LimitRelease, 80 + crest * 3);
  set(TuneSpeed, 8 + base[KeepHuman] * .9f);
  set(DereverbTail, 220 + nn[4] * 430);
  set(TransientProtect, 80);
  set(ResonanceFocus, 1.4f + nn[5] * 2);
  set(HarmonicProtect, 92);
  set(SatCrossLow, 190 + nn[0] * 100);
  set(SatCrossHigh, 3300 + nn[1] * 1400);
  set(FetThreshold, std::clamp(level - 1, -28.0f, -10.0f));
  set(FetRatio, 4 + nn[2] * 3);
  set(FetAttack, .4f + nn[2]);
  set(FetRelease, 70 + crest * 4);
  set(FetGain, .5f);
  set(Width, base[VoiceType] == 0 ? 100 : (corr < 0 ? 85 : 114));
  set(StereoLow, 140 + nn[0] * 40);
  set(Spread, base[VoiceType] == 0 ? 3 : 20);
  set(ModRate, .25f + nn[7] * .6f);
  set(ModDepth, 15 + nn[7] * 25);
  set(ModMix, base[ModMix]);
  set(HarmonyDetune, 5 + base[KeepHuman] * .08f);
  set(HarmonyFormant, 0);
  set(PitchFormant, base[PitchFormant]);
  set(PreampGain, nn[0] * 2);
  set(ReverbSize, 35 + nn[6] * 30);
  set(ReverbPredelay, 20 + crest);
  set(ReverbDecay, 1 + nn[6]);
  set(ReverbShimmer, base[ReverbShimmer]);
  set(DelayMix, base[DelayMix] * (.8f + nn[6] * .3f));
  set(DelayFeedback, 18 + nn[6] * 18);
  set(DelayTone, 4000 + (1 - brightness) * 1800);
  set(DelayDrive, 15 + nn[0] * 15);
  set(ThrowSensitivity, 120 + crest * 4);
  set(VerbAMix, 7 + nn[6] * 4);
  set(VerbAPredelay, 5 + crest * .4f);
  set(VerbADecay, .25f + nn[6] * .1f);
  set(VerbAFilter, 6500 + dark * 1800);
  set(VerbAMod, 8 + nn[6] * 8);
  set(VerbBMix, 5 + nn[6] * 4);
  set(VerbBPredelay, 25 + crest);
  set(VerbBDecay, 1.1f + nn[6] * .8f);
  set(VerbBFilter, 5300 + dark * 1500);
  set(VerbBMod, 12 + nn[6] * 12);
  set(VerbCMix, base[VoiceType] == 4 ? 8 : 3);
  set(VerbCPredelay, 45 + crest);
  set(VerbCDecay, 4.3f + nn[6]);
  set(VerbCFilter, 4200 + dark * 1000);
  set(VerbCMod, 18 + nn[6] * 15);
  set(SculptorTilt, -1.1f - brightness);
  set(SculptorBody, .4f + nn[0]);
  set(SculptorPresence, .8f + dark * 3);
  set(SculptorAir, .5f + dark * 2);
  set(SculptorProtect, 85);
  set(AlignAmount, 88);
  set(AlignRange, 40);
  set(AlignSpeed, 120 + base[KeepHuman]);
  // These blends are intentionally conservative to preserve intelligibility.
  for (auto k : {GateParallel, DeessParallel, AirParallel, LimitParallel,
                 StereoParallel, PitchParallel, RoomParallel, ResonanceParallel,
                 FetParallel, SculptorParallel, AlignParallel})
    set(k, 100);
  set(WarmParallel, 90 + nn[0] * 8);
}
} // namespace v11
