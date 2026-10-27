# V10 TRIPLE GOD ORIGIN — estado de validación

## Implementación y procedencia

VST3 Windows x64, C++20/JUCE 8.0.6; identificador separado `01EFCDAB8291EBFA4D37473754673130`. Interfaz de cabina con 26 módulos, análisis individual/global, 26 mezclas/envíos paralelos y 280 presets (50 de efectos). MLP y CNN propias entrenadas con datos sintéticos; scripts y pesos versionados, RNNoise existente con licencia BSD.

## Revisión local aprobada

Las cinco pruebas Windows se ejecutaron bajo Wine con códigos de salida Windows explícitos. Todas aprobaron la revisión actual:

- **VocalDSP:** 54 combinaciones de perfiles/tasas/bloques, estado, mono y bloques variables. Arranque medido después de la latencia declarada.
- **VST3Module:** fábrica del módulo real, nuevo nombre/CID, buses estéreo con sidechain opcional inactivo, proceso y descarga.
- **AlienInterface:** 26 módulos y botones AI, 113 perillas, 26 barras paralelas, 280 presets, bypass enlazado y capturas de las tres páginas.
- **AlienRack:** regresión de los doce bloques base, cincuenta presets del subconjunto original, ducking, trece rutas de análisis y persistencia A/B.
- **TripleGodDSP:** 280 nombres/configuraciones distintos procesados; 50 efectos nuevos; salida finita, no silenciosa y techo de muestras. Corrección 223 → 220 Hz, quintas/octavas y ocho voces. Tres reverbs independientes con suma de error cuadrático 4.46e−19 y colas corta/media/larga diferenciadas. Tape ducking −20.4 dB; Throw silencioso durante voz continua y retorno después de la frase. Alineación de 25 ms encontrada con confianza 1 sobre el ejemplo sintético, error relativo 3.39e−15. Trece rutas adicionales de análisis de módulos avanzados.

Impulso a 48 kHz: Monitor 0 muestras; Studio declaradas 7164 y pico 7165 (respuesta IIR); Linear declaradas y pico 7282. Bypass coincide exactamente con el retardo declarado. Las etapas de color usan 8× en Studio/Linear.

Reducción de ruido sintético: 9.08/8.93/8.85 dB a 44.1/48/96 kHz. De-esser 3.02 dB; expansión de señal tenue 23.53 dB. Reducción del producto de alias de 20.59 dB en la prueba existente. Bus de tres notas conserva sus fundamentales. Cadena Natural: pico 220.5 Hz para entrada de 223 Hz y objetivo 220 Hz.

La compensación tonal evita atenuar uniformemente la voz y la protección armónica limita la reducción de parciales válidos. El shimmer transpone una proyección de la red sin reducir todo el campo de realimentación; las colas independientes se prueban por separado y juntas. La macro limita su ganancia automática de salida.

## IA y alcance

La CNN obtuvo 99.24 % de exactitud balanceada sobre 3600 candidatos sintéticos independientes, no sobre voces de cantantes reales. La MLP obtuvo MAE 0.00386 frente a la política DSP definida en 3000 ejemplos separados; no mide preferencia musical. Archivo: `Models/V8_METRICS.json`.

La alineación compara envolventes contra el sidechain Lead reference, tiene rango ±60 ms y añade 80 ms compensados cuando se activa. No usa una referencia inexistente ni edita una línea temporal arbitraria.

Los seis previos y 48V/impedancia son modelos digitales, no control físico de una interfaz. Las referencias a fabricantes y artistas no certifican equivalencia ni participación. El limitador controla picos de muestras, no true-peak certificado.

## Puerta de publicación nativa

El workflow `publish-v10-release.yml` debe compilar con MSVC en Windows 2022, aprobar las cinco pruebas CTest, instalar el VST3, comparar el módulo instalado por SHA-256, cargarlo/procesarlo y desinstalarlo conservando las versiones previas. Solo entonces publica el instalador, checksum y tres capturas de la interfaz.

**Pendiente:** registrar el resultado final de esa ejecución y verificar descargas públicas. Studio One 7 y evaluación perceptual con voces reales siguen pendientes. El release se mantiene como pre-release y el instalador no tiene firma digital. No se afirma superioridad a UAD, iZotope, Soothe u otros productos ni una calidad universal de estudio.
