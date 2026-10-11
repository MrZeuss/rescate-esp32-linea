# Rescate ESP32 — Robots de línea / Rescue Line

Repositorio con **dos robots** independientes para competencia tipo rescate / seguidor de línea.

## Proyectos

| Carpeta | Descripción |
|---------|-------------|
| **`Rescate_ESP32`** | Carro complejo: HW-871, 3× VL53L0X, GY-91, laberinto, telemetría ESP-NOW |
| **`Monitor_Rescate`** | Monitor táctil 240×320 (ajuste en vivo + modo prueba) |
| **`Monitor_Rescate_800`** | Monitor 800×480 |
| **`Carro_Linea_Verde`** | Segundo carro (simplificado): N20+encoder, verdes en intersecciones, obstáculo 10 cm |

## Hardware principal

- Placa **ESP32-DO** (driver de motores integrado: M1=2/4, M2=13/27; encoders 16/25 y 17/26)
- Barra de línea 5 IR (HW-871)
- Sensores según cada proyecto (ver `config.h` / `README.md` de cada carpeta)

## Uso

Abrir el `.ino` correspondiente en Arduino IDE, seleccionar placa **ESP32**, ajustar pines en `config.h` si hace falta, y subir.
