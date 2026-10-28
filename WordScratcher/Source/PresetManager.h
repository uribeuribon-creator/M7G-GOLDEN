#pragma once
#include "Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>
namespace scratch {
struct ScratchPreset {
  juce::String name, category;
  float inertia = 0, friction = 0, motorTorque = 0, releaseTime = 0;
  float pitchRange = 0, vinylNoise = 0, crackle = 0, tension = 0;
  bool vinylMode = true;
  int crossfaderCurve = 0;
  float hotCuePositions[8]{};
  float transformerRate = 0, echoDelay = 0, echoFeedback = 0;
  juce::String description;
  Values values{};
};
class PresetManager {
public:
  PresetManager() {
    presets.reserve(300);
    for (int category = 0; category < 10; ++category)
      for (int index = 0; index < 30; ++index) {
        ScratchPreset q;
        q.name = juce::String::fromUTF8(names[size_t(category)][size_t(index)]);
        q.category = categories[size_t(category)];
        q.values = defaults();
        auto &p = q.values;
        p[Pattern] = float(category);
        p[Variant] = float(index);
        p[Inertia] = .85f + .0042f * ((index * 7 + category * 3) % 30);
        p[Friction] = .00045f + .000075f * ((index * 11 + category * 5) % 30);
        p[MotorTorque] = .3f + .025f * ((index * 3 + category) % 30);
        p[ReleaseTime] = 110.f + float((index * 19 + category * 37) % 500);
        p[PitchRange] = 70.f + float(index % 11) * 2.7f;
        p[Tension] = 45.f + (index * 7 + category * 11) % 52;
        p[Speed] = .75f + .035f * (index % 13);
        p[TransformerRate] = 4.f + float((index * 3 + category) % 18);
        p[Depth] = 55.f + float((index * 5 + category * 7) % 40);
        p[Duration] = 470.f + float((index * 37 + category * 53) % 700);
        p[VinylNoise] = category == 7 ? 32.f + index : 5.f + index % 9;
        p[Crackle] = category == 7 ? 25.f + index : 3.f + index % 11;
        p[Drive] = 8.f + (index + category) % 18;
        p[EchoDelay] = 105.f + float((index * 31 + category * 43) % 350);
        p[EchoFeedback] = .22f + .012f * (index % 25);
        p[LoFi] = category == 7 ? 40.f + (index % 11) * 4.f : 0.f;
        for (int cue = 0; cue < 8; ++cue)
          p[size_t(FirstCue + cue)] = std::clamp(
              (cue + .1f * std::sin(float(index + cue))) / 8.f, 0.f, 1.f);
        auto has = [&](const char *s) { return q.name.containsIgnoreCase(s); };
        if (has("Fast") || has("Agite"))
          p[Speed] = 1.55f + .012f * index;
        if (has("Slow"))
          p[Speed] = .48f + .006f * index;
        if (has("Aggressive") || has("Heavy") || has("Tropel")) {
          p[Tension] = 92;
          p[Friction] *= 1.35f;
          p[Drive] = 28;
          p[Depth] = 94;
        }
        if (has("Soft") || has("Light")) {
          p[Tension] = 34;
          p[Inertia] = .975f;
          p[Depth] = 46;
          p[Drive] = 4;
        }
        if (has("Reverse"))
          p[Reverse] = 1;
        if (has("Intro"))
          p[FirstCue] = .03f;
        if (has("Outro"))
          p[FirstCue] = .65f;
        if (has("Middle"))
          p[FirstCue] = .35f;
        if (has("Digital")) {
          p[VinylMode] = 0;
          p[VinylNoise] = p[Crackle] = 0;
        }
        if (has("Clean") || has("HiFi")) {
          p[VinylNoise] = p[Crackle] = p[LoFi] = 0;
          p[Drive] = 2;
        }
        if (has("LoFi") || has("Radio") || has("Phone"))
          p[LoFi] = has("Phone") ? 97.f : has("Radio") ? 84.f : 68.f;
        if (has("Dusty")) {
          p[VinylNoise] = 52;
          p[Crackle] = 62;
        }
        if (has("Crackle"))
          p[Crackle] = has("Min") ? 8.f : 82.f;
        if (has("Vinyl")) {
          p[VinylMode] = 1;
          p[VinylNoise] = 35;
          p[Crackle] = 30;
        }
        if (has("Echo")) {
          p[EchoMix] = 62;
          p[EchoFeedback] = has("Tail") ? .67f : .48f;
          p[Duration] += 180;
        }
        if (has("Long")) {
          p[Duration] = 1450;
          p[ReleaseTime] = 720;
        }
        if (has("Short") || has("Stab"))
          p[Duration] = 310;
        if (has("33"))
          p[Rpm] = 0;
        if (has("45"))
          p[Rpm] = 1;
        if (has("Tension Max") || has("Heavy Tension"))
          p[Tension] = 100;
        if (has("Tension Min") || has("Light Tension"))
          p[Tension] = 18;
        if (has("Motor Off")) {
          p[MotorTorque] = 0;
          p[ReleaseTime] = 420;
        }
        if (has("Motor On"))
          p[MotorTorque] = 1.3f;
        if (has("50%"))
          p[PitchRange] = 50;
        if (has("100%"))
          p[PitchRange] = 100;
        if (has("Hamster"))
          p[CrossfaderCurve] = 2;
        else if (has("Soft"))
          p[CrossfaderCurve] = 1;
        if (has("2-Click"))
          p[TransformerRate] = 8;
        if (has("3-Click"))
          p[TransformerRate] = 12;
        if (has("4-Click") || has("Machine Gun"))
          p[TransformerRate] = 22;
        if (has("Transformer") && category != 1)
          p[TransformerRate] += 7;
        if (has("Tape")) {
          p[LoFi] = 38;
          p[Drive] = 24;
        }
        if (has("Endless")) {
          p[Hold] = 1;
          p[Duration] = 2000;
        }
        if (has("Brake")) {
          p[Brake] = 1;
          p[MotorTorque] = 0;
          p[Inertia] = .96f;
          p[Duration] = 1350;
          p[ReleaseTime] = 630;
        }
        if (has("Release")) {
          p[ReleaseAction] = 1;
          p[MotorTorque] = 1.15f;
          p[ReleaseTime] = has("Long") || has("Largo") ? 920.f : 480.f;
        }
        // Signature categories still select actual chirp/crab/tear gestures by
        // name.
        if (category == 9) {
          p[Pattern] = float(index % 9);
          p[Drive] += 12;
          p[EchoMix] = index % 4 == 0 ? 34.f : 0.f;
        }
        if (category == 7) {
          if (has("Baby"))
            p[Pattern] = 4;
          if (has("Tear"))
            p[Pattern] = 5;
          if (has("Orbit"))
            p[Pattern] = 5;
          if (has("Word Chop"))
            p[Pattern] = 3;
          if (has("Slow Drag"))
            p[Pattern] = 6;
        }
        if (category >= 6 && has("Transformer"))
          p[Pattern] = 1;
        if (category >= 6 && has("Chirp"))
          p[Pattern] = 0;
        if (category >= 6 && (has("Flare") || has("Crab")))
          p[Pattern] = 2;
        if (category == 8 && has("Baby"))
          p[Pattern] = 4;
        if (category == 8 && (has("Tear") || has("Orbit")))
          p[Pattern] = 5;
        p[Crossfader] = p[CrossfaderCurve] == 2 ? 0.f : 1.f;
        q.inertia = p[Inertia];
        q.friction = p[Friction];
        q.motorTorque = p[MotorTorque];
        q.releaseTime = p[ReleaseTime];
        q.pitchRange = p[PitchRange];
        q.vinylNoise = p[VinylNoise];
        q.crackle = p[Crackle];
        q.tension = p[Tension];
        q.vinylMode = p[VinylMode] > .5f;
        q.crossfaderCurve = int(p[CrossfaderCurve]);
        q.transformerRate = p[TransformerRate];
        q.echoDelay = p[EchoDelay];
        q.echoFeedback = p[EchoFeedback];
        for (int cue = 0; cue < 8; ++cue)
          q.hotCuePositions[cue] = p[size_t(FirstCue + cue)];
        q.description = "Audio capturado real / " + q.category + " / " +
                        (q.vinylMode ? "tono de vinilo"
                                     : "tono conservado con Rubber Band") +
                        " / Makkine Seven Golden";
        presets.push_back(std::move(q));
      }
  }
  std::vector<ScratchPreset> presets;

private:
  inline static const std::array<std::array<const char *, 30>, 10> names{{
      {{"Chirp Basic",         "Chirp Fast",
        "Chirp Slow Drag",     "Chirp Double",
        "Chirp Triple",        "Chirp Baby",
        "Chirp Long",          "Chirp Short Stab",
        "Chirp Echo",          "Chirp Reverse",
        "Chirp Paisa Slow",    "Chirp Medallo Fast",
        "Chirp Aggressive",    "Chirp Soft",
        "Chirp Vinyl Heavy",   "Chirp Digital Clean",
        "Chirp 33RPM",         "Chirp 45RPM",
        "Chirp LoFi",          "Chirp HiFi",
        "Chirp Tape",          "Chirp Dusty",
        "Chirp Heavy Tension", "Chirp Light Tension",
        "Chirp Cut",           "Chirp Transformer Blend",
        "Chirp Echo Tail",     "Chirp Brake",
        "Chirp Release Long",  "Chirp Motor Off"}},
      {{"Transformer Basic",
        "Transformer Fast Cut",
        "Transformer Slow",
        "Transformer Machine Gun",
        "Transformer 2-Click",
        "Transformer 3-Click",
        "Transformer 4-Click",
        "Transformer Paisa",
        "Transformer Street",
        "Transformer Club",
        "Transformer Radio",
        "Transformer LoFi",
        "Transformer Hi Cut",
        "Transformer Crab Blend",
        "Transformer Echo",
        "Transformer Reverse",
        "Transformer Baby Transformer",
        "Transformer Long Hold",
        "Transformer Short Stab",
        "Transformer Heavy",
        "Transformer Light",
        "Transformer Crackle",
        "Transformer Clean",
        "Transformer Vinyl",
        "Transformer Digital",
        "Transformer 50% Pitch",
        "Transformer 100% Pitch",
        "Transformer Hamster",
        "Transformer Brake",
        "Transformer Release"}},
      {{"Flare 1-Click",    "Flare 2-Click",
        "Flare Orbit 1",    "Flare Orbit 2",
        "Flare Autobahn",   "Crab Basic",
        "Crab Fast",        "Crab Slow",
        "Crab Paisa Style", "Crab Medallo",
        "Crab Street",      "Crab Aggressive",
        "Crab Soft",        "Crab Echo",
        "Crab Reverse",     "Crab Vinyl",
        "Crab Digital",     "Crab Heavy",
        "Crab Light",       "Crab Tension Max",
        "Crab Tension Min", "Crab 33",
        "Crab 45",          "Crab LoFi",
        "Crab HiFi",        "Crab Dusty",
        "Crab Clean",       "Crab Transformer Combo",
        "Crab Chirp Combo", "Crab Endless"}},
      {{"Word Chop Intro",       "Word Chop Outro",
        "Word Chop Middle",      "Word Chop Repeat",
        "Word Chop Stutter",     "Word Chop Echo",
        "Word Chop Reverse",     "Word Chop Paisa Accent",
        "Word Chop Fast",        "Word Chop Slow",
        "Word Chop Heavy",       "Word Chop Light",
        "Word Chop Vinyl",       "Word Chop Digital",
        "Word Chop Brake",       "Word Chop Release",
        "Word Chop Transformer", "Word Chop Chirp",
        "Word Chop Flare",       "Word Chop Crab",
        "Word Chop LoFi",        "Word Chop HiFi",
        "Word Chop Dusty",       "Word Chop Clean",
        "Word Chop 33",          "Word Chop 45",
        "Word Chop Motor Off",   "Word Chop Tension",
        "Word Chop Crackle",     "Word Chop Endless Loop"}},
      {{"Baby Scratch Basic",
        "Baby Slow",
        "Baby Fast",
        "Baby Paisa",
        "Baby Street",
        "Baby Soft",
        "Baby Aggressive",
        "Baby Echo",
        "Baby Reverse",
        "Baby Vinyl",
        "Baby Digital",
        "Baby LoFi",
        "Baby HiFi",
        "Baby Dusty",
        "Baby Clean",
        "Baby 33",
        "Baby 45",
        "Baby Heavy",
        "Baby Light",
        "Baby Tension",
        "Baby Crackle",
        "Baby Brake",
        "Baby Release",
        "Baby Transformer",
        "Baby Chirp",
        "Baby Flare",
        "Baby Crab",
        "Baby Motor Off",
        "Baby Endless",
        "Baby Word Hold"}},
      {{"Tear Basic",   "Tear Slow",       "Tear Fast",    "Tear Paisa",
        "Tear Street",  "Tear Aggressive", "Tear Soft",    "Tear Echo",
        "Tear Reverse", "Tear Vinyl",      "Tear Digital", "Tear LoFi",
        "Tear HiFi",    "Tear Dusty",      "Tear Clean",   "Tear 33",
        "Tear 45",      "Tear Heavy",      "Tear Light",   "Tear Tension",
        "Tear Crackle", "Tear Brake",      "Tear Release", "Tear Transformer",
        "Tear Chirp",   "Tear Flare",      "Tear Crab",    "Tear Motor Off",
        "Tear Endless", "Tear Word Hold"}},
      {{"Slow Drag Basic",        "Slow Drag Paisa",
        "Slow Drag Street",       "Slow Drag Heavy",
        "Slow Drag Light",        "Slow Drag Vinyl",
        "Slow Drag Digital",      "Slow Drag LoFi",
        "Slow Drag HiFi",         "Slow Drag Dusty",
        "Slow Drag Clean",        "Slow Drag 33",
        "Slow Drag 45",           "Slow Drag Echo",
        "Slow Drag Reverse",      "Slow Drag Brake Long",
        "Slow Drag Release Long", "Slow Drag Transformer",
        "Slow Drag Chirp",        "Slow Drag Flare",
        "Slow Drag Crab",         "Slow Drag Tension Max",
        "Slow Drag Tension Min",  "Slow Drag Crackle Max",
        "Slow Drag Crackle Min",  "Slow Drag Motor Off",
        "Slow Drag Motor On",     "Slow Drag Echo Tail",
        "Slow Drag Endless",      "Slow Drag Word Hold"}},
      {{"LoFi Dusty Vinyl", "LoFi Tape Hiss", "LoFi Crackle Max",
        "LoFi Radio",       "LoFi Phone",     "LoFi Street",
        "LoFi Paisa",       "LoFi Heavy",     "LoFi Light",
        "LoFi Slow",        "LoFi Fast",      "LoFi Echo",
        "LoFi Reverse",     "LoFi Brake",     "LoFi Release",
        "LoFi Transformer", "LoFi Chirp",     "LoFi Flare",
        "LoFi Crab",        "LoFi Baby",      "LoFi Tear",
        "LoFi Orbit",       "LoFi Word Chop", "LoFi Slow Drag",
        "LoFi 33",          "LoFi 45",        "LoFi Motor Off",
        "LoFi Tension",     "LoFi Pitch 50%", "LoFi Pitch 100%"}},
      {{"Extreme Fast",        "Extreme Slow",        "Extreme Pitch Up",
        "Extreme Pitch Down",  "Extreme Reverse",     "Extreme Brake",
        "Extreme Release",     "Extreme Transformer", "Extreme Chirp",
        "Extreme Flare",       "Extreme Crab",        "Extreme Echo",
        "Extreme Dusty",       "Extreme Clean",       "Extreme Vinyl",
        "Extreme Digital",     "Extreme 33",          "Extreme 45",
        "Extreme Heavy",       "Extreme Light",       "Extreme Tension",
        "Extreme Crackle",     "Extreme Motor Off",   "Extreme Motor On",
        "Extreme Word Repeat", "Extreme Stutter",     "Extreme Orbit",
        "Extreme Tear",        "Extreme Baby",        "Extreme Endless"}},
      {{"Makkine Chop",
        "Makkine Slow Paisa",
        "Makkine Street",
        "Makkine Medallo",
        "Makkine Agite",
        "Makkine Tropel",
        "Makkine Soponcio",
        "Makkine Chirrete",
        "Makkine Care",
        "Makkine Faltón",
        "Makkine Camelle",
        "Makkine Peye",
        "Makkine Tinto",
        "Makkine Tiguere",
        "Makkine Miguele",
        "Makkine Pri",
        "Makkine Tiempo",
        "Makkine Cobro Caro",
        "Makkine Raro",
        "Makkine Guíros",
        "Makkine 2112",
        "Makkine Tiempo Real",
        "Makkine Brake Paisa",
        "Makkine Release Largo",
        "Makkine Transformer Paisa",
        "Makkine Chirp Paisa",
        "Makkine Flare Paisa",
        "Makkine Crab Paisa",
        "Makkine Baby Paisa",
        "Makkine Endless Medallo"}},
  }};
};
} // namespace scratch
