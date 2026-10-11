#pragma once
// ============================================================
// evasion.h — Maniobra de evasión del cubo de 10 cm
// Secuencia (evadiendo por la izquierda; espejo si es derecha):
//   1. Frenar
//   2. Girar 90 grados hacia el lado de evasión (gyro)
//   3. Avanzar recto P.tEvLateral (separarse de la línea)
//   4. Girar 90 grados de vuelta (queda paralelo a la línea)
//   5. Avanzar recto P.tEvParalelo (pasar el cubo)
//   6. Girar 90 grados hacia la línea
//   7. Avanzar hasta pisar la línea (timeout P.tEvBuscar)
//   8. Avanzar T_EV_SOBREPASO y girar 90 grados para alinearse
// Es BLOQUEANTE a propósito: es una maniobra discreta, como las
// de los robots de rescate, no un lazo continuo.
// Devuelve true si reencontró la línea.
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "parametros.h"
#include "motores.h"
#include "sensores_linea.h"
#include "giroscopio.h"

bool ejecutarEvasion() {
  const float lado = EVADIR_POR_IZQUIERDA ? +90.0f : -90.0f;

  frenar();
  delay(250);

  // 2) alejarse de la línea
  girarGrados(lado);
  gyroGuardarRumbo();
  unsigned long t0 = millis();
  while (millis() - t0 < P.tEvLateral) {
    gyroActualizar();
    avanzarRectoGyro(P.velEvasion);
    delay(2);
  }
  frenar(); delay(150);

  // 4) paralelo a la línea, pasando el cubo
  girarGrados(-lado);
  gyroGuardarRumbo();
  t0 = millis();
  while (millis() - t0 < P.tEvParalelo) {
    gyroActualizar();
    avanzarRectoGyro(P.velEvasion);
    delay(2);
  }
  frenar(); delay(150);

  // 6) volver hacia la línea
  girarGrados(-lado);
  gyroGuardarRumbo();
  t0 = millis();
  bool encontrada = false;
  while (millis() - t0 < P.tEvBuscar) {
    gyroActualizar();
    lineaLeer();
    if (!lineaTodoBlanco()) { encontrada = true; break; }
    avanzarRectoGyro(P.velEvasion);
    delay(2);
  }

  if (encontrada) {
    // 8) cruzar la línea un poco y alinearse con ella
    t0 = millis();
    while (millis() - t0 < T_EV_SOBREPASO) {
      gyroActualizar();
      avanzarRectoGyro(P.velEvasion);
      delay(2);
    }
    frenar(); delay(150);
    girarGrados(lado);          // alinear con el sentido de avance
  } else {
    frenar();
  }
  return encontrada;
}
