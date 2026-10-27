# M7G ALIEN VOICE — 0.3.0

Plugin vocal VST3 y Standalone para Windows x64, C++17 / JUCE 8.0.6. Interfaz negra y dorada de 12 módulos, 50 presets de fábrica y análisis automático por módulo o para toda la cadena. Está orientado a Studio One 7 mediante el formato VST3 estándar.

**[Descargar instalador Windows x64 (.exe)](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/v0.3.0/M7G-ALIEN-VOICE-Setup-Windows-x64.exe)** · [Ver interfaz compilada](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/v0.3.0/M7G-ALIEN-VOICE-interface.png) · [SHA-256](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/v0.3.0/M7G-ALIEN-VOICE-Setup-Windows-x64.exe.sha256)

La versión 0.3.0 está publicada como pre-release. Compilación MSVC, cuatro pruebas de audio/interfaz y ciclo de instalación/actualización/desinstalación aprobados en Windows 2022; la prueba dentro de Studio One 7 sigue pendiente.

## Los 12 módulos

| Módulo | Controles y función |
|---|---|
| Gate | Umbral, rango, ataque y release. Limpieza neuronal activable para pistas individuales. |
| De-ess AI | Frecuencia, umbral, cantidad y Listen para escuchar el contenido retirado. |
| Warmth Tube | Drive, bias, tono y salida; saturación asimétrica con sobremuestreo 4×. |
| Presence EQ | Campana de frecuencia, ganancia y Q; control adaptativo de cuerpo/aspereza. |
| Air | Realce de agudos, shine, armónicos y filtro superior. |
| Opto Compress | Peak reduction, ratio, ganancia y recuperación; compresión estéreo enlazada de rodilla suave. |
| Limiter | Ceiling, release, fast y lookahead. El techo es de picos de muestras, no true-peak certificado. |
| Reverb | Red de ocho retardos, tamaño, decay, pre-delay, envío y ducking. |
| Delay | Divisiones sincronizadas al tempo del host, feedback, tono, envío y ducking independiente. |
| Stereo | Width, pan, graves mono y spread con suma mono conservada. |
| Modulation | Chorus, flanger y vibrato; rate, depth y mix. |
| Pitch | Afinación monofónica: amount, retune, tonalidad y escala automática/cromática/mayor/menor. |

Cada módulo tiene activación y botón **AI**. **Analizar todo** mide tres segundos de audio reproducido y propone ajustes sobre los controles. Las perillas siguen disponibles para edición manual y automatización del DAW. Si no hay señal suficiente, el análisis lo indica y permite volver a intentarlo. Cambiar de preset cancela un análisis pendiente.

La reducción de ruido usa el modelo neuronal preentrenado RNNoise de Xiph.Org, incluido localmente. Los botones AI y el resto del control musical usan mediciones y reglas DSP adaptativas; no son doce modelos neuronales entrenados. No se envían grabaciones a Internet ni se requieren claves. No se ha demostrado equivalencia con UAD ni se garantiza el resultado de un estudio físico para cualquier grabación.

## Los 50 presets

- 12 para voces principales: rap, pop, RnB, trap, reggaeton, acústica, balada, rock, voces graves/agudas y locución.
- 8 para dobles.
- 8 para refuerzos.
- 8 para adlibs.
- 8 para armonías.
- 6 para buses vocales.

Los presets contienen configuraciones distintas, no modelos de cantantes ni imitaciones de artistas. El menú del plugin y los programas del host ofrecen los 50 presets. A/B guarda dos configuraciones completas y Copy A/B permite transferir la configuración al otro slot. Las comparaciones se conservan dentro del estado de la sesión.

**BUS VOCAL** conserva las notas simultáneas y omite la corrección monofónica y la salida neuronal. El afinador está diseñado para pistas de una sola voz: no separa un coro ni genera armonías. Una voz aislada no permite inferir siempre la tonalidad de la canción; cuando falte evidencia se puede elegir Key y Scale manualmente.

## Reverb y delay con ducking

Los dos efectos tienen ducking independiente. El detector usa el nivel de la voz para reducir el envío audible mientras hay señal y recuperarlo suavemente en las pausas. 0 % desactiva el ducking; el máximo permite hasta 24 dB de atenuación. El delay sigue el tempo indicado por el DAW; usa 120 BPM si el host no comunica un tempo válido. Feedback y decay cambian la cola real, y el plugin la declara al host.

## Controles globales

Input y Output ajustan las ganancias; Mix combina señal original y procesada con compensación de latencia. Auto Level mantiene una normalización limitada. Bypass devuelve la señal original con la misma latencia. **Solo / Delta** escucha la diferencia entre entrada y procesamiento. Los medidores y gráficos usan niveles, FFT, correlación, reducción de ganancia y LFO reales. El indicador 4× informa del sobremuestreo fijo.

El menú `...` restaura el preset, copia A/B y muestra información del motor. Los ajustes de los módulos están expuestos como parámetros automatizables. La latencia incluye procesamiento neuronal, afinación y lookahead; el host recibe la compensación correspondiente.

## Instalar en Studio One 7

1. Cerrar Studio One y ejecutar **M7G-ALIEN-VOICE-Setup-Windows-x64.exe**.
2. Abrir Studio One 7 y buscar **M7G ALIEN VOICE** en el explorador de **Efectos**.
3. Insertarlo en una pista mono/estéreo o bus vocal y elegir el preset de la función correspondiente.
4. Reproducir voz y usar AI por módulo o Analizar todo; ajustar al gusto y comparar con A/B a un volumen equivalente.
5. Si el nombre antiguo sigue en caché, revisar **Studio One > Opciones > Ubicaciones > Plug-ins VST**, restablecer la lista de bloqueados y reiniciar para volver a escanear.

Se instala en `C:\Program Files\Common Files\VST3\M7G ALIEN VOICE.vst3`. El instalador conserva la entrada interna de desinstalación y retira los bundles antiguos después de copiar el nuevo. Los identificadores VST3 y los parámetros originales se mantienen. La versión 0.3.0 amplía y modifica el procesamiento; no promete reproducir de forma idéntica el sonido de sesiones de 0.2.x. La prueba directa en Studio One 7 sigue pendiente. El flujo de publicación comprueba la instalación y desinstalación en Windows. Instalador sin firma digital.

## Compilar

Windows: Visual Studio 2022 Desktop C++, CMake ≥3.22, Git y NSIS 3. `BuildInstaller.ps1` compila, ejecuta las pruebas y genera el instalador. Se incluye compatibilidad de arrays de pila de RNNoise para MSVC. El bootstrap NSIS es Unicode x86, compatible con la distribución oficial de NSIS; exige Windows x64 e instala el módulo VST3 AMD64 en las carpetas y registro de 64 bits.

Linux: dependencias JUCE habituales (ALSA, X11, Xrandr, Xinerama, Xcursor, FreeType, Fontconfig, OpenGL), CMake y C++17.

```sh
git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
ctest --test-dir build -C Release --output-on-failure
```

Omitir el clone si JUCE ya existe. En el entorno cloud: `bash Tools/setup-cloud.sh` y `bash Tools/build-windows-cloud.sh`. Los checkouts de cada tarea ya están aislados; no es necesario crear worktrees.

JUCE está fijado a 8.0.6, commit `51a8a6d7aeae7326956d747737ccf1575e61e209`. RNNoise v0.1.1 conserva sus pesos originales y avisos BSD en `ThirdParty/rnnoise/COPYING`. Revisar la licencia de JUCE antes de distribuir un producto cerrado.

## Validación

Consultar `VALIDATION.md`. Las pruebas comprueban procesamiento real de todos los presets, cambios de los doce módulos, ducking de reverb/delay, análisis, persistencia A/B, interfaz y carga del módulo VST3. Las pruebas sintéticas y Wine no sustituyen la escucha de voces reales ni la validación dentro del DAW.
