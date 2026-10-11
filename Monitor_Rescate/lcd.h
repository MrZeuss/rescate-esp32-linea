#pragma once
// ============================================================
// lcd.h — Pantalla ST7789 240x320 vía Arduino_GFX + FRAMEBUFFER
// Librería requerida: "GFX Library for Arduino" (moononournation)
//
// ANTI-PARPADEO: todo se dibuja en un lienzo en RAM
// (Arduino_Canvas, 240x320x2 = 150 KB) y se vuelca a la
// pantalla de UNA sola vez con gfx->flush(). El ojo nunca ve
// el borrado intermedio. Requiere PSRAM habilitada en el IDE
// (PSRAM: "OPI PSRAM"), que ya es parte de la configuración.
// API: lcdInit(); dibujar con gfx->...; al final gfx->flush()
// ============================================================
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "config_monitor.h"

static Arduino_DataBus *_bus = new Arduino_ESP32SPI(
    PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_MOSI, GFX_NOT_DEFINED);

static Arduino_GFX *_panel = new Arduino_ST7789(
    _bus, PIN_LCD_RST, 0 /*rotación*/, true /*IPS*/, 240, 320);

// El lienzo en RAM: aquí dibuja toda la interfaz
static Arduino_Canvas *gfx = new Arduino_Canvas(240, 320, _panel);

// Paleta del tablero (RGB565 definidos aquí mismo)
#define COL_FONDO   0x0000   // negro
#define COL_TEXTO   0xFFFF   // blanco
#define COL_TENUE   0x8410   // gris
#define COL_OK      0x07E0   // verde
#define COL_ALERTA  0xF800   // rojo
#define COL_ACENTO  0x05FF   // celeste
#define COL_BOTON   0x2124   // gris oscuro
#define COL_AMAR    0xFFE0   // amarillo

bool lcdInit() {
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);      // backlight ON
  if (!gfx->begin()) return false;     // inicia panel + framebuffer
  gfx->fillScreen(COL_FONDO);
  gfx->flush();
  return true;
}
