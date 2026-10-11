#pragma once
// ============================================================
// color.h — 2× TCS3200 / GY-31 (cuadros verdes 25×25 mm)
// Mide frecuencia relativa R/G/B en cada lado.
// Verde si G/(R+B) > COLOR_UMBRAL_VERDE
// ============================================================
#include <Arduino.h>
#include "config.h"

static unsigned long _tVerdeIzq = 0, _tVerdeDer = 0;

void colorInit() {
  pinMode(PIN_S0, OUTPUT);
  pinMode(PIN_S1, OUTPUT);
  pinMode(PIN_S2, OUTPUT);
  pinMode(PIN_S3, OUTPUT);
  pinMode(PIN_OUT_IZQ, INPUT);
  pinMode(PIN_OUT_DER, INPUT);
  // Escala 20% (S0=HIGH, S1=LOW) — típico GY-31
  digitalWrite(PIN_S0, HIGH);
  digitalWrite(PIN_S1, LOW);
}

static unsigned long _pulso(int pinOut, bool s2, bool s3) {
  digitalWrite(PIN_S2, s2 ? HIGH : LOW);
  digitalWrite(PIN_S3, s3 ? HIGH : LOW);
  delayMicroseconds(80);
  unsigned long t = pulseIn(pinOut, LOW, COLOR_MAX_PULSO_US);
  if (t == 0) t = COLOR_MAX_PULSO_US;
  return t;
}

// Devuelve "fuerza" de verde (mayor = más verde). Usa 1/pulso ≈ frecuencia.
static float _verdeRatio(int pinOut) {
  // Filtros TCS3200: R=LOW,LOW  G=HIGH,HIGH  B=LOW,HIGH
  unsigned long r = _pulso(pinOut, false, false);
  unsigned long b = _pulso(pinOut, false, true);
  unsigned long g = _pulso(pinOut, true,  true);
  float fr = 1.0f / (float)r;
  float fb = 1.0f / (float)b;
  float fg = 1.0f / (float)g;
  float den = fr + fb;
  if (den < 1e-9f) return 0;
  return fg / den;
}

void colorActualizar() {
  float vi = _verdeRatio(PIN_OUT_IZQ);
  float vd = _verdeRatio(PIN_OUT_DER);
  if (vi >= COLOR_UMBRAL_VERDE) _tVerdeIzq = millis();
  if (vd >= COLOR_UMBRAL_VERDE) _tVerdeDer = millis();
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

// Diagnóstico Serial
void colorDebug() {
  float vi = _verdeRatio(PIN_OUT_IZQ);
  float vd = _verdeRatio(PIN_OUT_DER);
  Serial.printf("verde I=%.2f D=%.2f  mem I=%d D=%d\n",
                vi, vd, colorVerdeIzqReciente(), colorVerdeDerReciente());
}
