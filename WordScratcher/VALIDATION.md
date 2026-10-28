# WORD SCRATCHER PRO - 300 — validación

Código de producto: `c76cb404ec3464467c9a09088ca42fd7344105a4`.
JUCE 7.0.12: `4f43011b96eb0636104cb3e433894cda98243626`.
Rubber Band 3.3.0: `2be46b0dffb13273a67396c77bc9278736bb03d2`.

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

Ambos modos comparten la compensación de latencia. La detección automática espera
el final del fragmento y su pausa: no anticipa palabras. Se recomienda mezcla o
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
