#pragma once
// ============================================================
// maquina_estados.h — Cerebro del robot
// Estados:
//   ESPERA          esperando BTN_START (BTN_CALIB calibra)
//   SEGUIR          PID normal sobre la línea
//   CRUCE           todo negro: recto sin PID; si persiste -> META
//   PUNTEADA        todo blanco: sample & hold; timeout ->
//                   LABERINTO si hay paredes, si no RECUPERAR
//   RECUPERAR       pivotar hacia el último lado conocido
//   PUENTE_SUBIDA   pitch alto: PWM de torque
//   PUENTE_BAJADA   pitch negativo: lento + freno al llegar
//   EVASION         maniobra bloqueante del cubo
//   LABERINTO       mano derecha (laberinto.h); sale al pisar línea
//   META            frenar, LED encendido, fin
//
// Transición al laberinto = DOBLE EVIDENCIA:
//   línea blanca sostenida  +  paredes detectadas por los VL53L0X
// (un hueco de la punteada dura menos y no tiene paredes cerca)
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "parametros.h"
#include "motores.h"
#include "sensores_linea.h"
#include "giroscopio.h"
#include "distancia.h"
#include "obstaculo.h"
#include "control_pid.h"
#include "evasion.h"
#include "laberinto.h"

enum Estado {
  ESPERA, SEGUIR, CRUCE, PUNTEADA, RECUPERAR,
  PUENTE_SUBIDA, PUENTE_BAJADA, EVASION, LABERINTO, META
};

static Estado _estado = ESPERA;
static unsigned long _tEntradaEstado = 0;

static const char *NOMBRES[] = {
  "ESPERA", "SEGUIR", "CRUCE", "PUNTEADA", "RECUP",
  "SUBIDA", "BAJADA", "EVASION", "LABER", "META"
};

static void _cambiarEstado(Estado nuevo) {
  _estado = nuevo;
  _tEntradaEstado = millis();
}

static unsigned long _tEnEstado() { return millis() - _tEntradaEstado; }

const char *estadoNombre() { return NOMBRES[_estado]; }
Estado estadoActual() { return _estado; }

void estadoIniciarCarrera() {
  pidReset();
  gyroResetYaw();
  digitalWrite(PIN_LED_META, LOW);
  _cambiarEstado(SEGUIR);
}

void estadoDetenerTodo() {
  frenar();
  _cambiarEstado(ESPERA);
}

// ------------------------------------------------------------
// Se llama una vez por loop, DESPUÉS de lineaLeer(),
// gyroActualizar() y distanciaActualizar()
// ------------------------------------------------------------
void maquinaEstados() {
  float pitch = gyroPitch();

  switch (_estado) {

    case ESPERA:
      detener();
      break;

    // --------------------------------------------------------
    case SEGUIR: {
      // Prioridad 1: obstáculo sobre la línea
      if (obstDetectado()) { _cambiarEstado(EVASION); break; }
      // Prioridad 2: puente
      if (pitch > PITCH_SUBIDA)  { _cambiarEstado(PUENTE_SUBIDA); break; }
      if (pitch < PITCH_BAJADA)  { _cambiarEstado(PUENTE_BAJADA); break; }
      // Prioridad 3: patrones de línea
      if (lineaTodoNegro())  { _cambiarEstado(CRUCE);    break; }
      if (lineaTodoBlanco()) { _cambiarEstado(PUNTEADA); break; }
      // Normal: PID
      pidCalcular(P.velBase);
      break;
    }

    // --------------------------------------------------------
    case CRUCE: {
      // Recto, corrección congelada en cero (la barra transversal
      // generaría un error espurio gigante si el PID siguiera activo)
      mover(P.velCruce, P.velCruce);
      if (!lineaTodoNegro() && _tEnEstado() > 80) {
        pidReset();
        _cambiarEstado(SEGUIR);
      } else if (_tEnEstado() > T_META) {
        // negro sostenido mucho más largo que un cruce = META
        _cambiarEstado(META);
      }
      break;
    }

    // --------------------------------------------------------
    case PUNTEADA: {
      if (obstDetectado()) { _cambiarEstado(EVASION); break; }
      // sample & hold: pidCalcular congela la última corrección
      // -> mantiene la curvatura que traía en la curva punteada
      pidCalcular(P.velBase);
      if (!lineaTodoBlanco()) {          // apareció un punto
        _cambiarEstado(SEGUIR);
      } else if (_tEnEstado() > T_GAP_TIMEOUT) {
        // ¿Fin de línea con paredes alrededor? -> entrada al laberinto
        if (paredCerca()) {
          labIniciar();
          _cambiarEstado(LABERINTO);
        } else {
          _cambiarEstado(RECUPERAR);
        }
      }
      break;
    }

    // --------------------------------------------------------
    case RECUPERAR: {
      int lado = lineaUltimoLado();
      if (lado == 0) lado = +1;
      mover(lado * P.velRecuperar, -lado * P.velRecuperar);
      if (lineaHayLinea() && abs(lineaPosicion()) < 800) {
        pidReset();
        _cambiarEstado(SEGUIR);
      } else if (_tEnEstado() > T_RECUPERAR_MAX) {
        estadoDetenerTodo();
      }
      break;
    }

    // --------------------------------------------------------
    case PUENTE_SUBIDA: {
      // PID activo con PWM base alto: que el control no le quite
      // torque al motor a mitad de subida
      pidCalcular(P.velSubida);
      if (pitch < PITCH_PLANO) {
        // ¿Arriba hay plataforma de laberinto (final) o sigue la línea?
        if (lineaTodoBlanco() && paredCerca()) {
          labIniciar();
          _cambiarEstado(LABERINTO);
        } else {
          pidReset();
          _cambiarEstado(SEGUIR);
        }
      }
      break;
    }

    // --------------------------------------------------------
    case PUENTE_BAJADA: {
      // La gravedad empuja (a = g sin(alfa)): bajar MUY lento
      pidCalcular(P.velBajada);
      if (pitch > -PITCH_PLANO) {
        frenar();                        // matar la inercia al llegar
        delay(150);
        pidReset();
        _cambiarEstado(SEGUIR);
      }
      break;
    }

    // --------------------------------------------------------
    case EVASION: {
      bool ok = ejecutarEvasion();       // bloqueante
      pidReset();
      _cambiarEstado(ok ? SEGUIR : RECUPERAR);
      break;
    }

    // --------------------------------------------------------
    case LABERINTO: {
      // laberinto.h maneja paredes y giros; devuelve true al
      // pisar la línea de salida con la barra de sensores
      if (labActualizar()) {
        pidReset();
        _cambiarEstado(SEGUIR);
      }
      break;
    }

    // --------------------------------------------------------
    case META:
      frenar();
      digitalWrite(PIN_LED_META, HIGH);
      break;
  }
}
