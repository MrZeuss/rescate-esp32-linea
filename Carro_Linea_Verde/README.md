# Carro_Linea_Verde — Segundo robot (ESP32-DO)

Proyecto **separado** de `Rescate_ESP32`.  
Pinout de motores y encoders según **ejemplos oficiales ESP32-DO**.

---

## 1. Listado de componentes

| Cant. | Componente | Uso |
|------:|------------|-----|
| 1 | **ESP32-DO** | MCU + driver de motores |
| 2 | Motor **N20 con encoder** (cuadratura A/B) | Tracción |
| 1 | Barra 5 IR (HW-871) | Línea negra 19 mm |
| 2 | Sensor color GY-31 / TCS3200 | Cuadros verdes 25×25 mm |
| 1 | VL53L0X *(o HC-SR04)* | Obstáculo 10 cm |
| 1 | Pulsador START | Arranque / parada |
| 1 | Batería + regulación | — |

---

## 2. Conexiones oficiales ESP32-DO

### Motores (driver integrado)

| Motor | Pin A | Pin B |
|-------|------:|------:|
| M1 (izquierdo) | **2** | **4** |
| M2 (derecho) | **13** | **27** |

Sentido (ejemplo fabricante):

- **Adelante:** `analogWrite(pinA, 0)` + `analogWrite(pinB, velocidad)`
- **Atrás:** `analogWrite(pinA, velocidad)` + `analogWrite(pinB, 0)`
- **Libre:** ambos a 0

En el firmware, velocidad **positiva** = adelante.

### Encoders N20 (cuadratura)

| Encoder | Canal A | Canal B |
|---------|--------:|--------:|
| Enc 1 → M1 | **16** | **25** |
| Enc 2 → M2 | **17** | **26** |

ISR en ambos flancos (CHANGE), igual que el ejemplo oficial.

### Línea HW-871

| OUT1…OUT5 (izq→der) | 39, 32, 33, 34, 35 |
| VCC / GND | 3.3 V / GND |

### Color GY-31 × 2 *(pines libres; 25 y 26 son encoders)*

| Señal | GPIO |
|-------|-----:|
| S0 | 5 |
| S1 | 12 |
| S2 | 14 |
| S3 | 18 |
| OUT izquierdo | 15 |
| OUT derecho | 23 |

### Obstáculo VL53L0X

| SDA | SCL | XSHUT | VIN |
|----:|----:|-------|-----|
| 21 | 22 | 3.3 V | 3.3 V |

### Pulsador

| START | GPIO **19** → GND |

---

## 3. Calibración de encoders

1. Suba el firmware.  
2. En Serial (115200), tecla **`e`**: imprime conteos M1 y M2.  
3. Haga girar el carro 90° a mano o con un comando de prueba.  
4. Anote el valor medio y póngalo en `PULSOS_90` / `PULSOS_180` en `config.h`.

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
  config.h
  motores.h      ← pinout y sentido oficiales
  encoders.h     ← cuadratura 16/25 y 17/26
  linea.h  color.h  obstaculo.h
  control_pid.h  evasion.h  maquina_estados.h
  README.md
```

Serial útil: **`d`** = debug color, **`e`** = lecturas encoder.
