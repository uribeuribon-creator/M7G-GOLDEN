# Validation — Engineer ONE

Windows x64 VST3 + ARA 2.2.0, JUCE 7.0.12, Rubber Band 3.3.0. All test expectations apply to the included monophonic synthetic vocal fixture, not every possible recording.

## Local Windows-binary checks (Wine)

- Adaptive note detection: five note regions, A minor estimated without user settings.
- Automatic IA version 1, measured stable-note centre deviations: +5.54, +0.51, +0.25, +0.11, −0.04 cents from integer targets. Natural vibrato retained; no static random detuning between interpretations.
- Second IA click generates different PCM from the original capture and keeps valid musical targets.
- Exact 302400 samples for 6.3 seconds at 48 kHz; leading silence/impulse and dry unvoiced breath/noise kept sample for sample.
- Actual line/per-note WAVs: integer PCM24, 48000 Hz, line BWF origin 600000 samples for 12.5 s; per-note timestamp adds its exact start.
- Bandlimited 96k → 48k conversion: exact duration; impulse remains at index 500 for original index 1000, without added delay.
- Actual SPSC capture retains all input samples, pauses and host timestamp; host stop finalizes the file.
- Actual processor IA call, internal preview, optional note retouch, and VST3 state restore/regeneration pass without manual pitch settings.
- Real JUCE GUI: IA button triggers automatic generation, preview is audible, line WAV exists for native drag, optional editor hidden by default, controls stay inside resized bounds.
- Real ARA API factory/document/roles connected to test host: assigned region capture, project key signature, automatic application to existing modification, sample-aligned playback at nonzero timeline position, archive/restore of original and corrected PCM without external files. Separate editor-view and editor-renderer instances share preview audio/control.

## Native Windows release gate

`publish-engineer-one-release.yml` compiles from the exact full Source.zip on Windows 2022 / Visual Studio, runs four CTest suites (DSP/capture/state, UI, ARA integration/archive, real VST3 factory/load/process), verifies the payload is AMD64, tests install/reinstall/global-licence preservation/earlier products/uninstall, and publishes only after every job succeeds. Native logs, screenshot, demo WAVs, source manifest and per-asset SHA-256 accompany the release.

## Limits of evidence

Studio One 7 itself is not installed in this environment. ARA test-host success does not certify PreSonus integration, native OS file drag acceptance, the edition's ARA availability, or absolute drop placement. The user must validate those in Studio One. ARA integration processes the existing event non-destructively; there is no arbitrary DAW event deletion/replacement API in this plugin. No trained neural network, polyphonic source separation, universal perfect-tuning result or CPU-budget claim is made. The demo is synthetic and not a singer endorsement or a real-vocal listening evaluation.
