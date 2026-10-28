# Makkine Engineer ONE - Studio One Edition / Windows VST3 + ARA

**CAPTURA → IA AFINA SOLA → ESCUCHA → ARRASTRA.** La edición de notas es opcional y está cerrada por defecto.

- Captura la entrada de la pista con posición/tempo del host, o lee el evento mediante ARA.
- Cada click en IA analiza la toma original y genera una interpretación natural distinta: YIN adaptativo, decisiones de notas y frase, tonalidad automática, máscaras de ataques/ruido/aire y Rubber Band R3 con preservación de formantes.
- Pitch original gris y pitch realmente medido del resultado dorado; escucha A/B, mute, retoques opcionales de pitch/tiempo/vibrato/gain.
- Arrastre externo de línea/nota mediante WAV real PCM24 / 48 kHz, duración y silencios conservados, timestamp Broadcast WAV del host.
- ARA aplica el audio al evento existente en su posición original y guarda original/corrección/retoques en su documento.
- Instalador, VST3 portable, fuentes completas con JUCE/Rubber Band/ARA, captura de la interfaz real, demostración sintética original/IA y SHA-256.

**Instala:** cierra Studio One, ejecuta el `.exe`, abre Studio One y escanea VST3. Busca **Makkine Engineer ONE - Studio One Edition**. Se instala en la carpeta VST3 común de Windows. Conserva los plugins anteriores y la licencia M7G existente.

**Límites de integración:** un drop de archivo VST3 no puede imponer su posición en el arreglo ni borrar/reemplazar eventos. El WAV lleva origen BWF; el DAW decide la posición del drop. Para corregir el evento directamente en su sitio, usa ARA. La integración ARA se valida con un host de prueba; aún requiere comprobación manual en Studio One 7 y una edición con soporte ARA. No es una integración certificada por PreSonus.

El botón IA es un motor automático DSP de análisis/decisión, sin un modelo neuronal entrenado ni servicios remotos. Es corrección offline después de capturar, no afinador de entrada en tiempo real ni garantía de perfección para cualquier grabación. Recomendado para una voz monofónica cruda. Ver README y validación incluidas para el alcance exacto.
