#pragma once
// ============================================================
// tactil.h — Touch capacitivo GT911 (I2C, dirección 0x5D)
// Driver mínimo por registros de 16 bits, sin librerías:
//   0x814E = estado (bit7 = dato listo, bits 0-3 = n° de dedos)
//   0x8150..0x8153 = X e Y del primer punto (little endian)
// Tras leer, hay que escribir 0 en 0x814E para liberar el buffer.
// API: tactilInit(), tactilPunto(x, y)
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include "config_monitor800.h"

static uint8_t _dirGT = DIR_GT911;   // se autodetecta en tactilInit

static void _gtDir(uint16_t reg) {
  Wire.beginTransmission(_dirGT);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
}

static bool _gtLeer(uint16_t reg, uint8_t *buf, uint8_t n) {
  _gtDir(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)_dirGT, (int)n) < n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

static void _gtEscribir(uint16_t reg, uint8_t val) {
  _gtDir(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void tactilInit() {
  // Secuencia de reset con INT en LOW -> el GT911 arranca en 0x5D
  pinMode(PIN_TP_RST, OUTPUT);
  pinMode(PIN_TP_INT, OUTPUT);
  digitalWrite(PIN_TP_INT, LOW);
  digitalWrite(PIN_TP_RST, LOW);
  delay(10);
  digitalWrite(PIN_TP_RST, HIGH);
  delay(60);
  pinMode(PIN_TP_INT, INPUT);

  Wire.begin(PIN_TP_SDA, PIN_TP_SCL);
  Wire.setClock(400000);

  // --- Diagnóstico: escaneo del bus del touch ---
  // (con USB CDC Enabled los GPIO 19/20 los captura el USB nativo
  //  y este escaneo no encuentra NADA: poner CDC en Disabled)
  Serial.println(F("Escaneando bus I2C del touch..."));
  int encontrados = 0;
  for (uint8_t d = 8; d < 120; d++) {
    Wire.beginTransmission(d);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  dispositivo en 0x%02X\n", d);
      encontrados++;
    }
  }
  if (!encontrados) Serial.println(F("  NINGUNO (revisar USB CDC / flex del touch)"));

  // --- Autodetección de la dirección del GT911 (0x5D o 0x14) ---
  Wire.beginTransmission(0x5D);
  if (Wire.endTransmission() == 0) _dirGT = 0x5D;
  else {
    Wire.beginTransmission(0x14);
    if (Wire.endTransmission() == 0) _dirGT = 0x14;
  }
  Serial.printf("GT911 usando direccion 0x%02X\n", _dirGT);
}

// true mientras hay un dedo, con coordenadas 0..799 / 0..479
bool tactilPunto(int16_t &x, int16_t &y) {
  uint8_t st;
  if (!_gtLeer(0x814E, &st, 1)) return false;
  if (!(st & 0x80)) return false;          // sin dato nuevo
  uint8_t n = st & 0x0F;
  if (n == 0) { _gtEscribir(0x814E, 0); return false; }

  uint8_t d[4];
  bool ok = _gtLeer(0x8150, d, 4);         // XL XH YL YH del punto 1
  _gtEscribir(0x814E, 0);                  // liberar buffer
  if (!ok) return false;

  x = d[0] | (d[1] << 8);
  y = d[2] | (d[3] << 8);
  if (TP_INVERTIR_X) x = 799 - x;
  if (TP_INVERTIR_Y) y = 479 - y;
  return true;
}
