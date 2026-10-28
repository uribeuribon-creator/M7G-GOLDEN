# M7G ALIEN TUNE GOD V1 — verified Windows release

Source: `c3e4c8cbf03e19b57e828d3d79a4fb7a23d5a2dd`. Tag: `tune-v1.0.0`.

[Native Windows workflow](https://github.com/uribeuribon-creator/M7G-GOLDEN/actions/runs/40607570338) · [Public release](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/tune-v1.0.0)
· [Direct installer](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/tune-v1.0.0/M7G-ALIEN-TUNE-GOD-V1-Windows-x64.exe)

The public workflow summary reports **Success**, Windows job 6m 37s, total
6m 41s. Its publication step runs only after all three CTest suites, installer
smoke test, screenshot/demo rendering and CPU benchmark succeed. Build source
matches the public release/tag. After environment API authentication expired,
verification used the public GitHub workflow/release pages; authenticated verbose
native logs were not retrieved. Owners can inspect them through the run link.

## Native gates

- TuneAudio: real output tuning, synthetic pitch detection, vibrato and formant
  response, diatonic third, independent doubles and generated-effect isolation;
  all 100 names and distinct finite audio renders; actual ONNX inference;
  signed shared licence, session/A-B/favourites; aligned bypass at 44.1/48/96 kHz,
  both qualities and alternate/zero/oversized blocks.
- TuneInterface: six main rotary controls, four banks of 25, search and choir
  switches, actual note-history screenshots.
- TuneVST3: real AMD64 DLL factory, independent product/class ID
  `01EFCDAB8291EBFA4D37473754756E31`, stereo buses, audio processing and unload.
- Installer smoke: installed/built DLL hash equality, installed VST3 loading,
  ONNX DLL present, existing licence preserved on reinstall, earlier-product
  fixtures (including Alien Voice V11 and Transitions) preserved, registration
  and uninstall. Runs only in a disposable Windows CI runner.

JUCE 7.0.12 is pinned; ONNX Runtime CPU 1.20.1 archive checked by SHA-256.
ONNX and Microsoft's Visual C++ app-local redistributables are in the VST3 bundle.
No Python or model download is required to use it.

## Public download checks

Ten assets downloaded without credentials: HTTP 200, Content-Disposition
attachment, response length and GitHub's public per-asset SHA-256 digest agree.
Installer `.sha256` also agrees; PE product/version metadata and bootstrap checked.
Three PNGs are 1440 × 900. Demo WAV is 24 seconds, stereo 48 kHz/16-bit, with
an eight-preset track list; it uses a synthetic vowel, not a singer. The shared
licence RSA signature is valid with ALL/PERPETUAL scope. Published model hashes
agree with source ONNX files. Model metrics are synthetic-only, not real-singer
certification.

Installer: **5,989,882 bytes**, unsigned NSIS bootstrap installing AMD64 VST3.

```text
38d620827ac0626e9e3b6d50f832abd668c1e4889c00b7f80508c3bd43179bbd
```

## Native CPU measurement

One instance, 48 kHz/256, four logical CPU runner, editors closed; approximately
3.003 seconds per realtime-paced row. Process CPU includes the ONNX worker.
Callback wall share also includes scheduling pauses; short Windows CPU counters
have coarse resolution. Every row performed 280 neural inference batches.

| Quality | Harmonies | Process CPU / one core | Callback wall / audio duration | ONNX batches |
| --- | ---: | ---: | ---: | ---: |
| Studio | 0 | 1.041% | 8.824% | 280 |
| Studio | 4 | 22.896% | 22.564% | 280 |
| Monitor | 0 | 1.041% | 7.779% | 280 |
| Monitor | 4 | 19.774% | 21.060% | 280 |

The four-harmony runs exceed 15% of one core on this machine. Percentages against
all four logical CPUs differ, and neither a physical laptop nor an i5 was tested.
The requested <5% / <15% CPU targets are **not certified**. These are short
synthetic measurements, not worst-case results for every preset or open editors.

## Additional local Windows-executable measurements under Wine

- Seven steady synthetic pitches, 65–880 Hz: detector deviation approximately
  -0.013 to +1.698 cents. This is not a general real-vocal accuracy guarantee.
- Detuned 224.7 Hz vowel, 220 Hz target: measured output -0.5 cents Studio,
  0 cents Monitor.
- Natural vibrato mean -0.047 cents, standard deviation 20.64; robot mean
  -0.221 cents, deviation 12.47. Robot reduces vibrato, does not perfectly remove
  every fluctuation. An experimental phase refinement was rejected after it
  worsened that measurement; it is not in the released source.
- Formant control changes spectral centre 972 → 1037 Hz without moving pitch.
  Octave-up source-envelope comparison: original 830 Hz, compensated 745 Hz,
  plain resampling 1471 Hz, for the specified synthetic vowel/band. This supports
  envelope compensation, not perfect timbre preservation on any singer.

## Remaining practical evaluation

Actual Studio One 7 use and listening on real singers remain pending. AUTO TONO
can be ambiguous; manual scale selection is available. Voice ID estimates
acoustic traits, not identity, gender, anatomy or genre. Harmonies/doubles are
resynthesised from the input, not generative singers or real additional takes.

At 48 kHz: **Studio 42.67 ms / Monitor 10.67 ms**, plus pitch-observation/settling
time. Monitor loses spectral resolution, particularly in low voices. There is
no zero-latency tuning mode. The release remains an engineering pre-release;
no comparative superiority, perfect correction or commercial-engine claim.
