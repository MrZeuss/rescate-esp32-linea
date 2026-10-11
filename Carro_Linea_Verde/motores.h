#pragma once
// ============================================================
// motores.h — Driver integrado ESP32-DO
// Pines oficiales: M1 = 2,4  |  M2 = 13,27
//
// Convención del fabricante (ejemplo oficial):
//   pinA = vel, pinB = 0  →  un sentido ("atrás" en el ejemplo)
//   pinA = 0, pinB = vel  →  sentido opuesto ("adelante")
// Aquí: velocidad POSITIVA = adelante (pinB activo).
// ============================================================
#include <Arduino.h>
#include "config.h"

static int _pwmI = 0, _pwmD = 0;

// vel > 0 = adelante, vel < 0 = atrás, 0 = libre
static void _setMotor(int pinA, int pinB, int vel) {
  vel = constrain(vel, -255, 255);
  if (vel > 0) {
    analogWrite(pinA, 0);
    analogWrite(pinB, vel);       // adelante (ejemplo oficial)
  } else if (vel < 0) {
    analogWrite(pinA, -vel);      // atrás
    analogWrite(pinB, 0);
  } else {
    analogWrite(pinA, 0);
    analogWrite(pinB, 0);
  }
}

void motoresInit() {
  pinMode(PIN_MI_A, OUTPUT);
  pinMode(PIN_MI_B, OUTPUT);
  pinMode(PIN_MD_A, OUTPUT);
  pinMode(PIN_MD_B, OUTPUT);
  analogWrite(PIN_MI_A, 0); analogWrite(PIN_MI_B, 0);
  analogWrite(PIN_MD_A, 0); analogWrite(PIN_MD_B, 0);
}

// M1 = izquierdo, M2 = derecho (ajuste con INVERTIR_* si hace falta)
void mover(int izq, int der) {
  if (INVERTIR_IZQ) izq = -izq;
  if (INVERTIR_DER) der = -der;
  _pwmI = izq;
  _pwmD = der;
  _setMotor(PIN_MI_A, PIN_MI_B, izq);
  _setMotor(PIN_MD_A, PIN_MD_B, der);
}

void frenar() {
  _pwmI = _pwmD = 0;
  analogWrite(PIN_MI_A, 255); analogWrite(PIN_MI_B, 255);
  analogWrite(PIN_MD_A, 255); analogWrite(PIN_MD_B, 255);
}

void detener() {
  _pwmI = _pwmD = 0;
  analogWrite(PIN_MI_A, 0); analogWrite(PIN_MI_B, 0);
  analogWrite(PIN_MD_A, 0); analogWrite(PIN_MD_B, 0);
}

int motorPwmIzq() { return _pwmI; }
int motorPwmDer() { return _pwmD; }
