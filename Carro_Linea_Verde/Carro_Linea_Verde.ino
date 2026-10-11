// ============================================================
// Carro_Linea_Verde.ino — Segundo robot (Rescue Line simplificado)
//
// SEPARADO del proyecto Rescate_ESP32 (carro complejo).
//
// Hardware:
//   ESP32-DO + 2× N20 con encoder
//   HW-871 (5 IR) + 2× GY-31/TCS3200 (verde izq/der)
//   VL53L0X o HC-SR04 frontal (obstáculo 10 cm)
//
// Reglas (RoboCup Junior Rescue Line / rescate avanzado):
//   - Verde izq/der ante intersección → giro 90°
//   - Verde ambos → 180° (punto muerto)
//   - Sin verde → de frente
//   - L2 lateral → memoriza lado para el cuadrado F1
//   - Obstáculo → evasión por encoders y reenganche
//
// Ver README.md: listado de componentes y conexiones.
// ============================================================

#include "config.h"
#include "motores.h"
#include "encoders.h"
#include "linea.h"
#include "color.h"
#include "obstaculo.h"
#include "control_pid.h"
#include "maquina_estados.h"

static bool _stEst = HIGH, _stRaw = HIGH, _calEst = HIGH, _calRaw = HIGH;
static unsigned long _tSt = 0, _tCal = 0;

static bool _flanco(bool lec, bool &raw, bool &est, unsigned long &t) {
  if (lec != raw) { raw = lec; t = millis(); }
  if (millis() - t >= T_DEBOUNCE_BTN && lec != est) {
    bool fueAlta = (est == HIGH);
    est = lec;
    return fueAlta && est == LOW;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n=== Carro_Linea_Verde (Rescue Line) ==="));

#if PIN_LED >= 0
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
#endif
  pinMode(PIN_BTN_START, INPUT_PULLUP);
  pinMode(PIN_BTN_CALIB, INPUT_PULLUP);

  motoresInit();
  encodersInit();
  lineaInit();
  colorInit();
  obstInit();

  Serial.println(F("CALIB: barre la linea | START: correr"));
  Serial.println(F("Serial 'd' = debug color"));
}

void loop() {
  if (_flanco(digitalRead(PIN_BTN_CALIB), _calRaw, _calEst, _tCal)
      && estadoActual() == ESPERA) {
    Serial.println(F("Calibrando linea..."));
    lineaCalibrar();
    Serial.println(F("Linea OK"));
  }

  if (_flanco(digitalRead(PIN_BTN_START), _stRaw, _stEst, _tSt)) {
    if (estadoActual() == ESPERA) {
      delay(250);
      estadoIniciar();
      Serial.println(F("CARRERA"));
    } else {
      estadoDetener();
      Serial.println(F("STOP"));
    }
  }

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'd' || c == 'D') colorDebug();
    if (c == 'e' || c == 'E') {
      Serial.printf("enc M1=%ld  M2=%ld\n", encIzq(), encDer());
    }
  }

  lineaLeer();
  colorActualizar();
  obstActualizar();
  maquinaEstados();
}
