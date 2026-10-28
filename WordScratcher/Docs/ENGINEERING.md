# WORD SCRATCHER PRO - 300 — implementation contract

Independent JUCE 7.0.12 / C++20 VST3 (Windows/macOS) and AU (macOS).
No change to Alien Voice, Transitions or Tune. Same optional distribution licence.

A real five-second stereo capture ring, preallocated double word snapshots and
four-point Lagrange readers drive actual audio from the virtual platter position.
A 1 ms physical integrator uses target-position error/friction, inertia and motor
release torque. Automatic gesture families operate the same physical engine as
manual dragging, eight actual hot cues and the crossfader. Vinyl speed/direction
changes pitch; multi-level low-pass snapshots reduce high-speed aliasing.

DIGITAL uses bundled, unmodified Rubber Band 3.3.0, revision
2be46b0dffb13273a67396c77bc9278736bb03d2. It reads source frames at unit spacing
forward/reverse, then uses real-time R2 time stretching to follow platter speed
while retaining pitch. Ratios are bounded 0.25–4; near-stop is faded, not an
unbounded stretch. Startup padding/trimming and a bounded output FIFO accommodate
the variable DSP schedule. Fixed common lookahead is reported to the host; the
initial design uses 4096 samples plus a 128-sample processing block (88 ms at
48 kHz). This is a playback/mixing design, not zero-latency hardware turntablism.
Every delay/gate is aligned; bypass preserves the same dry delay.

300 exact requested names, 10 categories x30. Every preset contains actual physics,
cue, gate, pattern, texture and echo settings, applied to APVTS and session XML.
Four-point interpolation, reverse traces, speed/pitch tracking, DIGITAL pitch
retention and 300 distinct audible renders are release gates. Effects should be
obvious at default strength. No decorative-only effect or invented listening test.

Intelligent capture is energy/onset/silence segmentation with a learned noise
floor and bounded word selection, not ASR or semantic understanding. Automatic
scratch triggers at detected word ends and can duck against new live speech.
Quiet noise-only captures are rejected; vinyl noise/crackle are audible DSP,
gated with the scratch source. Echo feedback and output peaks are bounded.

UI: grouped 300-entry searchable ComboBox, real waveform/cursor, interactive
platter, eight cues, actual crossfader, automatic/manual controls, animated knob
values and advanced physical parameters. Spanish tooltips. Persistent presets,
A/B and favourites, fixed-bound atomic telemetry, no heavy UI work on audio.

Vendor GPL-2.0-or-later notices and JUCE 7 GPLv3/commercial notices included.
Source/binary distribution uses the GPLv3-or-later path; proprietary closed-source
distribution requires appropriate commercial licences. Existing products' code
licensing is not changed. Include framework/dependency provenance and source.

Native Windows tests/load/install/reinstall/coexistence/uninstall. Mac universal
VST3/AU targets are source-only here, requiring JUCE 7-compatible SDK 14; native
CTest/auval remains a separate gate before any Mac release. Publish source package and Windows installer,
actual screenshots, synthetic-audio demonstration, checksum and CPU measurements.
Actual Studio One operation and listening on real users' recordings require
separate evaluation. No unsupported comparative sonic superiority claim.
