# V11 — Alien Intelligence

Build on the tested V10 DSP, with a simpler cockpit and independent AUTO/MANUAL
control. No claims of equivalence to licensed commercial engines or benchmark
superiority. Recorded vocals, not song generation.

* 26 modules, at most two main knobs plus one parallel slider; Spanish help.
* AUTO: bounded, smoothed internal control values. A small MLP runs on the
  message/control thread; audio-thread rules respond to level and spectrum.
  No host automation is written by AUTO. Intent, enable switches, musical key,
  routing, algorithm choices and processing mode remain explicit user choices.
* MANUAL: latch effective values and makeup, commit them outside the audio
  callback, then accept normal manual/host edits. This is independent of bypass.
* Pitch note graph: stable-note hysteresis, a 15-cent dead band, slow pitch
  centre tracking and adjustable preservation of vibrato. Existing cepstral
  formant compensation remains. No polyphonic Melodyne/DNA claim.
* Gate LEARN: two seconds of measured input noise, a robust level percentile,
  rejection of voiced/contaminated learning, persistent profile. Never infer a
  geographical noise profile from a city name.
* 360 presets in six workflow banks; search, previous/next, random, persistent
  favourites, session A/B. Factory creator: Makkine Seven Golden.
* Offline RSA-signed perpetual global M7G.license; one key covers every module.
  Public verification key only in the product. The release includes a signed
  distribution licence; this is not a per-customer anti-copy service.
* CPU: skip heavy processing after both input and effect tails are quiet;
  retain delay/reverb tails and wake on input. Avoid neural denoising when its
  blend is zero. Animate only visible cards, 60 Hz with signal, 15 Hz idle.
  Benchmark actual callback cost; 10 instances below 15% on an i5 is a target,
  not a verified claim without that physical machine and Studio One.
* Native Windows verification: automation freeze, manual edits, gate learning,
  pitch tolerance/vibrato, state recall, banks, licence rejection, silence wake,
  UI control count, DLL loading, installer coexistence and uninstall.

V11 has a distinct VST3 class ID and installer registration. V10 remains
available and installed sessions keep their previous product.
