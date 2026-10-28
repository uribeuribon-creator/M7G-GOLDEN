#pragma once
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <set>
#include <thread>
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif JUCE_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif
inline void check(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
inline juce::AudioBuffer<float> vocal(double sr = 48000, int channels = 2) {
  int n = int(sr * 6.3);
  juce::AudioBuffer<float> b(channels, n);
  b.clear();
  double phase = 0;
  uint32_t random = 87129;
  for (int i = 0; i < n; ++i) {
    double t = i / sr;
    float note = 0;
    double begin = 0, end = 0;
    if (t >= .38 && t < 1.4) {
      note =
          55.31f +
          float(.06 * std::sin(2 * juce::MathConstants<double>::pi * .6 * t) +
                .28 * std::sin(2 * juce::MathConstants<double>::pi * 5.7 * t));
      begin = .38;
      end = 2.2;
    }
    if (t >= 1.4 && t < 2.2) {
      note =
          56.80f +
          float(.05 * std::sin(2 * juce::MathConstants<double>::pi * .7 * t));
      begin = .38;
      end = 2.2;
    }
    if (t >= 2.75 && t < 3.55) {
      note = 60.065f;
      begin = 2.75;
      end = 4.3;
    }
    if (t >= 3.55 && t < 4.3) {
      note =
          62.4f +
          float(.07 * std::sin(2 * juce::MathConstants<double>::pi * .8 * t));
      begin = 2.75;
      end = 4.3;
    }
    if (t >= 4.9 && t < 6.0) {
      note = 57.28f +
             float(.35 * std::sin(2 * juce::MathConstants<double>::pi * 6 * t));
      begin = 4.9;
      end = 6.;
    }
    float x = 0;
    if (note > 0) {
      double hz = 440 * std::exp2((note - 69) / 12.);
      phase += 2 * juce::MathConstants<double>::pi * hz / sr;
      for (int h = 1; h <= 24; ++h) {
        double f = hz * h;
        double formant = .22 + .9 * std::exp(-std::pow((f - 760) / 240, 2)) +
                         .5 * std::exp(-std::pow((f - 2300) / 430, 2));
        x += float(std::sin(phase * h) * formant / h);
      }
      x *= .30f * float(std::min(1., std::min(t - begin, end - t) / .006));
    }
    random = random * 1664525u + 1013904223u;
    float noise = (int32_t(random) / 2147483648.f);
    if ((t > .25 && t < .34) || (t > 2.45 && t < 2.58))
      x += noise * .025f;
    if (i == int(sr * .1))
      x = .25f;
    for (int c = 0; c < channels; ++c)
      b.setSample(c, i, x * (c == 0 ? 1.f : .94f));
  }
  return b;
}
inline std::vector<float> pitches(const engineer::Analysis &a, double begin,
                                  double end) {
  std::vector<float> v;
  for (auto f : a.frames)
    if (f.voiced && f.time > begin && f.time < end)
      v.push_back(f.note);
  return v;
}
inline float spread(const std::vector<float> &v) {
  if (v.empty())
    return 0;
  float mean = std::accumulate(v.begin(), v.end(), 0.f) / float(v.size()),
        power = 0;
  for (float x : v)
    power += (x - mean) * (x - mean);
  return std::sqrt(power / float(v.size()));
}
inline uint64_t fingerprint(const juce::AudioBuffer<float> &b) {
  uint64_t hash = 1469598103934665603ull;
  for (int i = 0; i < b.getNumSamples(); ++i) {
    int32_t q = int32_t(b.getSample(0, i) * 1e7);
    hash ^= uint32_t(q);
    hash *= 1099511628211ull;
  }
  return hash;
}
inline void dispatch(int milliseconds) {
  auto start=juce::Time::getMillisecondCounter();
  while(juce::Time::getMillisecondCounter()-start<uint32_t(milliseconds)) {
   #if JUCE_WINDOWS
    MSG message;while(PeekMessage(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessage(&message);}
   #elif JUCE_MAC
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,.001,false);
   #endif
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
