#pragma once
// ============================================================
// distancia.h — 3x VL53L0X láser ToF (frontal, derecha, izquierda)
// Librería requerida: "VL53L0X" de Pololu (Library Manager)
// Comparten I2C (SDA=21, SCL=22) con el MPU-6500.
//
// Truco de las direcciones: los VL53L0X salen de fábrica todos
// con dirección 0x29 -> chocarían en el bus. Al arrancar se
// mantienen apagados (XSHUT en LOW) y se van despertando de a
// uno, asignándole a cada uno su dirección nueva antes de
// despertar al siguiente.
//
// Lectura NO bloqueante para el lazo: los 3 se leen en ráfaga
// cada 35 ms (modo continuo) y las funciones dist*() devuelven
// el valor cacheado. 8190 mm = "no veo pared / fuera de rango".
// API: distanciaInit(), distanciaActualizar(),
//      distFrontal(), distDerecha(), distIzquierda(), paredCerca()
// ============================================================
#include <Arduino.h>
#include <VL53L0X.h>
#include "config.h"

#define DIST_SIN_PARED 8190

static VL53L0X _tofF, _tofD, _tofI;
static uint16_t _dF = DIST_SIN_PARED, _dD = DIST_SIN_PARED, _dI = DIST_SIN_PARED;
static bool _okF = false, _okD = false, _okI = false;
static unsigned long _tUltimaLectura = 0;

static bool _despertarUno(VL53L0X &s, int pinXshut, uint8_t dirNueva) {
  digitalWrite(pinXshut, HIGH);
  delay(10);
  s.setTimeout(60);
  if (!s.init()) return false;
  s.setAddress(dirNueva);
  s.setMeasurementTimingBudget(33000);
  s.startContinuous(33);
  return true;
}

bool distanciaInit() {
  pinMode(PIN_XSHUT_FRONTAL, OUTPUT);
  pinMode(PIN_XSHUT_DER, OUTPUT);
  pinMode(PIN_XSHUT_IZQ, OUTPUT);
  digitalWrite(PIN_XSHUT_FRONTAL, LOW);
  digitalWrite(PIN_XSHUT_DER, LOW);
  digitalWrite(PIN_XSHUT_IZQ, LOW);
  delay(20);

  _okF = _despertarUno(_tofF, PIN_XSHUT_FRONTAL, DIR_VL53_FRONTAL);
  Serial.printf("VL53 FRONTAL   (XSHUT=%d): %s\n", PIN_XSHUT_FRONTAL, _okF ? "OK -> 0x30" : "FALLA");
  _okD = _despertarUno(_tofD, PIN_XSHUT_DER, DIR_VL53_DER);
  Serial.printf("VL53 DERECHO   (XSHUT=%d): %s\n", PIN_XSHUT_DER, _okD ? "OK -> 0x31" : "FALLA");
  _okI = _despertarUno(_tofI, PIN_XSHUT_IZQ, DIR_VL53_IZQ);
  Serial.printf("VL53 IZQUIERDO (XSHUT=%d): %s\n", PIN_XSHUT_IZQ, _okI ? "OK -> 0x32" : "FALLA");
  return _okF && _okD && _okI;
}

static uint16_t _filtrar(VL53L0X &s, uint16_t v) {
  if (s.timeoutOccurred() || v > 2000) return DIST_SIN_PARED;
  return v;
}

void distanciaActualizar() {
  if (!(_okF || _okD || _okI)) return;
  if (millis() - _tUltimaLectura < 35) return;
  _tUltimaLectura = millis();
  if (_okF) _dF = _filtrar(_tofF, _tofF.readRangeContinuousMillimeters());
  if (_okD) _dD = _filtrar(_tofD, _tofD.readRangeContinuousMillimeters());
  if (_okI) _dI = _filtrar(_tofI, _tofI.readRangeContinuousMillimeters());
}

uint16_t distFrontal()   { return _dF; }
uint16_t distDerecha()   { return _dD; }
uint16_t distIzquierda() { return _dI; }
bool distanciaOk()       { return _okF || _okD || _okI; }
bool distOkFrontal()     { return _okF; }
bool distOkDerecha()     { return _okD; }
bool distOkIzquierda()   { return _okI; }

bool paredCerca() {
  return _dF < DIST_TRANS_FRENTE ||
         _dD < DIST_TRANS_LADO   ||
         _dI < DIST_TRANS_LADO;
}
