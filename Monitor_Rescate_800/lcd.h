#pragma once
// ============================================================
// lcd.h — Panel RGB paralelo 800x480 vía Arduino_GFX
// Librería: "GFX Library for Arduino" (moononournation)
//
// A diferencia del ST7789 (SPI), el panel RGB se refresca solo
// desde un framebuffer en PSRAM (768 KB) por DMA: todo lo que
// se dibuja aparece de inmediato y SIN parpadeo, siempre que la
// interfaz redibuje solo las zonas que cambian (interfaz.h lo
// hace: dibujo estático una vez + actualización de valores).
// Requiere PSRAM "OPI PSRAM" en el IDE.
// ============================================================
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "config_monitor800.h"

static Arduino_ESP32RGBPanel *_rgb = new Arduino_ESP32RGBPanel(
    PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
    PINS_R,
    PINS_G,
    PINS_B,
    0 /*hs pol*/, LCD_H_FP, LCD_H_PW, LCD_H_BP,
    0 /*vs pol*/, LCD_V_FP, LCD_V_PW, LCD_V_BP,
    1 /*pclk activo en flanco negativo*/, LCD_PCLK);

static Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    800, 480, _rgb, 0 /*rotación*/, true /*auto_flush*/);

// Paleta (RGB565)
#define COL_FONDO   0x0000
#define COL_TEXTO   0xFFFF
#define COL_TENUE   0x8410
#define COL_OK      0x07E0
#define COL_ALERTA  0xF800
#define COL_ACENTO  0x05FF
#define COL_BOTON   0x2124
#define COL_AMAR    0xFFE0
#define COL_PANEL   0x10A2   // gris muy oscuro para fondos de sección

bool lcdInit() {
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
  if (!gfx->begin()) return false;
  gfx->fillScreen(COL_FONDO);
  return true;
}
