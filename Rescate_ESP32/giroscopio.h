#pragma once
// ============================================================
// giroscopio.h — IMU del GY-91 (MPU-6500/9250) por I2C
//
// Mejoras tomadas del sketch de prueba en Arduino Nano:
//   - rango gyro ±250 °/s (131 LSB/°/s) → más resolución en giros 90°
//   - calibración con muchas muestras (~2000)
//   - zona muerta en °/s para reducir deriva en reposo
//   - pivote con velocidad proporcional al error (menos sobrepaso)
//
// Yaw:   integración del gyro Z
// Pitch: acelerómetro filtrado (puente)
//
// Dirección: 0x68 (SA0 libre/GND). WHO típico: 0x70.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "parametros.h"
#include "motores.h"

static float _yaw = 0, _pitch = 0;
static float _biasGz = 0;
static float _yawObjetivoRecto = 0;
static unsigned long _tAnteriorGyro = 0;
static bool _mpuOk = false;
static uint8_t _who = 0;

static void _mpuEscribir(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_DIR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static uint8_t _mpuLeerReg(uint8_t reg) {
  Wire.beginTransmission(MPU_DIR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom((int)MPU_DIR, 1) < 1) return 0;
  return Wire.read();
}

// Una transacción: accel XYZ (pitch) + gyro Z (yaw)
static bool _mpuLeer(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gz) {
  Wire.beginTransmission(MPU_DIR);
  Wire.write(0x3B);                       // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU_DIR, 14) < 14) return false;
  ax = (Wire.read() << 8) | Wire.read();
  ay = (Wire.read() << 8) | Wire.read();
  az = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();               // temperatura
  Wire.read(); Wire.read();               // gyro X
  Wire.read(); Wire.read();               // gyro Y
  gz = (Wire.read() << 8) | Wire.read();
  return true;
}

bool gyroInit() {
  delay(50);
  _who = _mpuLeerReg(0x75);               // WHO_AM_I
  bool idOk = (_who == MPU_WHO_ESPERADO || _who == 0x70 ||
               _who == 0x68 || _who == 0x71);
  if (!idOk) {
    Serial.printf("MPU WHO_AM_I=0x%02X (esperado 0x%02X)\n", _who, MPU_WHO_ESPERADO);
  }

  // Despertar (igual que el sketch del Nano)
  _mpuEscribir(0x6B, 0x00);
  delay(100);
  _mpuEscribir(0x1A, 0x03);   // DLPF ~44 Hz (CONFIG)
  _mpuEscribir(0x1B, 0x00);   // gyro ±250 °/s  → 131 LSB/(°/s)
  _mpuEscribir(0x1C, 0x00);   // accel ±2g
  delay(50);

  int16_t a, b, c, d;
  _mpuOk = _mpuLeer(a, b, c, d);
  if (_mpuOk && !idOk) {
    Serial.println(F("ADVERTENCIA: MPU responde con WHO inesperado"));
  }
  _tAnteriorGyro = micros();
  if (_mpuOk) Serial.printf("MPU OK WHO=0x%02X dir=0x%02X ±250dps\n", _who, MPU_DIR);
  return _mpuOk;
}

void gyroCalibrar() {
  if (!_mpuOk) return;

  Serial.println(F("Calibrando gyro: mantenga el carro INMOVIL..."));
  delay(500);

  long suma = 0;
  int validas = 0;
  int16_t ax, ay, az, gz;
  for (int i = 0; i < GYRO_CAL_MUESTRAS; i++) {
    if (_mpuLeer(ax, ay, az, gz)) {
      suma += gz;
      validas++;
    }
    delay(2);
  }

  if (validas == 0) {
    Serial.println(F("ERROR: calibracion gyro sin muestras"));
    return;
  }

  _biasGz = (float)suma / (float)validas;
  _yaw = 0;
  _tAnteriorGyro = micros();
  Serial.printf("Gyro offset Z: %.3f (%d muestras)\n", _biasGz, validas);
}

void gyroActualizar() {
  if (!_mpuOk) return;
  int16_t ax, ay, az, gz;
  if (!_mpuLeer(ax, ay, az, gz)) return;

  unsigned long ahora = micros();
  float dt = (ahora - _tAnteriorGyro) * 1e-6f;
  _tAnteriorGyro = ahora;
  if (dt <= 0.0f || dt > 0.1f) return;

  // Velocidad angular Z (°/s), misma fórmula que el Nano
  float wz = ((float)gz - _biasGz) / GYRO_LSB_POR_DPS;

  // Zona muerta: evita acumular ruido cuando está casi quieto
  if (fabsf(wz) < GYRO_ZONA_MUERTA) wz = 0.0f;

  _yaw += SIGNO_YAW * wz * dt;

  // Pitch para el puente (no estaba en el sketch del Nano)
  float p = atan2f(-(float)ax, sqrtf((float)ay * ay + (float)az * az))
            * 57.2958f * SIGNO_PITCH;
  _pitch = 0.9f * _pitch + 0.1f * p;
}

float gyroYaw()   { return _yaw; }
float gyroPitch() { return _pitch; }
void  gyroResetYaw() { _yaw = 0; }
void  gyroGuardarRumbo() { _yawObjetivoRecto = _yaw; }
bool  gyroOk() { return _mpuOk; }
uint8_t gyroWhoAmI() { return _who; }

// Pivote en el lugar. delta en grados: positivo = izquierda.
// Velocidad proporcional al error restante → menos sobrepaso que PWM fijo.
bool girarGrados(float delta) {
  if (!_mpuOk) return false;
  float objetivo = _yaw + delta;
  unsigned long t0 = millis();

  while (millis() - t0 < T_GIRO_MAX) {
    gyroActualizar();
    float err = objetivo - _yaw;
    if (fabsf(err) < TOL_GIRO) break;

    float mag = fabsf(err) * KP_GIRO_ANG;
    if (mag < (float)VEL_GIRO_MIN) mag = (float)VEL_GIRO_MIN;
    if (mag > (float)P.velGiro)    mag = (float)P.velGiro;
    int v = (err > 0.0f) ? (int)mag : -(int)mag;
    mover(-v, v);                          // izq atrás / der adelante = giro izq
    delay(2);
  }

  frenar();
  delay(80);
  // Breve asiento: sigue integrando sin motores (inercia residual)
  unsigned long t1 = millis();
  while (millis() - t1 < 40) {
    gyroActualizar();
    delay(2);
  }
  return fabsf(objetivo - _yaw) < TOL_GIRO * 2.0f;
}

void avanzarRectoGyro(int vel) {
  float err = _yawObjetivoRecto - _yaw;
  int corr = (int)(KP_YAW * err);
  mover(constrain(vel - corr, -255, 255), constrain(vel + corr, -255, 255));
}
