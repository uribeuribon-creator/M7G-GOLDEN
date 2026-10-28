# WORD SCRATCHER PRO - 300

**300 presets · 10 categorias · Makkine Seven Golden**

[Instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/word-scratcher-v1.0.0/WORD-SCRATCHER-PRO-300-Windows-x64.exe) · [Codigo completo con dependencias](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/word-scratcher-v1.0.0/WORD-SCRATCHER-PRO-300-Source.zip) · [Validacion](VALIDATION.md)

Scratch real para palabras/audio, con JUCE 7.0.12 y Rubber Band 3.3.0 incluido
en codigo fuente. Entrega VST3 Windows x64. Proyecto independiente:
conserva Alien Voice, Transitions y Tune. Misma licencia global M7G incluida.

## Empezar

1. Inserta el plugin en una pista vocal y elige un preset. **AUTO SCRATCH** viene
   activado: reproduce una voz; al reunir 80 ms de una palabra nueva inicia el
   scratch de ese audio. Al terminar y detectar la pausa captura el fragmento completo.
2. La onda dorada es el fragmento capturado. El vinilo y su aguja representan
   la posicion real de lectura. Arrastra el disco para scratch manual.
3. CAPTURAR guarda la ultima palabra o fragmento; SCRATCH reproduce el gesto del
   preset; STOP desvanece el bus de efectos. HOLD permite repetir hasta otra
   captura/STOP o un limite de seguridad de 15 segundos.
4. VINYL cambia tono segun velocidad y sentido; DIGITAL usa time-stretch real
   para conservar el tono. La velocidad digital util esta limitada a 0.25–4x;
   cerca de la parada se desvanece. La reversa sigue siendo audio leido al reves.
5. Ocho hot cues saltan a posiciones del audio. Shift + click guarda la posicion
   actual de un cue. Se guardan con el preset/sesion.
6. CROSSFADER corta el scratch y su eco. CUT es rapido, SMOOTH suave y HAMSTER
   invierte los extremos. CORTE silencia ese bus. AUTO FADER aplica los cortes
   del gesto; desactivalo para manejar solo el fader manual.
7. MEZCLA cero conserva la voz limpia. INTENSIDAD cambia la fuerza; AGITE mueve
   el gesto mas rapido; TENSION, INERCIA, FRICCION y MOTOR cambian fisica real.
   RANGO limita velocidad; RUIDO/POLVO/LO-FI y ECO son procesamiento audible.
   MAS CONTROLES expone cortes, drive, recorrido, duracion, salida y variacion.
8. INTELIGENTE adapta la fuerza al tamano del fragmento y baja el efecto y restaura voz seca sobre una nueva entrada vocal.
   Desactivalo para scratch completamente al frente. Para una comparacion aislada,
   usa MEZCLA 100, INTENSIDAD 100 y reproduce palabras con espacios.
9. APRENDER RUIDO escucha un segundo de solo ruido de cuarto. SENSIBILIDAD y
   SILENCIO ENTRE FRAGMENTOS ajustan la captura. No se reconoce el significado:
   es segmentacion por energia, transitorios y pausas. Sonidos muy cortos o
   insuficientes se rechazan. La memoria es de cinco segundos.

Busca por nombre/categoria en el ComboBox agrupado. < > y RANDOM recorren el
resultado; FAV persiste entre sesiones. A/B guarda estados de parametros, y copiar
transfiere el lado activo al otro. Los knobs animan su movimiento hacia los
valores realmente aplicados. La descripcion y el XML acreditan Makkine Seven Golden.

## Sonido y latencia

Cuatro puntos Lagrange interpolan el audio, incluidas lecturas negativas. Capturas
filtradas a varias bandas reducen aliasing a alta velocidad. La fisica integra
error de posicion/friccion, inercia y torque en pasos de 1 ms. Chirps,
transformers, flares/crabs, chops, baby, tears/orbits, drags y releases generan
trayectorias y cortes diferentes; no son nombres sobre el mismo efecto vacio.

DIGITAL alimenta Rubber Band con muestras espaciadas a velocidad de origen y
varia el tiempo de salida. La entrada invertida permite reverse con tono retenido.
Un FIFO acotado y un ajuste pequeno de tiempo estabilizan el flujo; movimientos
rapidos pueden producir artefactos creativos, no una voz perfecta garantizada.
VINYL usa directamente el lector de velocidad variable. Ruido/crujidos se generan
solo con una captura valida y se cortan con el bus scratch. Eco filtrado con
feedback limitado; salida con proteccion de picos de muestra, no true-peak.

**Latencia a 48 kHz: 4224 muestras / 88 ms**, en ambos modos, para alinear voz
seca, vinilo y time-stretch. A 44.1 kHz: 3968 / 89.98 ms; a 96 kHz: 8320 / 86.67 ms.
El DAW debe compensar durante mezcla/reproduccion. No es un sistema de monitoreo
DJ de latencia cero. Permite preroll despues de iniciar o mover la reproduccion.
El primer gesto usa los primeros 80 ms de audio nuevo, mas la latencia declarada.
El gesto del fragmento completo se oye despues de su pausa de deteccion;
los nombres Intro/Outro/Middle se refieren al recorrido/cue, no a predecir palabras.

El fader silencia el bus procesado; la voz seca puede seguir segun MEZCLA,
INTELIGENTE, el estado de movimiento y BYPASS. El audio y el perfil de ruido no se
suben a internet. Las capturas son memoria temporal: no se incrustan grabaciones
personales en el estado XML. Al reabrir una sesion, reproduce para recapturar.

## Instalacion y compilacion

Windows 10/11 x64: cierra Studio One, ejecuta el .exe y vuelve a escanear VST3.
Instala `WORD SCRATCHER PRO - 300.vst3` en la carpeta comun VST3.
El codigo incluye objetivos AU/VST3 para compilar en macOS 11+ con SDK 14;
no se publica un binario Mac validado en esta entrega. Copia `.component` a
`~/Library/Audio/Plug-Ins/Components` y `.vst3` a `~/Library/Audio/Plug-Ins/VST3`.
La licencia va en `~/Library/M7G/M7G.license`; se conserva la existente.
El instalador Windows no tiene firma de distribuidor. JUCE 7 fijado necesita
un SDK Mac compatible: SDK 15 retiro una API de captura de pantalla del framework.

El archivo Source.zip incluye Source, Rubber Band, JUCE 7, fuentes, licencia y
scripts. Prerrequisitos: CMake, Python 3 para empaquetar, Git si falta JUCE, VS 2022
C++/NSIS en Windows; Xcode C++ con SDK 14/Ninja en Mac. Desde la raiz del proyecto:

```powershell
./WordScratcher/Build-Windows.ps1
```

```bash
bash WordScratcher/Build-macOS.sh
```

CMake directo: `cmake -S WordScratcher -B build -DCMAKE_BUILD_TYPE=Release`, seguido
de build/CTest. JUCE fijado a `4f43011b96eb0636104cb3e433894cda98243626` y Rubber Band
a `2be46b0dffb13273a67396c77bc9278736bb03d2`; no dependencias DSP descargadas al abrir.

**Licencias:** el subproyecto se distribuye por la via GPLv3-o-posterior, compatible
con JUCE 7 y Rubber Band GPL-2.0-or-later; COPYING/notices incluidos. Para una
edicion propietaria cerrada hacen falta las licencias comerciales correspondientes.
La activacion global M7G no concede licencias de terceros ni cambia otros productos.

## Verificacion y limites

Pruebas de interpolacion, ring de cinco segundos, movimiento/reversa reales,
relacion velocidad/tono de VINYL y retencion de tono DIGITAL, 300 renders distintos
y sustanciales, fader/eco/cues, rechazo de ruido aprendido, estado/A-B/favoritos,
latencia/bypass/rates/seek/buffers, UI real, carga VST3 e instalador Windows. AU/auval Mac pendiente.
La publicacion incluye capturas, CPU medida, checksum y demos de senal vocal y
habla sinteticas, claramente identificadas: no son grabaciones de cantantes.
Escucha con grabaciones del usuario y prueba directa en Studio One quedan
pendientes. No se afirma superioridad a productos comerciales ni CPU garantizada
en un portatil concreto. Construccion de ingenieria para evaluacion musical.
