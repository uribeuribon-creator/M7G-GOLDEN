# M7G ALIEN TUNE GOD V1 — engineering contract

Independent JUCE 7.0.12 / C++20 Windows x64 VST3; preserve Alien Voice and
Transitions. Same signed global perpetual M7G.license. No network at runtime.

Analysis precedes tuning decisions: downsampled YIN/normalised autocorrelation,
learned candidate scoring with ONNX Runtime on a worker, stable scale/note graph,
slow pitch-centre correction and a separately controlled fast robotic component.
Cepstral-envelope-compensated streaming spectral resynthesis; independent phases,
microtiming and modulation for four harmonies and two doubles. These are derived
vocal voices, not trained generative singers or recordings of real extra takes.
No claim of Melodyne/Antares equivalence or flawless automatic key detection.

Five selectable tuning profiles change actual correction/time/vibrato curves.
A second ONNX network estimates acoustic register/air/roughness from features.
Synthetic-only training is documented. It cannot reliably identify a person's
sex/gender, anatomical register, identity, genre or artist. Display estimates
and their confidence honestly; allow manual root/scale/model overrides.

Six main rotary controls: correction, human/robot, retune, humanize, formant,
harmony mix. Separate harmony/double switches, 1–4 voice count, diatonic interval
and per-generated-voice width/colour/air/reverb/drive/pan. Lead never enters those
FX. Four preset banks, 25 named recipes each, session A/B, favourites and search.
Live waveform and scrolling note history use atomic bounded snapshots.

Studio uses a 2048-sample window; Monitor 512 samples. At 48 kHz reported latency
is 42.67 / 10.67 ms. Detection additionally needs observed periods and note-change
settling; low notes cannot be identified perfectly within 15 ms. No zero-latency
pitch-processing claim. Monitor trades spectral resolution/low-register quality
for latency. Change quality while transport is stopped. Retune 0 is the fastest
control response, not zero acoustic or detection latency.

ORT sessions, allocation, loading and inference stay off the audio callback.
Audio memory is prepared in advance; quiet sections suspend spectral voices after
tails. Fixed bounds, smoothed controls, aligned bypass and safe feedback/output.
Original models and provenance included. Ship ORT MIT/third-party notices and
JUCE 7 GPLv3/commercial, SDK/font notices. CPU is measured, not promised on i5.

Validation must measure detector cents/octave accuracy, actual output tuning,
vibrato preservation, scale-correct harmony fundamentals, per-voice isolation,
double variation, formant preservation, exact bypass latency, all 100 presets,
rates/blocks/seek/state/A-B/licence, actual DLL+ORT loading, UI inventory and
native installation/reinstallation/coexistence/uninstall. Publish checksum,
screenshots, synthetic demonstration and native benchmark. Actual Studio One 7
and listening on real singers need separate direct evaluation.
