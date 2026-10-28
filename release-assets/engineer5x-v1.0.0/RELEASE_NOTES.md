# Makkine Engineer 5X - AI Vocal Tuner Evolution

Load one monophonic vocal WAV; automatically generate five philosophies:
CENTERED, NATURAL DRIFT (8-cent margin), PHRASE LEAD, VIBRATO KEEPER and
TRANSIENT SAFE. Not a live hard-tune effect or voice generator. No scratch DSP.

JUCE 7, original confidence-gated YIN analysis and phrase/attack/noise/vibrato
decisions; Rubber Band R3 with formant preservation. Background generation, five
columns of original-grey / actual-render pitch, internal play/solo/mute, original
comparison, zoom/seek, WAV drag or save. Live DAW input passes unchanged except
while previewing a file; offline rendering never runs in the audio callback.

32-bit float WAVs retain exact source length, leading/trailing silence, rate and
channel count. Rubber Band start latency is compensated. Original BWF time
reference is retained when supplied. Drop at the original event start: a plugin
cannot force the DAW cursor/drop location or infer absent event metadata.

Windows x64 installer and portable VST3 bundle, global signed distribution licence,
full offline-buildable source with JUCE/Rubber Band, real screenshot and five
clearly labelled synthetic vocal examples. Native DSP/UI/VST3/installer tests gate
publication. No private keys, artist voices or prerecorded scratch samples.

A single vocal, no instrumental/polyphonic bus. Target-note/phrase intent is
estimated, not infallible; perfectly centred targets are not a guarantee of zero
error in all recordings. Monophonic user recordings and Studio One 7 listening
remain separate evaluation. Unsigned Windows installer. GPLv3-or-later distribution;
commercial JUCE/Rubber Band licences needed for proprietary closed-source editions.

Mac universal AU/VST3 builds use a documented build-local JUCE 7 SDK compatibility
patch. Mac bundles are attached only after native tests and auval; ad-hoc signatures
are not Apple notarisation. Intel native execution and both binary slices verified.
