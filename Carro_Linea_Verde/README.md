# Carro_Linea_Verde — Segundo robot (ESP32-DO)

Proyecto **separado** de `Rescate_ESP32`.

---

## 1. Listado de componentes

| Cant. | Componente | Uso |
|------:|------------|-----|
| 1 | **ESP32-DO** | MCU + driver de motores |
| 2 | Motor **N20 + encoder** XY-N20 | Tracción + odometría |
| 2 | Rueda + soporte N20 | — |
| 1 | Sensor de línea **5 IR** | Seguidor (GPIO 39…35) |
| 2 | **GY-AS7341** (color, I2C) | Cuadros verdes izq / der |
| 1 | **VL53L0X** (I2C) | Obstáculo ~10 cm |
| 1 | Módulo UPS 18650 (5 V) | Alimentación |
| 1 | Pulsador START | Arranque / parada |

---

## 2. Conexiones (corregidas)

![Conexiones Carro_Linea_Verde](../docs/images/conexiones-esp32-do.jpg)

### Motores (driver integrado ESP32-DO)

| Motor | Conector placa | GPIO control |
|-------|----------------|--------------|
| **M1** (izquierdo) | M1 | **2** y **4** |
| **M2** (derecho) | M2 | **13** y **27** |

Adelante = PWM en pin B; atrás = PWM en pin A (ejemplo fabricante).

### Encoders (además de alimentación del encoder)

Cada N20 lleva cable de encoder: **VCC, GND, A, B**.

| Motor | Encoder | Canal A | Canal B |
|-------|---------|--------:|--------:|
| M1 | **Encoder 0** | **16** | **25** |
| M2 | **Encoder 1** | **17** | **26** |

### Sensor de línea 5 IR

Cinco señales a los ADC/GPIO de la fila inferior de la ESP-DO, en el **orden físico de la placa**:

| Canal (izq → der) | GPIO | Nota |
|-------------------|-----:|------|
| 0 (extremo izq) | **39** | |
| 1 | **32** | |
| 2 (centro) | **33** | sensor central |
| 3 | **34** | |
| 4 (extremo der) | **35** | |
| VCC / GND | 3.3 V / GND | |

### Bus I2C (header IIC de la ESP-DO)

La placa permite **3 conexiones simultáneas** al mismo bus (**SDA = 21**, **SCL = 22**):

| Puerto IIC | Dispositivo | Dirección típica |
|------------|-------------|------------------|
| 1 | GY-AS7341 **izquierdo** | 0x39 |
| 2 | GY-AS7341 **derecho** | 0x39* |
| 3 | VL53L0X frontal | 0x29 |

\*Si ambos AS7341 comparten la misma dirección, hace falta un **mux I2C** (p. ej. TCA9548A) o un segundo bus por software. XSHUT del VL53 puede ir a 3.3 V si solo hay un láser.

### Pulsador

| START | GPIO **19** → GND (INPUT_PULLUP) |

### Alimentación

- GND común entre batería, ESP-DO, motores (vía driver) y sensores.  
- Lógica de sensores: **3.3 V**.  
- Motores: según UPS (5 V típico en ESP-DO).

---

## 3. Calibración de encoders

1. Suba el firmware.  
2. Serial (115200), tecla **`e`**: conteos Encoder 0 / Encoder 1.  
3. Gire 90° y ajuste `PULSOS_90` / `PULSOS_180` en `config.h`.

---

## 4. Reglas de pista (resumen)

| Verde | Acción en intersección |
|-------|------------------------|
| Solo izquierda | Giro 90° izq |
| Solo derecha | Giro 90° der |
| Ambos | 180° (punto muerto) |
| Ninguno | De frente |
| L2 lateral | Memoria de lado para el cuadrado F1 |
| Obstáculo ~10 cm | Evasión + recuperar línea |

---

## 5. Archivos

```
Carro_Linea_Verde/
  Carro_Linea_Verde.ino
  config.h  motores.h  encoders.h
  linea.h   color.h    obstaculo.h
  control_pid.h  evasion.h  maquina_estados.h
  README.md
```
