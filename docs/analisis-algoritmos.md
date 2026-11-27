# Análisis de algoritmos y problemas conocidos

Resumen de una revisión estática del código (no se compiló ni se ejecutó).
No hay DSP de audio propio: el audio lo reproduce el módulo DY-SV5W o Pure Data.

## Programación

| Algoritmo | Dónde | Observación |
|---|---|---|
| Máquina de estados de presencia | `readRadar()` | `state` es `bool`: el estado 2 colapsa a 1. `timerHasStarted` no se reinicia al perder presencia. Sin histéresis al volver a 0. |
| Planificador rotativo | `globalUpdateNoInteractive()` | Sin sincronización entre dispositivos. |
| Urna aleatoria (Fisher-Yates) | `include/utils.h` | Correcta pero sin uso; arreglo fijo de 100. |
| Media móvil con buffer deslizante | `include/movingAverage.h` | `getFilteredDistance()` nunca se llama. N=10 fijo dentro de la función. |
| Parser de tramas LD2410 | `lib/ld2410/` (terceros) | Máquina de estados byte a byte; modo ingeniería sin implementar. |
| Parser de comandos de texto | `recvMsg()`, `SerialRecvMsg()` | Cadena de `if/else` con `String`; sin validar rangos. |

## DSP y señales de control

| Algoritmo | Dónde | Observación |
|---|---|---|
| Media móvil (FIR boxcar) | `movingAverage.h` | Sin uso; división entera trunca. Una mediana sería mejor ante ruido impulsivo. |
| Decaimiento exponencial de LEDs | `fadeToBlackBy` en `Segment.h` | Depende de la tasa del `loop()`, no del reloj. |
| Pulso viajero (fase `beat16`) | `Segment::pulseAnimation2` | Posible escritura fuera del buffer de LEDs. |
| Respiración PWM (`beatsin16` + ease) | `PWMLed::breathingBrightness` | Seno + ease cuadrático lo deforma; sin corrección gamma. |
| PWM 16 bits a 200 Hz | `PWMLed::begin` | 200 Hz puede verse o parpadear en cámara. |
| Rampa de volumen (`line~`) | patch `.pd` | Lineal en amplitud, no en dB. |

## Errores y riesgos priorizados

1. `bool state` convierte 2 en 1 (estado "estacionario" inalcanzable).
2. `Segment(..., 0, MAX_NUM_LEDS, ...)` calcula `numLeds = MAX_NUM_LEDS + 1`
   sobre un arreglo de `MAX_NUM_LEDS`; `leds[ledPos + i]` puede salirse del arreglo.
3. Distancia filtrada calculada y no usada; sin histéresis al perder presencia.
4. Animaciones dependientes de la tasa del loop.
5. En `loadPreferences()` el tiempo de rotación de `CAPULLO` (360 s) se
   sobrescribe siempre por el valor general (180 s).
6. Patch Pd con rutas absolutas de Windows y `urn 4` frente a ~16 archivos.

## No cubierto

PDFs de esquemáticos, comandos de configuración de `ld2410.cpp` y el cableado
detallado del `.pd`.
