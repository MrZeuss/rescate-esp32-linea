// ============================================================
// Rescate_ESP32.ino — Robot de rescate (liga nacional)
// Placa: ESP32-DO con driver de motores integrado
//
// Hardware:
//   HW-871     barra 5 canales (TCRT5000, salidas digitales)
//   3x VL53L0X distancia láser (frontal / derecha / izquierda)
//   MPU-6500   yaw (giros 90°) y pitch (puente)
//   ESP-NOW    telemetría hacia Monitor_Rescate / _800
//
// ARCHIVOS:
//   config.h          pines y constantes de calibración
//   motores.h         puente H del ESP32-DO
//   sensores_linea.h  HW-871
//   giroscopio.h      MPU-6500
//   distancia.h       3x VL53L0X
//   obstaculo.h       cubo (VL53 frontal)
//   control_pid.h     lazo PD + sample & hold
//   evasion.h         maniobra del cubo
//   laberinto.h       mano derecha
//   maquina_estados.h cerebro SEGUIR/CRUCE/.../META
//   parametros.h      ajustes en vivo + NVS
//   telemetria.h      ESP-NOW bidireccional
//   comun_espnow.h    structs compartidos con el monitor
//
// Diagnóstico en pista: Serial (115200) y/o monitor ESP-NOW.
//
// SECUENCIA EN PISTA:
//   1. Encender el carro QUIETO (calibra bias del gyro ~1 s)
//   2. BTN_CALIB: confirma/calibra la barra HW-871
//   3. Sobre la línea, BTN_START (o START desde el monitor)
//   4. BTN_START / STOP remoto: parada de emergencia
// ============================================================

#include <Wire.h>
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
#include "telemetria.h"
#include "maquina_estados.h"
#include "modo_prueba.h"

static bool _btnStartEstable = HIGH;
static bool _btnCalibEstable = HIGH;
static bool _btnStartRaw     = HIGH;
static bool _btnCalibRaw     = HIGH;
static unsigned long _tBtnStart = 0;
static unsigned long _tBtnCalib = 0;

static bool _flancoBajada(bool lectura, bool &raw, bool &estable,
                          unsigned long &tCambio) {
  if (lectura != raw) {
    raw = lectura;
    tCambio = millis();
  }
  if ((millis() - tCambio) >= T_DEBOUNCE_BTN && lectura != estable) {
    bool fueAlta = (estable == HIGH);
    estable = lectura;
    return fueAlta && (estable == LOW);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n=== Rescate ESP32-DO / HW-871 / VL53 / MPU-6500 ==="));

  pinMode(PIN_LED_META, OUTPUT);
  digitalWrite(PIN_LED_META, LOW);
  pinMode(PIN_BTN_CALIB, INPUT_PULLUP);
  pinMode(PIN_BTN_START, INPUT_PULLUP);

  paramsCargar();
  motoresInit();
  lineaInit();

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  Serial.println(F("Iniciando VL53L0X..."));
  if (!distanciaInit()) {
    Serial.println(F("ADVERTENCIA: algun VL53L0X no responde (revisar XSHUT)"));
    delay(1000);
  }

  if (gyroInit()) {
    Serial.println(F("Calibrando MPU (~4 s)... NO MOVER"));
    gyroCalibrar();
    Serial.println(F("MPU OK"));
  } else {
    Serial.println(F("ADVERTENCIA: MPU no responde"));
  }

  if (USAR_ESPNOW) {
    if (teleInit()) Serial.println(F("ESP-NOW activo (canal 1) -> monitor"));
    else            Serial.println(F("ADVERTENCIA: fallo al iniciar ESP-NOW"));
  }

  Serial.println(F("Listo. CALIB / START o comandos del monitor."));
}

void loop() {
  bool rawCalib = digitalRead(PIN_BTN_CALIB);
  bool rawStart = digitalRead(PIN_BTN_START);

  if (_flancoBajada(rawCalib, _btnCalibRaw, _btnCalibEstable, _tBtnCalib)
      && estadoActual() == ESPERA) {
    Serial.println(F("Calibrando linea HW-871..."));
    delay(200);
    lineaCalibrar();
    Serial.println(lineaContrasteOk() ? F("Linea OK") : F("Linea: poco contraste"));
  }

  if (_flancoBajada(rawStart, _btnStartRaw, _btnStartEstable, _tBtnStart)) {
    if (modoPruebaActivo()) {
      // en prueba: START no lanza la carrera
    } else if (estadoActual() == ESPERA) {
      delay(300);
      estadoIniciarCarrera();
    } else {
      estadoDetenerTodo();
    }
  }

  if (teleTomarStart() && estadoActual() == ESPERA && !modoPruebaActivo())
    estadoIniciarCarrera();
  if (teleTomarStop()) {
    estadoDetenerTodo();
  }

  pruebaProcesar();

  lineaLeer();
  gyroActualizar();
  distanciaActualizar();

  if (modoPruebaActivo()) {
    // FSM detenida: solo sensores + telemetría + motores manuales
  } else {
    maquinaEstados();
  }
  teleEnviar((uint8_t)estadoActual());
}
