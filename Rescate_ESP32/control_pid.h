#pragma once
// ============================================================
// control_pid.h — Lazo PD del seguidor de líneas
// Física: el error es la posición ponderada (-2000..+2000).
// KD amortigua; recuerda que al adelantar/atrasar la barra de
// sensores cambia el brazo d y hay que re-sintonizar KP y KD.
//
// SAMPLE & HOLD: si no hay línea (hueco de la punteada), la
// corrección NO se pone en cero: se CONGELA la última, para que
// el carro mantenga la curvatura que traía. Ver pidCalcular().
// API: pidReset(), pidCalcular(velBase) -> aplica a los motores
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "parametros.h"
#include "motores.h"
#include "sensores_linea.h"

static float _errorPrevio = 0;
static float _corrCongelada = 0;
static unsigned long _tPrevioPID = 0;

void pidReset() {
  _errorPrevio = 0;
  _corrCongelada = 0;
  _tPrevioPID = millis();
}

// Calcula y APLICA la corrección. Devuelve la corrección usada.
float pidCalcular(int velBase) {
  float corr;

  if (lineaHayLinea()) {
    float error = (float)lineaPosicion();          // objetivo: 0 (centro)
    unsigned long ahora = millis();
    float dt = (ahora - _tPrevioPID) / 1000.0f;
    if (dt < 0.001f) dt = 0.001f;
    _tPrevioPID = ahora;

    float derivada = (error - _errorPrevio) / dt;
    _errorPrevio = error;

    corr = P.kp * error + P.kd * derivada * 0.001f;    // KD escalado a ms
    _corrCongelada = corr;                          // memoria para huecos
  } else {
    corr = _corrCongelada;                          // SAMPLE & HOLD
  }

  // posicion > 0 = línea hacia la DERECHA -> girar a la derecha
  // -> rueda izquierda más rápida, derecha más lenta
  int velIzq = constrain((int)(velBase + corr), -P.velMax, P.velMax);
  int velDer = constrain((int)(velBase - corr), -P.velMax, P.velMax);
  mover(velIzq, velDer);
  return corr;
}
