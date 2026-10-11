#pragma once
#include <Arduino.h>
#include "config.h"

static const int PINES[5] = { PIN_L0, PIN_L1, PIN_L2, PIN_L3, PIN_L4 };
static const int PESOS[5] = { -2000, -1000, 0, 1000, 2000 };

static uint16_t _norm[5];
static int _pos = 0, _negros = 0, _lado = 0;
static bool _hay = false, _okCal = false;

void lineaInit() {
  for (int i = 0; i < 5; i++) pinMode(PINES[i], INPUT);
}

static bool _esNegroRaw(int i) {
  bool alto = digitalRead(PINES[i]) == HIGH;
  return LINEA_NEGRO_LOW ? !alto : alto;
}

void lineaCalibrar() {
  unsigned long t0 = millis();
  while (millis() - t0 < T_CALIB_MS) {
#if PIN_LED >= 0
    digitalWrite(PIN_LED, (millis() / 100) % 2);
#endif
    delay(5);
  }
#if PIN_LED >= 0
  digitalWrite(PIN_LED, LOW);
#endif
  _okCal = true;
}

void lineaLeer() {
  long sum = 0, sp = 0;
  _negros = 0;
  for (int i = 0; i < 5; i++) {
    bool n = _esNegroRaw(i);
    _norm[i] = n ? 1000 : 0;
    if (n) _negros++;
    sum += _norm[i];
    sp  += (long)_norm[i] * PESOS[i];
  }
  _hay = _negros > 0;
  if (_hay) {
    _pos = (int)(sp / sum);
    if (_norm[0] > 500 && _norm[4] <= 500) _lado = -1;
    else if (_norm[4] > 500 && _norm[0] <= 500) _lado = +1;
    else if (abs(_pos) < 500) _lado = 0;
  }
}

int  lineaPos()       { return _pos; }
int  lineaNegros()    { return _negros; }
bool lineaBlanco()    { return _negros == 0; }
bool lineaCruce()     { return _negros >= N_NEGRO_CRUCE; }
bool lineaHay()       { return _hay; }
int  lineaLado()      { return _lado; }
bool lineaCalibrada() { return _okCal; }
uint16_t lineaVal(int i) { return _norm[i]; }

// Marcador L2: negro solo en un extremo (~15 cm lateral)
// -1 = L2 a la izquierda, +1 = derecha, 0 = no
int lineaMarcadorL2() {
  bool izq = _norm[0] > 500 && _norm[1] > 500 && _norm[3] <= 500 && _norm[4] <= 500;
  bool der = _norm[4] > 500 && _norm[3] > 500 && _norm[0] <= 500 && _norm[1] <= 500;
  // con línea central aún visible
  if (izq && _norm[2] > 500) return -1;
  if (der && _norm[2] > 500) return +1;
  return 0;
}
