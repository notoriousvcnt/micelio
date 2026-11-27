# Arquitectura

```
LD2410C --UART 256000--> ESP32 --PWM/RMT--> LEDs PWM / WS2812B
                           |
                           +--UART 9600--> DY-SV5W        (variante DY-SV5W)
                           +--UART 115200--> Raspberry Pi (variante RASPI)
                           +--WiFi--> OTA + WebSerial
```

## Bucle principal (`loop()`)

1. `ArduinoOTA.handle()` si hay WiFi.
2. Con `SENSOR`: `readRadar()`. Sin `SENSOR`: `globalUpdateNoInteractive()` (modo rotativo).
3. `sendDataToRASPI()` + `SerialRecvMsg()` (variante RASPI) o `sendDataToAudioModule()`.
4. `updateLights()` y `updateBuiltinLed()`.

## Estados

| `state` | Significado |
|---|---|
| 0 | Sin presencia: luces se apagan, volumen bajo |
| 1 | Presencia: pulso de luz, volumen alto |
| 2 | Presencia estacionaria (ver nota) |

> **Nota:** `state` está declarado como `bool`, por lo que el valor 2 se
> convierte en 1. Hoy el estado 2 no se distingue del 1. Ver
> [analisis-algoritmos.md](analisis-algoritmos.md).

El estado 2 se activa cuando la energía estacionaria es ≥ 80, la distancia
≤ 100 cm y la condición dura más de 5 s (`stillTimeThresholdMs`).

## Modo rotativo

Cada `rotativaTimeSeconds` el contador `RotativaCounter` avanza módulo
`TOTAL_DEVICES` (8). El dispositivo cuyo `deviceNumber` coincide pasa a estado 1;
los demás a 0. No hay sincronización entre dispositivos.

## Configuración persistente

Se guarda en NVS con `Preferences` (namespace `deviceSettings`): volúmenes,
brillo PWM, velocidades, tamaño de pulso, tiempo de rotación, nombre e IP.

## Módulos propios (`firmware-esp32/include/`)

| Archivo | Contenido |
|---|---|
| `Segment.h` | Animaciones de tiras WS2812B (pulso viajero, respiración) |
| `PWMLed.h` | LED PWM de 16 bits con respiración senoidal |
| `movingAverage.h` | Media móvil de distancia (hoy sin usar) |
| `utils.h` | `RandomUrn`, selección aleatoria sin repetición (sin usar) |
