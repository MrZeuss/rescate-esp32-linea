#pragma once
// ============================================================
// encoders.h — Cuadratura oficial ESP32-DO + N20
//   Encoder 1 (M1 / izq): A=16  B=25
//   Encoder 2 (M2 / der): A=17  B=26
// Lógica de ISR basada en el ejemplo del fabricante.
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "motores.h"

static volatile long encoder0Pos = 0;   // M1 izquierdo
static volatile long encoder1Pos = 0;   // M2 derecho

void IRAM_ATTR change_left_a() {
  if (digitalRead(PIN_ENC0_A) == HIGH) {
    if (digitalRead(PIN_ENC0_B) == LOW) encoder0Pos++;
    else                                encoder0Pos--;
  } else {
    if (digitalRead(PIN_ENC0_B) == HIGH) encoder0Pos++;
    else                                 encoder0Pos--;
  }
}

void IRAM_ATTR change_left_b() {
  if (digitalRead(PIN_ENC0_B) == HIGH) {
    if (digitalRead(PIN_ENC0_A) == HIGH) encoder0Pos++;
    else                                 encoder0Pos--;
  } else {
    if (digitalRead(PIN_ENC0_A) == LOW) encoder0Pos++;
    else                                encoder0Pos--;
  }
}

void IRAM_ATTR change_right_a() {
  if (digitalRead(PIN_ENC1_A) == HIGH) {
    if (digitalRead(PIN_ENC1_B) == LOW) encoder1Pos--;
    else                                encoder1Pos++;
  } else {
    if (digitalRead(PIN_ENC1_B) == HIGH) encoder1Pos--;
    else                                 encoder1Pos++;
  }
}

void IRAM_ATTR change_right_b() {
  if (digitalRead(PIN_ENC1_B) == HIGH) {
    if (digitalRead(PIN_ENC1_A) == HIGH) encoder1Pos--;
    else                                 encoder1Pos++;
  } else {
    if (digitalRead(PIN_ENC1_A) == LOW) encoder1Pos--;
    else                                encoder1Pos++;
  }
}

void encodersInit() {
  pinMode(PIN_ENC0_A, INPUT_PULLUP);
  pinMode(PIN_ENC0_B, INPUT_PULLUP);
  pinMode(PIN_ENC1_A, INPUT_PULLUP);
  pinMode(PIN_ENC1_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC0_A), change_left_a,  CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC0_B), change_left_b,  CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC1_A), change_right_a, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC1_B), change_right_b, CHANGE);
}

void encReset() {
  noInterrupts();
  encoder0Pos = 0;
  encoder1Pos = 0;
  interrupts();
}

long encIzq() {
  noInterrupts(); long v = encoder0Pos; interrupts();
  return v;
}
long encDer() {
  noInterrupts(); long v = encoder1Pos; interrupts();
  return v;
}

// Magnitud media (útil para giros / avances)
long encPromedioAbs() {
  return (abs(encIzq()) + abs(encDer())) / 2;
}

// Giro pivote. sentido +1 = izquierda, -1 = derecha
bool girarPulsos(int pulsos, int sentido, int vel = VEL_GIRO) {
  encReset();
  int vi = -sentido * vel;
  int vd =  sentido * vel;
  mover(vi, vd);
  unsigned long t0 = millis();
  while (encPromedioAbs() < pulsos && millis() - t0 < 4000) {
    delay(1);
  }
  frenar();
  delay(80);
  return encPromedioAbs() >= pulsos;
}

bool avanzarPulsos(int pulsos, int vel) {
  encReset();
  mover(vel, vel);
  unsigned long t0 = millis();
  while (encPromedioAbs() < pulsos && millis() - t0 < 5000) delay(1);
  frenar();
  delay(60);
  return true;
}
