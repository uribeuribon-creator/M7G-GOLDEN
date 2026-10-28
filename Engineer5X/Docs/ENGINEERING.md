# Engineering record

Independent JUCE 7 C++20 file-based vocal processor. Manufacturer M7G7, plugin
code E5x1, bundle audio.makkine.engineer5x. No scratch algorithm or live tuner
chain. One mono/stereo file produces five persistent float32 WAVs.

Analysis: anti-phase-safe mono analysis, fourth-order antialias lowpass and
decimation near 12 kHz; 5 ms hop, centred adaptive 4-period/256–1024 sample YIN
window, zero-padded FFT autocorrelation, cumulative mean normalised difference,
normalised correlation confidence, interpolated lag and voiced continuity.
Energy/noise/air/onset masks protect unpitched audio. Confident voiced gaps of
240 ms segment musical phrases; breath cannot extend a phrase. Stable-region
centres and local trends distinguish fixed note deviation from 3–10 Hz vibrato.
Targets are chromatic or a selected major/minor scale, not inferred intent from
an unidentified generative model.

Five different control plans: full note-error centring; hard 8-cent permission
with gradual correction outside; robust phrase-wide offset; slow-centre-only
correction; attack/air/breath-safe stable correction. Symmetric smoothing avoids
delaying control envelopes. The V1 result is measured; one bounded trial residual
refinement is accepted only when it improves the same eligible voiced frames.
Every candidate is rendered from original PCM rather than cascading resynthesis.

Rubber Band 3.3.0 R3, formants preserved, linked stereo, high-consistency pitch,
no internal threading, time ratio exactly 1.0. It runs on a cancellable background
worker in streaming mode so its pitch ratio can vary (offline mode fixes pitch).
Preferred start padding and returned start delay compensate the audio clock.
The full FFT-source-window lookahead advances dynamic controls separately, so
fast pitch changes affect the matching original source time. End flushing is
explicit. Output has exactly N original frames; unvoiced/protected areas blend
to their exact original samples. No noise gate chopping, normalization, creative
hard tune, saturation or limiter is baked into the WAV.

Preview runs only interpolated reads of immutable, precomputed PCM at host rate;
mono hosts downmix stereo and stereo hosts duplicate mono. Source/version switches
and mute use short fades. No render, FFT, filesystem I/O, allocation or ownership
destruction is required by the audio callback. A raw atomic publication with a
hazard pointer and monotonically increasing serial protects project lifetimes;
UI/background retain owning references. File cancellation and new loads use a
generation counter. APVTS changes debounce into background rerenders; the editor
is not required for analysis. UI graphs cache waveform/pitch drawing at 20 Hz.

Exports retain rate/channels/leading and trailing silence and original BWF origin.
Dragging transfers a real persistent WAV path; the DAW chooses the drop position.
State saves the original pathname and parameters rather than embedding personal
audio. Missing source/invalid media produce visible errors. Existing products and
global signed M7G licence remain independent.

Windows release is built from its extracted complete Source archive and gated on
native DSP/UI/VST3/install/reinstall/coexistence/uninstall tests. Mac builds both
architectures and runs native CTest/auval before attaching AU/VST3. Pinned JUCE 7
is copied to the build directory for one documented NSView snapshot compatibility
patch on recent SDKs; the original dependency and audio DSP are untouched.

Distribution: GPLv3-or-later with full corresponding source, JUCE/Rubber Band and
font notices. Synthetic demo only, no claims of universal perfect tuning, artist
voice generation, proprietary algorithm equivalence or guaranteed laptop CPU.
