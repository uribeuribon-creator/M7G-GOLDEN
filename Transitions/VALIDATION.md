# Transitions V1 — release validation

Validated 27 October 2026. Source: `a7cbdccbe84af009f665916c88925b12f1f2ceae`.

[Windows build and complete test log](https://github.com/uribeuribon-creator/M7G-GOLDEN/actions/runs/40592068890)
· [Public release](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/transitions-v1.0.0)
· [Direct Windows installer](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/transitions-v1.0.0/M7G-ALIEN-TRANSITIONS-GOD-V1-Windows-x64.exe)

## Native Windows results

Windows Server 2022 x64 runner, Visual Studio 2022, pinned JUCE 7.0.12.
All three CTest suites passed, plus the separate installation smoke test.

- **TransitionAudio:** 150 distinct requested names produce 150 distinct audible
  renders; pre-vocal INTRO, 300 ms gap CAMBIO and final-segment SALIDA timing;
  fixed 2000 ms latency and exact aligned clean signal at zero amount/mix or
  bypass; 44.1/48/96 kHz; alternate block sizes; actual host BPM changes rhythm;
  AMOUNT differs from MIX; session/A-B/bank/favourite/licence recall; invalid
  import retains authorisation; silence suspension, seeking, mono/stereo and
  zero/oversized callbacks. DSP suite: 46.94 seconds.
- **TransitionInterface:** exactly four knobs, three banks of 50, search, preset
  navigation/random, favourites, A/B controls, bypass and actual screenshot
  rendering. Three native 1280 × 800 PNGs published.
- **TransitionVST3:** actual Windows DLL factory, product name and independent
  class ID `01EFCDAB8291EBFA4D37473754726E31`, stereo buses, audio and unloading.
  At 48 kHz, declared host latency is 96000 samples.
- **InstallerSmoke:** install and installed/built DLL hash equality, installed
  DLL loading, existing signed licence preserved on reinstall, previous-product
  fixtures including Alien Voice V11 preserved, registration and uninstall.
  This fixture test runs only on a disposable CI runner.

## Public download verification

All nine release assets downloaded without credentials: HTTP 200 and attachment
headers; sizes and SHA-256 agree with GitHub asset metadata. Installer checksum
also agrees with the published `.sha256` file. Installer PE/version metadata,
PNG dimensions, WAV format and shared licence RSA signature checked.

Installer: **2,373,963 bytes**.

```text
c61a4e3972a4ab3ab6c74fee39868804e702d5eeb85ee4cca3dd8dda2082ca4e
```

The NSIS bootstrap is x86 and installs the separately verified Windows AMD64
VST3 payload. Both run on Windows 10/11 x64. Installer is unsigned.

Demo: approximately 46 seconds, stereo 48 kHz/16-bit WAV, ten effects with
track list; generated from a synthetic vowel, not a real singer.

## Ten-instance callback benchmark

Runner has four logical CPUs. Each row processes 5.00267 seconds of audio,
48 kHz, block 256, ten varied factory presets, editors closed.

| Input | Wall time | Equivalent share of one core |
| --- | ---: | ---: |
| Silence | 0.070785 s | 1.41494% |
| Synthetic vocal phrases | 0.346597 s | 6.92825% |

These are short callback-only measurements on the CI machine, not a physical
laptop/i5 certification or a worst-case estimate for all 150 effects. Capture,
detection and dry delay continue in silence; heavy effects sleep after tails.

## Limits requiring user evaluation

Actual Studio One 7 use and real-vocal listening have not been directly tested.
Use playback/mixing with DAW plug-in delay compensation: fixed **2000 ms**
lookahead is essential for pre-vocal effects, not zero-latency monitoring.
Allow preroll after playback start/seek. Eight preallocated effect voices,
finite snapshots and a 30-second hold safety limit bound processing. Sonic
quality and CPU on the user's session still need evaluation. This release is
marked pre-release; no comparative superiority or proprietary-engine claim.
