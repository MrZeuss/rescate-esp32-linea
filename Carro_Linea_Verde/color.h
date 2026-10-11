#pragma once
// ============================================================
// color.h — 2× GY-AS7341 por I2C (cuadros verdes)
//
// Bus principal: SDA=21 SCL=22 (header IIC de la ESP-DO).
// Si ambos módulos responden en 0x39, conecte el derecho a un
// segundo bus (SoftWire) en PIN_I2C2_SDA / PIN_I2C2_SCL, o use mux.
//
// API estable: colorInit(), colorActualizar(),
//   colorVerdeIzqReciente(), colorVerdeDerReciente(), colorLimpiarMemoria()
//
// Nota: lectura mínima por registro I2C (canal verde). Para máxima
// precisión instale la librería del AS7341 y refine el umbral.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

#ifndef COLOR_DIR_I2C
#define COLOR_DIR_I2C 0x39
#endif

static unsigned long _tVerdeIzq = 0, _tVerdeDer = 0;
static bool _okIzq = false, _okDer = false;

static bool _i2cPing(TwoWire &w, uint8_t addr) {
  w.beginTransmission(addr);
  return w.endTransmission() == 0;
}

// Lectura muy simplificada: intenta leer 2 bytes; si el dispositivo
// responde y hay actividad, se considera candidato a "verde" vía umbral
// calibrado en pista (mejorar con librería AS7341).
static bool _leerVerde(TwoWire &w, uint8_t addr, float &ratio) {
  w.beginTransmission(addr);
  w.write(0x00);
  if (w.endTransmission(false) != 0) return false;
  if (w.requestFrom((int)addr, 2) < 2) return false;
  uint16_t v = (w.read() << 8) | w.read();
  // Placeholder: normaliza a un ratio artificial hasta calibrar
  ratio = (v > 0) ? (float)v / 1000.0f : 0.0f;
  return true;
}

void colorInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);
  delay(50);

  _okIzq = _i2cPing(Wire, COLOR_DIR_I2C);
  Serial.printf("AS7341 bus1 (izq?): %s @0x%02X\n",
                _okIzq ? "OK" : "NO", COLOR_DIR_I2C);

  // Segundo intento en el mismo bus (solo uno responderá si comparten dir.)
  _okDer = false;
  Serial.println(F("AS7341 der: mismo bus I2C — si no hay mux, use SoftWire"));
}

void colorActualizar() {
  float r = 0;
  if (_okIzq && _leerVerde(Wire, COLOR_DIR_I2C, r)) {
    if (r >= COLOR_UMBRAL_VERDE) _tVerdeIzq = millis();
    // Sin segundo sensor separado: espejo temporal desactivado
  }
  (void)_okDer;
}

bool colorVerdeIzqReciente() {
  return (millis() - _tVerdeIzq) < T_MEMORIA_VERDE_MS;
}
bool colorVerdeDerReciente() {
  return (millis() - _tVerdeDer) < T_MEMORIA_VERDE_MS;
}

void colorLimpiarMemoria() {
  _tVerdeIzq = 0;
  _tVerdeDer = 0;
}

void colorDebug() {
  float r = 0;
  bool ok = _okIzq && _leerVerde(Wire, COLOR_DIR_I2C, r);
  Serial.printf("AS7341 ok=%d ratio=%.2f  mem I=%d D=%d\n",
                ok, r, colorVerdeIzqReciente(), colorVerdeDerReciente());
}
