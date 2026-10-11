#pragma once
// ============================================================
// enlace.h — ESP-NOW del lado del monitor
// Recibe: PaqueteTele (20 Hz) y CMD_INFORME con los parámetros
//         actuales del robot (cada 2 s).
// Envía:  CMD_SET (parámetros editados), CMD_START, CMD_STOP,
//         CMD_GUARDAR (grabar en la flash del robot).
// El monitor adopta los parámetros informados por el robot
// hasta que el usuario edita algo (g_editado): desde ahí manda
// su propia copia y el robot obedece.
// API: enlaceInit(), enlaceHayTele(), g_tele, g_params,
//      enlaceEnviarSet(), enlaceEnviarCmd(cmd)
// ============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config_monitor.h"
#include "comun_espnow.h"

PaqueteTele g_tele = {};
Parametros  g_params = {};
volatile bool g_paramsValidos = false;   // ya recibimos un informe
volatile bool g_editado = false;         // el usuario tocó algo
volatile unsigned long g_tUltimoTele = 0;
volatile bool g_versionErronea = false;  // llega tele con OTRO tamaño

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
static void _alRecibir(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
#else
static void _alRecibir(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len > 0 && data[0] == 'T') {
    if (len == sizeof(PaqueteTele)) {
      memcpy((void*)&g_tele, data, sizeof(PaqueteTele));
      g_tUltimoTele = millis();
      g_versionErronea = false;
    } else {
      // Llega telemetría pero con tamaño distinto: robot y monitor
      // compilados con versiones DIFERENTES de comun_espnow.h
      g_versionErronea = true;
    }
  } else if (len == sizeof(PaqueteCmd) && data[0] == 'C') {
    PaqueteCmd c;
    memcpy(&c, data, sizeof(c));
    if (c.cmd == CMD_INFORME && !g_editado) {
      g_params = c.p;                    // sincronizar con el robot
      g_paramsValidos = true;
    }
  }
}

bool enlaceInit() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CANAL_ESPNOW, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(_alRecibir);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, MAC_BROADCAST, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  return true;
}

bool enlaceHayTele() {
  return (millis() - g_tUltimoTele) < T_ENLACE_MS;
}

void enlaceEnviarCmd(uint8_t cmd) {
  PaqueteCmd c;
  c.tipo = 'C';
  c.cmd  = cmd;
  c.p    = g_params;
  esp_now_send(MAC_BROADCAST, (uint8_t*)&c, sizeof(c));
}

void enlaceEnviarSet() {
  g_editado = true;
  enlaceEnviarCmd(CMD_SET);
}

void enlaceEnviarTest(uint8_t sub, int16_t pwmIzq, int16_t pwmDer) {
  PaqueteTest x;
  x.tipo = 'X';
  x.sub = sub;
  x.pwmIzq = pwmIzq;
  x.pwmDer = pwmDer;
  esp_now_send(MAC_BROADCAST, (uint8_t*)&x, sizeof(x));
}
