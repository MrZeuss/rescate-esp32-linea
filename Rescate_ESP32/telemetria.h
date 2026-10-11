#pragma once
// ============================================================
// telemetria.h — Radio ESP-NOW (entrenamiento + modo prueba)
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "comun_espnow.h"
#include "parametros.h"
#include "motores.h"
#include "sensores_linea.h"
#include "giroscopio.h"
#include "distancia.h"
#include "obstaculo.h"
#include "modo_prueba.h"

#if USAR_ESPNOW
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static volatile bool _fStart = false, _fStop = false, _fGuardar = false;
static unsigned long _tUltTele = 0, _tUltInforme = 0;
static bool _teleOk = false;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
static void _alRecibir(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
#else
static void _alRecibir(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len <= 0) return;

  if (data[0] == 'X' && len == (int)sizeof(PaqueteTest)) {
    PaqueteTest x;
    memcpy(&x, data, sizeof(x));
    switch (x.sub) {
      case TEST_ENTER:     pruebaMarcarEnter(); break;
      case TEST_EXIT:      pruebaMarcarExit();  break;
      case TEST_GYRO_CERO: pruebaMarcarGyroCero(); break;
      case TEST_MOTOR:     pruebaMarcarMotor(x.pwmIzq, x.pwmDer); break;
    }
    return;
  }

  if (data[0] == 'C' && len == (int)sizeof(PaqueteCmd)) {
    PaqueteCmd c;
    memcpy(&c, data, sizeof(c));
    switch (c.cmd) {
      case CMD_SET:     P = c.p;        break;
      case CMD_START:   _fStart = true; break;
      case CMD_STOP:    _fStop = true;  break;
      case CMD_GUARDAR: _fGuardar = true; break;
    }
  }
}

bool teleInit() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CANAL_ESPNOW, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(_alRecibir);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, MAC_BROADCAST, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  _teleOk = true;
  return true;
}

void teleEnviar(uint8_t estado) {
  if (!_teleOk) return;

  if (_fGuardar) { _fGuardar = false; paramsGuardar(); }

  unsigned long ahora = millis();

  if (ahora - _tUltInforme >= T_INFORME_MS) {
    _tUltInforme = ahora;
    PaqueteCmd inf;
    inf.tipo = 'C';
    inf.cmd  = CMD_INFORME;
    inf.p    = P;
    esp_now_send(MAC_BROADCAST, (uint8_t*)&inf, sizeof(inf));
  }

  if (ahora - _tUltTele < T_TELE_MS) return;
  _tUltTele = ahora;

  PaqueteTele t;
  t.tipo     = 'T';
  t.estado   = estado;
  t.posicion = (int16_t)lineaPosicion();
  t.patron   = 0;
  for (int i = 0; i < 5; i++) {
    if (lineaValor(i) > UMBRAL_NEGRO) t.patron |= (1 << i);
    t.linea[i] = lineaValorCrudo(i);
  }
  t.yaw10    = (int16_t)(gyroYaw()   * 10.0f);
  t.pitch10  = (int16_t)(gyroPitch() * 10.0f);
  t.dF = distFrontal(); t.dD = distDerecha(); t.dI = distIzquierda();
  t.pwmIzq = (int16_t)motorPwmIzq();
  t.pwmDer = (int16_t)motorPwmDer();
  t.flags = 0;
  if (obstActivoCrudo())  t.flags |= TF_OBSTACULO;
  if (lineaCalibrada())   t.flags |= TF_CALIBRADA;
  if (distOkFrontal())    t.flags |= TF_VL53F_OK;
  if (distOkDerecha())    t.flags |= TF_VL53D_OK;
  if (distOkIzquierda())  t.flags |= TF_VL53I_OK;
  if (gyroOk())           t.flags |= TF_MPU_OK;
  if (lineaContrasteOk()) t.flags |= TF_LINEA_OK;
  if (modoPruebaActivo()) t.flags |= TF_MODO_PRUEBA;
  t.ms = ahora;
  esp_now_send(MAC_BROADCAST, (uint8_t*)&t, sizeof(t));
}

bool teleTomarStart() { bool v = _fStart; _fStart = false; return v; }
bool teleTomarStop()  { bool v = _fStop;  _fStop  = false; return v; }

#else
bool teleInit()              { return false; }
void teleEnviar(uint8_t)     {}
bool teleTomarStart()        { return false; }
bool teleTomarStop()         { return false; }
#endif
