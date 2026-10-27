# M7G ALIEN VOICE V10 — TRIPLE GOD ORIGIN

Cabina vocal VST3 para Windows 10/11 x64, C++20 y JUCE 8.0.6. Interfaz de nave espacial negra/dorada/cian, iconos alien, tipografía graffiti y lema **LAS MAKKINAS PRODUCEN SOLAS**. Banco de **280 presets hechos por Makkine Seven Golden**: 230 de producción vocal y 50 de efectos clásicos/futuristas.

[Instalador Windows x64](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/v10.0.0/M7G-ALIEN-VOICE-V10-TRIPLE-GOD-ORIGIN-Windows-x64.exe) · [GitHub Releases](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/v10.0.0)

La publicación se condiciona a pruebas de audio, interfaz, instalación, carga del módulo instalado y desinstalación en Windows. Consultar `VALIDATION.md` para resultados, artefactos y límites. Se publica como pre-release mientras falten la prueba directa en Studio One 7 y la evaluación perceptual con grabaciones reales.

## Cabina y procesamiento

Las tres páginas muestran **Cabina Vocal**, **Alien Intelligence** y **Triple God**. Hay 26 módulos, análisis individual con icono alien, **Analizar todo**, automatización, A/B persistente, Mix, Input/Output y Bypass con latencia alineada. Cada módulo incluye una sección paralela: en reverb/delay/armonías controla el envío; en los demás mezcla ramas seca y procesada. El limitador conserva un techo final de seguridad cuando está activado, incluso con su mezcla reducida.

- Gate con low-cut, de-esser con Listen, presencia y Air con armónicos.
- Seis previos originales **M7G UNISON DRIVE**: Iron 73, Punch 512, Edge 4K, Air 737, Velvet Box y Dirty TG. Ganancia dependiente del perfil, carga estimada de fuente, respuesta filtrada, estado magnético y no linealidad. PAD reduce 20 dB antes del modelo. **48V y la impedancia son controles virtuales: no encienden phantom ni modifican eléctricamente una interfaz.**
- Compresión óptica y FET en serie, New York paralelo y Punch. Saturación de tres bandas con leyes Tube, Tape, Transistor, Transformer y Crush, y mezcla propia. EQ paralelo limpio/sucio.
- **EQ Sculptor** tonal amplio y acotado, con compensación de nivel de la curva. **Resonance Guard** usa máscara estéreo enlazada, persistencia temporal y protección de armónicos/transitorios. No identifica de forma infalible qué resonancias son musicalmente indeseables.
- Afinación monofónica **Natural/Robot**, selección automática o manual de tonalidad/escala y modificación de envolvente/formantes. El detector combina YIN, autocorrelación y una CNN de candidatos. La tonalidad inferida es tentativa: una voz aislada no define siempre la armonía de la canción.
- **Harmony God** genera ocho voces con vocoder de fase, envolvente cepstral, paneos y microafinación: octavas, quintas, coro y modo angelical. No separa notas de un coro de entrada ni incorpora el motor licenciado Elastique.
- **Triple Verb** ejecuta tres redes independientes simultáneamente: cámara corta A, espacio medio B y catedral larga C con shimmer. Mezcla, pre-delay, decay, filtro y modulación propios; GOD SPACE activa tres capas. El envío y los ajustes comunes de reverb también afectan a las capas.
- **Smart Delay**: Slap, Throw al final de frase, Ping-Pong, Tape Ducking y Ping-Pong → Reverb. Tempo del host o tiempo manual, tono, saturación y ducking. Slap usa su tiempo de 80–150 ms. Throw mide actividad vocal y espera la pausa ajustable antes de abrir el efecto. La sincronía usa 120 BPM si el host no informa un tempo válido.
- **Vocal Align** mide correlación de envolventes con una principal enviada al sidechain **Lead reference**, y ajusta hasta ±60 ms mediante lectura cruzada de retardos. Requiere referencia; no edita una línea temporal ni corrige frases arbitrariamente separadas. Su anticipación añade 80 ms compensados cuando se activa.

**BUS VOCAL** evita afinación monofónica, generación de armonías y salida neuronal sobre notas simultáneas.

## ALIEN AUTO PRODUCER / ESPECTACULAR

Macro 0–100 % automatizable. Analiza continuamente nivel, espectro y periodicidad; una MLP local combina objetivos de limpieza, color, dinámica y espacio con los ajustes manuales. Al 80–100 % activa GOD SPACE y una combinación de las tres reverbs con delay ducked. La selección de previo y tonalidad manual sigue disponible. La macro gestiona su nivel y compensa pérdidas de salida de forma gradual, con ganancia limitada y un objetivo aproximado de −18 dBFS RMS antes del trim de salida.

**Analizar todo** y los botones por módulo miden tres segundos de audio reproducido y aplican recomendaciones. Si no hay señal, se indica. La alineación necesita audio en el sidechain. Cambiar preset cancela el análisis. Los resultados se aplican también si se cierra el editor. En Auto Producer activo, los parámetros manuales son el punto de partida de la macro; Auto Producer a cero permite control manual completo.

No se promete una mezcla terminada de ingeniería universal ni un máster apto para Spotify a partir de cualquier grabación. La voz procesada sigue formando parte de una mezcla musical con otras pistas, niveles y decisiones artísticas.

## IA local y procedencia

- RNNoise v0.1.1, modelo neuronal de Xiph.Org, con sus pesos originales y avisos BSD.
- MLP propia 12 → 16 → 8, entrenada con objetivos de una política DSP acotada sobre características sintéticas. Error absoluto medio en 3000 ejemplos separados: 0.00386. Aprende esos objetivos técnicos; no gustos estéticos de ingenieros.
- CNN propia de evaluación de candidatos de pitch, entrenada con señales armónicas/ruido sintéticas. Exactitud balanceada en 3600 candidatos de otra semilla: 99.24 %. **Esta cifra no es precisión de afinación sobre cantantes reales.**

Pesos en `Source/V8ModelWeights.h`, entrenamiento reproducible en `Tools/train-v8-models.py`, métricas y límites en `Models/V8_METRICS.json`. El prefijo interno V8 conserva la procedencia del trabajo que evolucionó a V10. El binario no necesita Python, GPU, claves ni servicios de Internet. Los botones de análisis usan modelos pequeños y reglas/mediciones DSP; no son 26 modelos neuronales distintos.

El DSP es propio sobre JUCE y RNNoise permitido por su licencia. Las funciones de Neve/API/SSL/Avalon/Manley/Chandler, Melodyne, Antares, Saturn, Lexicon/Bricasti, Studer, Soothe, RX o C-Suite son referencias de requisitos, no motores incorporados ni emulaciones certificadas. No existe evidencia de superioridad universal frente a esas marcas. Las referencias a artistas en presets describen estilos, no participación, aprobación o clonación vocal.

## Modos y latencia

| Modo | Comportamiento |
|---|---|
| Monitor | Cero muestras declaradas y comprobables por impulso. Omite RNNoise de salida, de-reverb/espectro, pitch, armonías, alineación, lookahead y sobremuestreo. Conserva procesamiento causal y efectos de envío. |
| Studio | Etapas de color no lineales a 8× con reconstrucción IIR; ventanas espectrales y voces activas con compensación de latencia. |
| Linear | Reconstrucción FIR 8× y EQ espectral. «Fase lineal» se refiere a esos filtros; una cadena no lineal completa no es un filtro de fase lineal. |

Detener el transporte al cambiar modo o activar alineación. Studio/Linear son modos de mezcla, con anticipación apreciable; no son modos de monitorización sin retardo con toda la cadena activa. El DAW recibe el retardo para su compensación. Dry/wet, paralelo y bypass se alinean. El limitador controla picos de muestras: no es un limitador true-peak certificado.

## Instalar y usar en Studio One 7

1. Cerrar Studio One y ejecutar **M7G-ALIEN-VOICE-V10-TRIPLE-GOD-ORIGIN-Windows-x64.exe**.
2. Buscar **M7G ALIEN VOICE V10 TRIPLE GOD ORIGIN** en Efectos VST3. Si no aparece, restablecer bloqueados y volver a escanear los VST3 en las opciones de Studio One.
3. Elegir el preset según función y género, reproducir la voz y usar Analizar todo o Auto Producer. Comparar a nivel equivalente con A/B.
4. Para alinear dobles/armonías: insertar V10 en esa pista, activar su sidechain, enviar la principal a **Lead reference**, activar Vocal Align y analizar reproduciendo ambas pistas. El envío de referencia no es una salida audible del plugin.
5. Para grabar, usar Monitor; para mezclar con la cadena completa, usar Studio/Linear y compensación del DAW.

Carpeta: `C:\Program Files\Common Files\VST3\M7G ALIEN VOICE V10 TRIPLE GOD ORIGIN.vst3`. V10 tiene identificador y registro de desinstalación propios y conserva la versión anterior. No se ha probado dentro de Studio One 7 en este entorno. Instalador sin firma digital.

## Compilar y validar

Windows: Visual Studio 2022 Desktop C++, CMake ≥3.22, Git y NSIS 3. Ejecutar `BuildInstaller.ps1`; compila VST3/Standalone, ejecuta CTest y empaqueta. NSIS usa bootstrap Unicode x86, exige Windows x64 y copia el módulo AMD64 a las carpetas/registro de 64 bits.

Linux/cloud: `bash Tools/setup-cloud.sh` y `bash Tools/build-windows-cloud.sh`. JUCE fijado a 8.0.6 (`51a8a6d7aeae7326956d747737ccf1575e61e209`). El SDK Microsoft se obtiene con xwin y el binario se compila con Clang/LLVM. El workflow `publish-v10-release.yml` usa MSVC en Windows nativo y publica únicamente después de las pruebas de instalación y carga.

Para reproducir entrenamiento: Python, NumPy y scikit-learn; `OPENBLAS_NUM_THREADS=2 OMP_NUM_THREADS=2 python3 Tools/train-v8-models.py`. No se requiere para compilar los pesos ya incluidos.

Revisar la licencia JUCE antes de distribuir un producto cerrado. Se incluyen avisos BSD de RNNoise y licencias de las tipografías DejaVu, Liberation y Rubik Wet Paint. El instalador también incluye la ficha de los modelos.
