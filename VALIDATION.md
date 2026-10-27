# MAKKINE VOICE 7 — validación 0.2.1

Actualización de nombre, sin cambios en los algoritmos de audio.

- Compilación de VST3, Standalone e instalador Windows x64 completada con Clang/LLVM y SDK Microsoft.
- El módulo VST3 devuelve el nombre **MAKKINE VOICE 7** mediante la interfaz que consulta el host.
- Los módulos 0.2.0 y 0.2.1 se cargaron, procesaron audio estéreo y descargaron correctamente bajo Wine.
- Identificador de clase VST3 conservado: `01EFCDAB8291EBFA4D374737476C6441`.
- La suite Windows bajo Wine pasó las 54 combinaciones de perfiles/tasas/bloques, con el mismo comportamiento DSP medido.
- Instalador verificado como PE AMD64, ProductName MAKKINE VOICE 7 y versión 0.2.1.
- La actualización conserva el formato de estado y parámetros; instala el bundle con el nombre nuevo y elimina el bundle antiguo después de copiarlo correctamente. Esta migración y la reapertura de sesiones todavía requieren prueba en Windows real y Studio One 7.
- El instalador no tiene firma digital. La compilación nativa con MSVC presenta una incompatibilidad previa con los arrays C99 de RNNoise; esta versión se ha compilado con Clang, sin modificar RNNoise.

El registro siguiente corresponde a las verificaciones previas de 0.2.0. La interfaz de Studio One 7 y las comparaciones perceptuales con hardware/UAD siguen pendientes.

---

# Validación 0.2.0 — Windows VST3 y Linux

Artefactos generados: VST3 Windows x64, Standalone Windows x64, instalador NSIS amd64, VST3/Standalone Linux.

Compilación Windows: Clang/LLVM 19.1.7, SDK y runtime oficial Microsoft obtenidos con xwin 0.10.0 verificado por SHA-256. Runtime C++ enlazado estáticamente; las importaciones del VST3 son DLL de Windows. Instalador NSIS 3.11 amd64 compilado, sin firma digital. Tools/build-windows-cloud.sh ejercitado con éxito para repetir compilación y empaquetado.

Pruebas realizadas:

- Linux: CTest VocalDSP 1/1 aprobado. 54 combinaciones de 6 perfiles, 44.1/48/96 kHz y bloques 64/256/1024. Audio finito, señal no nula, techo de muestras −1 dBFS, retorno al silencio, estado, mono y bloques variables.
- Windows: ejecutable M7GSmoke.exe ejecutado bajo Wine 10; mismas pruebas aprobadas.
- Windows: M7GVST3Load.exe cargó el módulo VST3 real, obtuvo su fábrica, creó el componente, configuró buses estéreo, procesó audio y descargó el módulo. Resultado aprobado. Latencia comunicada al host: 1958 muestras a 48 kHz.
- RNNoise: 800 cuadros por tasa; reducción de ruido sintético de −9.08 dB (44.1 kHz), −8.93 dB (48 kHz) y −8.85 dB (96 kHz), medido tras 5 segundos de adaptación.
- De-esser: −4.42 dB de banda alta en prueba sintética. Expansor: −23.60 dB de señal tenue.
- Sobremuestreo 4×: reducción de 20.59 dB del producto de alias a 23 kHz en prueba con seno de 5 kHz, frente al waveshaper sin sobremuestreo.
- Compresión de rodilla suave: comprobada reducción sostenida y conservación del inicio del transitorio; detector enlazado para estéreo.
- Bus polifónico: conservó las tres fundamentales de un acorde de prueba; este perfil no corrige el conjunto como una sola nota y usa una ruta sin salida neuronal, con latencia alineada.
- Afinación ADLIBS: entrada 223 Hz, salida con pico medido 221 Hz, objetivo 220 Hz. Correlación con interpolación, seguimiento con histéresis y selección de escala tentativa, cromática cuando falta evidencia.

El instalador .exe fue generado y se verificó como PE x64. Su ejecución de instalación/desinstalación no se completó en el prefijo Wine de pruebas: faltaban carpetas/registro del shell Windows y la inicialización no terminó. No se declara instalación nativa comprobada. La carga y el procesamiento del módulo VST3 sí pasaron bajo Wine.

Studio One 7 es el DAW objetivo, mediante VST3 Windows 10/11 x64, pistas mono/estéreo y buses. No se ha ejecutado Studio One 7 en esta máquina. Faltan prueba directa de escaneo, inserción, reapertura de sesión, automatización e instalación/desinstalación en Windows real.

No existe certificación de equivalencia con UAD, comparación perceptual con hardware, garantía de resultado profesional para cualquier grabación ni evidencia de usar los modelos más avanzados del mundo. La IA integrada es el modelo RNNoise de Xiph.Org, v0.1.1; el control musical restante usa algoritmos DSP propios y JUCE. La calidad percibida sobre voces cantadas reales sigue pendiente de evaluación.
