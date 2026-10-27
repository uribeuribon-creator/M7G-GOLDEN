#pragma once
#include "RackParameters.h"
namespace rack {
inline constexpr int presetCount = 280;
struct Preset {
  juce::String name;
  int role = 0;
  int genre = 0;
  int variation = 0;
};
inline const std::array<Preset, presetCount> &presets() {
  static const auto bank = [] {
    std::array<Preset, presetCount> b{};
    const char *special[]{"SINO | ESPECTACULAR",    "SINO | Natural Silk",
                          "SINO | Robot Platinum",  "GOD | Eight Voice Choir",
                          "GOD | Fifth Dimension",  "GOD | Octave Cathedral",
                          "GOD | Angelic Shimmer",  "IRON | Urban Weight",
                          "VALVE | Intimate Gold",  "CONSOLE | Clean Pressure",
                          "SPACE | Chamber Bloom",  "SPACE | Echo Orbit",
                          "BUS | Transparent Glue", "MONITOR | Record Dry"};
    for (int i = 0; i < 14; ++i)
      b[size_t(i)] = {special[i], i == 12 ? 5 : (i >= 3 && i <= 6 ? 4 : 0), 0,
                      i};
    const char *genres[]{"Rap",       "Trap",     "Reggaeton",
                         "Dancehall", "Afrobeat", "RnB"};
    const char *roles[]{"Lead",   "Doubles", "Support",
                        "Adlibs", "Harmony", "Bus"};
    const char *artists[6][6] = {
        {"Drake", "Kendrick", "J. Cole", "Nas", "Travis Scott", "M7G"},
        {"Travis Scott", "Future", "Don Toliver", "Lil Baby", "21 Savage",
         "M7G"},
        {"Feid", "Bad Bunny", "J Balvin", "Rauw Alejandro", "Karol G", "M7G"},
        {"Sean Paul", "Vybz Kartel", "Popcaan", "Shenseea", "Koffee", "M7G"},
        {"Burna Boy", "Wizkid", "Tems", "Rema", "Ayra Starr", "M7G"},
        {"The Weeknd", "SZA", "Brent Faiyaz", "PARTYNEXTDOOR", "Chris Brown",
         "M7G"}};
    for (int g = 0; g < 6; ++g)
      for (int r = 0; r < 6; ++r)
        for (int v = 0; v < 6; ++v) {
          int i = 14 + g * 36 + r * 6 + v;
          b[size_t(i)] = {juce::String(genres[g]) + " | " + artists[g][v] +
                              " | " + roles[r],
                          r, g, v};
        }
    const char *effects[]{
        "Victrola Neon",       "Gramophone Quantum",     "Cassette Nebula",
        "Vinyl Andromeda",     "Valve CyberSoul",        "Opera Hologram",
        "Baroque Plasma",      "Crooner Android",        "Tape DeLorean",
        "Disco Singularity",   "Cathedral Zero Gravity", "Choir Chrononaut",
        "Vellum Starship",     "Gospel Photon",          "Velvet Saturn",
        "Radio Orion",         "Motown Cyborg",          "Shellac Supernova",
        "Cinema Nanowave",     "Magnetic Eclipse",       "Angel Arpeggio",
        "Celestial Chamber",   "Gregorian Stardust",     "Halo Hyperdrive",
        "Cathedral Chrome",    "Boombap Mech",           "Soul Transistor",
        "Golden Tesla",        "Retro Reactor",          "Dusty Solaris",
        "Echo Voyager",        "Dub Time Machine",       "Drake Chronosphere",
        "Tape Wormhole",       "Throw Nebulizer",        "PingPong Olympus",
        "Stereo Atlantis",     "Chamber Spacesuit",      "Orbit Ballroom",
        "Wide Renaissance",    "Telephone Titan",        "Radio Empyrean",
        "Victorian Megaphone", "AM Astronaut",           "Gramophone Galaxy",
        "Alien Crooner",       "Cyber Gospel",           "Unison Stardust",
        "Robo Bolero",         "Makkine Genesis"};
    for (int i = 0; i < 50; ++i)
      b[size_t(230 + i)] = {juce::String("FX | ") + effects[i],
                            i / 5 == 3 || i / 5 == 4 ? 4 : 3, 0, i};
    return b;
  }();
  return bank;
}
inline constexpr const char *presetCreator = "Makkine Seven Golden";
inline Values presetValues(int index) {
  index = juce::jlimit(0, presetCount - 1, index);
  const auto &p = presets()[size_t(index)];
  int r = p.role, g = p.genre, v = p.variation % 6;
  auto x = defaults();
  x[VoiceType] = float(r);
  x[Sino] = index < 14 ? 0.0f : float(20 + g * 4 + v * 3);
  x[WarmDrive] = float(30 + g * 5 + v * 6);
  x[WarmBias] = float(20 + v * 8);
  x[PreampStyle] = float(v % 6);
  x[WarmTone] = float(g - 2) * .5f;
  x[AirAmount] = float(26 + v * 6 + g * 2);
  x[AirGain] = .5f + v * .25f;
  x[CompReduction] = float(28 + r * 4 + v * 3);
  x[CompRatio] = 2.2f + r * .45f + v * .12f;
  x[CompGain] = r == 5 ? .5f : 1.5f;
  x[NoiseOn] = r == 5 ? 0.0f : 1.0f;
  x[NoiseAmount] = float(35 + v * 6);
  x[TuneOn] = r == 5 ? 0.0f : 1.0f;
  x[TuneAmount] = float(55 + v * 8);
  x[TuneSpeed] = r == 3 ? 7.0f : float(20 + v * 8);
  x[PitchStyle] = g == 1 && v < 3 ? 1.0f : 0.0f;
  x[PitchFormant] = (v == 3 ? -.8f : (v == 4 ? .6f : 0));
  x[EQGain] = r == 5 ? .6f : float(12 + v * 3) * .1f;
  x[EQFreq] = float(2300 + g * 160 + v * 90);
  x[EQQ] = .8f + v * .12f;
  x[GateThreshold] = float(-54 + v);
  x[GateRange] = r == 5 ? 10.0f : 22.0f;
  x[ReverbMix] = r == 5 ? 3.0f : float(5 + g + v * 2 + (r == 4 ? 5 : 0));
  x[ReverbDecay] = .8f + g * .16f + v * .19f;
  x[ReverbStyle] = float((v + g) % 3);
  x[ReverbSize] = float(30 + v * 9);
  x[ReverbPredelay] = float(12 + v * 5);
  x[ReverbDuck] = float(65 + v * 5);
  x[ReverbShimmer] = r == 4 && v > 2 ? float(8 + v * 5) : 0.0f;
  x[DelayMix] = r == 5 ? 1.0f : float(3 + v * 2 + (r == 3 ? 8 : 0));
  x[DelayNote] = float((g + v) % 6);
  x[DelayFeedback] = float(18 + v * 6);
  x[DelayDuck] = float(72 + v * 4);
  x[DelayTone] = float(3600 + v * 620);
  x[DelayStyle] = float(v % 3);
  x[Width] = r == 0 ? 100.0f : float(104 + v * 5);
  x[Spread] = (r == 1 || r == 4) ? float(15 + v * 4) : 5.0f;
  x[ModMix] = (r == 1 || r == 3 || r == 4) ? float(5 + v * 2) : 0.0f;
  x[ModRate] = .2f + v * .12f;
  x[ModDepth] = float(15 + v * 6);
  x[ModType] = float(v % 3);
  x[ModOn] = x[ModMix] > 0 ? 1.0f : 0.0f;
  x[DereverbOn] = r == 5 ? 0.0f : 1.0f;
  x[DereverbAmount] = float(15 + v * 4);
  x[DereverbTail] = float(180 + g * 40 + v * 25);
  x[ResonanceOn] = 1;
  x[ResonanceAmount] = float(18 + g * 3 + v * 4);
  x[ResonanceFocus] = 1.4f + v * .35f;
  x[SaturationOn] = 1;
  x[SatLow] = float(12 + v * 5);
  x[SatMid] = float(18 + g * 3 + v * 4);
  x[SatHigh] = float(8 + v * 3);
  x[SatCrossLow] = float(180 + g * 20 + v * 13);
  x[SatCrossHigh] = float(2800 + g * 210 + v * 190);
  x[FetOn] = r == 5 ? 0.0f : 1.0f;
  x[FetThreshold] = float(-14 - v);
  x[FetRatio] = 4 + v * .6f;
  x[FetAttack] = .35f + v * .18f;
  x[FetRelease] = float(55 + v * 17);
  x[HarmonyOn] = r == 4 ? 1.0f : 0.0f;
  x[HarmonyMix] = r == 4 ? float(18 + v * 5) : 0.0f;
  x[HarmonyPattern] = float(v % 4);
  x[HarmonyDetune] = float(4 + v * 2);
  x[HarmonyFormant] = v == 4 ? 1.0f : 0.0f;
  if (index < 14) {
    x[Sino] =
        index == 0 ? 90.0f : (index == 1 ? 45.0f : (index == 2 ? 75.0f : 0));
    if (index == 2) {
      x[PitchStyle] = 1;
      x[TuneAmount] = 100;
      x[TuneSpeed] = 5;
    }
    if (index >= 3 && index <= 6) {
      x[HarmonyOn] = 1;
      x[HarmonyMix] = 34;
      x[HarmonyPattern] =
          float(index == 3 ? 2 : (index == 4 ? 1 : (index == 5 ? 0 : 3)));
      x[ReverbShimmer] = index == 6 ? 45.0f : 0;
    }
    if (index == 7) {
      x[PreampStyle] = 0;
      x[WarmDrive] = 80;
      x[SatLow] = 55;
    }
    if (index == 8) {
      x[PreampStyle] = 1;
      x[WarmDrive] = 65;
      x[WarmBias] = 65;
    }
    if (index == 9) {
      x[PreampStyle] = 2;
      x[WarmDrive] = 35;
      x[FetRatio] = 8;
    }
    if (index == 10) {
      x[ReverbMix] = 24;
      x[ReverbDecay] = 3.4f;
      x[ReverbShimmer] = 18;
    }
    if (index == 11) {
      x[DelayMix] = 28;
      x[DelayFeedback] = 62;
      x[DelayStyle] = 1;
    }
    if (index == 12) {
      x[Sino] = 0;
      x[WarmDrive] = 20;
      x[SatLow] = 8;
      x[SatMid] = 10;
      x[SatHigh] = 5;
      x[CompRatio] = 1.8f;
    }
    if (index == 13) {
      x[EngineMode] = 0;
      x[Sino] = 0;
      x[NoiseOn] = x[TuneOn] = x[DereverbOn] = x[ResonanceOn] = x[HarmonyOn] =
          0;
      x[ReverbMix] = x[DelayMix] = x[ModMix] = 0;
      x[ReverbOn] = x[DelayOn] = x[ModOn] = 0;
      x[Spread] = 0;
    }
  }
  x[PreampGain] = float((g + v) % 4);
  x[PreampImpedance] = float(800 + (v % 6) * 450);
  x[CompParallel] = float(65 + (v % 6) * 5);
  x[SatParallel] = float(55 + (v % 6) * 7);
  x[EQParallel] = float(78 + (v % 6) * 4);
  x[EQDirty] = float((v % 6) * 4);
  x[SatStyle] = float(v % 5);
  x[Punch] = (g == 1 || g == 2) && r != 5 ? 1.0f : 0.0f;
  x[GodSpace] = index == 0 ? 1.0f : 0.0f;
  x[VerbAMix] = float(7 + v % 5);
  x[VerbBMix] = float(5 + v % 4);
  x[VerbCMix] = r == 4 ? float(5 + v % 5) : 3.0f;
  x[VerbADecay] = .24f + (v % 5) * .035f;
  x[VerbBDecay] = 1.1f + (v % 5) * .2f;
  x[VerbCDecay] = 4.2f + (v % 5) * .45f;
  x[VerbAFilter] = float(6000 + (v % 5) * 600);
  x[VerbBFilter] = float(4200 + (v % 5) * 700);
  x[VerbCFilter] = float(3500 + (v % 5) * 500);
  x[DelayStyle] = r == 3 ? 1.0f : (v % 3 == 0 ? 2.0f : 3.0f);
  x[DelayDrive] = float(15 + (v % 6) * 7);
  x[SlapTime] = float(90 + (v % 6) * 9);
  x[SculptorOn] = 1;
  x[SculptorAmount] = float(20 + (v % 6) * 4);
  x[SculptorBody] = .5f + (g % 3) * .3f;
  x[SculptorTilt] = -1.0f - (g % 3) * .3f;
  if (index >= 230) {
    int e = index - 230, group = e / 5, k = e % 5;
    x[Sino] = 0;
    x[HarmonyOn] = 0;
    x[ModOn] = 0;
    x[GodSpace] = 0;
    x[ReverbMix] = 8;
    x[DelayMix] = 8;
    if (group == 0) {
      x[PitchStyle] = 1;
      x[TuneAmount] = 100;
      x[TuneSpeed] = 5;
      x[PitchFormant] = float(k - 2) * 1.1f;
    }
    if (group == 1) {
      x[ModOn] = 1;
      x[ModType] = float(k % 3);
      x[ModMix] = float(35 + k * 12);
      x[ModRate] = .4f + k * .6f;
      x[ModDepth] = float(45 + k * 10);
    }
    if (group == 2) {
      x[SaturationOn] = 1;
      x[SatStyle] = float(k);
      x[SatLow] = 55;
      x[SatMid] = 75;
      x[SatHigh] = 45;
      x[SatParallel] = float(35 + k * 10);
      x[WarmDrive] = 70;
    }
    if (group == 3) {
      x[HarmonyOn] = 1;
      x[HarmonyPattern] = 0;
      x[HarmonyMix] = float(28 + k * 7);
      x[HarmonyFormant] = float(k - 2);
    }
    if (group == 4) {
      x[HarmonyOn] = 1;
      x[HarmonyPattern] = 3;
      x[HarmonyMix] = float(25 + k * 6);
      x[VerbCMix] = float(12 + k * 4);
      x[VerbCDecay] = 5.0f + k;
      x[ReverbShimmer] = float(30 + k * 6);
      x[ReverbMix] = 18;
    }
    if (group == 5) {
      x[PreampStyle] = 5;
      x[WarmDrive] = float(65 + k * 7);
      x[PhantomModel] = k % 2 ? 1.0f : 0.0f;
      x[PreampImpedance] = float(400 + k * 250);
      x[AirFilter] = float(6000 + k * 600);
    }
    if (group == 6) {
      x[DelayStyle] = 1;
      x[DelayMix] = float(20 + k * 4);
      x[DelayFeedback] = float(32 + k * 7);
      x[DelayNote] = float(1 + k % 5);
      x[ThrowSensitivity] = float(90 + k * 30);
      x[DelayDuck] = 95;
    }
    if (group == 7) {
      x[DelayStyle] = k % 2 ? 4.0f : 2.0f;
      x[DelayMix] = float(20 + k * 4);
      x[DelayFeedback] = float(30 + k * 7);
      x[Width] = 135;
      x[Spread] = 30;
      x[ReverbMix] = 14;
    }
    if (group == 8) {
      x[GateLowcut] = float(220 + k * 45);
      x[AirFilter] = float(2600 + k * 400);
      x[AirOn] = 1;
      x[EQFreq] = 1700;
      x[EQGain] = 5;
      x[WarmDrive] = 80;
      x[SculptorOn] = 0;
      x[PreampStyle] = float(k);
    }
    if (group == 9) {
      x[PitchStyle] = 1;
      x[TuneAmount] = 100;
      x[PitchFormant] = float(k - 2) * 1.5f;
      x[HarmonyOn] = 1;
      x[HarmonyPattern] = float(k % 4);
      x[HarmonyMix] = 25;
      x[ModOn] = 1;
      x[ModType] = 1;
      x[ModMix] = 30;
      x[SatStyle] = float(k);
      x[SatMid] = 65;
      x[DelayStyle] = 4;
      x[ReverbShimmer] = 35;
    }
  }
  for (size_t i = 0; i < x.size(); ++i)
    x[i] = juce::jlimit(specs()[i].min, specs()[i].max, x[i]);
  return x;
}
} // namespace rack
