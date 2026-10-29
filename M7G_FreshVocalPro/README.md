# M7G Fresh Vocal Pro Beast

Plugin nuevo de mezcla vocal para Windows 10/11 x64, VST3 normal, JUCE 7.0.12 y C++17. Los plugins anteriores permanecen independientes.

## Instalar y usar

Cierra Studio One, ejecuta el instalador y vuelve a escanear VST3. Busca **M7G Fresh Vocal Pro Beast**. Se instala en `C:\Program Files\Common Files\VST3`. Incluye la licencia global M7G y conserva una licencia existente.

- **Simple — 600×400:** entrada, reverb, delay y salida, medidores y lista desplazable de 35 presets.
- **Advanced — 1200×750:** Dynamics, EQ, Space y Character, con knobs, estilos, gráficos y medidor de reducción.
- **Expert — 1400×900:** ocho bandas de EQ y arrastre de módulos en la barra lateral para cambiar el orden real de la cadena.
- Cambio de tamaño interpolado durante 200 ms; transición entre secciones durante 150 ms. Los ajustes de audio se conservan al cambiar de vista.
- Pulsa el título de una sección para activar/desactivar su procesamiento. El bypass conserva la compensación de latencia.
- **Load/Save:** presets XML con todos los parámetros y el orden de módulos. **A/B:** dos estados independientes. **Ctrl+Z / Ctrl+Y:** deshacer y rehacer.
- Arrastre vertical de knobs; rueda en pasos finos; doble clic restaura el valor inicial. Los presets interpolan controles durante 300 ms; una edición manual cancela la interpolación.

Los niveles de entrada/salida son dB reales y abren en **0 dB**, para mantener la ganancia unitaria. Entrada −100 a +12 dB; salida −85 a +6 dB. Los valores positivos 37/55 del texto no se usaron como ganancia en dB.

## Cadena y controles

Orden inicial: entrada → Dynamics (de-esser + compresor) → EQ → Character → Space en paralelo → salida → limitador. Expert permite las 24 permutaciones de las cuatro secciones. La barra lateral de Advanced conserva el orden visual Dynamics/EQ/Space/Character; Expert muestra el orden de audio.

**Dynamics:** detector de sibilancia 4–10 kHz, banda de foco 2–12 kHz, RMS/peak, confirmación de 20 ms, anticipación de 2 ms y ganancia suavizada. Compresión estéreo enlazada de tres bandas LR24 a 200 Hz/2 kHz, compensación de fase de la banda baja, rodilla suave, compresión ascendente/descendente, mezcla paralela y color. Smooth/Punchy/Aggressive controlan ratio y tiempos. Sobremuestreo 2×.

**EQ:** ocho bandas disponibles en el motor; Advanced muestra cuatro y Expert las ocho. Dos shelves a 80 Hz/12 kHz y seis peaks a 160, 350, 700, 1500, 2500 y 6000 Hz. Los estilos Warm/Neutral/Bright/Airy ajustan los valores reales de la curva. Cambiar de vista conserva el sonido.

**Character:** modelos Warm/Aggressive/Saturated, armónicos pares/impares, tres intensidades de cinta y ancho mid/side. Sobremuestreo 4× con filtros anti-alias. La eliminación de DC se aplica a la distorsión añadida. A Amount 0 se conserva la voz limpia. El gráfico muestra la curva real **salida − entrada** y queda plano en cero cuando no se añade carácter. El ancho mid/side actúa sobre contenido estéreo existente; no inventa una segunda toma a partir de mono.

**Space:** ocho combs y cuatro allpass por canal, modulación de las líneas, predelay, filtro de reverb 200 Hz–10 kHz y delay filtrado 400 Hz–8 kHz con feedback 25%. La voz controla un ducking enlazado de hasta 12 dB en los retornos; la señal directa se conserva. TIME es un control de tamaño/decay 0–10, con mapeo suave de RT60 limitado a aproximadamente 0,2–8 s según el tipo. El knob mantiene efecto en todo su recorrido. Radar con 20 partículas y curva con actualización objetivo de 60 Hz; OpenGL compone las vistas cuando el host/controlador lo permite. Las vistas ocultas no repintan sus gráficos.

El delay sigue el tempo comunicado por el DAW; usa 120 BPM cuando no se informa tempo. Las subdivisiones musicales son:

| Tempo | 1/16 | 1/8 | 1/4 |
|---|---:|---:|---:|
| 120 BPM | 125 ms | 250 ms | 500 ms |
| 100 BPM | 150 ms | 300 ms | 600 ms |

**Salida:** limitador enlazado, techo de muestra −0,5 dBFS, anticipación 1 ms y release 50 ms. No es un medidor/limitador true-peak.

## Inserto normal de Studio One

Este es un **VST3 normal, sin ARA**. Arrástralo al rack de insertos de una pista o bus vocal. Procesa la entrada en tiempo real, sigue el tempo comunicado por Studio One y guarda todos los parámetros en el estado normal del plugin/proyecto. No captura eventos ni requiere integración o licencia ARA.

La compensación de latencia se informa al host a partir de las muestras reales: 142 a 44,1 kHz, 154 a 48 kHz y 298 a 96 kHz en las pruebas. El bypass y el orden de módulos conservan esa cifra.

Studio One 7 y sus controladores gráficos requieren prueba manual. Las capturas automáticas de CI usan la ruta gráfica de JUCE sin OpenGL para servidores sin GPU; el plugin mantiene OpenGL habilitado.

## Presets

25 presets de fábrica con los nombres y bancos del texto, más 10 M7G: Golden Perfect, IA Control Vocal, Kid Laroi Syrup, Beast Punch, Diamond Air, Velvet Tape, Cathedral Gold, Dembow Echo, Radio Forward y Midnight Doubles. Incluye Reggaeton 1–10, Reggaeton big Reverb, TRAP PESADO 1–2, Voz Femenina 1–3, Backing Vocal, Mid Control y Warm Vocal.

«IA Control Vocal» es un preset de mezcla con detección/envolventes automáticas; este producto **no es un afinador ni incluye una red neuronal**. La decisión fue confirmada por el usuario. No se promete superioridad sonora sobre otro fabricante ni una calidad objetiva asociada a un precio.

## Proyecto compilable

El paquete Source.zip contiene las fuentes completas, JUCE fijado a un commit, fuentes tipográficas/licencias, `.jucer` y proyectos Visual Studio 2022 exportados con Projucer oficial 7.0.12.

- CMake: ejecuta `M7G_FreshVocalPro/Build-Windows.ps1` con VS2022 Desktop C++, CMake >=3.22, Python 3 y NSIS.
- Visual Studio: abre `M7G_FreshVocalPro/Builds/VisualStudio2022/M7G Fresh Vocal Pro Beast.sln`, selecciona **Release / x64** y compila.
- Projucer: abre/resalva `M7G_FreshVocalPro.jucer`. JUCE está en `../JUCE7`. La herramienta `Tools/prepare-projucer.py` descarga Projucer oficial si necesitas regenerar los proyectos.
- `Build-Windows.ps1 -AlsoBuildProjucer` valida ambas rutas. Las soluciones generadas y el paquete completo permiten compilar sin descargar JUCE; solo se descarga Projucer cuando se solicita regenerarlos.

La apariencia sigue la especificación escrita. Las cinco fotos y el video mencionados no se adjuntaron a esta petición; la comparación visual exacta queda pendiente.

Ver COPYING y NOTICES.md para licencias, y VALIDATION.md para evidencias y límites de pruebas.
