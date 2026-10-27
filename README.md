# M7G Golden Voice AI — 0.2.0

Plugin vocal VST3 y aplicación Standalone, C++17 / JUCE 8.0.6. Interfaz negra y dorada de 700 × 520, sin perillas: selector de voz e intensidad automatizable.

## Qué hace esta versión

- Seis perfiles: voz principal, dobles, refuerzos, adlibs, armonías / coros y bus vocal.
- Análisis continuo de nivel y tono, ganancia automática limitada a ±12 dB, corrección cromática, saturación asimétrica normalizada con sobremuestreo 4× y bloqueo de DC, de-esser de banda alta, puerta/expansor suave con seguimiento de ruido entre frases, filtros dinámicos para cuerpo y aspereza, tres bandas de ecualización, compresión estéreo enlazada de rodilla suave con recuperación dependiente del programa y techo de salida de −1 dBFS.
- Afinación mediante correlación normalizada y dos cabezas de retardo con fundido; funciona con bloques pequeños del host sin reiniciar el análisis.
- Reverberación y delay estéreo con ducking y sincronización al tempo del host (120 BPM si no hay tempo disponible); mezcla de efectos dependiente del perfil.
- Entrada/salida mono o estéreo; estado de parámetros guardable por el DAW; medidores de entrada, salida y reducción de compresión real.
- Procesamiento local, sin API, credenciales ni envío de grabaciones.

La reducción de ruido usa **RNNoise**, un modelo neuronal recurrente preentrenado de Xiph.Org (v0.1.1, incluido localmente). La afinación, ecualización, dinámica y selección de efectos usan análisis DSP adaptativo determinista. El modelo no fue entrenado por M7G ni garantiza resultados óptimos en canto. El resto de algoritmos son propios y JUCE: no son copias ni emulaciones certificadas de Auto-Tune, Neve, LA-2A u otros productos. No se garantiza un resultado profesional con cualquier grabación.

Los perfiles de armonías y dobles procesan pistas ya grabadas; no generan voces nuevas ni separan cantantes. El afinador estima una escala a partir de notas estables únicamente cuando reúne suficiente evidencia. Las melodías ambiguas usan afinación cromática. La voz aislada no permite determinar siempre la tonalidad de la canción. La limpieza combina RNNoise con un expansor suave entre frases. Se conserva un 35 % de señal residual para proteger consonantes y armónicos. No es un separador de fuentes. No incluye generación de armonías ni escritura de automatización en el DAW. La adaptación interna sí funciona durante la reproducción.

## Instalador Windows

`BuildInstaller.ps1` compila con Visual Studio x64, ejecuta las pruebas, comprueba la cabecera PE x64 del plugin y empaqueta mediante NSIS 3. Produce `dist/M7G-Golden-Voice-AI-Setup-Windows-x64.exe`. Instala en la carpeta VST3 estándar de Windows y registra desinstalación. Requiere Windows 10/11 x64. El certificado de firma de código no está configurado.

El flujo `.github/workflows/windows-installer.yml` hace la compilación y adjunta el instalador cuando se ejecuta en GitHub Actions. Crear el archivo de flujo no equivale a ejecutarlo.

El SDK oficial ya está disponible. VST3 Windows x64 e instalador compilados desde Linux; validación en Studio One 7 pendiente.

## Compilar en Windows

Instalar Git, CMake ≥3.22 y Visual Studio 2022 con “Desktop development with C++” y Windows SDK. Ejecutar `Build.bat` desde una consola. Descarga JUCE 8.0.6 si falta, compila VST3/Standalone y ejecuta las pruebas.

Artefacto: `build/M7G_Golden_Voice_AI_artefacts/Release/VST3/M7G Golden Voice AI.vst3`. Copiar la carpeta completa a `C:\Program Files\Common Files\VST3`, volver a escanear plugins en el DAW y cargarlo en una pista vocal. Los archivos VST3 para Windows y Linux son distintos. La validación en Studio One 7 debe realizarse en Windows.

## Compilar en Linux

Dependencias habituales Debian: compilador C++, CMake, pkg-config, libasound2-dev, libx11-dev, libxext-dev, libxinerama-dev, libxrandr-dev, libxcursor-dev, libfreetype-dev, libfontconfig1-dev y libgl1-mesa-dev.

```sh
git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
ctest --test-dir build -C Release --output-on-failure
```

Omitir el clone si JUCE ya existe. JUCE se mantiene fuera de los archivos versionados y está fijado a la etiqueta 8.0.6 (commit `51a8a6d7aeae7326956d747737ccf1575e61e209`). No hay descargas de dependencias durante el procesamiento de audio.

## Uso

Insertar en una pista vocal aislada; seleccionar su función; reproducir audio. El detector de tono necesita aproximadamente 100 ms de señal sonora; el modelo de ruido puede necesitar varios segundos para adaptarse al ambiente. La ganancia se adapta más despacio para evitar saltos. Ajustar intensidad para reducir o aumentar el tratamiento. Usar el bypass del DAW para comparar a volumen equivalente. El techo de salida es de picos de muestras, no un limitador true-peak certificado.

La afinación por retardo tiene latencia nominal de 40 ms aproximadamente, más el retardo de los filtros (la mitad corresponde al bloque neuronal), declarada al host, y puede producir modulación o artefactos. Probar primero en una copia de la sesión. 0 % conserva la señal con el retardo nominal y protección de salida; no equivale a bypass bit a bit. La detección está pensada para una sola voz entre 35 y 1400 Hz.

## Validación y alcance

`M7GSmoke` procesa señales sintéticas en los seis perfiles a 44.1, 48 y 96 kHz con bloques de 64, 256 y 1024 muestras. Comprueba señal no nula, valores finitos, techo de salida, retorno al silencio después de la cola, restauración del estado y dirección de corrección de afinación. También comprueba el de-esser, la atenuación de ruido tenue y bloques variables en mono. Estas pruebas no certifican calidad perceptual. Faltan sesiones de escucha con voces reales, validación de interfaz en DAW, compatibilidad de hosts y medición rigurosa de tiempo real antes de una versión comercial.

JUCE requiere una licencia compatible con la distribución elegida: revisar `JUCE/LICENSE.md` antes de distribuir un producto cerrado.

RNNoise se incluye con su modelo predeterminado y avisos BSD en `ThirdParty/rnnoise/COPYING`; el instalador conserva esos avisos. Se comprueba la inferencia neuronal y la atenuación sobre ruido sintético a 44.1, 48 y 96 kHz.

## Studio One 7 — Windows x64

1. Cerrar Studio One y ejecutar el instalador Windows x64.
2. Abrir Studio One 7 y buscar **M7G Golden Voice AI** en el explorador de **Efectos**.
3. Arrastrarlo a una pista de audio mono/estéreo o a un bus vocal. Para grupos con varias voces, seleccionar **BUS VOCAL**: conserva las notas simultáneas y omite la corrección monofónica y la salida neuronal para evitar alterar el conjunto.
4. Si no aparece, revisar **Studio One > Opciones > Ubicaciones > Plug-ins VST**, restablecer la lista de bloqueados y reiniciar el DAW para volver a escanear.

El instalador coloca el bundle en `C:\Program Files\Common Files\VST3\M7G Golden Voice AI.vst3`. El plugin declara compensación de latencia, automatización de parámetros y persistencia de estado al host VST3. Está preparado como efecto de audio, no como instrumento. Estos requisitos se han implementado; todavía no se ha probado dentro de Studio One 7.

La calidez se obtiene mediante armónicos pares e impares controlados y compresión suave. Es un diseño original: no es una emulación medida de equipos UAD ni garantiza el resultado de un estudio físico con cualquier grabación.
