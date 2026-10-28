# Makkine Engineer 5X - AI Vocal Tuner Evolution

[Instalador Windows](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-Windows-x64.exe) · [Mac AU/VST3](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-macOS-universal.zip) · [Código completo](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/download/engineer5x-v1.0.0/Makkine-Engineer-5X-Source.zip) · [Demos y captura](https://github.com/uribeuribon-creator/M7G-GOLDEN/releases/tag/engineer5x-v1.0.0) · [Validación](VALIDATION.md)

Ingenieria de afinacion vocal por archivo, JUCE 7/C++20. Cinco renders naturales
de una misma toma monofonica, con Rubber Band R3 y preservacion de formantes.
Proyecto independiente, sin modulo de scratch. VST3 Windows x64 y objetivos AU/
VST3 universales para Mac. Misma licencia publica global M7G incluida.

## Uso

1. Inserta en una pista del DAW, carga una vocal WAV cruda o arrastra el archivo
   sobre la interfaz. Tambien admite AIFF/FLAC mono o estereo.
2. El analisis y los cinco renders se calculan automaticamente en segundo plano,
   incluso sin editor abierto. No se hace analisis pesado en el callback de audio.
3. Las cinco columnas muestran pitch original gris y pitch del render medido en
   color. Click para mover escucha; rueda o +/- para zoom, barra para recorrer.
4. ESCUCHAR elige y reproduce una version. SOLO fija la escucha exclusiva; MUTE
   silencia esa version. ORIGINAL / A-B compara conservando la posicion. PAUSA
   detiene la escucha sin perder la posicion. El motor de audio del DAW debe
   estar activo; la escucha sale por ese DAW, no por otra tarjeta de sonido.
5. ARRASTRAR WAV A LA DAW entrega un archivo real ya renderizado. Alternativas:
   GUARDAR WAV o CARPETA WAV. Los archivos persistentes no se borran al cerrar
   el plugin, para conservar referencias de las sesiones del DAW.
6. Coloca el archivo en **el mismo inicio del evento original**. Conserva todo el
   silencio inicial/final, numero exacto de muestras, frecuencia y canales. La
   latencia de Rubber Band se compensa antes de exportar. Se conserva el origen
   BWF cuando existe. El plugin no controla el punto donde sueltas el archivo ni
   puede adivinar la posicion de un evento si el WAV no contiene ese dato.
7. Puedes poner un autotune artistico despues del WAV elegido. Este plugin no
   modifica automaticamente el audio en vivo: cuando no estas escuchando un
   render, la entrada del DAW pasa sin cambios ni latencia añadida.

## Las cinco filosofias

| Version | Decision |
| --- | --- |
| V1 CENTERED | Corrige toda la desviacion estimada y verifica el resultado; una revision adicional del render si mejora el centrado estable. |
| V2 NATURAL DRIFT | Dentro de 8 cents conserva el audio. Fuera corrige progresivamente; conserva vibrato identificado. |
| V3 PHRASE LEAD | Un desplazamiento robusto por frase; mantiene el contorno y las diferencias internas. |
| V4 VIBRATO KEEPER | Centra la tendencia lenta y deja la oscilacion rapida del vibrato. |
| V5 TRANSIENT SAFE | Corrige zonas estables; conserva ataques, sibilantes y respiraciones con transiciones. |

CUANTO CORRIGE viene al 100%. ESCALA CROMATICA conserva las doce notas; selecciona
mayor/menor y tonica si conoces la tonalidad. Cambiar esos controles recalcula las
cinco versiones tras una pausa corta. VOLUMEN solo afecta escucha, no exportacion.

## Analisis y limites

YIN con ventana adaptativa, autocorrelacion FFT, confianza, continuidad, energia y bandas espectrales;
segmentacion por pausas, agrupacion de notas y deteccion de oscilacion vocal.
Las frases se agrupan por voz periodica, evitando que ruido/respiracion unan frases.
Es inteligencia adaptativa de DSP, no una red neuronal generativa ni el algoritmo
propietario de Melodyne/VocAlign. No hay API, subida de audio ni modelo que descargar.

Una sola vocal cantada, no un bus polifonico con instrumental. Las notas destino
se infieren de centros estables y escala elegida; la intencion musical no puede
deducirse infaliblemente de una toma. Un centro calculado al 100% no garantiza
cero cents en cada muestra, deteccion perfecta o ausencia universal de artefactos.
Glissandos, voces muy roncas, notas extremas o ruido fuerte pueden requerir elegir
otra version/escala. No hay alineacion ritmica de otra toma sin referencia.

Exporta WAV **32-bit float**, sin normalizacion/limitador ni recorte de picos.
El perfil de ruido decide donde corregir; no es un denoiser que elimine ambiente.
Las pausas y zonas sin pitch se conservan, no se afinan ni se recortan.

Limites de carga: 22.05–192 kHz, uno/dos canales, hasta diez minutos y un presupuesto
de 512 MiB de PCM para original + cinco versiones (el menor de esos limites).
Archivos grandes requieren fragmento o mono. El tiempo de render depende de la CPU
y duracion; no es un efecto de afinacion instantanea durante grabacion.

Al guardar el proyecto se conservan ruta original, parametros, seleccion y mutes;
al reabrir se vuelve a analizar el archivo. Conserva el WAV original en su ruta.
Los renders estan en el directorio de datos del usuario `M7G/Engineer5X/Exports`.

## Compilar e instalar

Windows 10/11 x64: CMake, Visual Studio 2022 C++, NSIS; ejecutar desde la raiz:
`./Engineer5X/Build-Windows.ps1`. Instala el .exe con Studio One cerrado, reabre
y reescanea VST3. El instalador conserva los demas productos y la licencia M7G.
El instalador no tiene firma de distribuidor.

Mac: CMake/Ninja, Python 3 y Xcode; `bash Engineer5X/Build-macOS.sh` construye AU y
VST3 arm64/x86_64. Los binarios publicados pasaron pruebas nativas Intel; la
seccion ARM se compilo y verifico, sin ejecucion directa en Apple Silicon.
Una copia de JUCE en el build recibe el parche incluido para
SDK 15+, sin modificar el framework fijado ni su DSP. Copia bundles a las carpetas
`/Library/Audio/Plug-Ins/Components` para AU (ruta validada) y
`/Library/Audio/Plug-Ins/VST3` para VST3; Finder puede pedir permiso de administrador.
Reabre el DAW y reescanea los plugins. Licencia en
`~/Library/M7G/M7G.license`. Firmas ad-hoc no son notarizacion Apple.

El Source.zip completo incluye JUCE, Rubber Band, fuentes, scripts y pruebas.
Distribucion GPLv3-o-posterior; consulta COPYING y NOTICES.md. Para distribucion
propietaria cerrada necesitas las licencias comerciales correspondientes.

Validacion: pruebas de afinacion real, vibrato, umbral de 8 cents, desplazamiento
por frase, ataques protegidos, duracion/silencio/BWF, cinco WAV distintos, UI,
preview, estado, carga VST3 e instalador. Las demos son sinteticas, identificadas
como tales. Escucha con cantantes y prueba directa en Studio One 7 pendientes.
