# M7G ALIEN VOICE — validación 0.3.0

## Comprobaciones locales completadas

Compilación Windows x64 con Clang/LLVM 19 y SDK Microsoft. Los ejecutables Windows y el módulo VST3 real se ejecutaron bajo Wine 10. El controlador de pruebas comprueba el código de salida de Windows explícitamente, además del resultado de Wine.

- **VocalDSP:** 54 combinaciones de perfiles, tasas 44.1/48/96 kHz y bloques 64/256/1024; señal finita, no nula, retorno al silencio, estado, mono y bloques variables.
- **AlienRack:** 50 presets con nombres y configuraciones diferentes; procesamiento de los 50 sin valores no finitos ni superación del techo de muestras. Pruebas aisladas de los doce módulos.
- **Ducking:** reverb y delay redujeron su componente húmedo en 20.4 dB durante la señal de prueba; ambas colas recuperaron nivel al terminar la voz.
- **Análisis:** doce rutas individuales y la ruta global midieron tres segundos de audio y cambiaron los parámetros seleccionados. Persistencia de las dos configuraciones A/B tras guardar y restaurar el estado.
- **AlienInterface:** doce módulos, doce botones AI individuales y análisis global; 49 perillas enlazadas a parámetros, 50 entradas de preset y bypass funcional. Captura PNG renderizada desde el editor compilado, con tipografías incluidas.
- **VST3Module:** carga de la DLL real, fábrica VST3, nombre **M7G ALIEN VOICE**, buses estéreo, procesamiento y descarga. Identificador conservado: `01EFCDAB8291EBFA4D374737476C6441`. Latencia declarada: 2006 muestras a 48 kHz.
- **RNNoise:** reducción de ruido sintético de 9.08/8.93/8.85 dB a 44.1/48/96 kHz, tras cinco segundos de adaptación.
- **De-esser y gate:** reducción de banda alta de 3.02 dB y atenuación de señal tenue de 23.53 dB en las pruebas aisladas.
- **Saturación 4×:** reducción de 20.59 dB del producto de alias a 23 kHz en la prueba con un seno de 5 kHz.
- **Afinador:** entrada de 223 Hz con objetivo 220 Hz; pico de salida de 220 Hz en AlienRack y 220.8 Hz en la prueba anterior. El perfil bus conserva las fundamentales simultáneas de un acorde.

## Puerta de publicación en Windows nativo

El workflow `publish-alien-release.yml` solo publica el instalador generado por MSVC después de aprobar:

1. Compilación Windows x64, comprobación PE AMD64 y las cuatro pruebas CTest: VocalDSP, VST3Module, AlienInterface y AlienRack.
2. Instalación silenciosa en la carpeta VST3 estándar; coincidencia SHA-256 del módulo instalado con el módulo probado.
3. Retirada de los dos bundles antiguos, registro con nombre y versión nuevos y carga/procesamiento del módulo instalado.
4. Desinstalación y retirada del módulo y de su entrada de registro.

La publicación incluye el instalador, su SHA-256 y una captura de la interfaz producida por el ejecutable Windows. El historial de GitHub Actions contiene los resultados nativos de cada publicación; las pruebas anteriores describen las mediciones locales, no sustituyen ese registro.

## Alcance pendiente

Studio One 7 es el DAW objetivo mediante VST3 Windows 10/11 x64, pistas mono/estéreo y buses. No se ha ejecutado Studio One 7 en este entorno. Siguen pendientes el escaneo, inserción, automatización y reapertura de sesiones dentro de ese DAW, y la evaluación perceptual con grabaciones vocales reales.

El instalador no tiene firma digital. El limitador controla picos de muestras; no se ha certificado como true-peak. El afinador es monofónico y no separa coros. RNNoise es el modelo neuronal local incluido; el resto del análisis usa DSP adaptativo. No existe certificación de equivalencia con hardware/UAD ni evidencia de un resultado universal o del uso de los modelos más avanzados del mundo.
