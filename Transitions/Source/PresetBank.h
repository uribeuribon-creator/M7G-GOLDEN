#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace transition {
enum class Bank { Intro, Bridge, Outro };
enum class Family {
  ReverseVerb,
  Whisper,
  Pitch,
  Granular,
  Tape,
  Stutter,
  Echo,
  Filter,
  Formant,
  Crush,
  Reverse,
  Stretch,
  Vinyl,
  Shimmer,
  Radio,
  PingPong,
  Pan,
  Chorus,
  Octave,
  Freeze,
  PitchLadder,
  Breath,
  LoFi,
  Telephone,
  Hall,
  Slap,
  Flanger,
  Phaser,
  Distortion,
  Hiss,
  Robot,
  Fifth,
  Repeat,
  HalfSpeed,
  Chops,
  Scratch,
  Riser,
  PitchRecover,
  Width,
  Alien,
  Saturation,
  Air,
  Dark,
  Spiral,
  Tremolo,
  Plate,
  Room,
  Warble
};
struct Recipe {
  const char *name = "";
  Bank bank = Bank::Intro;
  Family family = Family::Echo;
  float speed0 = 1, speed1 = 1, formant = 1, lowCut = 60, highCut = 15000,
        drive = 1.8f, feedback = .45f, beats = .5f, grain = .065f,
        modRate = .7f, depth = .5f, decay = 1.8f, spread = .6f;
  int repeats = 2;
  bool hold = false, deess = false, chopped = false, randomPitch = false,
       stack = false, reverseEcho = false;
  int panMode = 0;
};
inline constexpr int presetCount = 150;
inline const char *bankName(int i) {
  static const char *n[]{"INTRO - ENTRADA", "CAMBIO - PUENTE",
                         "SALIDA - FINAL"};
  return n[std::clamp(i, 0, 2)];
}
inline const std::array<Recipe, presetCount> &recipes() {
  static const auto all = [] {
    std::array<Recipe, presetCount> result{};
    static const char *names[3][50] = {
        {"Reverse Reverb Gold",
         "Whisper to Full",
         "Pitched Chipmunk Fall",
         "Granular Cloud In",
         "Tape Start Fast",
         "Stutter 1/16 Build Up",
         "Echo Throw Intro",
         "Dark Filter Open",
         "Formant Shift Up",
         "Bitcrush to Clean",
         "Reverse Word",
         "Double Repeat Intro",
         "Time Stretch Short to Normal",
         "Vinyl Stop Reverse",
         "Shimmer Intro",
         "Distorted Radio to Clean",
         "Ping Pong Intro",
         "Auto-Pan Intro",
         "Chorus Spread Intro",
         "Octave Up Stack",
         "Octave Down Rumble",
         "Granular Freeze to Voice",
         "Reverse Echo",
         "Pitch Ladder Up (3 notes)",
         "Breathing Intro",
         "Lo-Fi to Hi-Fi",
         "Telephone to Studio",
         "Hall Reverb Intro",
         "Slap Delay Intro",
         "Flanger Intro",
         "Phaser Intro",
         "Distortion Fade In",
         "Tape Hiss Intro",
         "Robot to Human",
         "Harmonized 5th Up Intro",
         "Triple Repeat Intro",
         "Half Speed to Normal",
         "Reverse Half Speed",
         "Granular Stutter Intro",
         "Echo 1/8 Dot Intro",
         "Deep Reverb Swell",
         "Bright Air Open",
         "Saturation Warm Intro",
         "DeEssed Whisper",
         "Vocal Chops Intro",
         "Scratch Intro",
         "Riser from Vocal",
         "Pitch Drop Recover",
         "Stereo Wide Intro",
         "Alien Artifact Intro"},
        {"Half-Speed Bridge",
         "Tape Stop Bridge",
         "Stutter Bridge 1/8",
         "Reverse Word Bridge",
         "Echo Infinity Bridge",
         "Pitch Dive Bridge",
         "Time Stretch Freeze Bridge",
         "Filter Sweep Bridge",
         "Granular Gap Filler",
         "Reverb Throw Bridge",
         "Delay Ping Pong Bridge",
         "Bitcrush Bridge",
         "Formant Down Bridge",
         "Chop 4x Bridge",
         "Scratch Bridge",
         "Riser from Last Word",
         "Downward Spiral Bridge",
         "Upward Spiral Bridge",
         "Tremolo Bridge",
         "AutoPan Bridge",
         "Chorus Bridge",
         "Distortion Bridge",
         "Vinyl Brake Bridge",
         "Double Speed Bridge",
         "Reverse Reverb Bridge",
         "Shimmer Bridge",
         "Octave Stack Bridge",
         "Whisper Echo Bridge",
         "Breath Bridge",
         "Robot Bridge",
         "Telephone Bridge",
         "Hall Bridge",
         "Room Bridge",
         "Plate Bridge",
         "Flanger Bridge",
         "Phaser Bridge",
         "Lo-Fi Bridge",
         "Tape Warble Bridge",
         "Pitch Random Bridge",
         "Stutter 1/16T Bridge",
         "Echo 1/4 Bridge",
         "Freeze Bridge",
         "Granular Cloud Bridge",
         "Saturation Bridge",
         "Air Bridge",
         "Dark to Bright Bridge",
         "Pan Left to Right Bridge",
         "Triple Repeat Bridge",
         "Vocal Chopper Bridge",
         "Alien Transition Bridge"},
        {"Tape Stop Classic",
         "Echo Throw Infinite",
         "Stutter Repeat 1/16",
         "Pitch Drop Down",
         "Time Stretch Freeze Tail",
         "Reverse Tail",
         "Granular Dissolve",
         "Reverb Shimmer Tail",
         "Delay 1/8 Dot Tail",
         "Ping Pong Tail",
         "Half-Speed Tail",
         "Bitcrush Tail",
         "Filter Close Tail",
         "Auto-Pan Tail",
         "Flanger Tail",
         "Phaser Tail",
         "Distortion Tail",
         "Vinyl Stop Tail",
         "Chop 8x Tail",
         "Scratch Tail",
         "Octave Down Tail",
         "Octave Up Tail",
         "Formant Down Tail",
         "Whisper Tail",
         "Breath Tail",
         "Robot Tail",
         "Telephone Tail",
         "Hall Tail",
         "Room Tail",
         "Plate Tail",
         "Lo-Fi Tail",
         "Tape Warble Tail",
         "Pitch Random Tail",
         "Stutter 1/32 Tail",
         "Echo 1/4 Tail",
         "Freeze Tail",
         "Granular Cloud Tail",
         "Saturation Tail",
         "Air Tail",
         "Dark Tail",
         "Pan Left Tail",
         "Triple Repeat Tail",
         "Vocal Chopper Tail",
         "Downward Spiral Tail",
         "Upward Spiral Tail",
         "Tremolo Tail",
         "Chorus Tail",
         "Shimmer Tail",
         "Alien Dissolve Tail",
         "Infinite Repeat Tail"},
    };
    static constexpr Family families[3][50] = {
        {Family::ReverseVerb, Family::Whisper,     Family::Pitch,
         Family::Granular,    Family::Tape,        Family::Stutter,
         Family::Echo,        Family::Filter,      Family::Formant,
         Family::Crush,       Family::Reverse,     Family::Repeat,
         Family::Stretch,     Family::Vinyl,       Family::Shimmer,
         Family::Radio,       Family::PingPong,    Family::Pan,
         Family::Chorus,      Family::Octave,      Family::Octave,
         Family::Freeze,      Family::ReverseVerb, Family::PitchLadder,
         Family::Breath,      Family::LoFi,        Family::Telephone,
         Family::Hall,        Family::Slap,        Family::Flanger,
         Family::Phaser,      Family::Distortion,  Family::Hiss,
         Family::Robot,       Family::Fifth,       Family::Repeat,
         Family::HalfSpeed,   Family::Reverse,     Family::Granular,
         Family::Echo,        Family::Hall,        Family::Air,
         Family::Saturation,  Family::Whisper,     Family::Chops,
         Family::Scratch,     Family::Riser,       Family::PitchRecover,
         Family::Width,       Family::Alien},
        {Family::HalfSpeed,   Family::Tape,       Family::Stutter,
         Family::Reverse,     Family::Echo,       Family::Pitch,
         Family::Stretch,     Family::Filter,     Family::Granular,
         Family::Hall,        Family::PingPong,   Family::Crush,
         Family::Formant,     Family::Chops,      Family::Scratch,
         Family::Riser,       Family::Spiral,     Family::Spiral,
         Family::Tremolo,     Family::Pan,        Family::Chorus,
         Family::Distortion,  Family::Vinyl,      Family::Tape,
         Family::ReverseVerb, Family::Shimmer,    Family::Octave,
         Family::Whisper,     Family::Breath,     Family::Robot,
         Family::Telephone,   Family::Hall,       Family::Room,
         Family::Plate,       Family::Flanger,    Family::Phaser,
         Family::LoFi,        Family::Warble,     Family::Pitch,
         Family::Stutter,     Family::Echo,       Family::Freeze,
         Family::Granular,    Family::Saturation, Family::Air,
         Family::Filter,      Family::Pan,        Family::Repeat,
         Family::Chops,       Family::Alien},
        {Family::Tape,     Family::Echo,       Family::Stutter,
         Family::Pitch,    Family::Stretch,    Family::Reverse,
         Family::Granular, Family::Shimmer,    Family::Echo,
         Family::PingPong, Family::HalfSpeed,  Family::Crush,
         Family::Filter,   Family::Pan,        Family::Flanger,
         Family::Phaser,   Family::Distortion, Family::Vinyl,
         Family::Chops,    Family::Scratch,    Family::Octave,
         Family::Octave,   Family::Formant,    Family::Whisper,
         Family::Breath,   Family::Robot,      Family::Telephone,
         Family::Hall,     Family::Room,       Family::Plate,
         Family::LoFi,     Family::Warble,     Family::Pitch,
         Family::Stutter,  Family::Echo,       Family::Freeze,
         Family::Granular, Family::Saturation, Family::Air,
         Family::Dark,     Family::Pan,        Family::Repeat,
         Family::Chops,    Family::Spiral,     Family::Spiral,
         Family::Tremolo,  Family::Chorus,     Family::Shimmer,
         Family::Alien,    Family::Repeat},
    };
    for (int bank = 0; bank < 3; ++bank)
      for (int i = 0; i < 50; ++i) {
        auto &r = result[size_t(bank * 50 + i)];
        r.name = names[bank][i];
        r.bank = Bank(bank);
        r.family = families[bank][i];
        r.highCut = 11000 + float(i % 7) * 700;
        r.lowCut = 55 + float(i % 4) * 8;
        r.grain = .045f + float(i % 6) * .009f;
        r.modRate = .35f + float(i % 5) * .23f;
        r.spread = .4f + float(i % 5) * .1f;
        r.drive = 1.5f + float(i % 5) * .35f;
        r.feedback = .35f + float(i % 5) * .07f;
        using F = Family;
        switch (r.family) {
        case F::Pitch:
          r.speed0 = bank == 0 ? 2.0f : 1;
          r.speed1 = bank == 0 ? 1.0f : .35f;
          break;
        case F::Tape:
          r.speed0 = bank == 0 ? .08f : 1;
          r.speed1 = bank == 0 ? 1 : .02f;
          break;
        case F::Vinyl:
          r.speed0 = bank == 0 ? -1 : 1;
          r.speed1 = bank == 0 ? -.2f : 0;
          r.highCut = 6500;
          break;
        case F::HalfSpeed:
          r.speed0 = .5f;
          r.speed1 = bank == 0 ? 1.0f : .5f;
          break;
        case F::Reverse:
          r.speed0 = r.speed1 = bank == 0 && i == 37 ? -.5f : -1;
          break;
        case F::Formant:
          r.formant = bank == 0 ? 1.4f : .7f;
          break;
        case F::Octave:
          r.speed0 = r.speed1 =
              (bank == 0 && i == 20) || (bank == 2 && i == 20) ? .5f : 2;
          break;
        case F::Fifth:
          r.speed0 = r.speed1 = 1.498307f;
          break;
        case F::Stretch:
          r.speed0 = bank == 0 ? .35f : .12f;
          r.speed1 = bank == 0 ? 1 : .04f;
          break;
        case F::Spiral:
          r.speed0 = 1;
          r.speed1 = (i == 17 || i == 44) ? 2.5f : .2f;
          break;
        case F::Stutter:
          r.beats = (bank == 1 && i == 39)   ? 1.0f / 6
                    : (bank == 2 && i == 33) ? .125f
                    : bank == 1              ? .5f
                                             : .25f;
          break;
        case F::Echo:
          r.beats = (bank == 0 && i == 39) || (bank == 2 && i == 8)    ? .75f
                    : (bank == 1 && i == 40) || (bank == 2 && i == 34) ? 1
                                                                       : .5f;
          break;
        case F::Hall:
          r.decay = bank == 0 && i == 40 ? 4.0f : 2.4f;
          break;
        case F::Room:
          r.decay = .4f;
          break;
        case F::Plate:
          r.decay = 1.6f;
          break;
        case F::Shimmer:
          r.decay = bank == 2 && i == 47 ? 5.5f : 3.2f;
          break;
        case F::Radio:
        case F::Telephone:
          r.lowCut = 440;
          r.highCut = 3200;
          break;
        case F::Whisper:
        case F::Breath:
          r.lowCut = 2200;
          r.highCut = 10500;
          break;
        case F::Dark:
          r.highCut = 1200;
          break;
        case F::Slap:
          r.beats = .115f;
          break;
        default:
          break;
        }
        if (r.family == F::Repeat)
          r.repeats = ((bank == 0 && i == 35) || (bank == 1 && i == 47) ||
                       (bank == 2 && i == 41))
                          ? 3
                          : 2;
        if (r.family == F::Chops)
          r.repeats = bank == 2 && i == 18 ? 8 : 4;
        if (r.family == F::Freeze || (bank == 1 && i == 4) ||
            (bank == 2 && (i == 1 || i == 49)))
          r.hold = true;
        if (bank == 0 && i == 22) {
          r.reverseEcho = true;
          r.beats = .5f;
        }
        if (bank == 0 && i == 43) {
          r.deess = true;
          r.highCut = 7500;
        }
        if (bank == 0 && i == 38) {
          r.chopped = true;
          r.grain = .025f;
          r.beats = .25f;
        }
        if ((bank == 1 && i == 38) || (bank == 2 && i == 32))
          r.randomPitch = true;
        if ((bank == 0 && i == 19) || (bank == 1 && i == 26))
          r.stack = true;
        if (bank == 0 && i == 20)
          r.highCut = 900;
        if (bank == 1 && i == 46)
          r.panMode = 1;
        if (bank == 2 && i == 40)
          r.panMode = 2;
        if (bank == 1 && i == 23)
          r.speed0 = r.speed1 = 2;
        if (bank == 1 && i == 38) {
          r.speed0 = .75f;
          r.speed1 = 1.5f;
        }
        if (bank == 2 && i == 32) {
          r.speed0 = .65f;
          r.speed1 = 1.4f;
        }
      }
    return result;
  }();
  return all;
}
inline float durationMs(int bank, float amount) {
  return (bank == 0 ? 200.0f : 400.0f) + (bank == 0 ? 1300.0f : 1100.0f) *
                                             std::clamp(amount, 0.0f, 100.0f) *
                                             .01f;
}
} // namespace transition
