#pragma once
// Evasión del cubo de 10 cm por encoders (sin gyro).
#include <Arduino.h>
#include "config.h"
#include "motores.h"
#include "encoders.h"
#include "linea.h"

bool ejecutarEvasion() {
  const int sentido = EVADIR_POR_IZQUIERDA ? +1 : -1;

  frenar();
  delay(150);

  girarPulsos(PULSOS_90, sentido, VEL_EVASION);
  avanzarPulsos(PULSOS_EV_LATERAL, VEL_EVASION);
  girarPulsos(PULSOS_90, -sentido, VEL_EVASION);
  avanzarPulsos(PULSOS_EV_PARALELO, VEL_EVASION);
  girarPulsos(PULSOS_90, -sentido, VEL_EVASION);

  // Buscar línea
  encReset();
  bool ok = false;
  unsigned long t0 = millis();
  while (millis() - t0 < 3000) {
    lineaLeer();
    if (!lineaBlanco()) { ok = true; break; }
    mover(VEL_EVASION, VEL_EVASION);
    delay(2);
  }
  if (ok) {
    avanzarPulsos(6, VEL_EVASION);
    girarPulsos(PULSOS_90, sentido, VEL_EVASION);
  } else {
    frenar();
  }
  return ok;
}
