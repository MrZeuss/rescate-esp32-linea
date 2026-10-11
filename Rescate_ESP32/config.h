#pragma once
// ============================================================
// config.h — Pines y constantes de calibración (ESP32-DO)
//
// Hardware:
//   ESP32-DO | HW-871 | 3x VL53L0X | GY-91 (MPU) | monitor ESP-NOW
//
// Este es el ÚNICO archivo que conviene tocar en la pista.
// ============================================================
//
// ============================================================
//   MAPA DE CONEXIONES  (módulo  →  pin ESP32-DO)
// ============================================================
//
// --- HW-871  (seguidor de línea, 5 canales digitales) --------
//   HW-871          ESP32-DO
//   -------------   --------
//   GND             GND
//   VCC             3.3 V   (o 5 V + divisor en cada OUT; ver notas)
//   OUT1 (izq)      GPIO 39
//   OUT2            GPIO 32
//   OUT3 (centro)   GPIO 33
//   OUT4            GPIO 34
//   OUT5 (der)      GPIO 35
//
// --- GY-91  (MPU-9250/6500 + barómetro; usamos el IMU por I2C) ---
//   GY-91           ESP32-DO
//   -------------   --------
//   VIN             (sin conectar si alimenta por 3V3)
//   3V3             3.3 V
//   GND             GND
//   SCL             GPIO 22
//   SDA             GPIO 21
//   SDD/SA0         GND     → dirección I2C 0x68
//                   (a 3.3 V → dirección 0x69)
//   NCS             3.3 V   → fuerza modo I2C en el IMU (no SPI)
//   CSB             3.3 V   → deja el BMP280 en reposo / I2C
//
//   Nota: SDD/SA0 es el selector de dirección (equivalente a AD0
//   de un MPU-6500 suelto). NCS y CSB no van al ESP32.
//
// --- VL53L0X frontal  (obstáculo / laberinto) ----------------
//   VL53 frontal    ESP32-DO
//   -------------   --------
//   VIN / VCC       3.3 V
//   GND             GND
//   SDA             GPIO 21   (bus I2C compartido)
//   SCL             GPIO 22
//   XSHUT           GPIO 25   → dirección reasignada 0x30
//
// --- VL53L0X derecho ----------------------------------------
//   VL53 derecho    ESP32-DO
//   -------------   --------
//   VIN / VCC       3.3 V
//   GND             GND
//   SDA             GPIO 21
//   SCL             GPIO 22
//   XSHUT           GPIO 26   → dirección reasignada 0x31
//
// --- VL53L0X izquierdo --------------------------------------
//   VL53 izquierdo  ESP32-DO
//   -------------   --------
//   VIN / VCC       3.3 V
//   GND             GND
//   SDA             GPIO 21
//   SCL             GPIO 22
//   XSHUT           GPIO 17   → dirección reasignada 0x32
//
// --- Motores  (driver integrado del ESP32-DO) ----------------
//   Señal           GPIO
//   -------------   ----
//   Motor 1 A       GPIO  2
//   Motor 1 B       GPIO  4
//   Motor 2 A       GPIO 13
//   Motor 2 B       GPIO 27
//   (M1 = izquierdo si M1_ES_IZQUIERDO = true)
//
// --- Pulsadores y LED ---------------------------------------
//   Elemento        ESP32-DO
//   -------------   --------
//   BTN_CALIB       GPIO 18  → GND  (INPUT_PULLUP)
//   BTN_START       GPIO 19  → GND  (INPUT_PULLUP)
//   LED_META        GPIO 14
//
// --- Monitor  (otro ESP32; sin cables de datos) --------------
//   Comunicación por radio ESP-NOW, canal 1 (WiFi).
//   No ocupa GPIO de sensores.
//
// --- Resumen de GPIO usados ---------------------------------
//    2, 4, 13, 27     motores
//   14                LED meta
//   17, 25, 26        XSHUT de los tres VL53L0X
//   18, 19            pulsadores
//   21, 22            I2C (GY-91 + VL53L0X)
//   32, 33, 34, 35, 39  HW-871
//
// ============================================================

// ---------- HW-871: BARRA DE 5 SENSORES DE LÍNEA ----------
// Orden físico de IZQUIERDA a DERECHA visto desde atrás del carro.
// Si la barra quedó al revés, invierte el orden aquí.
#define PIN_LINEA_0   39   // OUT1 extremo izquierdo (input-only)
#define PIN_LINEA_1   32   // OUT2
#define PIN_LINEA_2   33   // OUT3 centro
#define PIN_LINEA_3   34   // OUT4 (input-only)
#define PIN_LINEA_4   35   // OUT5 extremo derecho (input-only)

// true  = el HW-871 entrega HIGH sobre negro (poco frecuente)
// false = el HW-871 entrega LOW  sobre negro (típico con LM393)
// Si los círculos del monitor salen al revés, cambie este flag.
#define LINEA_NEGRO_ALTO  false

// true  = leer OUT digitales del HW-871 (recomendado)
// false = leer ADC analógico (solo si el módulo tiene pines A0..A4)
#define LINEA_MODO_DIGITAL  true

// Oversampling solo en modo analógico (mejora SNR a mayor altura)
#define LINEA_MUESTRAS_ADC  4

// Umbral normalizado 0..1000 para "veo negro" (modo analógico).
// En modo digital el valor es 0 u 1000; este umbral casi no importa.
#define UMBRAL_NEGRO      500
#define N_SENSORES_CRUCE    4   // >= sensores en negro = cruce / meta
#define T_CALIBRACION    4000   // ms de barrido (útil sobre todo en analógico)
#define CONTRASTE_MIN_ADC 80    // rango mínimo útil por canal (analógico)

// ---------- VARIOS ----------
#define PIN_LED_META  14
#define PIN_BTN_CALIB 18   // pulsador a GND (INPUT_PULLUP)
#define PIN_BTN_START 19   // pulsador a GND (INPUT_PULLUP)
#define T_DEBOUNCE_BTN 40  // ms de flanco estable en botones

// ---------- MOTORES (driver integrado ESP32-DO) ----------
#define PIN_M1_A   2
#define PIN_M1_B   4
#define PIN_M2_A   13
#define PIN_M2_B   27

#define M1_ES_IZQUIERDO     true
#define INVERTIR_MOTOR_IZQ  false
#define INVERTIR_MOTOR_DER  false

// ---------- I2C: GY-91 (MPU) + 3x VL53L0X ----------
#define PIN_SDA  21
#define PIN_SCL  22
#define MPU_DIR  0x68          // SA0 libre o a GND; si SA0 a 3V3 use 0x69
#define MPU_WHO_ESPERADO 0x70  // MPU-6500 (GY-91 de prueba); también 0x71

// ============================================================
//                 CONSTANTES DE CALIBRACIÓN
// ============================================================

// ---------- VELOCIDADES (PWM 0..255) ----------
#define VEL_BASE       130
#define VEL_MAX        220
#define VEL_CRUCE      120
#define VEL_GIRO       120
#define VEL_RECUPERAR  100
#define VEL_SUBIDA     230
#define VEL_BAJADA      70
#define VEL_EVASION    120

// ---------- CONTROL PD ----------
#define KP  0.085f
#define KD  0.90f

// ---------- TIEMPOS DE LA MÁQUINA DE ESTADOS (ms) ----------
#define T_CRUCE           350
#define T_META            600
#define T_GAP_TIMEOUT     900
#define T_RECUPERAR_MAX  2500
#define T_DEBOUNCE_OBST    60

// ---------- PUENTE (grados de pitch) ----------
#define PITCH_SUBIDA     10.0f
#define PITCH_BAJADA    -10.0f
#define PITCH_PLANO       5.0f
#define SIGNO_PITCH       1

// ---------- GIROSCOPIO (GY-91 / MPU-6500) ----------
// Ajustes tomados del sketch de prueba en Arduino Nano
// (rango fino ±250 °/s, calibración larga, zona muerta anti-deriva).
#define GYRO_LSB_POR_DPS    131.0f  // ±250 °/s (GYRO_CONFIG = 0x00)
#define GYRO_CAL_MUESTRAS   2000    // ~4 s inmóvil al encender
#define GYRO_ZONA_MUERTA    0.35f   // °/s: bajo esto no integra (anti-deriva)
#define KP_YAW              4.0f    // corrección de rumbo al avanzar recto
#define TOL_GIRO            1.0f    // grados para dar el giro por terminado
#define KP_GIRO_ANG         8.0f    // PWM por grado de error (acercamiento fino)
#define VEL_GIRO_MIN         45     // PWM mínimo al acercarse al objetivo
#define T_GIRO_MAX         2500     // timeout de un pivote (ms)
#define SIGNO_YAW             1     // -1 si gira al revés de lo pedido

// ---------- EVASIÓN DEL OBSTÁCULO (cubo de 10 cm) ----------
#define EVADIR_POR_IZQUIERDA  true
#define T_EV_LATERAL     700
#define T_EV_PARALELO   1100
#define T_EV_BUSCAR     2500
#define T_EV_SOBREPASO   150

// ============================================================
//        SENSORES LÁSER VL53L0X (obstáculo + laberinto)
// ============================================================
// Cableado: VIN->3.3V | GND | SDA->21 | SCL->22 | XSHUT->GPIO
#define PIN_XSHUT_FRONTAL  25
#define PIN_XSHUT_DER      26
#define PIN_XSHUT_IZQ      17

#define DIR_VL53_FRONTAL   0x30
#define DIR_VL53_DER       0x31
#define DIR_VL53_IZQ       0x32

#define DIST_OBSTACULO     120    // mm -> EVASION

#define DIST_TRANS_FRENTE  400
#define DIST_TRANS_LADO    300

#define DIST_PARED_OBJ      80
#define KP_PARED           0.8f
#define CORR_PARED_MAX      60
#define VEL_LABERINTO      110
#define DIST_FRENTE_GIRO   100
#define DIST_DER_ABIERTA   250
#define T_LAB_AVANCE_APERTURA 450
#define T_LAB_AVANCE_POST_GIRO 500

// ============================================================
//        TELEMETRÍA ESP-NOW (entrenamiento / monitor)
// ============================================================
// Poner en false para la competencia (radio prohibida).
// Los parámetros en NVS persisten aunque se apague la radio.
#define USAR_ESPNOW  true
#define T_TELE_MS    50      // 20 Hz
#define T_INFORME_MS 2000
