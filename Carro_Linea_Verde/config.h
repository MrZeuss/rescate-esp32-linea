#pragma once
// ============================================================
// config.h — Carro_Linea_Verde (ESP32-DO, pinout oficial)
// ============================================================
//
// MOTORES (driver integrado):
//   M1 (izq)  A=2   B=4
//   M2 (der)  A=13  B=27
//   Adelante = PWM en pin B  (según ejemplo del fabricante)
//
// ENCODERS N20 (cuadratura):
//   Enc M1: A=16  B=25
//   Enc M2: A=17  B=26
//
// LÍNEA HW-871: 39, 32, 33, 34, 35
// COLOR GY-31:  S0=5 S1=12 S2=14 S3=18  OUT_I=15 OUT_D=23
// OBSTÁCULO:    VL53 SDA=21 SCL=22  (XSHUT a 3.3V)
// UI: START=19  CALIB=0 (pull-up)  LED= opcional por Serial
// ============================================================

// ---------- Motores ESP32-DO ----------
#define PIN_MI_A    2    // M1
#define PIN_MI_B    4
#define PIN_MD_A   13    // M2
#define PIN_MD_B   27
#define INVERTIR_IZQ  false
#define INVERTIR_DER  false

// ---------- Encoders (oficial) ----------
#define PIN_ENC0_A  16   // M1
#define PIN_ENC0_B  25
#define PIN_ENC1_A  17   // M2
#define PIN_ENC1_B  26

// Pulsos para giros (CALIBRAR en pista mirando Serial)
#define PULSOS_90           28
#define PULSOS_180          56
#define PULSOS_EV_LATERAL   18
#define PULSOS_EV_PARALELO  35
#define PULSOS_EV_VUELTA    18

// ---------- Línea HW-871 ----------
#define PIN_L0  39
#define PIN_L1  32
#define PIN_L2  33
#define PIN_L3  34
#define PIN_L4  35
#define LINEA_NEGRO_LOW   true
#define N_NEGRO_CRUCE     4
#define T_CALIB_MS     3000

// ---------- Color TCS3200 / GY-31 (pines libres; 25/26 son encoders) ----------
#define PIN_S0  5
#define PIN_S1  12
#define PIN_S2  14
#define PIN_S3  18
#define PIN_OUT_IZQ  15
#define PIN_OUT_DER  23
#define COLOR_UMBRAL_VERDE  1.15f
#define COLOR_MAX_PULSO_US  50000UL

// ---------- Obstáculo ----------
#define OBST_USAR_VL53  true
#define PIN_VL53_SDA    21
#define PIN_VL53_SCL    22
#define PIN_US_TRIG     21
#define PIN_US_ECHO     22
#define DIST_OBST_MM    120
#define T_DEBOUNCE_OBST  80

// ---------- UI ----------
#define PIN_BTN_START  19
#define PIN_BTN_CALIB   0    // GPIO0 (boot); use con cuidado o solo Serial
#define PIN_LED        -1    // -1 = sin LED de hardware
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
