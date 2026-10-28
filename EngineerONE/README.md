# Makkine Engineer ONE - Studio One Edition

Windows x64 VST3 + ARA 2. Automatic offline vocal correction using JUCE 7, adaptive YIN analysis, phrase-aware note targeting and Rubber Band R3 with formants preserved. Separate product/ID; Engineer 5X and the scratch plugins are preserved.

## Usar en Studio One 7

1. Cierra Studio One, ejecuta el instalador y vuelve a escanear VST3. Busca **Makkine Engineer ONE - Studio One Edition**. La licencia global M7G se incluye; se conserva una licencia existente.
2. En inserto VST3: pulsa **CAPTURAR**, reproduce la pista y pulsa **STOP** o detén el transporte. El plugin captura la entrada cruda, incluidos sus silencios. No se debe saltar ni hacer un loop durante una captura: un cambio de posición termina la toma antes del salto.
3. Pulsa el botón dorado **IA**. Detecta notas, frases, ataques, respiraciones y vibrato; estima la tonalidad y corrige automáticamente. **No necesitas elegir escala ni editar notas.** Cuando la tonalidad es ambigua, usa la cromática conservadora. Cada click parte de la captura original y crea una interpretación distinta en transiciones y movimiento humano, sin una desafinación fija aleatoria.
4. **ESCUCHAR**, **ORIGINAL / A-B**, **MUTE** y **STOP** permiten comparar dentro del plugin a través del motor de audio del DAW.
5. Arrastra la onda, una nota o el botón con mano **ARRASTRA ESTA LÍNEA A TU PROYECTO**. Los WAV reales se preparan en segundo plano: PCM de **24 bits, 48 kHz**, con metadato Broadcast WAV de posición cuando el host la informa. También están disponibles por clic derecho > Mostrar WAV.

### ARA y alineación

Usa el plugin como extensión ARA del evento, mediante la función de Studio One para editar audio con una extensión ARA (requiere una edición del DAW con soporte ARA). **CAPTURAR** lee directamente el evento asignado/seleccionado. Después de IA, el renderer reproduce la corrección **no destructivamente en el evento original**, en su posición exacta. El menú contextual y **APLICAR EN EVENTO ARA** vuelven a aplicar la versión actual; no borran ni sustituyen físicamente el evento o el archivo original.

ARA aporta tonalidad y tempo del contexto musical cuando Studio One los proporciona. En inserto normal se conserva la posición de muestras/BPM comunicada por el transporte; la tonalidad se estima desde el audio. El tempo no determina la afinación.

La línea completa conserva exactamente la duración original (redondeada a 48 kHz) y sus silencios; cada nota exportada lleva su propio comienzo en el timestamp BWF. **Un arrastre de archivo VST3 no puede obligar a Studio One a colocarlo en una posición absoluta.** Studio One decide la posición de drop. Usa la opción del DAW para colocar/mover el archivo al origen BWF, o ARA para procesar el evento en su sitio sin arrastrar. Si mueves/recortas el evento ARA después del análisis, vuelve a capturar y pulsa IA para actualizar la exportación; se bloquea el arrastre con el timestamp antiguo.

En inserto normal, **APLICAR EN PISTA** también puede reproducir la versión corregida durante el tramo de la captura, siguiendo la posición del host. Es procesamiento de salida del inserto, no una edición del arreglo.

### Retoques opcionales

Por defecto están cerrados. **RETOQUES OPCIONALES** permite seleccionar una nota y modificar pitch, tiempo, vibrato y volumen. En el roll: arrastre vertical = pitch, Shift + arrastre = tiempo, Alt = vibrato, Ctrl = gain. Con los retoques cerrados, arrastrar una nota exporta su WAV. La siguiente pulsación de IA vuelve a la captura original y crea otra interpretación automática.

## Alcance real

- Vocal monofónica mono/estéreo, 22.05–192 kHz; hasta 10 minutos o 180 MB de PCM de entrada. No separa acordes ni voces ya mezcladas.
- Análisis y Rubber Band en trabajador; captura SPSC con escritura a disco fuera del callback. Ningún modelo neuronal entrenado, servicio remoto ni API de terceros: «IA» designa el motor automático de análisis/decisión DSP, no un modelo neural generativo.
- Mantiene el vibrato y los ataques mediante máscaras; no promete afinación perfecta en toda grabación ni una interpretación musical objetivamente única. Revisa el resultado, sobre todo con ruido, distorsión o escalas ambiguas.
- Corrección **offline tras capturar**, no afinador de entrada en tiempo real. Playback precomputado, latencia añadida reportada de 0 muestras; la latencia de grabación/monitorización la controla el DAW.
- ARA archiva original, audio corregido y retoques dentro de su documento. Un inserto VST3 normal guarda la ruta de la captura persistente y regenera la interpretación al abrir: al trasladar el proyecto a otro equipo, copia también la captura o consolida el resultado en el DAW.
- WAV y capturas persisten bajo `%APPDATA%\M7G\EngineerONE`; no se eliminan al cerrar el plugin ni al desinstalar, porque el DAW puede referenciarlos. Puedes archivarlos/limpiarlos después de consolidar el proyecto.

## Construcción reproducible

Visual Studio 2022 Desktop C++, CMake >=3.22, Python 3, NSIS. Ejecuta `EngineerONE/Build-Windows.ps1`. JUCE 7.0.12 y ARA SDK 2.2.0 están fijados a commits; el archivo Source.zip incluye sus fuentes para compilar sin descargarlos. Rubber Band 3.3.0 está incluido. CTest ejecuta DSP/captura/estado, interfaz, host ARA simulado con archivo/restauración y carga del VST3 real.

Licencias: GPLv3-or-later; ver COPYING y NOTICES.md. No afiliado ni certificado por PreSonus, Celemony o Antares. La validación con host ARA simulado no equivale a una prueba manual de Studio One 7.
