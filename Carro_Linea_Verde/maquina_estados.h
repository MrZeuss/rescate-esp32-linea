#pragma once
// ============================================================
// maquina_estados.h — Lógica Rescue Line (versión verde)
//
// Intersecciones (cuadros verdes 25×25 mm):
//   solo IZQ  → giro 90° izquierda
//   solo DER  → giro 90° derecha
//   ambos     → punto muerto, giro 180°
//   ninguno   → recto por el cruce
//
// L2 (marcador negro lateral ~15 cm): memoriza lado;
// al próximo cruce sin verde (entrada a cuadrado F1) gira a ese lado
// y sigue la línea del perímetro hasta recuperar la salida.
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "motores.h"
#include "encoders.h"
#include "linea.h"
#include "color.h"
#include "obstaculo.h"
#include "control_pid.h"
#include "evasion.h"

enum Estado {
  ESPERA, SEGUIR, PUNTEADA, CRUCE, EVASION,
  GIRO_IZQ, GIRO_DER, GIRO_180, RECTO_CRUCE,
  CUADRADO, RECUPERAR, META
};

static Estado _est = ESPERA;
static unsigned long _tEst = 0;
static int _ladoL2 = 0;          // -1 izq, +1 der, 0 ninguno
static unsigned long _tL2 = 0;
static bool _enCuadrado = false;

static const char *NOMBRES[] = {
  "ESPERA", "SEGUIR", "PUNTEADA", "CRUCE", "EVASION",
  "G_IZQ", "G_DER", "G_180", "RECTO", "CUAD", "RECUP", "META"
};

static void _cambiar(Estado e) { _est = e; _tEst = millis(); }
static unsigned long _dt() { return millis() - _tEst; }

const char *estadoNombre() { return NOMBRES[_est]; }
Estado estadoActual() { return _est; }

void estadoIniciar() {
  pidReset();
  colorLimpiarMemoria();
  _ladoL2 = 0;
  _enCuadrado = false;
#if PIN_LED >= 0
  digitalWrite(PIN_LED, LOW);
#endif
  _cambiar(SEGUIR);
}

void estadoDetener() {
  frenar();
  _cambiar(ESPERA);
}

static void _decidirCruce() {
  bool vi = colorVerdeIzqReciente();
  bool vd = colorVerdeDerReciente();

  Serial.printf("CRUCE verde I=%d D=%d L2=%d\n", vi, vd, _ladoL2);

  if (vi && vd) {
    _cambiar(GIRO_180);                 // punto muerto
  } else if (vi && !vd) {
    _cambiar(GIRO_IZQ);
  } else if (vd && !vi) {
    _cambiar(GIRO_DER);
  } else if (_ladoL2 != 0 && (millis() - _tL2) < 5000) {
    // Entrada al cuadrado de caminos alternativos (sin verde)
    _enCuadrado = true;
    _cambiar(_ladoL2 < 0 ? GIRO_IZQ : GIRO_DER);
    _ladoL2 = 0;
  } else {
    _cambiar(RECTO_CRUCE);              // de frente
  }
  colorLimpiarMemoria();
}

void maquinaEstados() {
  switch (_est) {

    case ESPERA:
      detener();
      break;

    case SEGUIR: {
      if (obstDetectado()) { _cambiar(EVASION); break; }

      // Memoria de L2 (marcador lateral negro)
      int l2 = lineaMarcadorL2();
      if (l2 != 0) {
        _ladoL2 = l2;
        _tL2 = millis();
        Serial.printf("L2 lado=%d\n", l2);
      }

      if (lineaCruce()) { _cambiar(CRUCE); break; }
      if (lineaBlanco()) { _cambiar(PUNTEADA); break; }
      pidCalcular(VEL_BASE);
      break;
    }

    case PUNTEADA:
      if (obstDetectado()) { _cambiar(EVASION); break; }
      pidCalcular(VEL_BASE);
      if (!lineaBlanco()) _cambiar(SEGUIR);
      else if (_dt() > T_GAP_MS) _cambiar(RECUPERAR);
      break;

    case CRUCE:
      // Avanza un poco sobre el cruce para centrar, luego decide
      mover(VEL_CRUCE, VEL_CRUCE);
      if (_dt() > 120) {
        _decidirCruce();
      }
      break;

    case RECTO_CRUCE:
      mover(VEL_CRUCE, VEL_CRUCE);
      if (!lineaCruce() && _dt() > T_RECTO_CRUCE_MS) {
        // Negro muy largo = posible meta (baldosa 8 / zona final)
        if (_dt() > T_META_MS && lineaCruce()) _cambiar(META);
        else { pidReset(); _cambiar(SEGUIR); }
      } else if (_dt() > T_META_MS && lineaCruce()) {
        _cambiar(META);
      }
      break;

    case GIRO_IZQ:
      girarPulsos(PULSOS_90, +1);
      pidReset();
      _cambiar(_enCuadrado ? CUADRADO : SEGUIR);
      break;

    case GIRO_DER:
      girarPulsos(PULSOS_90, -1);
      pidReset();
      _cambiar(_enCuadrado ? CUADRADO : SEGUIR);
      break;

    case GIRO_180:
      girarPulsos(PULSOS_180, +1);
      pidReset();
      _cambiar(SEGUIR);
      break;

    case CUADRADO:
      // Sigue el perímetro del cuadrado hasta un cruce de salida
      if (lineaCruce() && _dt() > 400) {
        // Salida del cuadrado: recto hacia la línea principal
        _enCuadrado = false;
        _cambiar(RECTO_CRUCE);
        break;
      }
      if (lineaBlanco()) pidCalcular(VEL_BASE);  // hold
      else pidCalcular(VEL_BASE);
      break;

    case EVASION: {
      bool ok = ejecutarEvasion();
      pidReset();
      _cambiar(ok ? SEGUIR : RECUPERAR);
      break;
    }

    case RECUPERAR: {
      int lado = lineaLado();
      if (lado == 0) lado = +1;
      mover(lado * 90, -lado * 90);
      if (lineaHay() && abs(lineaPos()) < 900) {
        pidReset();
        _cambiar(SEGUIR);
      } else if (_dt() > 2500) {
        estadoDetener();
      }
      break;
    }

    case META:
      frenar();
#if PIN_LED >= 0
      digitalWrite(PIN_LED, HIGH);
#endif
      break;
  }
}
