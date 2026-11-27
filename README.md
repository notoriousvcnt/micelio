# MICELIO

Instalación interactiva que reacciona a la presencia de personas con luz
(tiras WS2812B y LEDs PWM) y sonido (módulo DY-SV5W, o una Raspberry Pi Zero 2 W
con Pure Data). Un radar LD2410C detecta presencia y un ESP32 coordina todo.

## Estructura del repositorio

```
firmware-esp32/   Firmware del ESP32 (PlatformIO, Arduino)
audio-rpi/        Patch de Pure Data para la Raspberry Pi Zero 2 W
docs/             Arquitectura, protocolo, comandos, esquemáticos y análisis
```

## Cómo funciona

1. El LD2410C detecta presencia y si la persona permanece quieta.
2. El ESP32 cambia de estado, anima las luces y ajusta el volumen.
3. Con la variante `RASPI`, el ESP32 envía `State` y `Level` por serie a Pure Data,
   que reproduce WAV al azar con una rampa de volumen de 2 s.
4. Hasta 8 dispositivos se turnan (modo rotativo) cada `rotativaTimeSeconds`
   (180 s por defecto).

Más detalle en [docs/arquitectura.md](docs/arquitectura.md).

## Hardware (pines del ESP32)

| Componente | Pines |
|---|---|
| LD2410C (radar) | RX 32, TX 33 |
| DY-SV5W (audio) | RX 16, TX 17, Busy 5 |
| WS2812B (5 tiras) | 13, 14, 21, 25, 26 |
| LEDs PWM (5) | 4, 19, 22, 23, 27 |
| Raspberry Pi Zero 2 W | RX 1, TX 3 |
| LED integrado | 2 |

Esquemas en [docs/esquematicos/](docs/esquematicos/).

## Variantes de firmware

Definidas en `firmware-esp32/platformio.ini`:

| Entorno | Audio | Notas |
|---|---|---|
| `esp32-dy-sv5w` (por defecto) | Módulo DY-SV5W | |
| `esp32-rpi` | Pure Data en Raspberry Pi | flag `RASPI` |
| `esp32-capullo` | | flag `CAPULLO`, 1200 LEDs |
| `esp32-test-pines` | | activa las 5 tiras RGB y los 5 PWM |

> **Importante:** el `platformio.ini` original no estaba en el repo. El actual se
> reconstruyó desde los `#ifdef` del código y **no se ha compilado**. Revisa los
> comentarios `TODO` (placa, versiones de librerías) antes de usarlo.

## Puesta en marcha

1. Copia `firmware-esp32/include/secrets.example.h` a `secrets.h` y completa el WiFi.
   Sin `secrets.h` el firmware compila con valores ficticios.
2. En `platformio.ini`, ajusta `DEVICE_NAME`, `DEVICE_IP`, `DEVICE_NUMBER`,
   `GATEWAY_IP` y `SUBNET_IP` para cada placa. La primera vez, activa
   `-DUPLOAD_DEVICE_PROPERTIES` para guardarlos en NVS y luego quítalo.
3. `pio run -e <entorno> -t upload` (primera carga por USB; después por OTA).
4. En la Pi: instala Pure Data con `cyclone` y `comport`, configura el audio
   ([docs/configuracion-audio-rpi.txt](docs/configuracion-audio-rpi.txt)) y ajusta
   las rutas de los WAV (ver [audio-rpi/README.md](audio-rpi/README.md)).

## Control remoto

Consola WebSerial por WiFi para ver el estado y cambiar parámetros.
Lista de comandos en [docs/comandos-webserial.md](docs/comandos-webserial.md).

## Documentación

- [Arquitectura](docs/arquitectura.md)
- [Protocolo serie ESP32 → Raspberry Pi](docs/protocolo-serie.md)
- [Comandos WebSerial](docs/comandos-webserial.md)
- [Análisis de algoritmos y problemas conocidos](docs/analisis-algoritmos.md)

## Licencias

Código propio: por definir. La librería `ld2410` (`firmware-esp32/lib/ld2410/`)
mantiene su propia licencia (`LICENSE` en esa carpeta).
