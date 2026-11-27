# Audio en Raspberry Pi Zero 2 W

`puredata/micelio-player.pd` reproduce archivos WAV al azar (sin repetir, con
`cyclone/urn`) y ajusta el volumen con una rampa de 2 s según los mensajes
`State` / `Level` que envía el ESP32 por serie (ver `../docs/protocolo-serie.md`).

## Requisitos

- Pure Data con las externas `cyclone` y `comport`.
- Salida de audio configurada: `../docs/configuracion-audio-rpi.txt`.
- El patch declara `-path /home/pi/Documents/pd`.

## Archivos de audio

La lista `audio-files` del patch contiene **rutas absolutas de Windows**
(`C:/Users/.../FREELANCE/2024/MICELIO/CODIGOS/pd/*.wav`) que no existen en la Pi.
El patch tiene objetos para listar los WAV de una carpeta (`file glob`, `*.wav`);
revisa que la lista se regenere con la carpeta de la Pi o edítala a mano.

Además, `cyclone/urn 4` sugiere un conjunto de 4 elementos mientras la lista
trae unos 16 archivos: confirma que el tamaño se actualiza dinámicamente.
Ambos puntos quedaron sin verificar.

Los WAV no están en el repositorio.
