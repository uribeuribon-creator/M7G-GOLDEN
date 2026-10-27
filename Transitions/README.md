# M7G ALIEN TRANSITIONS GOD V1

**V1 TRANSITIONS · 150 FX · Makkine Seven Golden**

[Descargar instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/transitions-v1.0.0/M7G-ALIEN-TRANSITIONS-GOD-V1-Windows-x64.exe) · [Validación nativa](VALIDATION.md) · [Release con demo y capturas](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/transitions-v1.0.0)

Independent Windows x64 VST3 vocal-transition designer. Three banks of 50 effects:
**INTRO – ENTRADA**, **CAMBIO – PUENTE**, **SALIDA – FINAL**. Four controls,
Spanish tooltips, blue/gold cockpit, live colour waveform, searchable browser,
previous/next/random, favourites and session A/B. Built with C++20/JUCE 7.0.12.
Alien Voice and its existing releases are separate products and remain intact.

## How to use

1. Close Studio One, install the Windows `.exe`, reopen and rescan VST3.
2. Insert **M7G ALIEN TRANSITIONS GOD V1** on the vocal track. Choose a bank and
   one of its 50 presets. Play the recorded vocal through the plug-in.
3. Set **CANTIDAD** for transformation intensity and **MIX** for the balance.
   They have different functions. Either at zero gives the aligned clean voice.
4. **TIEMPO** sets the next transition's duration: INTRO 200–1500 ms; CAMBIO and
   SALIDA 400–1500 ms. Very short phrases/gaps limit the available region.
5. **SENSIBILIDAD DE PALABRA** detects softer words when raised. Lower it if room
   noise triggers effects. Detection uses RMS/hysteresis and 250 ms quiet gaps.
6. **STOP FX** fades current/pending effects; the clean vocal continues. BYPASS
   keeps the same latency. The menu copies A/B and filters favourites.

### The two-second anticipation is essential

**Fixed latency: 2000 ms.** The DAW must compensate it during playback/mixing.
The capture buffer provides already received audio for an effect *before the
latency-delayed clean phrase*. This cannot anticipate an unheard live phrase
without lookahead. It is not a zero-latency recording monitor. Allow preroll
when playback starts or after a seek; stale audio/events are flushed on seek.

INTRO precedes a clean phrase. CAMBIO waits for silence of at least 300 ms and
ends before the next clean phrase. SALIDA transforms the last available segment
of the phrase, with a bounded effect tail where appropriate. Only the selected
bank operates. Short preview fragments (up to 420 ms) fit the longest INTRO
inside the two-second lookahead; full-word reversal is limited by that capture.

Echo/stutter timing follows the DAW BPM/PPQ (120 BPM fallback) and accounts for
output latency. Subdivisions include quarter, eighth, sixteenth, sixteenth
triplet and thirty-second, plus dotted eighth and slap. Short windows may not
contain a complete repetition. Settings apply to new triggers to avoid
rescheduling an effect while it is playing.

## The 150 recipes

Every requested factory name is included. The recipes combine actual original
DSP: reverse snapshots, anti-causal reflection/echo taps, granular pitch and
pitch-preserving time remapping, cepstral formant shifts, tape speed/wow,
beat slicing, chorus/flanger/allpass phaser, filters, bit/sample reduction,
distortion/saturation, pan/width, ring/amplitude modulation, filtered feedback
delay/ping-pong and modulated stereo FDN reverbs with pitched shimmer input.

Formant processing uses a 1024-sample internal wet window. Granular/pitch effects
are creative transition algorithms, not proprietary Elastique/Melodyne engines.
The effect-trajectory display is a preset guide; the upper waveform shows actual
output activity: gold INTRO, cyan CAMBIO, blue clean vocal, violet SALIDA.

“Infinite” presets hold until the next phrase, STOP FX, transport stop or a
30-second safety limit. Feedback is bounded. Eight preallocated effect voices
preserve tails; excessive overlapping triggers can reach that limit and the
UI reports it. Amount/mix changes and event edges are smoothed. A sample-peak
safety limiter protects overlapping effects; this is not certified true-peak.

## Global authorisation and CPU

The existing signed perpetual **M7G.license** unlocks every effect. The installer
preserves an existing file and supplies the shared distribution licence when
absent. It uses `%ProgramData%\M7G\M7G.license`; another valid file may be
imported into the user's M7G application-data directory. Invalid import attempts
retain an existing valid authorisation. No per-effect activation or network/
audio upload. Missing licence passes the aligned clean vocal.

Effect processing sleeps without active transitions/tails. Capture, detection
and dry delay must keep running to hear the next phrase: CPU use is low in
silence, not literally zero. All buffers are allocated in prepare, not at a
trigger. The release includes a measured ten-instance callback benchmark.
CPU varies with presets, overlaps, rate, hardware and open editors. Software UI
updates at 60 Hz with signal, 15 Hz idle; no GPU acceleration claim.

## Build / validation

Windows: Visual Studio 2022 C++ tools, Git, CMake and NSIS:

```powershell
./Transitions/Build-Windows.ps1
# The workflow also runs InstallerSmoke.ps1 in a disposable Windows runner.
```

Pinned JUCE 7.0.12 revision `4f43011b96eb0636104cb3e433894cda98243626`.
The build creates only this product in `build-transitions-windows`; it does not
rebuild/replace Alien Voice. Native tests cover all 150 audio recipes, timing,
latency, host BPM, state/A-B/favourites, licence, silence, seeking, mono/stereo,
zero/oversized blocks, exactly four knobs, three browsers, actual DLL loading
and installation/reinstallation/coexistence/uninstall. Release assets include
native screenshots, checksum, benchmark and a **synthetic-vowel** demo reel.
The demo is not a recording of a real singer or artist.

Engineering pre-release. Real-vocal listening and actual Studio One 7 operation
need direct evaluation. No demonstrated superiority/equivalence to named
commercial products. The installer is unsigned. Review JUCE 7's GPLv3/commercial
licensing before proprietary commercial distribution; this project's activation
file does not grant a JUCE commercial licence. Framework/SDK/font notices are
included. No new licence for the user's own code is implied.
