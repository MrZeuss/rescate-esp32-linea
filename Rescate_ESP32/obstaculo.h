#pragma once
// ============================================================
// obstaculo.h — Detección del cubo de 10 cm (VL53L0X frontal)
// Obstáculo = distFrontal() < P.distObstaculo (ajustable en vivo).
// Antirrebote: activo T_DEBOUNCE_OBST ms seguidos para confirmar.
// API: obstDetectado(), obstActivoCrudo()
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "parametros.h"
#include "distancia.h"

static unsigned long _tPrimeraDeteccion = 0;
static bool _obstActivoPrevio = false;

bool obstDetectado() {
  bool activo = (distFrontal() < P.distObstaculo);
  if (activo && !_obstActivoPrevio) _tPrimeraDeteccion = millis();
  _obstActivoPrevio = activo;
  return activo && (millis() - _tPrimeraDeteccion >= T_DEBOUNCE_OBST);
}

// Sin antirrebote (solo telemetría / monitor)
bool obstActivoCrudo() {
  return distFrontal() < P.distObstaculo;
}
