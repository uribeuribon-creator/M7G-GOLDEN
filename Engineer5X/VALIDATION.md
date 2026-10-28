# Makkine Engineer 5X — validacion

Pruebas de senal y archivos, no solo controles/nombres. Referencia sintetica original
generada matematicamente por Tests/Support.h; no grabacion de cantante.

## Publicacion y pruebas nativas aprobadas

[Instalador Windows](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-Windows-x64.exe) · [AU/VST3 Mac](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-macOS-universal.zip) · [Fuente completa y evidencia](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/engineer5x-v1.0.0)

Codigo construido: `713761cc77478144a0e924c6650f1623b062f220`.
[Build nativo](https://github.com/uribeuribon-creator/M7G-GOLDEN/actions/runs/40740080325) y
[validacion/publicacion final](https://github.com/uribeuribon-creator/M7G-GOLDEN/actions/runs/40746487320).

- Windows MSVC x64: **tres suites aprobadas**, audio, interfaz y fabrica VST3.
  Instalacion, reinstalacion, licencia, productos anteriores, carga del VST3
  instalado y desinstalacion aprobadas. Fuente extraida del mismo ZIP publicado.
- Mac Intel: **dos suites aprobadas**, audio e interfaz. AU y VST3 universales
  con arm64/x86_64 verificados; firma ad-hoc verificada. La ejecucion nativa fue
  Intel, no Apple Silicon.
- AU: carga real del bundle/factory, inicializacion, cuatro parametros, estado
  y render estereo aprobados por el host nativo adicional. Energia medida 1152.371.
  **AU VALIDATION SUCCEEDED** en la herramienta `auval` de Apple, tras instalar
  en la carpeta comun de componentes y refrescar su registro.
- La publicacion recupero los mismos binarios compilados del build indicado;
  no sustituyo el DSP ni recompilo desde otra revision. Procedencia y registros
  nativos adjuntos. Los cambios posteriores del repositorio son validacion/docs.
- **38 descargas publicas HTTP 200** como adjuntos; todos los SHA-256 coinciden
  con archivos y digests de GitHub. Los **3776 archivos** del Source.zip y su
  manifiesto se verificaron. Incluye JUCE 7/Rubber Band completos y ningun proyecto
  de scratch ni clave privada.
- Los seis WAV publicos son float32, 48 kHz, estereo, 302400 muestras/6.3 s:
  original sintetico y cinco versiones. La captura es del editor real.

| Archivo | Bytes | SHA-256 |
| --- | ---: | --- |
| Windows .exe | 2483254 | `91157a560b512bd0f412c2eed1dc51f783b73111aaf6abaf2d2d927cf71747e8` |
| Windows VST3.zip | 3183824 | `ac0ad603e5de3e58a8512065f921e1bd868b5d6d73dbfb9c35fc4ecf03bad769` |
| Mac universal.zip | 9608998 | `2bc92b15ff311f337f50c2a0b1935a4883d5a8742b57cb77af08affa040e711b` |
| Source.zip | 23686453 | `e4cfe4b6735d3266dbceae061c1a43bb7aa1ccf728f8b4931046b5fb39fa0a32` |

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

El manifiesto incluido identifica el codigo exacto de los binarios. Las revisiones
del manual y utilidades de validacion posteriores no cambian esos binarios.

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
