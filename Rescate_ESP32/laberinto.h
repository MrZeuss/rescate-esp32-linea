#pragma once
// ============================================================
// laberinto.h — Resolución del laberinto: regla de la mano derecha
//
// Teorema práctico: en un laberinto simplemente conexo (todas
// las paredes conectadas al borde, como el de la final), seguir
// siempre la pared derecha garantiza encontrar la salida.
//
// El lazo de control es EL MISMO que el seguidor de líneas, con
// otro sensor: error = distDerecha - DIST_PARED_OBJ, y un P
// sobre el PWM diferencial. La pared "es" la línea.
//
// Tres comportamientos:
//   1. Pared al frente (< DIST_FRENTE_GIRO)  -> pivote 90 izq
//   2. Derecha abierta (> DIST_DER_ABIERTA)  -> avanzar, 90 der, avanzar
//   3. Si no, seguir la pared derecha con control P
// Salida: cuando la barra de sensores vuelve a pisar línea negra.
//
// API: labIniciar(), labActualizar() -> true al encontrar la salida
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "motores.h"
#include "sensores_linea.h"
#include "giroscopio.h"
#include "distancia.h"

void labIniciar() {
  gyroGuardarRumbo();
}

// Avance recto bloqueante que aborta si aparece la línea de salida
// o si surge una pared al frente. Devuelve true si pisó la línea.
static bool _labAvanzar(unsigned long durMs) {
  unsigned long t0 = millis();
  while (millis() - t0 < durMs) {
    gyroActualizar();
    distanciaActualizar();
    lineaLeer();
    if (lineaNumNegros() >= 2) { frenar(); return true; }
    if (distFrontal() < DIST_FRENTE_GIRO) break;    // pared: replantear
    avanzarRectoGyro(VEL_LABERINTO);
    delay(2);
  }
  return false;
}

// Llamar una vez por loop estando en el estado LABERINTO.
// Devuelve true cuando el carro pisa la línea de salida.
bool labActualizar() {
  // ¿Salida? (dos sensores en negro evita falsos por sombras)
  if (lineaNumNegros() >= 2) { frenar(); return true; }

  uint16_t f = distFrontal();
  uint16_t d = distDerecha();

  // --- 1) Pared al frente: esquina -> girar a la izquierda ---
  if (f < DIST_FRENTE_GIRO) {
    frenar(); delay(120);
    girarGrados(+90);
    gyroGuardarRumbo();
    return false;
  }

  // --- 2) Se abrió la pared derecha: doblar hacia allá ---
  if (d > DIST_DER_ABIERTA) {
    // avanzar hasta despejar la esquina de la pared
    if (_labAvanzar(T_LAB_AVANCE_APERTURA)) return true;
    frenar(); delay(120);
    girarGrados(-90);
    gyroGuardarRumbo();
    // entrar al pasillo nuevo
    if (_labAvanzar(T_LAB_AVANCE_POST_GIRO)) return true;
    return false;
  }

  // --- 3) Seguir la pared derecha (control P) ---
  // err > 0: lejos de la pared -> corregir hacia la derecha
  // -> rueda izquierda más rápida (misma convención que la línea)
  float err = (float)d - (float)DIST_PARED_OBJ;
  int corr = (int)constrain(KP_PARED * err, -(float)CORR_PARED_MAX, (float)CORR_PARED_MAX);
  mover(VEL_LABERINTO + corr, VEL_LABERINTO - corr);
  return false;
}
