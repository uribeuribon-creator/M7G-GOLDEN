# Source and distribution notices

This new WordScratcher subproject is supplied under GPL-3.0-or-later, using the
compatible GPL distribution route for JUCE 7 and Rubber Band. See COPYING.
Other directories/products keep their existing licensing; no new licence is
asserted for unrelated user code.

Rubber Band Library 3.3.0, Particular Programs Ltd: GPL-2.0-or-later or commercial.
Unmodified full upstream source (excluding Git metadata) is in ThirdParty/rubberband,
including COPYING, AUTHORS and upstream notices. The upstream single-file build
uses its builtin resampler/FFT (vDSP on Apple). Application config selects R2
real-time, short-window, smooth-transient, soft detector and independent phase.
See ThirdParty/PROVENANCE.json. Commercial non-GPL redistribution requires the
appropriate rights; M7G activation is independent of those terms.

JUCE 7.0.12, Raw Material Software: GPLv3 or commercial. Exact source is included
in the offline source archive with original notices. SDK/font notices are included
in installers/source. DejaVu and Rubik Wet Paint keep their original licences.

Original preset names follow the user's specification. Demo speech is generated
from original text by eSpeak NG, not human-recorded speech; see Tests/Assets/README.md.
Neither that fixture nor a canned scratch sound is embedded in the plugin. Every
production scratch reads the user's captured signal.

Distribution licence M7G.license is the existing public signed ALL/PERPETUAL file.
No private signing key, cloud service, telemetry or per-preset licence is included.
