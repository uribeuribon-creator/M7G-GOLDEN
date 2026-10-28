#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace tune {
enum Param {
  Correction,
  Robot,
  Retune,
  Human,
  Formant,
  HarmonyMix,
  Model,
  Key,
  ScaleKind,
  AutoKey,
  Quality,
  HarmonyOn,
  DoublesOn,
  VoiceCount,
  Interval,
  CustomInterval,
  DoubleHuman,
  Bypass,
  FirstFX,
  Count = FirstFX + 36
};
inline constexpr std::array<const char *, FirstFX> ids{
    "correction", "robot",  "retune",   "human",   "formant",     "harmonyMix",
    "model",      "key",    "scale",    "autoKey", "quality",     "harmonyOn",
    "doublesOn",  "voices", "interval", "custom",  "doubleHuman", "bypass"};
inline constexpr std::array<const char *, 6> fxIds{"width", "colour", "air",
                                                   "verb",  "drive",  "pan"};
using Values = std::array<float, Count>;
inline Values defaults() {
  Values p{};
  p[Correction] = 90;
  p[Retune] = 80;
  p[Human] = 75;
  p[HarmonyMix] = 35;
  p[AutoKey] = 1;
  p[VoiceCount] = 3;
  p[DoubleHuman] = 70;
  p[CustomInterval] = 7;
  p[ScaleKind] = 1;
  for (int v = 0; v < 6; ++v) {
    int f = FirstFX + v * 6;
    p[size_t(f)] = 90;
    p[size_t(f + 1)] = v < 4 ? 40 : 55;
    p[size_t(f + 2)] = 15;
    p[size_t(f + 3)] = v < 4 ? 18 : 5;
    p[size_t(f + 4)] = 4;
    p[size_t(f + 5)] = (v % 2 ? 1.f : -1.f) * (v < 4 ? .65f : .8f);
  }
  return p;
}
inline constexpr std::array<const char *, 5> models{
    "ALIEN NATURAL", "REGGAETON FLOW", "TRAP AUTO", "POP CRYSTAL",
    "CLASSIC VINTAGE"};
inline constexpr std::array<const char *, 4> banks{
    "VOCES PRINCIPALES", "EFECTO ROBOT", "ARMONIAS", "DOBLES"};
struct Preset {
  const char *name;
  Values p;
};
inline const std::array<Preset, 100> &presets() {
  static const auto a = []() {
    constexpr const char *names[4][25] = {
        {"Natural Golden",   "Pecho de Seda",      "Aire Lunar",
         "Flow Humano",      "Cristal Principal",  "Terciopelo 73",
         "Melisma Libre",    "Pop de Oro",         "Raiz Natural",
         "Grave Imperial",   "Agudo Celeste",      "Balada Orbital",
         "Vocal Intima",     "Susurro Firme",      "R&B Nebula",
         "Reggaeton Limpio", "Dembow Suave",       "Afro Flow",
         "Dulce Cobre",      "Soul de Medianoche", "Principal Presente",
         "Vintage Futuro",   "Luz de Estudio",     "Natural Flexible",
         "Golden Signature"},
        {"Robot 2000",          "Trap de Titanio", "Astro Flow",
         "Digital Dorado",      "Neon Dembow",     "Cromo Duro",
         "Robot Suave",         "Ciudad Futura",   "Catedral Auto",
         "Cyber Melisma",       "Vocoder Soul",    "Laser Callejero",
         "Urbano Magnetico",    "Cassette Robot",  "Pluton Hook",
         "Mercurio Glide",      "Cristal Rapido",  "Retro Cyborg",
         "Espejo Digital",      "Cobre Electrico", "Robot Intimo",
         "Dembow Hacker",       "Satin Auto",      "Estrella Urbana",
         "Alien Hard Signature"},
        {"Tercera de Seda",      "Quinta Imperial",    "Octava Celeste",
         "Bajo de Titan",        "Coro Golden",        "Dembow Choir",
         "Nebula Gospel",        "Tercera Menor Azul", "Pop Cuatro Voces",
         "Reggaeton Angels",     "Coro de Medianoche", "Seda y Cristal",
         "Quintas del Cosmos",   "Octavas del Tiempo", "Coro Catedral",
         "Armonia Intima",       "Trap Coral",         "Afro Choir",
         "Cobre y Niebla",       "Octava del Sol",     "Tres Planetas",
         "Coro Stereo",          "Plate Choir",        "Cuatro Constelaciones",
         "Alien Choir Signature"},
        {"Dobles Reales Golden", "Wide de Seda",      "Tight Principal",
         "Lo-Fi Gemelos",        "Trap Doubles",      "Pop Mirror",
         "Dembow Ancho",         "Vintage Doble",     "Refuerzo Intimo",
         "Stereo Plata",         "Doble Aire",        "Satin Twins",
         "Retro Tight",          "Voces del Cosmos",  "Cobre Paralelo",
         "Reflejo Lunar",        "Doble Oscuro",      "Dobles Limpios",
         "Soul Doubles",         "Gemelos Digitales", "Doble Plate",
         "Octava y Dobles",      "Arena Vocal",       "Neon Twins",
         "Makkine Seven Doubles"}};
    std::array<Preset, 100> r{};
    for (int b = 0; b < 4; ++b)
      for (int i = 0; i < 25; ++i) {
        auto &q = r[size_t(b * 25 + i)];
        q.name = names[b][i];
        q.p = defaults();
        auto &p = q.p;
        p[Model] = float(i % 5);
        p[Correction] = 75.f + i * .85f;
        p[Retune] = 45.f + i * 4;
        p[Human] = 55.f + (i % 9) * 4;
        p[Robot] =
            b == 1 ? std::clamp(68.f + i * 1.3f, 0.f, 100.f) : float(i % 5) * 2;
        if (b == 1) {
          p[Retune] = float((i % 7) * 5);
          p[Human] = float((i % 6) * 6);
        }
        if (b == 2) {
          p[HarmonyOn] = 1;
          p[VoiceCount] = float(i % 4 + 1);
          p[Interval] = float(i % 6);
          p[HarmonyMix] = 40.f + i * 1.3f;
          p[AutoKey] = 0;
          p[Key] = float((i / 6) % 12);
          p[ScaleKind] = i % 3 == 0 ? 2.f : 1.f;
        }
        if (b == 2) {
          constexpr int voiceCounts[25]{1, 1, 1, 1, 4, 4, 4, 1, 4, 4, 4, 2, 3,
                                        2, 4, 1, 4, 4, 3, 1, 3, 4, 4, 4, 4};
          constexpr int patterns[25]{0, 1, 2, 3, 4, 0, 4, 0, 4, 4, 4, 0, 1,
                                     2, 4, 0, 4, 4, 1, 2, 4, 4, 4, 4, 4};
          p[VoiceCount] = float(voiceCounts[i]);
          p[Interval] = float(patterns[i]);
          if (i == 7)
            p[ScaleKind] = 2;
        }
        if (b == 3) {
          p[DoublesOn] = 1;
          p[HarmonyMix] = 40.f + i * 1.1f;
          p[DoubleHuman] = 30.f + i * 2.5f;
          if (i == 21) {
            p[HarmonyOn] = 1;
            p[Interval] = 2;
            p[VoiceCount] = 1;
          }
        }
        for (int v = 0; v < 6; ++v) {
          int f = FirstFX + v * 6;
          p[size_t(f)] = 60.f + (i % 9) * 4;
          p[size_t(f + 1)] = 25.f + (i * 7 + v * 9) % 60;
          p[size_t(f + 2)] = float((i * 3 + v * 7) % 35);
          p[size_t(f + 3)] = b == 2 ? 18.f + i : float(i % 7) * 2;
          p[size_t(f + 4)] = float(i % 8);
          p[size_t(f + 5)] = (v % 2 ? 1.f : -1.f) * (.35f + .02f * (i % 20));
        }
      }
    return r;
  }();
  return a;
}
} // namespace tune
