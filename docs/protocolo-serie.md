# Protocolo serie ESP32 ↔ Raspberry Pi

UART a 115200 baudios, texto ASCII, una línea por mensaje terminada en `\n`.
El ESP32 usa los pines RX 1 / TX 3 (el puerto `Serial`); en la Pi, `/dev/ttyS0`.

## ESP32 → Pi (`sendDataToRASPI()`, cada 1000 ms)

```
State 0|1
Level <float>
```

- `State 1` con `Level` = `RPI_highVolume` (1.0 por defecto) si hay presencia.
- `State 0` con `Level` = `RPI_lowVolume` (0.2 por defecto) si no.
- También se envía `PreGain <valor>` y `Start 1` al recibir `s` desde la Pi.

## Pi → ESP32

- `s` + salto de línea: la Pi pide el arranque. El ESP32 responde `Start 1` y
  `PreGain 2.0` (`PREGAIN_CONFIG`).

## Lado Pure Data

El patch lee el puerto con `comport 0 115200`, acumula bytes hasta CR/LF y
despacha con `route State Level PreGain Start`. No hay checksum: una línea
corrupta simplemente no coincide con ninguna ruta y se descarta.

Cuántos de estos mensajes llegan realmente a la cadena de audio del patch
no se verificó en el análisis (cableado del `.pd` pendiente de revisar).
