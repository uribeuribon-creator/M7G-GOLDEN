# M7G ALIEN TUNE GOD V1

**V1 TUNE GOD · 100 presets · Makkine Seven Golden**

Plugin VST3 Windows x64 independiente para afinacion monofonica, efecto robot,
cuatro armonias y dos dobles derivados de la toma principal. JUCE 7.0.12 / C++20,
ONNX Runtime 1.20.1 CPU local. Conserva Alien Voice y Alien Transitions.

## Uso en Studio One

1. Cierra el DAW, ejecuta el instalador, vuelve a abrir y escanea VST3.
2. Inserta **M7G ALIEN TUNE GOD V1** en una voz monofonica. No afina acordes ni
   separa varias personas cantando juntas. Elige un banco y un preset.
3. AUTO TONO estima una escala a partir de notas observadas. Al principio usa
   cromatica; puede ser ambiguo. Para control musical, desactivalo y elige
   tonica/escala. Las armonias usan C mayor provisional si no hay escala fiable.
4. CORRECCION IA ajusta el centro; HUMAN <> ROBOT agrega la parte rapida y
   cuantizada. RETUNE cambia la respuesta, HUMANIZE conserva mas expresion y
   FORMANTE cambia el color de la garganta con compensacion espectral.
5. CREAR ARMONIAS activa 1–4 voces diatonicas. Selecciona intervalo o personalizada.
   DOBLES REALES activa dos simulaciones por resintesis, no otras tomas grabadas.
   MIX ARMONIAS mezcla ambos grupos; la principal se mantiene en el centro.
6. Selecciona una tarjeta de voz para editar ANCHO, COLOR, AIRE, REVERB,
   DISTORSION y PAN de esa voz. HUMANIZAR DOBLES cambia microtiempo/afinacion.
   Esos efectos no se aplican a la principal.
7. Usa A/B y copiar para comparar; FAV persiste. Buscar y < > RANDOM trabajan en
   el banco actual. Cuatro bancos de 25: principales, robot, armonias y dobles.

## Latencia y alcance real

**A 48 kHz: ESTUDIO 42.67 ms (2048 muestras); MONITOR 10.67 ms (512 muestras).**
Cambia calidad con reproduccion detenida. El DAW debe compensar durante mezcla.
Monitor reduce resolucion espectral, especialmente en voces graves. No hay
modo de afinacion de latencia cero. RETUNE 0 significa la respuesta mas rapida
del control, no deteccion instantanea o latencia cero. La deteccion necesita
periodos observados, con ventanas adaptativas de 42–85 ms a 12 kHz y asentamiento
en cambios. El tiempo de deteccion es adicional a la latencia del flujo.

Un detector YIN/autocorrelacion propone notas; una CNN ONNX puntua candidatos en
un trabajador fuera del audio. El grafo de notas controla una correccion lenta
del centro y otra artistica rapida. Cinco perfiles cambian curvas/tiempos reales:
ALIEN NATURAL, REGGAETON FLOW, TRAP AUTO, POP CRYSTAL, CLASSIC VINTAGE. Son perfiles
DSP, no cinco modelos generativos entrenados. El modelo de registro/aire/aspereza
adapta suavemente la respuesta y muestra estimaciones acusticas. No identifica
sexo/genero, identidad, genero musical o registro anatomico.

Resintesis espectral con bloqueo de fase y envolvente cepstral para conservar
formantes. Cada voz generada tiene fases, variaciones y microtiempo propios;
no son cantantes generados por una IA de voz ni tomas reales independientes.
No contiene Melodyne, Antares, CREPE ni otros motores propietarios. Naturalidad,
key detection y calidad no estan garantizadas para cualquier señal.

El piano muestra historial de notas detectadas y destinos calculados: principal
oro, voces generadas azul/violeta. No es un editor offline de blobs ni una
medicion independiente de la afinacion de salida. La onda superior es salida real.
Interfaz por software; no se afirma aceleracion GPU.

## IA, licencia y dependencias

Modelos originales con entrenamiento sintetico documentado en Models/METRICS.json;
las puntuaciones sinteticas no certifican precision en cantantes reales. Ejecutar
Tune/Tools/export-models.py reproduce la exportacion con Python/NumPy/scikit-learn/
ONNX; no se necesita Python para usar el plugin. No se envia audio a internet.
Inferencia y sesiones ONNX fuera del callback; filtros/memorias preparados antes
del audio. Efectos pesados se suspenden despues de silencio/colas; el retardo y
la deteccion siguen funcionando. El informe publicado mide CPU del proceso,
incluyendo ONNX, con una prueba a tiempo real. No se promete un porcentaje i5.

Una sola licencia RSA firmada **M7G.license** desbloquea todo para siempre. El
instalador conserva el archivo existente y aporta la licencia compartida cuando
falta, en `%ProgramData%\M7G`. Se puede importar otra en el perfil del usuario.
Una licencia invalida deja pasar voz seca alineada. Sin activacion por modulo.

ORT CPU y DLL redistribuibles de Visual C++ van junto al VST3: no requiere
instalar Python, descargar modelos ni instalar componentes al abrir el DAW.
ONNX MIT/notices, JUCE 7 GPLv3/comercial, SDK y fuentes conservan sus licencias.
Consulta NOTICES.md antes de comercializar; la activacion M7G no concede una
licencia comercial JUCE. Instalador sin firma Authenticode.

## Compilar y verificar

Windows 10/11 x64, Visual Studio 2022 C++, CMake, Git, NSIS:

```powershell
./Tune/Build-Windows.ps1
```

JUCE 7.0.12 fijado a `4f43011b96eb0636104cb3e433894cda98243626`. Archivo ORT
verificado por SHA-256 antes de extraer. Pruebas nativas de afinacion de salida,
deteccion/armonias, aislamiento de efectos, dobles, 100 presets, licencia/estado/
A-B, latencia/seguridad, ONNX real, interfaz y carga DLL. El flujo tambien prueba
instalacion/reinstalacion/conservacion de productos previos/desinstalacion en una
maquina desechable. Adjunta demo de vocal sintetica, capturas, checksum y CPU.

Version de evaluacion de ingenieria: faltan escucha con cantantes reales y
prueba directa dentro de Studio One 7. No se demuestra superioridad a otros
productos ni resultados perfectos con cualquier voz.
