#pragma once
// ============================================================
// sensores_linea.h — HW-871 (5x TCRT5000)
//
// Por defecto lee las salidas DIGITALES OUT1..OUT5.
// Si el módulo tiene pines analógicos, ponga LINEA_MODO_DIGITAL
// en false en config.h (incluye oversampling y calibración).
//
// API:
//   lineaInit(), lineaCalibrar(), lineaLeer()
//   lineaPosicion()     -2000..+2000
//   lineaNumNegros(), lineaTodoBlanco(), lineaTodoNegro()
//   lineaUltimoLado(), lineaHayLinea(), lineaCalibrada()
//   lineaValor(i)       0..1000 (normalizado)
//   lineaValorCrudo(i)  digital: 0/4095 ; analógico: ADC 0..4095
//   lineaContrasteOk()  true si la calibración tiene contraste útil
// ============================================================
#include <Arduino.h>
#include "config.h"

static const int PINES_LINEA[5] = {
  PIN_LINEA_0, PIN_LINEA_1, PIN_LINEA_2, PIN_LINEA_3, PIN_LINEA_4
};
static const int PESOS[5] = { -2000, -1000, 0, 1000, 2000 };

static uint16_t _calMin[5], _calMax[5];
static uint16_t _valNorm[5];
static uint16_t _valCrudo[5];
static int  _posicion = 0;
static int  _numNegros = 0;
static int  _ultimoLado = 0;
static bool _hayLinea = false;
static bool _calibrada = false;
static bool _contrasteOk = true;

void lineaInit() {
  for (int i = 0; i < 5; i++) {
#if LINEA_MODO_DIGITAL
    pinMode(PINES_LINEA[i], INPUT);
#else
    // ADC: no hace falta pinMode especial en ESP32
#endif
    _calMin[i] = 4095;
    _calMax[i] = 0;
    _valNorm[i] = 0;
    _valCrudo[i] = 0;
  }
#if !LINEA_MODO_DIGITAL
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
#endif
}

#if LINEA_MODO_DIGITAL
// HIGH/LOW del HW-871 -> "más = más negro" unificado
static uint16_t _leerCruda(int i) {
  bool alto = digitalRead(PINES_LINEA[i]) == HIGH;
  bool negro = LINEA_NEGRO_ALTO ? alto : !alto;
  return negro ? 4095 : 0;
}
#else
static uint16_t _leerUnaADC(int i) {
  uint16_t v = analogRead(PINES_LINEA[i]);
  if (!LINEA_NEGRO_ALTO) v = 4095 - v;
  return v;
}

static uint16_t _leerCruda(int i) {
  uint32_t suma = 0;
  for (int n = 0; n < LINEA_MUESTRAS_ADC; n++) suma += _leerUnaADC(i);
  return (uint16_t)(suma / LINEA_MUESTRAS_ADC);
}
#endif

void lineaCalibrar() {
#if LINEA_MODO_DIGITAL
  // En digital no hay rango ADC: solo confirma lecturas y marca calibrada.
  // Sigue sirviendo el mismo botón en pista (parpadeo LED).
  unsigned long t0 = millis();
  while (millis() - t0 < 800) {
    for (int i = 0; i < 5; i++) _valCrudo[i] = _leerCruda(i);
    digitalWrite(PIN_LED_META, (millis() / 100) % 2);
    delay(2);
  }
  digitalWrite(PIN_LED_META, LOW);
  _calibrada = true;
  _contrasteOk = true;
  for (int i = 0; i < 5; i++) {
    _calMin[i] = 0;
    _calMax[i] = 4095;
  }
#else
  for (int i = 0; i < 5; i++) { _calMin[i] = 4095; _calMax[i] = 0; }
  unsigned long t0 = millis();
  while (millis() - t0 < T_CALIBRACION) {
    for (int i = 0; i < 5; i++) {
      uint16_t v = _leerCruda(i);
      if (v < _calMin[i]) _calMin[i] = v;
      if (v > _calMax[i]) _calMax[i] = v;
    }
    digitalWrite(PIN_LED_META, (millis() / 100) % 2);
    delay(2);
  }
  digitalWrite(PIN_LED_META, LOW);
  _calibrada = true;
  _contrasteOk = true;
  for (int i = 0; i < 5; i++) {
    uint16_t rango = _calMax[i] - _calMin[i];
    if (rango < CONTRASTE_MIN_ADC) {
      // Conserva el poco contraste medido; no fuerza 0..4095
      if (rango < 1) { _calMin[i] = 0; _calMax[i] = 4095; }
      _contrasteOk = false;
    }
  }
#endif
}

void lineaLeer() {
  long suma = 0, sumaPesos = 0;
  _numNegros = 0;

  for (int i = 0; i < 5; i++) {
    uint16_t v = _leerCruda(i);
    _valCrudo[i] = v;

#if LINEA_MODO_DIGITAL
    _valNorm[i] = (v > 2048) ? 1000 : 0;
#else
    uint16_t vmin = _calMin[i], vmax = _calMax[i];
    if (vmax <= vmin) { vmin = 0; vmax = 4095; }
    v = constrain(v, vmin, vmax);
    _valNorm[i] = (uint32_t)(v - vmin) * 1000UL / (vmax - vmin);
#endif

    if (_valNorm[i] > UMBRAL_NEGRO) _numNegros++;
    suma      += (long)_valNorm[i];
    sumaPesos += (long)_valNorm[i] * PESOS[i];
  }

  _hayLinea = (_numNegros > 0);
  if (_hayLinea) {
    _posicion = (int)(sumaPesos / suma);
    if      (_valNorm[0] > UMBRAL_NEGRO && _valNorm[4] <= UMBRAL_NEGRO) _ultimoLado = -1;
    else if (_valNorm[4] > UMBRAL_NEGRO && _valNorm[0] <= UMBRAL_NEGRO) _ultimoLado = +1;
    else if (abs(_posicion) < 500) _ultimoLado = 0;
  }
}

int  lineaPosicion()   { return _posicion; }
int  lineaNumNegros()  { return _numNegros; }
bool lineaTodoBlanco() { return _numNegros == 0; }
bool lineaTodoNegro()  { return _numNegros >= N_SENSORES_CRUCE; }
int  lineaUltimoLado() { return _ultimoLado; }
bool lineaHayLinea()   { return _hayLinea; }
uint16_t lineaValor(int i) { return _valNorm[i]; }
bool lineaCalibrada()  { return _calibrada; }
bool lineaContrasteOk(){ return _contrasteOk; }
uint16_t lineaValorCrudo(int i) { return _valCrudo[i]; }
