# Especificación de ingeniería — M7G ALIEN VOICE V8 ULTIMATE AI GOD

Documento de requisitos, escrito antes de modificar el DSP de V8. Es una especificación pública de trabajo, no una afirmación de credenciales personales ni de superioridad sonora.

## Referencias funcionales y decisiones

| Referencia solicitada | Función que se toma como referencia | Implementación propia y alcance |
|---|---|---|
| Melodyne DNA | Edición de alturas polifónicas | V8 procesa afinación monofónica y genera ocho armonías; no implementa separación DNA ni edita notas independientes de un acorde. |
| Antares Throat Modeling | Cambios de envolvente/formantes | Envolvente cepstral, desplazamiento de envolvente y compensación en el armonizador. Sin modelo anatómico licenciado. |
| FabFilter Saturn | Saturación multibanda | Tres bandas, drive independiente y sobremuestreo 8× en los modos de estudio. |
| Lexicon 480L / Bricasti M7 | Reverberación algorítmica difusa | Red de retardos realimentados, difusión, amortiguación y modulación. Sin copiar programas, circuitos ni respuestas impulsionales comerciales. |
| UAD Studer | Memoria y no linealidad magnética | Coloración con estado e histéresis acotada; no es una emulación calibrada de un grabador concreto. |
| Soothe2 / Pro-Q 3 | Atenuación dinámica de resonancias | Detector espectral con envolvente local, reducción limitada, ataque/release y protección de transitorios. |
| RX Spectral Denoise | Reducción de ruido | RNNoise existente y control espectral propio; no es RX ni reproduce su red. |
| Neve / Avalon / SSL | Previo de transformador, válvula y consola | Tres topologías de color propias; sin atribuir equivalencia eléctrica o perceptual a productos concretos. |
| LA-2A / 1176 | Nivelación óptica y control rápido en serie | Seguidores de envolvente distintos, memoria óptica y etapa FET rápida; transformador dentro de las etapas sobremuestreadas. |
| MicroShift / EchoBoy / Capitol Chambers | Microafinación, ecos y espacio | Armonizador/microdetune propio, delay con ducking y reverberador de cámaras con shimmer. |
| Elastique | Transposición preservando formantes | Vocoder de fase y envolvente cepstral propios; no se incluye ni se anuncia el motor licenciado Elastique. |

Las marcas identifican referencias de funciones. No son dependencias, certificaciones ni colaboraciones. Una voz aislada no determina siempre la tonalidad de la canción y un solo plugin no garantiza un máster adecuado para Spotify.

## Arquitectura

Cadena objetivo: entrada y medición → denoise neuronal → reducción espectral de reverberación → corrección Natural/Robot → previo con estado → compresión óptica/FET en serie → EQ/resonancias dinámicas → saturación de tres bandas → de-esser → microdetune/estéreo → armonías → reverb con shimmer → delay con ducking → salida y limitador.

Se conservarán filtros lineales estables donde su función sea filtrado o reconstrucción. La distorsión armónica se introduce en etapas controlables y medibles, no en medidores, selectores ni ganancias de utilidad. Se preasignan buffers y planes FFT en prepareToPlay; no se realizan entrenamiento, red, escrituras a disco ni reserva de buffers en el callback de audio.

### Modelos locales

- Mantener RNNoise con sus pesos y licencia originales.
- Entrenar una red pequeña de autoajuste con rasgos RMS, crest, espectro y periodicidad. Los objetivos iniciales son una política DSP explícita sobre ejemplos sintéticos; documentar que esto no equivale a aprender preferencias estéticas de mezcladores profesionales.
- Entrenar una CNN pequeña para evaluar candidatos de altura sobre curvas de autocorrelación/YIN; combinar su puntuación con los detectores matemáticos, no sustituir una medición fiable por una predicción sin confianza.
- Separar semillas/conjuntos de entrenamiento y evaluación. Guardar scripts, pesos, métricas y ficha de datos. No anunciar una CNN si no existen pesos entrenados y evaluación independiente.

### Modos de procesamiento

- **Monitor:** latencia declarada y medida de cero muestras, etapas causales; desactiva las operaciones que necesitan acumular ventanas (RNNoise, de-reverb espectral, afinación y armonías) y el lookahead/sobremuestreo. La interfaz debe explicarlo.
- **Studio:** etapas no lineales con 8× y filtros de reconstrucción IIR; compensación de todos los retardos de ventanas, dry/wet y bypass.
- **Linear:** reconstrucción FIR de fase lineal y compensación de su retardo; conserva las etapas espectrales. La fase lineal se refiere a estos filtros, no a una cadena no lineal completa.

### ALIEN AUTO PRODUCER / ESPECTACULAR

Macro automatizable 0–100 %, análisis continuo de nivel, espectro y periodicidad y política neuronal local. Debe producir cambios audibles graduales, mantener límites de ganancia y permitir comparación de nivel. No realizar cambios de preset ni de latencia silenciosamente. La selección manual de tonalidad tiene prioridad.

### HARMONY GOD

Ocho voces reales con niveles/paneos, octavas, quintas, coro y modo angelical. Transposición mediante vocoder de fase, detección de frecuencia instantánea, síntesis overlap-add y compensación de envolvente cepstral. Desplazamiento de formantes y humanización controlables. No separar coros de entrada ni anunciar ocho voces si solo existe un chorus.

### Presets e interfaz

230 nombres y configuraciones distintos, organizados por Rap, Trap, Reggaeton, Dancehall, Afrobeat y R&B, y por función vocal. Las referencias a artistas indican estilos de producción, no participación, aprobación ni clonación de sus voces. Mantener estética negra/dorada y exponer todos los nuevos controles, valores y limitaciones de modo. Mantener estado, automatización, A/B y análisis por módulo/global.

## Puerta de publicación

1. Compilar C++20/JUCE 8.0.6 para Windows x64. Mantener V8 como producto/identificador separado de 0.3.0 para conservar sesiones y el instalador anterior.
2. Procesar los 230 presets y medir salida finita, estabilidad, techo, silencio, estéreo/mono y estado.
3. Medir fundamentales de las armonías, corrección de altura, efecto sobre formantes, reducción de resonancias, recuperación del ducking, cambios de topología y saturación multibanda.
4. Medir retardo de impulso y compararlo con el declarado en los tres modos; comprobar dry/wet y bypass alineados.
5. Ejecutar los modelos y registrar evaluación sintética separada; conservar explícita la validación pendiente con voces reales.
6. Compilar y ejecutar las pruebas en Windows nativo, instalar, cargar el VST3 instalado y desinstalar.
7. Publicar un release separado con instalador `M7G-ALIEN-VOICE-V8-ULTIMATE-AI-GOD-Windows-x64.exe`, checksum y captura nativa. Descargar sin autenticación y verificar el checksum. Studio One 7 se declara objetivo; no se declara probado sin ejecutar ese DAW.

La publicación no implica una certificación de «mejor del mundo», equivalencia UAD o calidad comercial universal. Las mediciones y las limitaciones de la versión deben acompañar al binario.
