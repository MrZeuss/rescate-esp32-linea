#pragma once
// ============================================================
// modo_prueba.h — Diagnóstico remoto desde el monitor
// Mientras está activo: la FSM no corre; telemetría sigue;
// el monitor puede mandar PWM manual y reset de yaw.
// ============================================================
#include <Arduino.h>
#include "motores.h"
#include "giroscopio.h"

static volatile bool _modoPrueba = false;
static volatile bool _fTestEnter = false, _fTestExit = false;
static volatile bool _fTestGyroCero = false;
static volatile bool _fTestMotor = false;
static volatile int16_t _testPwmIzq = 0, _testPwmDer = 0;

bool modoPruebaActivo() { return _modoPrueba; }

void pruebaMarcarEnter() { _fTestEnter = true; }
void pruebaMarcarExit()  { _fTestExit = true; }
void pruebaMarcarGyroCero() { _fTestGyroCero = true; }
void pruebaMarcarMotor(int16_t izq, int16_t der) {
  _testPwmIzq = izq;
  _testPwmDer = der;
  _fTestMotor = true;
}

// Llamar una vez por loop (fuera de callbacks ESP-NOW)
void pruebaProcesar() {
  if (_fTestEnter) {
    _fTestEnter = false;
    frenar();
    _modoPrueba = true;
    Serial.println(F("MODO PRUEBA: activo"));
  }
  if (_fTestExit) {
    _fTestExit = false;
    frenar();
    _modoPrueba = false;
    Serial.println(F("MODO PRUEBA: salida"));
  }
  if (!_modoPrueba) return;

  if (_fTestGyroCero) {
    _fTestGyroCero = false;
    gyroResetYaw();
    Serial.println(F("PRUEBA: yaw = 0"));
  }
  if (_fTestMotor) {
    _fTestMotor = false;
    mover(_testPwmIzq, _testPwmDer);
  }
}
