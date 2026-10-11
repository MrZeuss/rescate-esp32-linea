#pragma once
// ============================================================
// comun_espnow.h — Estructuras COMPARTIDAS robot <-> monitor
// ¡Este archivo debe ser IDÉNTICO en robot y monitores!
// ============================================================
#include <Arduino.h>

#define CANAL_ESPNOW  1

// ---------- Comandos del monitor hacia el robot ----------
enum : uint8_t {
  CMD_SET     = 1,
  CMD_START   = 2,
  CMD_STOP    = 3,
  CMD_GUARDAR = 4,
  CMD_INFORME = 5
};

// ---------- Subcomandos de prueba (paquete tipo 'X') ----------
enum : uint8_t {
  TEST_ENTER     = 1,  // entra a modo prueba (FSM detenida)
  TEST_EXIT      = 2,  // sale; frena motores
  TEST_MOTOR     = 3,  // aplica pwmIzq / pwmDer
  TEST_GYRO_CERO = 4   // yaw = 0 en la pose actual
};

// ---------- Parámetros ajustables en vivo ----------
struct __attribute__((packed)) Parametros {
  uint8_t  velBase, velMax, velGiro, velSubida;
  uint8_t  velBajada, velEvasion, velRecuperar, velCruce;
  float    kp, kd;
  uint16_t distObstaculo;
  uint16_t tEvLateral, tEvParalelo, tEvBuscar;
  int8_t   trimIzq, trimDer;
};

// ---------- Telemetría del robot hacia el monitor ----------
struct __attribute__((packed)) PaqueteTele {
  char     tipo;         // 'T'
  uint8_t  estado;
  int16_t  posicion;
  uint8_t  patron;
  uint16_t linea[5];
  int16_t  yaw10;
  int16_t  pitch10;
  uint16_t dF, dD, dI;
  int16_t  pwmIzq, pwmDer;
  uint8_t  flags;
  uint32_t ms;
};

#define TF_OBSTACULO   0x01
#define TF_CALIBRADA   0x02
#define TF_VL53F_OK    0x04
#define TF_VL53D_OK    0x08
#define TF_VL53I_OK    0x10
#define TF_MPU_OK      0x20
#define TF_LINEA_OK    0x40
#define TF_MODO_PRUEBA 0x80

struct __attribute__((packed)) PaqueteCmd {
  char       tipo;       // 'C'
  uint8_t    cmd;
  Parametros p;
};

// Paquete de modo prueba / control manual de motores
struct __attribute__((packed)) PaqueteTest {
  char    tipo;     // 'X'
  uint8_t sub;      // TEST_*
  int16_t pwmIzq;   // -255..255 (TEST_MOTOR)
  int16_t pwmDer;
};

static const uint8_t MAC_BROADCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

static const char* const NOMBRES_ESTADO[] = {
  "ESPERA", "SEGUIR", "CRUCE", "PUNTEADA", "RECUP",
  "SUBIDA", "BAJADA", "EVASION", "LABER", "META"
};
