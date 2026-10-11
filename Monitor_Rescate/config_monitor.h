#pragma once
// ============================================================
// config_monitor.h — Waveshare ESP32-S3-Touch-LCD-2
// Pinout extraído del esquemático oficial del fabricante.
// Placa en Arduino IDE: "ESP32S3 Dev Module"
//   (USB CDC On Boot: Enabled, PSRAM: OPI PSRAM)
// ============================================================

// ---------- LCD ST7789T3 (SPI) 240x320 ----------
#define PIN_LCD_MOSI  38
#define PIN_LCD_SCLK  39
#define PIN_LCD_CS    45
#define PIN_LCD_DC    42
#define PIN_LCD_RST   -1     // reset por hardware en la placa
#define PIN_LCD_BL     1     // backlight (HIGH = encendido)

// ---------- Touch CST816D (I2C) ----------
#define PIN_TP_SDA    48
#define PIN_TP_SCL    47
#define PIN_TP_INT    46
#define DIR_CST816    0x15
// Si al tocar una esquina el Serial reporta la esquina opuesta,
// invierte el eje correspondiente (algunos paneles vienen espejados)
#define TP_INVERTIR_X  false
#define TP_INVERTIR_Y  false

// ---------- Interfaz ----------
#define T_REFRESCO_MS   150   // refresco del tablero
#define T_REPETIR_MS    220   // auto-repetición al mantener +/-
#define T_ENLACE_MS     600   // sin paquetes por más de esto = sin señal
