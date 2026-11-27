# Comandos WebSerial

Formato: `comando valor` (separados por un espacio). Los comandos sin valor son
`status` y `save`. Implementados en `recvMsg()` (`src/ESP32_LD2410_SV5W_WS2812B.cpp`).

La consola web se sirve con la librería WebSerial; la ruta habitual es
`http://<ip-del-dispositivo>/webserial` (verificar con tu versión de la librería).

## Generales

| Comando | Efecto |
|---|---|
| `status` | Nombre, estado del radar, distancia, estado y busy del reproductor |
| `save` | Guarda los parámetros en NVS y los recarga |
| `getParam <clave>` | Muestra un parámetro guardado (`PWMspeed`, `blobSize`, …) |
| `setState <0\|1\|2>` | Fuerza el estado |
| `setRotativaTime <s>` | Segundos entre turnos del modo rotativo |

## Luces

| Comando | Efecto |
|---|---|
| `setMinBrightness <0–1>` | Brillo mínimo PWM (fracción) |
| `setMaxBrightness <0–1>` | Brillo máximo PWM (fracción) |
| `setPWMspeed <bpm>` | Velocidad de respiración PWM |
| `setRGBspeed <v>` | Velocidad del pulso en las tiras |
| `setRGBfadePercent <0–100>` | Intensidad del decaimiento de la cola |
| `setBlobSize <n>` | Ancho del pulso en LEDs |
| `setRGBBrightness <v>` | Solo imprime el valor, no aplica nada |

## Audio con DY-SV5W (sin `RASPI`)

| Comando | Efecto |
|---|---|
| `setVolume <0–30>` | Volumen directo |
| `setLowVolume <n>` / `setHighVolume <n>` | Volúmenes por estado |

## Audio con Raspberry Pi (`RASPI`)

| Comando | Efecto |
|---|---|
| `RPIsetState <n>` | Fuerza el estado |
| `RPIsetVolume <x>` | Envía `Level x` a la Pi |
| `RPIsetLowVolume <x>` / `RPIsetHighVolume <x>` | Niveles por estado |
| `PreGain <x>` | Envía `PreGain x` a la Pi |

Los cambios no persisten hasta ejecutar `save`. Los valores no se validan en rango:
por ejemplo un `setBlobSize` muy grande puede escribir fuera del buffer de LEDs.
