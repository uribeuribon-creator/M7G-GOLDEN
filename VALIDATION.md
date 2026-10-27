# V11 ALIEN INTELLIGENCE validation

## Release gate

The Windows workflow builds C++20/JUCE 8.0.6 with MSVC x64, runs six CTest
suites, tests the actual installed VST3 DLL, checks four previous product
bundles survive installation, verifies the signed global licence survives
reinstallation/uninstallation, renders the compiled UI and records a native
ten-instance benchmark before publishing `v11.0.0` as a pre-release.

## Automated coverage

* **VocalDSP**: 54 sample-rate/block/role combinations, finite bounded output,
  silence/tails, RNNoise reduction, de-essing/gate, oversampled alias comparison,
  polyphonic bus preservation and monophonic tuning.
* **VST3Module**: actual DLL factory, new class identity, main stereo and
  optional disabled reference sidechain, initialization/process/unload.
* **AlienInterface**: 26 modules, no module above two main knobs, 55 total knobs,
  26 parallel sliders, 26 individual AI callbacks and global analysis; all 360
  presets distributed across six workflow banks; search, previous/next/random,
  persistent favourite toggling, AUTO/MANUAL attachment and separate bypass;
  four screenshots (default and three explicit pages).
* **AlienRack**: original 12-stage regression, 50-preset subset, module analysis,
  reverb/delay ducking and returning tails, A/B serialization.
* **TripleGodDSP**: all 360 distinct factory states rendered, finite/nonzero
  ceiling-limited output, latency/bypass in three modes, 8-voice harmony,
  triple reverb independent sums, delay modes/ducking/throw, reference alignment
  including phrase-end silence, additional analysis routes.
* **AlienIntelligence**: RSA signature and rights/owner tamper rejection;
  15-cent pitch tolerance, sustained 35-cent correction and expressive vibrato;
  live AUTO without host base writes, effective-value latch to MANUAL, no
  subsequent drift, normal manual editing and continued processing; two-second
  noise profile learning/rejection/recall; silence suspension and wake.

Early Windows-PE execution under Wine confirmed: valid signed licence; MANUAL
latch and no drift; learned floor -55.1 dBFS; 35-cent pitch-centre correction,
0.165-cent residual mean and 14.77-cent vibrato RMS in a synthetic fixture.
These are engineering checks, not singer/perceptual-quality ratings.

## Limits

No physical i5 performance certification, real-vocal comparative listening,
actual Studio One 7 run, true-peak certification or guaranteed inferred key.
The attached benchmark measures callback-only wall time for ten instances,
48 kHz / 256 samples, two seconds of harmonic input, closed editors; it reports
single-core equivalent utilization on the runner. It includes separate Monitor
and Studio/8x cases. Interpret hardware, thread scheduling and DAW percentages
separately. The requested ten instances under 15% CPU on an i5 remains a target.

No commercial proprietary engines or proven superiority to named products.
Monitor omits windowed pitch/spectral/neural output; full Studio/Linear latency
is reported to the host, with additional 80 ms when alignment is enabled.
The installer is unsigned. This build uses an included perpetual distribution
licence, not a sales backend or per-customer anti-copy service.
