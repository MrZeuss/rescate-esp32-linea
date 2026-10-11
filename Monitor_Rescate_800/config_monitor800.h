#pragma once
// ============================================================
// config_monitor800.h — Panel RGB 800x480 con ESP32-S3
//
// ¡SELECTOR DE MODELO! Descomenta UNA sola línea:
// ============================================================
#define MODELO_VIEWE_70      // 7.0" VIEWE UEDX80480070E-WB-A  <- tu placa
// #define MODELO_S043       // 4.3" Sunton ESP32-8048S043
// #define MODELO_S070       // 7.0" Sunton ESP32-8048S070

// ---------- VIEWE 7" (pines verificados en tu placa: la mitad
// visible de la imagen tenía geometría y colores correctos, lo
// que confirma sincronía y bus de datos; el panel de 7" usa la
// temporización larga estándar EK9716) ----------
#ifdef MODELO_VIEWE_70
  #define PIN_DE     40
  #define PIN_VSYNC  41
  #define PIN_HSYNC  39
  #define PIN_PCLK   42
  #define PINS_R     45, 48, 47, 21, 14
  #define PINS_G      5,  6,  7, 15, 16, 4
  #define PINS_B      8,  3, 46,  9,  1
  #define LCD_H_FP  210
  #define LCD_H_PW   30
  #define LCD_H_BP   16
  #define LCD_V_FP   22
  #define LCD_V_PW   13
  #define LCD_V_BP   10
  #define LCD_PCLK  12000000   // 16M tiembla con WiFi activo: 12M da margen al DMA
#endif

#ifdef MODELO_S043
  #define PIN_DE     40
  #define PIN_VSYNC  41
  #define PIN_HSYNC  39
  #define PIN_PCLK   42
  #define PINS_R     45, 48, 47, 21, 14
  #define PINS_G      5,  6,  7, 15, 16, 4
  #define PINS_B      8,  3, 46,  9,  1
  #define LCD_H_FP   8
  #define LCD_H_PW   4
  #define LCD_H_BP  43
  #define LCD_V_FP   8
  #define LCD_V_PW   4
  #define LCD_V_BP  12
  #define LCD_PCLK  12000000   // 16M tiembla con WiFi activo: 12M da margen al DMA
#endif

#ifdef MODELO_S070
  #define PIN_DE     41
  #define PIN_VSYNC  40
  #define PIN_HSYNC  39
  #define PIN_PCLK   42
  #define PINS_R     14, 21, 47, 48, 45
  #define PINS_G      9, 46,  3,  8, 16,  1
  #define PINS_B     15,  7,  6,  5,  4
  #define LCD_H_FP  210
  #define LCD_H_PW   30
  #define LCD_H_BP   16
  #define LCD_V_FP   22
  #define LCD_V_PW   13
  #define LCD_V_BP   10
  #define LCD_PCLK  12000000   // 16M tiembla con WiFi activo: 12M da margen al DMA
#endif

// Guía de ajuste fino de timings:
//   corrida a la IZQUIERDA -> SUBIR LCD_H_BP | a la DERECHA -> BAJARLO
//   corrida ARRIBA -> SUBIR LCD_V_BP | ABAJO -> BAJARLO
//   imagen tiembla/ondula -> BAJAR LCD_PCLK a 14000000 o 12000000

#define PIN_LCD_BL   2       // backlight (HIGH = encendido)

// ---------- Touch GT911 (I2C) ----------
#define PIN_TP_SDA   19
#define PIN_TP_SCL   20
#define PIN_TP_RST   38
#define PIN_TP_INT   18
#define DIR_GT911    0x5D
#define TP_INVERTIR_X  false
#define TP_INVERTIR_Y  false

// ---------- Interfaz ----------
#define T_REFRESCO_MS   150
#define T_REPETIR_MS    220
#define T_ENLACE_MS     600
