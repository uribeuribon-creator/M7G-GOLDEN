#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace scratch {
enum Param {
  Mix,
  Intensity,
  Speed,
  Tension,
  Inertia,
  Friction,
  MotorTorque,
  ReleaseTime,
  PitchRange,
  VinylNoise,
  Crackle,
  VinylMode,
  CrossfaderCurve,
  TransformerRate,
  EchoDelay,
  EchoFeedback,
  EchoMix,
  Pattern,
  Variant,
  Depth,
  Duration,
  Rpm,
  LoFi,
  Drive,
  AutoScratch,
  Intelligent,
  Sensitivity,
  Gap,
  Position,
  Touch,
  Crossfader,
  Hold,
  Bypass,
  Output,
  NoiseFloor,
  Reverse,
  AutoFader,
  Cut,
  Brake,
  ReleaseAction,
  FirstCue,
  Count = FirstCue + 8
};
inline constexpr std::array<const char *, FirstCue> ids{"mix",
                                                        "intensity",
                                                        "speed",
                                                        "tension",
                                                        "inertia",
                                                        "friction",
                                                        "motorTorque",
                                                        "releaseTime",
                                                        "pitchRange",
                                                        "vinylNoise",
                                                        "crackle",
                                                        "vinylMode",
                                                        "crossfaderCurve",
                                                        "transformerRate",
                                                        "echoDelay",
                                                        "echoFeedback",
                                                        "echoMix",
                                                        "pattern",
                                                        "variant",
                                                        "depth",
                                                        "duration",
                                                        "rpm",
                                                        "lofi",
                                                        "drive",
                                                        "autoScratch",
                                                        "intelligent",
                                                        "sensitivity",
                                                        "gap",
                                                        "position",
                                                        "touch",
                                                        "crossfader",
                                                        "hold",
                                                        "bypass",
                                                        "output",
                                                        "noiseFloor",
                                                        "reverse",
                                                        "autoFader",
                                                        "cut",
                                                        "brake",
                                                        "releaseAction"};
using Values = std::array<float, Count>;
inline Values defaults() {
  Values p{};
  p[Mix] = 100;
  p[Intensity] = 100;
  p[Speed] = 1;
  p[Tension] = 65;
  p[Inertia] = .91f;
  p[Friction] = .0012f;
  p[MotorTorque] = .65f;
  p[ReleaseTime] = 220;
  p[PitchRange] = 90;
  p[VinylNoise] = 10;
  p[Crackle] = 8;
  p[VinylMode] = 1;
  p[TransformerRate] = 9;
  p[EchoDelay] = 190;
  p[EchoFeedback] = .36f;
  p[EchoMix] = 0;
  p[Depth] = 75;
  p[Duration] = 650;
  p[Rpm] = 0;
  p[Drive] = 10;
  p[AutoScratch] = 1;
  p[Intelligent] = 1;
  p[Sensitivity] = 65;
  p[Gap] = 95;
  p[Position] = .5f;
  p[Crossfader] = 1;
  p[NoiseFloor] = -70;
  p[AutoFader] = 1;
  for (int i = 0; i < 8; ++i)
    p[size_t(FirstCue + i)] = i / 8.f;
  return p;
}
inline constexpr std::array<const char *, 10> categories{
    "CHIRPS",
    "TRANSFORMERS",
    "FLARES & CRABS",
    "WORD CHOPS",
    "BABY SCRATCHES",
    "TEAR & ORBIT",
    "SLOW DRAGS & RELEASES",
    "LO-FI & VINTAGE",
    "EXTREME & FX",
    "MAKKINE SIGNATURE"};
} // namespace scratch
