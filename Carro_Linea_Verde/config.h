#pragma once
// ============================================================
// config.h — Carro_Linea_Verde (ESP32-DO)
// ============================================================
//
// MOTORES (driver integrado):
//   M1 (izq)  A=2   B=4     + Encoder 0: A=16  B=25
//   M2 (der)  A=13  B=27    + Encoder 1: A=17  B=26
//   Adelante = PWM en pin B
//
// LÍNEA 5 IR (orden físico en la placa ESP-DO):
//   GPIO 39, 32, 33, 34, 35   →  canal 0..4 (33 = centro)
//
// BUS I2C — header IIC (3 conexiones simultáneas):
//   SDA=21  SCL=22
//   · GY-AS7341 izquierdo
//   · GY-AS7341 derecho
//   · VL53L0X frontal
//
// UI: START=19
// ============================================================

// ---------- Motores ESP32-DO ----------
#define PIN_MI_A    2    // M1
#define PIN_MI_B    4
#define PIN_MD_A   13    // M2
#define PIN_MD_B   27
#define INVERTIR_IZQ  false
#define INVERTIR_DER  false

// ---------- Encoders (oficial ESP32-DO) ----------
#define PIN_ENC0_A  16   // Encoder 0 → M1
#define PIN_ENC0_B  25
#define PIN_ENC1_A  17   // Encoder 1 → M2
#define PIN_ENC1_B  26

#define PULSOS_90           28
#define PULSOS_180          56
#define PULSOS_EV_LATERAL   18
#define PULSOS_EV_PARALELO  35
#define PULSOS_EV_VUELTA    18

// ---------- Línea 5 IR (GPIO en orden de la placa) ----------
#define PIN_L0  39   // izquierdo
#define PIN_L1  32
#define PIN_L2  33   // centro
#define PIN_L3  34
#define PIN_L4  35   // derecho
#define LINEA_NEGRO_LOW   true
#define N_NEGRO_CRUCE     4
#define T_CALIB_MS     3000

// ---------- Color GY-AS7341 (I2C; ver color.h) ----------
#define PIN_I2C_SDA  21
#define PIN_I2C_SCL  22
#define COLOR_DIR_I2C     0x39
#define COLOR_UMBRAL_VERDE  1.15f
// Si ambos AS7341 chocan en dirección, use mux o SoftWire en estos pines:
#define PIN_I2C2_SDA  5
#define PIN_I2C2_SCL  18

// ---------- Obstáculo VL53L0X (mismo bus I2C) ----------
#define OBST_USAR_VL53  true
#define PIN_VL53_SDA    PIN_I2C_SDA
#define PIN_VL53_SCL    PIN_I2C_SCL
#define PIN_US_TRIG     21
#define PIN_US_ECHO     22
#define DIST_OBST_MM    120
#define T_DEBOUNCE_OBST  80

// ---------- UI ----------
#define PIN_BTN_START  19
#define PIN_BTN_CALIB   0
#define PIN_LED        -1
#define T_DEBOUNCE_BTN 40

// ---------- Control ----------
#define VEL_BASE      120
#define VEL_MAX       200
#define VEL_GIRO      110
#define VEL_EVASION   110
#define VEL_CRUCE     100
#define KP            0.090f
#define KD            0.85f

#define T_GAP_MS            700
#define T_META_MS           700
#define T_RECTO_CRUCE_MS    280
#define T_MEMORIA_VERDE_MS 1500
#define T_L2_MS             200

#define EVADIR_POR_IZQUIERDA  true
