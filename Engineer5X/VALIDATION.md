# Makkine Engineer 5X — validacion

Pruebas de senal y archivos, no solo controles/nombres. Referencia sintetica original
generada matematicamente por Tests/Support.h; no grabacion de cantante.

## Audio local ejecutado como Windows PE x64

- Cinco filosofias producen cinco renders diferentes de 302400 muestras, dos
  canales a 48 kHz; exportacion float32 y BWF 221234 preservado.
- V1 centra la nota de prueba: error mediano -0.0084 cents, dispersion reducida
  de 23.88 a 6.66 cents en el tramo oscilante medido.
- V4 conserva la oscilacion: dispersion 26.00 cents, centro a -0.91 cents.
  Estas cifras describen el ensayo, no una garantia para cualquier cantante.
- V2 conserva exactamente el PCM de una nota de 6.5 cents de desviacion;
  prueba adicional del permiso estricto de 8 cents. V3 usa el mismo desplazamiento
  en las distintas notas de una frase. V5 protege doce frames de ataque del ensayo.
- En V3, desfase de la modulacion vocal medida: -0.40 ms. Silencio/marcador
  inicial, duracion exacta, canales y reloj BWF comprobados por separado.
- A 44.1 y 96 kHz se conserva la longitud: 277830 y 604800 muestras.
  Silencio no produce voz ni audio inventado. Cancelacion del analizador comprobada.
- Sin escucha de archivo, la entrada del DAW pasa identica con latencia declarada
  cero. Licencia, estado, buffers vacios/grandes y lectura de audio comprobados.
- Analisis con editor cerrado; cinco columnas funcionales de escuchar/solo/mute/
  guardar, fuentes reales de drag WAV, preview con conversion 48/44.1 kHz, mezcla
  estereo-a-mono y escalado de interfaz 1400x900 a 1120x720 comprobados.
- Fabrica VST3, carga y liberacion: identidad independiente
  `01EFCDAB8291EBFA4D37473745357831`, buses y procesamiento aprobados en el host
  Windows de prueba. Instalador NSIS generado.

La fase nativa Windows MSVC y macOS/auval precede a la publicacion automatica.
El dossier del repositorio se actualiza con su resultado y hashes tras verificar
los archivos publicos; el manifiesto incluido identifica siempre el commit exacto.

## Limites

No validacion directa de escucha con voces del usuario ni Studio One 7 todavia.
La grafica muestra pitch detectado, no una medicion infalible de toda voz posible.
Deteccion monofonica 55–1100 Hz; fuera de zonas fiables conserva audio original.
No alineacion de otra toma sin referencia. El drag conserva el reloj del archivo;
colocalo en el inicio del evento original. La posicion del raton no la controla
el plugin. Es render por archivo, no afinacion en vivo de latencia cero.

No se garantiza CPU en un equipo particular, acierto de intencion melodica,
cero cents en toda muestra ni ausencia universal de artefactos. Firma Windows
de distribuidor y notarizacion Apple no incluidas. Distribucion GPLv3-o-posterior.
