# Validación de M7G Fresh Vocal Pro Beast

Pruebas con señal sintética controlada; sus umbrales no garantizan el resultado en cualquier voz.

## Pruebas locales del binario Windows

- Latencia medida, también al desactivar/reordenar módulos: 142 muestras/44,1 kHz, 154/48 kHz y 298/96 kHz. Se prueban las 24 permutaciones.
- Tres bandas LR24: recombinación de ganancia plana a 80, 700, 5000 y 10000 Hz; compresión real con reducción y cambio de nivel.
- Sibilancia a 8 kHz: reducción detectada; una voz grave sin sibilancia no dispara el de-esser.
- EQ neutral, realce de shelf y corte de banda Expert medidos en audio.
- Saturación 4×: la componente alias de una prueba no lineal de 10 kHz baja frente al procesamiento sin sobremuestreo; Amount 0 conserva el cuerpo grave.
- Delay: 28800 muestras a 100 BPM y 24000 a 120 BPM para 1/4. Reverb con cola audible y ducking que responde a entrada vocal.
- 35 presets con audio distinto, parámetros completos XML, límite de salida, A/B y recuperación del estado.
- GUI: modos exactos 600×400, 1200×750 y 1400×900; morph, knobs, curvas de estilos, bypass, Ctrl+Z/Y, A/B, XML, drag de orden real, radar y curva animados.
- Projucer oficial 7.0.12: .jucer resalvado a Visual Studio 2022, VST3 normal, ARA deshabilitado y código de plugin cotejado con CMake.

- El factory VST3 debe exponer tres clases estándar de JUCE (audio, controlador y compatibilidad), sin clases de extensión ARA. El proyecto no incluye el SDK de ARA.

## Puerta de publicación Windows

La automatización compila desde el Source.zip exacto con CMake y MSBuild/Projucer, ejecuta tres suites CTest, comprueba el VST3 real de ambas rutas, el instalador, reinstalación, conservación de plugins/licencia anteriores y desinstalación. La publicación requiere que todos los pasos terminen correctamente. Se adjuntan logs nativos, capturas de las vistas, demostración de audio, manifiesto de fuente y SHA-256.

## Límites

No se ha ejecutado Studio One 7 en este entorno ni un benchmark de sus controladores GPU. Las capturas nativas de CI usan fallback gráfico para servidores sin GPU; el producto compila soporte OpenGL. No hay evaluación auditiva con las grabaciones del usuario ni comparación ciega con un plugin comercial. Las fotos/video de referencia no fueron adjuntadas y no se afirma coincidencia visual exacta. No incluye afinación ni red neuronal.

La prueba adicional de OpenGL bajo Wine/Xvfb devolvió un error X11 BadMatch. La interfaz y sus animaciones pasan con la ruta de software; el controlador OpenGL real de Windows/Studio One sigue pendiente de validación.
