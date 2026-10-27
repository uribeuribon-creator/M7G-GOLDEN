# M7G ALIEN TRANSITIONS GOD V1

JUCE 7.0.12 (4f43011b96eb0636104cb3e433894cda98243626), C++20,
Windows x64 VST3. Independent product and class ID; Alien Voice remains intact.

## Timing and causality

Pre-vocal effects require lookahead. A two-second circular capture buffer and
fixed two-second host-reported latency make INTRO audible 200–1500 ms before
the delayed clean phrase. DAW plug-in delay compensation aligns playback.
This is a mixing/playback effect, not a zero-latency recording monitor.

The detector uses short RMS frames, threshold hysteresis and a 250 ms gap.
CAMBIO requires at least 300 ms of silence. SALIDA is scheduled retrospectively
over the final 400–1500 ms of the phrase; lookahead provides the necessary
margin. Only one selected bank is active. Transport jumps clear obsolete
events and delay memory without allocating in the audio callback.

## Signal processing

150 explicit recipes, 50 in each requested bank. Original combinations of
reverse capture, anti-causal reflection taps, granular pitch/time remapping,
cepstral formant transposition, tape speed/wow, tempo-locked slicing/repeats,
filtered feedback delay, ping-pong, modulated FDN reverb/shimmer, chorus,
flanger, allpass phaser, distortion, bit/sample reduction, stereo/pan,
amplitude/ring modulation and envelope-derived breath effects. No proprietary
engines or sonic equivalence claims.

Bounded preallocated event pool and buffers. Quiet periods keep only capture,
detection and dry delay active; effect voices suspend after their tails end.
Feedback stays bounded; “infinite” recipes hold until the next phrase/STOP FX,
with a 30-second safety timeout. Existing clean audio is preserved outside
transition windows. AMOUNT and MIX zero produce exactly latency-aligned dry.

Four knobs: amount, duration, mix and word sensitivity. Timing uses sample
counters; BPM comes from AudioPlayHead (120 BPM fallback). Grain edges and
event edges crossfade. UI waveform colours represent measured output activity.

## Shared authorisation

Use the existing signed perpetual ALL-modules M7G.license/public verification
key. No signing key in the product. One file authorises both products; install
and uninstall preserve existing authorisation. Invalid imports retain an
existing valid authorisation. No network/audio upload is required.

## Verification and limits

Test detection/gaps, pre-voice INTRO and tail/bridge timing, DAW BPM subdivisions,
all 150 distinct recipes at multiple rates/block sizes, finite bounded audio,
dry/bypass latency, save/restore/A-B/favourites, quiet suspension, seeking,
zero-length/oversized blocks, licence validation, exactly four UI knobs,
factory DLL loading, native installation/reinstallation/coexistence/uninstall.
Publish native screenshots, benchmark, checksum and an audible effect reel.
Studio One 7 itself and real-vocal listening need direct evaluation; neither
comparative superiority nor hardware-specific CPU percentages are assumed.
