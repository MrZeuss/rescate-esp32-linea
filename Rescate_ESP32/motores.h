#pragma once
// ============================================================
// motores.h — Control del puente H integrado del ESP32-DO
// API: motoresInit(), mover(velIzq, velDer), frenar(), detener()
//      motorPwmIzq(), motorPwmDer()  (para la telemetría)
// Velocidades: -255..255 (negativo = reversa)
// TRIM: P.trimIzq / P.trimDer (-30..+30 %) compensan diferencias
// entre motores para que el carro vaya RECTO con PWM iguales.
// Se ajustan en vivo desde el monitor ESP-NOW.
// Requiere core ESP32 >= 2.0.5 (analogWrite disponible)
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "parametros.h"

static int _pwmIzq = 0, _pwmDer = 0;

static inline void _setMotorPins(int pinA, int pinB, int vel) {
  vel = constrain(vel, -255, 255);
  if (vel >= 0) {
    analogWrite(pinA, vel);
    analogWrite(pinB, 0);
  } else {
    analogWrite(pinA, 0);
    analogWrite(pinB, -vel);
  }
}

void motoresInit() {
  pinMode(PIN_M1_A, OUTPUT);
  pinMode(PIN_M1_B, OUTPUT);
  pinMode(PIN_M2_A, OUTPUT);
  pinMode(PIN_M2_B, OUTPUT);
  analogWrite(PIN_M1_A, 0); analogWrite(PIN_M1_B, 0);
  analogWrite(PIN_M2_A, 0); analogWrite(PIN_M2_B, 0);
}

// velIzq / velDer en -255..255
void mover(int velIzq, int velDer) {
  // Trim de línea recta (calibrable desde el monitor)
  velIzq = velIzq * (100 + P.trimIzq) / 100;
  velDer = velDer * (100 + P.trimDer) / 100;
  velIzq = constrain(velIzq, -255, 255);
  velDer = constrain(velDer, -255, 255);
  _pwmIzq = velIzq;
  _pwmDer = velDer;

  if (INVERTIR_MOTOR_IZQ) velIzq = -velIzq;
  if (INVERTIR_MOTOR_DER) velDer = -velDer;

  if (M1_ES_IZQUIERDO) {
    _setMotorPins(PIN_M1_A, PIN_M1_B, velIzq);
    _setMotorPins(PIN_M2_A, PIN_M2_B, velDer);
  } else {
    _setMotorPins(PIN_M1_A, PIN_M1_B, velDer);
    _setMotorPins(PIN_M2_A, PIN_M2_B, velIzq);
  }
}

// Freno activo (decaimiento lento: ambos terminales en alto)
void frenar() {
  _pwmIzq = 0; _pwmDer = 0;
  analogWrite(PIN_M1_A, 255); analogWrite(PIN_M1_B, 255);
  analogWrite(PIN_M2_A, 255); analogWrite(PIN_M2_B, 255);
}

// Rueda libre
void detener() {
  _pwmIzq = 0; _pwmDer = 0;
  analogWrite(PIN_M1_A, 0); analogWrite(PIN_M1_B, 0);
  analogWrite(PIN_M2_A, 0); analogWrite(PIN_M2_B, 0);
}

int motorPwmIzq() { return _pwmIzq; }
int motorPwmDer() { return _pwmDer; }
