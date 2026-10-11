#pragma once
#include <Arduino.h>
#include "config.h"
#include "motores.h"
#include "linea.h"

static float _errPrev = 0, _corrHold = 0;
static unsigned long _tPid = 0;

void pidReset() {
  _errPrev = 0; _corrHold = 0; _tPid = millis();
}

float pidCalcular(int velBase) {
  float corr;
  if (lineaHay()) {
    float err = (float)lineaPos();
    unsigned long ahora = millis();
    float dt = (ahora - _tPid) / 1000.0f;
    if (dt < 0.001f) dt = 0.001f;
    _tPid = ahora;
    float d = (err - _errPrev) / dt;
    _errPrev = err;
    corr = KP * err + KD * d * 0.001f;
    _corrHold = corr;
  } else {
    corr = _corrHold;   // punteada: sample & hold
  }
  int vi = constrain((int)(velBase + corr), -VEL_MAX, VEL_MAX);
  int vd = constrain((int)(velBase - corr), -VEL_MAX, VEL_MAX);
  mover(vi, vd);
  return corr;
}
