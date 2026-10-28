# WORD SCRATCHER PRO - 300 — validación

Código de producto: `94d105295290c9a801a64a140a89eed684a72129`.
JUCE 7.0.12: `4f43011b96eb0636104cb3e433894cda98243626`.
Rubber Band 3.3.0: `2be46b0dffb13273a67396c77bc9278736bb03d2`.

[Descarga Windows VST3](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/word-scratcher-v1.0.0/WORD-SCRATCHER-PRO-300-Windows-x64.exe) · [Release y evidencia](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/word-scratcher-v1.0.0) · [Ejecución nativa](https://github.com/uribeuribon-creator/M7G-GOLDEN/actions/runs/40717820413)

## Resultado nativo Windows

El build MSVC x64 y la publicación finalizaron correctamente el 28 de octubre
de 2026. **Tres suites aprobadas, cero fallos**: ScratchAudio, ScratchInterface
y ScratchVST3. El host de prueba reconoció el nombre, la identidad independiente,
los buses estéreo y el procesamiento del VST3.

Instalación, reinstalación, conservación de licencia y productos anteriores,
carga del VST3 instalado y desinstalación aprobadas en Windows. El instalador
incluye el bundle VST3 AMD64; su ejecutable NSIS de arranque puede ser PE32.

El inicio de captura del ensayo fue 194.5 ms para una palabra que empieza a
120 ms, y 1774.5 ms para la que empieza a 1700 ms, en ambos modos. El preroll del
detector explica esa diferencia; la salida añade la compensación declarada.
La diferencia de audio dentro de la primera palabra fue medible sin ruido/polvo:
238.993 VINYL / 141.34 DIGITAL en la métrica de energía del ensayo.

SHA-256 del instalador publicado:
`c03d913c8277ff8f4babd932aa8d1dc0402701f7a0b6aab22c3c8190868165a6`.

Las 22 descargas públicas respondieron HTTP 200 como archivos adjuntos. Todos
los hashes publicados coincidieron con los archivos descargados y los digests
de GitHub. El ZIP Source contiene 3784 entradas: se verificaron todos los hashes
de su manifiesto y su correspondencia con el commit construido. Incluye JUCE y
Rubber Band; no incluye claves privadas ni un proyecto de afinador.

## Rendimiento observado

Runner Windows, cuatro CPU lógicas, una instancia, editor cerrado, 48 kHz,
bloques de 256 muestras y cuatro segundos por caso. Tiempo de callback respecto
al audio reproducido; no porcentaje total del sistema ni garantía para portátiles.

| Modo | Entrada | Tiempo de callback/audio | Peor callback | Subdesbordamientos |
| --- | --- | ---: | ---: | ---: |
| VINYL | Silencio | 3.77% | 0.390 ms | 0 |
| VINYL | Vocal sintética | 5.77% | 0.893 ms | 0 |
| DIGITAL | Silencio | 3.77% | 0.359 ms | 0 |
| DIGITAL | Vocal sintética | 7.50% | 1.035 ms | 0 |

[Medición original](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/word-scratcher-v1.0.0/Word-Scratcher-300-Windows-performance.txt).

## DSP verificado

Las pruebas procesan audio, además de comprobar parámetros y nombres:

- Los **300 nombres exactos** generan **300 renders distintos**, todos finitos,
  con energía de efecto suficiente. Se comprueban también gestos representativos
  sin ruido ni crujidos para que la textura no oculte un efecto vacío.
- El lector Lagrange reproduce un polinomio cúbico con tolerancia de 1e-6.
  El ring conserva cinco segundos; movimiento positivo y negativo leen audio real.
- Con una señal de 196 Hz, VINYL produce 98 Hz a 0.5x y 392 Hz a 2x.
  DIGITAL conserva 196 Hz a 0.5x, 2x, -0.5x y -2x. Cero subdesbordamientos
  en estas pruebas; estimación con resolución de medio cent.
- Un fragmento digital corto de 80 ms sigue generando un efecto audible. El modo
  inteligente adapta la física a su duración y protege la siguiente entrada vocal.
- Una palabra entrante dispara una captura de sus propios primeros 80 ms y un
  scratch audible antes de terminar; no recicla el fragmento anterior. VINYL y
  DIGITAL se prueban por separado. Al detectar su final se actualiza la captura completa.
- CUT, SMOOTH, HAMSTER y CORTE abren/silencian el bus scratch, incluido el eco.
  El perfil aprendido rechaza el ruido de cuarto de la prueba.
- Estado XML, licencia global, cues, A/B, favoritos, silencio, cambios de posición,
  buffers vacíos/grandes y alineación de bypass/mezcla cero comprobados.
- La prueba de interfaz abre el editor real: búsqueda agrupada de 300 presets,
  15 knobs animados, panel avanzado, plato, fader, ocho cues, navegación y favoritos.
  Captura a 1440 × 940 y escalado a 1120 × 730 comprobados.

## Latencia y uso

| Frecuencia | Muestras notificadas al DAW | Tiempo |
| --- | ---: | ---: |
| 44.1 kHz | 3968 | 89.98 ms |
| 48 kHz | 4224 | 88 ms |
| 96 kHz | 8320 | 86.67 ms |

Ambos modos comparten la compensación de latencia. El primer gesto usa los primeros
80 ms capturados de la palabra nueva; el gesto completo espera su final y pausa.
No anticipa audio que aún no ha entrado. Se recomienda mezcla o
reproducción con compensación del DAW; no monitoreo DJ de latencia cero.

## Fuente y distribución

El archivo Source incluye el subproyecto, JUCE 7 completo, Rubber Band completo,
fuentes gráficas, pruebas, scripts, licencia pública y manifiesto SHA-256. La
compilación Windows de publicación extrae ese archivo y construye desde él. Se
comprueba su manifiesto completo y commit antes de publicar. No incluye claves
privadas ni las grabaciones personales capturadas durante el uso del plugin.

Distribución GPLv3-o-posterior de este subproyecto; avisos y código incluidos.
Instalador Windows sin firma de distribuidor. No se entrega binario Mac; AU/auval
sigue pendiente y JUCE 7 fijado requiere SDK 14. Los productos anteriores conservan sus rutas y códigos.

## Límites de la evidencia

Las demos son una vocal sintética y habla sintetizada con eSpeak NG, identificadas
como tales. El plugin instalado procesa exclusivamente el audio que recibe.
La captura usa energía/transitorios/pausas, no reconocimiento del significado
de palabras ni una red neuronal generativa.

La escucha con voces reales del usuario y la ejecución directa en Studio One 7
siguen pendientes. Las medidas de callback en runners no equivalen a una garantía
de CPU en un portátil concreto. No se afirma equivalencia o superioridad a otro
producto comercial ni ausencia universal de artefactos.
