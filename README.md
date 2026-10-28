# M7G — plugins de voz y efectos

## M7G Fresh Vocal Pro Beast — mezcla vocal / VST3 + ARA

[Instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/fresh-vocal-pro-v1.0.0/M7G-Fresh-Vocal-Pro-Beast-Windows-x64.exe) · [VST3 portable](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/fresh-vocal-pro-v1.0.0/M7G-Fresh-Vocal-Pro-Beast-Windows-VST3.zip) · [Proyecto completo con .jucer](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/fresh-vocal-pro-v1.0.0/M7G-Fresh-Vocal-Pro-Beast-Source.zip)

Dynamics, EQ de ocho bandas, saturación 4× y espacio paralelo con reverb/delay y
ducking. 35 presets, tres vistas, A/B, XML, undo/redo y orden de módulos arrastrable
en Expert. JUCE 7, C++17, OpenGL y ARA con parámetros por evento. Plugin de mezcla;
el preset **IA Control Vocal** utiliza el DSP automático y no incluye afinación.

[Manual](M7G_FreshVocalPro/README.md) · [Validación](M7G_FreshVocalPro/VALIDATION.md) · [Release, capturas y demos](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/fresh-vocal-pro-v1.0.0)

La apariencia sigue el texto; faltan las fotos y el video para cotejarla exactamente.
Prueba directa de Studio One 7/GPU pendiente. Los productos anteriores se conservan.

## Makkine Engineer ONE — Studio One Edition / VST3 + ARA

[Instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer-one-v1.0.0/Makkine-Engineer-ONE-Studio-One-Edition-Windows-x64.exe) · [VST3 portable](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer-one-v1.0.0/Makkine-Engineer-ONE-Windows-VST3.zip) · [Fuente completa](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer-one-v1.0.0/Makkine-Engineer-ONE-Studio-One-Edition-Source.zip)

**CAPTURA → IA afina sola → ESCUCHA → ARRASTRA.** Cada pulsación de IA genera
otra interpretación natural desde la captura original. Análisis YIN, tonalidad,
frases, vibrato y ataques; Rubber Band R3 con formantes. Los retoques de notas
son opcionales. Preview interno y WAV PCM24/48k con duración/silencios y origen BWF.

ARA procesa el evento en su posición original y guarda el audio en el proyecto.
El arrastre de WAV depende de la posición que permita Studio One; un VST3 no puede
forzarla ni borrar/reemplazar eventos del arreglo. Motor automático DSP local;
no incorpora una red neuronal entrenada. Validación ARA con host simulado;
comprobación directa en Studio One 7 pendiente.

[Manual y alcance](EngineerONE/README.md) · [Validación](EngineerONE/VALIDATION.md) · [Release y demos](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/engineer-one-v1.0.0)

## Makkine Engineer 5X — AI Vocal Tuner Evolution

[Instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-Windows-x64.exe) · [VST3 portable Windows](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-Windows-VST3.zip) · [AU/VST3 Mac universal](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-macOS-universal.zip) · [Código completo](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-Source.zip)

Carga una vocal y genera cinco versiones: CENTERED, NATURAL DRIFT, PHRASE LEAD,
VIBRATO KEEPER y TRANSIENT SAFE. Cinco columnas con pitch original gris y resultado
medido en color, escucha/solo/mute y drag WAV. JUCE 7, YIN adaptativo y Rubber Band
R3 con formantes. Procesamiento local por archivo, independiente de los demás plugins.
Tres suites Windows, dos Mac, instalador y auval aprobados; descargas verificadas.

Los WAV float32 conservan duración, silencios, frecuencia/canales y origen BWF
cuando existe. Colócalos en el mismo inicio del evento original. No corrige en vivo
la entrada del DAW; esa entrada pasa sin cambios cuando no escuchas un render.
Escucha con voces reales y prueba directa en Studio One 7 pendientes.

[Manual](Engineer5X/README.md) · [Validación](Engineer5X/VALIDATION.md) · [Demos y captura real](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/engineer5x-v1.0.0)

## WORD SCRATCHER PRO - 300 — scratch real de palabras

[Instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/word-scratcher-v1.0.0/WORD-SCRATCHER-PRO-300-Windows-x64.exe) · [Proyecto completo](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/word-scratcher-v1.0.0/WORD-SCRATCHER-PRO-300-Source.zip)

300 presets en diez categorías, captura real de cinco segundos, física de vinilo,
reversa, ocho cues y crossfader. VINYL sigue la velocidad; DIGITAL conserva el
tono con Rubber Band incluido. Captura automática, aprendizaje de ruido y ducking
inteligente. Producto independiente con la licencia global M7G.
Tres suites nativas e instalación/reinstalación/desinstalación aprobadas en Windows.

[Manual](WordScratcher/README.md) · [Validación](WordScratcher/VALIDATION.md) · [Demos, capturas y checksums](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/word-scratcher-v1.0.0)

**88 ms a 48 kHz**: para mezcla/reproducción con compensación del DAW.
Evaluación directa en Studio One 7 y escucha con grabaciones reales pendientes.
Código completo GPLv3-o-posterior con JUCE 7 y Rubber Band; paquetes sin firma
de distribuidor.

## M7G ALIEN TUNE GOD V1 — afinación y armonías

[Descargar instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/tune-v1.0.0/M7G-ALIEN-TUNE-GOD-V1-Windows-x64.exe) · [Manual](Tune/README.md) · [Validación y CPU](Tune/VALIDATION.md)

Producto independiente: corrección natural/robot, cuatro armonías, dos dobles
derivados de la toma, efectos por voz y 100 presets. ONNX local y licencia global.
Pruebas nativas e instalador aprobados; descarga y SHA-256 verificados. A 48 kHz,
Estudio 42.67 ms / Monitor 10.67 ms; no afinación de latencia cero. Objetivos de
CPU en portátil, escucha con cantantes reales y Studio One 7 pendientes.

## M7G ALIEN TRANSITIONS GOD V1 — 150 FX

[Descargar instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/transitions-v1.0.0/M7G-ALIEN-TRANSITIONS-GOD-V1-Windows-x64.exe) · [Manual](Transitions/README.md) · [Validación](Transitions/VALIDATION.md)

Producto independiente: INTRO, CAMBIO y SALIDA, 50 presets por banco y cuatro
perillas. Conserva Alien Voice y comparte su licencia global. Instalador y tres
suites aprobados en Windows; descarga pública y SHA-256 verificados.
**Anticipación fija de 2 segundos:** usar compensación de latencia del DAW en
mezcla/reproducción. Evaluación directa en Studio One 7 pendiente.

## M7G ALIEN VOICE V11 — ALIEN INTELLIGENCE

**LAS MAKKINAS PRODUCEN SOLAS.** Presets hechos por **Makkine Seven Golden**.

Windows x64 VST3 vocal processor: 26 modules, 360 presets, local processing,
blue/gold spaceship cockpit, simple Spanish help. Builds with C++20/JUCE 8.0.6.

[Descargar V11 Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/v11.0.0/M7G-ALIEN-VOICE-V11-ALIEN-INTELLIGENCE-Windows-x64.exe)

Native Windows validation: **six suites passed**, installation/reinstallation/
uninstallation passed; public direct download and checksum verified. Engineering
pre-release; actual Studio One 7 and physical-i5 CPU validation remain pending.

[Ver cabina](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/v11.0.0/M7G-V11-interface.png) · [Consumo medido](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/v11.0.0/V11-performance.txt) · [All releases](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases)

## Using V11

1. Close Studio One 7, install the Windows `.exe`, reopen and rescan VST3.
2. Choose a workflow bank and preset. **AUTO | ESPECTACULAR** starts in AUTO.
3. Play your recorded voice. AUTO adapts internal settings continuously; optional
   **ANALIZAR TODO** measures three seconds for an initial adjustment.
4. Switch **ALIEN AUTOMATION** to **MANUAL** to freeze effective settings and
   edit normally. Processing continues. **BYPASS** is a separate control.
5. **LEARN 2s**: play only the room noise for two seconds without singing.
   The measured noise profile persists in the session. Repeat if rejected.

Each module has at most two main knobs and one parallel mix slider. AUTO's
moving knobs display effective internal settings, not host automation lanes.
Manual/host values are base settings while AUTO is on. On switching to MANUAL,
the current effective values and makeup are captured and normal edits resume.
Routing, musical intent, enable switches and algorithm choices remain yours.

Six banks: **Voces principales, Dobles, Refuerzos, Adlibs, Armonias,
Efectos especiales**. Search filters the selected bank. Previous/next/random
use that list. **FAV** saves the current preset; **VER FAV** filters favourites.
Favourites persist under the user's M7G application-data directory. A/B is
stored with the session; the menu copies the current side to the other side.

**Pitch:** adjustable correction and KEEP HUMAN, stable note hysteresis,
15-cent dead band, slow centre tracking with preservation of faster vibrato,
streaming phase-vocoder with cepstral formant compensation. This is an original
monophonic engine, not Melodyne DNA or Elastique. Polyphonic buses avoid lead
correction/generated harmonies. No guarantee of flawless automatic key inference.

**Space:** three simultaneous FDN reverbs, per-layer wet sends, smart delay with
tempo sync and ducking/phrase throw. Layer power shares the master reverb enable;
use layer mix to mute independently. GOD SPACE imposes minimum layer sends.
For fully manual layer settings, choose MANUAL and turn GOD SPACE off.

**Alignment:** connect the principal vocal to the optional **Lead reference**
sidechain. Finite envelope/onset matching with 80 ms lookahead; no reference
means no automatic alignment. It is not an offline word-editing system.

## Licence

One RSA-signed **M7G.license** grants all modules permanently. The installer
provides a valid signed distribution licence in `%ProgramData%\M7G` and preserves
existing files. Import another signed file with the authorization button; it is
stored in the current user's application-data M7G folder. A valid signature is
required before **AUTHORIZED: M7G GOLDEN** appears. Missing/invalid licences pass
aligned dry audio, with no beeps, silence or per-module locks.

This release includes a distribution key; it is not a customer sales/account
service. The private signing key is outside the repository/binaries. The owner
can issue more licences with `Tools/issue-license.py --private <private.pem>
--owner <name> --output M7G.license`. Keep the same private key and preserve it
securely. Public signature verification runs at load/import, never in audio.
The licence file does not supersede JUCE or third-party licence obligations.

## CPU and latency

AUTO uses bounded, smoothed DSP rules. The small auto-mix MLP runs on the 20 Hz
control timer (inference twice a second), including when the editor is closed.
Pitch uses YIN/autocorrelation plus the existing small candidate CNN. RNNoise
is local. No large language/generative model, Python runtime, GPU or cloud API
is required to use the installed plug-in.

Heavy processing suspends after input below -60 dBFS and output tails below
-90 dBFS have stayed quiet. It resumes on input and never suspends a lesson or
analysis. Very quiet signals below -60 dBFS can be treated as silence. Monitor
omits neural output, spectral/pitch/harmony/align windows, oversampling and
lookahead. Studio runs the full 8x chain; Linear uses FIR reconstruction.
Stop transport before changing mode. Full-chain latency is about 150 ms at
48 kHz, plus 80 ms with alignment. The DAW must compensate during playback.
This is not zero-latency full-chain correction.

The interface uses JUCE software rendering: 60 Hz with audio, 15 Hz while idle,
visible modules only. Animations reflect measured signals. A 10-instance native
Windows benchmark is attached to the release; **under 15% CPU on a physical i5
is not certified**. CPU percentages depend on hardware, mode, preset, buffer,
sample rate, active harmonies and open editors.

## Build and validation

On Windows with Visual Studio 2022 C++ tools, CMake, Git and NSIS installed:

```powershell
./BuildInstaller.ps1
./Tests/InstallerSmoke.ps1
```

CMake fetch is explicit: the build script clones pinned JUCE 8.0.6 if absent.
RNNoise source/model are included (BSD). Embedded fonts retain their licences.
CMake produces VST3, standalone and tests. CI compiles x64, runs six suites,
loads the actual VST3 DLL, tests installation/coexistence/uninstall, renders
three cockpit pages, benchmarks ten instances and publishes a pre-release.
`Docs/V11_ENGINEERING.md` and `VALIDATION.md` describe scope and results.

V11 has a new class ID and coexists with V10/0.3/earlier products. The installer
is unsigned. Studio One 7 itself and real-vocal perceptual quality need direct
evaluation; automated signal tests do not prove the best sound in the world.
48V, impedance and analogue topologies are digital models, not interface
hardware control or calibrated replicas. Limiting is sample-peak, not certified
true-peak. Original models were trained on synthetic data: see
`Models/V8_METRICS.json`; those metrics are not real-singer accuracy claims.

JUCE is dual-licensed AGPLv3/commercial. Review the applicable JUCE licence before
closed-source commercial distribution. No new licence for the user's own code
is implied. No proprietary UAD/iZotope/Antares/Elastique/other engines are copied.
