#pragma once
// ============================================================
// tactil.h — Touch capacitivo CST816D (I2C, dirección 0x15)
// Driver mínimo por registros, sin librerías externas:
//   reg 0x02 = número de dedos
//   reg 0x03..0x04 = X (12 bits), reg 0x05..0x06 = Y (12 bits)
// API: tactilInit(), tactilPunto(x, y) -> true si hay un dedo
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include "config_monitor.h"

void tactilInit() {
  Wire.begin(PIN_TP_SDA, PIN_TP_SCL);
  Wire.setClock(400000);
  pinMode(PIN_TP_INT, INPUT);
}

// Devuelve true mientras hay un dedo en pantalla, con sus coordenadas
bool tactilPunto(int16_t &x, int16_t &y) {
  Wire.beginTransmission(DIR_CST816);
  Wire.write(0x02);                       // desde "número de dedos"
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)DIR_CST816, 5) < 5) return false;

  uint8_t dedos = Wire.read() & 0x0F;
  uint8_t xh = Wire.read(), xl = Wire.read();
  uint8_t yh = Wire.read(), yl = Wire.read();
  if (dedos == 0) return false;

  x = ((xh & 0x0F) << 8) | xl;            // 0..239
  y = ((yh & 0x0F) << 8) | yl;            // 0..319
  if (TP_INVERTIR_X) x = 239 - x;
  if (TP_INVERTIR_Y) y = 319 - y;
  return true;
}
