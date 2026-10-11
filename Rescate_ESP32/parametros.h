#pragma once
// ============================================================
// parametros.h — Parámetros ajustables en vivo + memoria flash
//
// La variable global P reemplaza a las constantes de velocidad,
// PID, evasión y obstáculo. Los #define de config.h ahora son
// solo los VALORES POR DEFECTO.
//
// Persistencia: paramsGuardar() graba P en la NVS (flash). Al
// encender, paramsCargar() restaura lo guardado. Así, cuando
// desactives ESP-NOW para la competencia (USAR_ESPNOW false),
// LA CALIBRACIÓN HECHA CON EL MONITOR PERSISTE EN EL ROBOT.
// API: paramsCargar(), paramsGuardar(), paramsPorDefecto()
// ============================================================
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "comun_espnow.h"

Parametros P;
static Preferences _prefs;

void paramsPorDefecto() {
  P.velBase      = VEL_BASE;
  P.velMax       = VEL_MAX;
  P.velGiro      = VEL_GIRO;
  P.velSubida    = VEL_SUBIDA;
  P.velBajada    = VEL_BAJADA;
  P.velEvasion   = VEL_EVASION;
  P.velRecuperar = VEL_RECUPERAR;
  P.velCruce     = VEL_CRUCE;
  P.kp           = KP;
  P.kd           = KD;
  P.distObstaculo = DIST_OBSTACULO;
  P.tEvLateral   = T_EV_LATERAL;
  P.tEvParalelo  = T_EV_PARALELO;
  P.tEvBuscar    = T_EV_BUSCAR;
  P.trimIzq      = 0;
  P.trimDer      = 0;
}

// Carga desde flash; si no hay nada guardado, usa los defaults
void paramsCargar() {
  paramsPorDefecto();
  if (_prefs.begin("rescate", true)) {           // solo lectura
    if (_prefs.isKey("params") &&
        _prefs.getBytesLength("params") == sizeof(Parametros)) {
      _prefs.getBytes("params", &P, sizeof(P));
    }
    _prefs.end();
  }
}

bool paramsGuardar() {
  if (!_prefs.begin("rescate", false)) return false;
  size_t n = _prefs.putBytes("params", &P, sizeof(P));
  _prefs.end();
  return n == sizeof(Parametros);
}
