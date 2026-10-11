#pragma once
#include <Arduino.h>
#include "config.h"

#if OBST_USAR_VL53
#include <Wire.h>
#include <VL53L0X.h>
static VL53L0X _tof;
static bool _tofOk = false;
static uint16_t _dist = 8190;
#else
static uint16_t _dist = 8190;
#endif

static unsigned long _tObst = 0;
static bool _prev = false;

bool obstInit() {
#if OBST_USAR_VL53
  Wire.begin(PIN_VL53_SDA, PIN_VL53_SCL);
  Wire.setClock(400000);
  delay(20);
  _tof.setTimeout(60);
  _tofOk = _tof.init();
  if (_tofOk) {
    _tof.setMeasurementTimingBudget(30000);
    _tof.startContinuous(30);
    Serial.println(F("VL53 frontal OK"));
  } else {
    Serial.println(F("VL53 frontal FALLA"));
  }
  return _tofOk;
#else
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  Serial.println(F("HC-SR04 listo"));
  return true;
#endif
}

void obstActualizar() {
#if OBST_USAR_VL53
  if (!_tofOk) return;
  uint16_t v = _tof.readRangeContinuousMillimeters();
  if (_tof.timeoutOccurred() || v > 2000) _dist = 8190;
  else _dist = v;
#else
  digitalWrite(PIN_US_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  unsigned long us = pulseIn(PIN_US_ECHO, HIGH, 20000);
  if (us == 0) _dist = 8190;
  else _dist = (uint16_t)(us / 58);   // ~mm
#endif
}

uint16_t obstDistMm() { return _dist; }

bool obstDetectado() {
  bool act = (_dist < DIST_OBST_MM);
  if (act && !_prev) _tObst = millis();
  _prev = act;
  return act && (millis() - _tObst >= T_DEBOUNCE_OBST);
}
