#pragma once
// ============================================================
// interfaz.h — Tablero táctil del monitor (240x320)
//
// Página TELE:    estado grande, barra de posición de línea,
//                 patrón de los 5 sensores, yaw/pitch,
//                 distancias láser F/D/I y PWM de cada motor.
// Página AJUSTES: 16 parámetros en páginas de 6, con botones
//                 [-] y [+] por fila (cada toque envía CMD_SET
//                 al robot: el efecto es INMEDIATO en pista).
//                 Abajo: START / STOP / GRABAR (flash del robot).
// API: uiInit(), uiTocar(x,y), uiDibujar()
// ============================================================
#include <Arduino.h>
#include "config_monitor.h"
#include "comun_espnow.h"
#include "lcd.h"
#include "enlace.h"

// ---------------- Tabla de parámetros editables ----------------
enum TipoParam : uint8_t { TP_U8, TP_F, TP_U16, TP_I8 };
struct DefParam {
  const char *nombre;
  TipoParam   tipo;
  void       *ptr;      // apunta dentro de g_params
  float       paso, minimo, maximo;
};

static DefParam TABLA[] = {
  {"Vel base",    TP_U8,  &g_params.velBase,      5, 40, 255},
  {"Vel max",     TP_U8,  &g_params.velMax,       5, 60, 255},
  {"Vel giro",    TP_U8,  &g_params.velGiro,      5, 60, 255},
  {"Vel subida",  TP_U8,  &g_params.velSubida,    5, 80, 255},
  {"Vel bajada",  TP_U8,  &g_params.velBajada,    5, 30, 200},
  {"Vel evasion", TP_U8,  &g_params.velEvasion,   5, 60, 255},
  {"Vel recup",   TP_U8,  &g_params.velRecuperar, 5, 50, 200},
  {"Vel cruce",   TP_U8,  &g_params.velCruce,     5, 60, 255},
  {"Kp",          TP_F,   &g_params.kp,       0.005f, 0, 1},
  {"Kd",          TP_F,   &g_params.kd,        0.05f, 0, 10},
  {"Dist obst",   TP_U16, &g_params.distObstaculo, 10, 50, 500},
  {"tEv lateral", TP_U16, &g_params.tEvLateral,    50, 100, 3000},
  {"tEv paralelo",TP_U16, &g_params.tEvParalelo,   50, 100, 4000},
  {"tEv buscar",  TP_U16, &g_params.tEvBuscar,     50, 500, 6000},
  {"Trim izq %",  TP_I8,  &g_params.trimIzq,        1, -30, 30},
  {"Trim der %",  TP_I8,  &g_params.trimDer,        1, -30, 30},
};
static const int N_PARAMS = sizeof(TABLA) / sizeof(TABLA[0]);
static const int POR_PAGINA = 6;
static const int N_PAGINAS = (N_PARAMS + POR_PAGINA - 1) / POR_PAGINA;

// ---------------- Estado de la interfaz ----------------
static uint8_t _pagina = 0;         // 0=TELE 1=AJUSTES 2=PRUEBA
static uint8_t _pagParam = 0;
static uint8_t _prSub = 0;          // 0 giro 1 mot 2 lin 3 dist
static int16_t _prVel = 120;
static bool _prHold = false;
static int16_t _prIzq = 0, _prDer = 0;
static unsigned long _tUltDibujo = 0, _tUltToque = 0, _tUltMot = 0;

static float _leerParam(int i) {
  switch (TABLA[i].tipo) {
    case TP_U8:  return *(uint8_t*) TABLA[i].ptr;
    case TP_F:   return *(float*)   TABLA[i].ptr;
    case TP_U16: return *(uint16_t*)TABLA[i].ptr;
    case TP_I8:  return *(int8_t*)  TABLA[i].ptr;
  }
  return 0;
}

static void _escribirParam(int i, float v) {
  v = constrain(v, TABLA[i].minimo, TABLA[i].maximo);
  switch (TABLA[i].tipo) {
    case TP_U8:  *(uint8_t*) TABLA[i].ptr = (uint8_t)(v + 0.5f); break;
    case TP_F:   *(float*)   TABLA[i].ptr = v;                   break;
    case TP_U16: *(uint16_t*)TABLA[i].ptr = (uint16_t)(v + 0.5f);break;
    case TP_I8:  *(int8_t*)  TABLA[i].ptr = (int8_t)roundf(v);   break;
  }
}

static void _formatear(int i, char *buf) {
  if (TABLA[i].tipo == TP_F) dtostrf(_leerParam(i), 0, 3, buf);
  else                       sprintf(buf, "%d", (int)_leerParam(i));
}

// ---------------- Dibujo ----------------
static void _boton(int x, int y, int w, int h, const char *txt, uint16_t col) {
  gfx->fillRoundRect(x, y, w, h, 6, COL_BOTON);
  gfx->drawRoundRect(x, y, w, h, 6, col);
  gfx->setTextColor(col);
  gfx->setTextSize(2);
  int tw = strlen(txt) * 12;
  gfx->setCursor(x + (w - tw) / 2, y + (h - 14) / 2);
  gfx->print(txt);
}

static void _tabs() {
  _boton(0, 290, 78, 30, "TELE",   _pagina == 0 ? COL_ACENTO : COL_TENUE);
  _boton(80, 290, 78, 30, "AJUS",  _pagina == 1 ? COL_ACENTO : COL_TENUE);
  _boton(160, 290, 80, 30, "PRUE", _pagina == 2 ? COL_ACENTO : COL_TENUE);
}

static void _dibujarPrueba() {
  gfx->fillRect(0, 0, 240, 290, COL_FONDO);
  bool ok = enlaceHayTele();
  bool pr = g_tele.flags & TF_MODO_PRUEBA;
  gfx->fillCircle(228, 10, 6, ok ? (pr ? COL_AMAR : COL_OK) : COL_ALERTA);

  _boton(0, 0, 70, 26, "ENT", COL_OK);
  _boton(74, 0, 70, 26, "SAL", COL_ALERTA);
  _boton(148, 0, 50, 26, "Y0", COL_ACENTO);

  const char *n[] = {"GIR", "MOT", "LIN", "DST"};
  for (int i = 0; i < 4; i++)
    _boton(i * 60, 30, 58, 24, n[i], _prSub == i ? COL_ACENTO : COL_TENUE);

  char buf[40];
  if (_prSub == 0) {
    gfx->setTextSize(4);
    gfx->setTextColor(COL_AMAR);
    sprintf(buf, "%5.1f", g_tele.yaw10 / 10.0f);
    gfx->setCursor(20, 100); gfx->print(buf);
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TEXTO);
    sprintf(buf, "pit %.1f", g_tele.pitch10 / 10.0f);
    gfx->setCursor(20, 180); gfx->print(buf);
    gfx->setTextSize(1);
    gfx->setCursor(20, 220);
    gfx->print(pr ? "modo prueba ON - gire a mano" : "pulse ENT");
  } else if (_prSub == 1) {
    sprintf(buf, "vel %d", _prVel);
    gfx->setTextSize(2); gfx->setTextColor(COL_TEXTO);
    gfx->setCursor(4, 60); gfx->print(buf);
    _boton(80, 58, 40, 28, "+", COL_TEXTO);
    _boton(130, 58, 40, 28, "-", COL_TEXTO);
    _boton(80, 100, 80, 40, "AVA", COL_OK);
    _boton(0, 150, 70, 40, "IZQ", COL_ACENTO);
    _boton(85, 150, 70, 40, "STO", COL_ALERTA);
    _boton(170, 150, 70, 40, "DER", COL_ACENTO);
    _boton(80, 200, 80, 40, "RET", COL_AMAR);
    _boton(0, 100, 55, 40, "I+", COL_TEXTO);
    _boton(185, 100, 55, 40, "D+", COL_TEXTO);
    gfx->setTextSize(1);
    sprintf(buf, "pwm I:%d D:%d", g_tele.pwmIzq, g_tele.pwmDer);
    gfx->setCursor(4, 250); gfx->print(buf);
  } else if (_prSub == 2) {
    for (int i = 0; i < 5; i++) {
      bool n = g_tele.patron & (1 << i);
      int cx = 24 + i * 48;
      if (n) gfx->fillCircle(cx, 100, 14, COL_TEXTO);
      else   gfx->drawCircle(cx, 100, 14, COL_TENUE);
      gfx->setTextSize(1); gfx->setTextColor(COL_ACENTO);
      sprintf(buf, "%d", g_tele.linea[i]);
      gfx->setCursor(cx - 12, 130); gfx->print(buf);
    }
    gfx->setTextSize(2); gfx->setTextColor(COL_AMAR);
    sprintf(buf, "pos %d", g_tele.posicion);
    gfx->setCursor(20, 180); gfx->print(buf);
  } else {
    gfx->setTextSize(2); gfx->setTextColor(COL_TEXTO);
    gfx->setCursor(4, 70);
    gfx->print("I ");
    if (!(g_tele.flags & TF_VL53I_OK)) gfx->print("---");
    else if (g_tele.dI >= 2000) gfx->print("libre");
    else { sprintf(buf, "%d mm", g_tele.dI); gfx->print(buf); }
    gfx->setCursor(4, 120);
    gfx->print("F ");
    if (!(g_tele.flags & TF_VL53F_OK)) gfx->print("---");
    else if (g_tele.dF >= 2000) gfx->print("libre");
    else { sprintf(buf, "%d mm", g_tele.dF); gfx->print(buf); }
    gfx->setCursor(4, 170);
    gfx->print("D ");
    if (!(g_tele.flags & TF_VL53D_OK)) gfx->print("---");
    else if (g_tele.dD >= 2000) gfx->print("libre");
    else { sprintf(buf, "%d mm", g_tele.dD); gfx->print(buf); }
  }
}

static void _dibujarTele() {
  gfx->fillRect(0, 0, 240, 290, COL_FONDO);

  // enlace
  bool ok = enlaceHayTele();
  gfx->fillCircle(228, 10, 6, ok ? COL_OK : COL_ALERTA);
  gfx->setTextSize(1);
  gfx->setTextColor(COL_TENUE);
  gfx->setCursor(4, 5);
  gfx->print(ok ? "ROBOT EN LINEA" : "SIN SENAL...");

  // diagnóstico: llegan paquetes pero de otra versión del protocolo
  if (g_versionErronea) {
    gfx->setTextColor(COL_ALERTA);
    gfx->setCursor(4, 16);
    gfx->print("PROTOCOLO DISTINTO! resubir robot Y monitor v3.1");
  }

  // estado grande
  gfx->setTextSize(4);
  gfx->setTextColor(ok ? COL_AMAR : COL_TENUE);
  gfx->setCursor(8, 28);
  gfx->print(NOMBRES_ESTADO[g_tele.estado < 10 ? g_tele.estado : 0]);

  // barra de posición de línea (-2000..+2000)
  gfx->drawRect(10, 74, 220, 20, COL_TENUE);
  gfx->drawFastVLine(120, 74, 20, COL_TENUE);
  int px = map(constrain(g_tele.posicion, -2000, 2000), -2000, 2000, 12, 226);
  gfx->fillRect(px - 3, 76, 6, 16, COL_ACENTO);

  // patrón de sensores (S0 izq ... S4 der)
  for (int i = 0; i < 5; i++) {
    bool negro = g_tele.patron & (1 << i);
    int cx = 40 + i * 40;
    if (negro) gfx->fillCircle(cx, 112, 10, COL_TEXTO);
    else       gfx->drawCircle(cx, 112, 10, COL_TENUE);
  }

  char buf[40];
  gfx->setTextColor(COL_TEXTO);
  gfx->setTextSize(2);

  sprintf(buf, "yaw:%5.1f pit:%4.1f", g_tele.yaw10 / 10.0f, g_tele.pitch10 / 10.0f);
  gfx->setCursor(4, 138); gfx->print(buf);

  // distancias laser (999 = sin pared)
  sprintf(buf, "I:%3d F:%3d D:%3d",
          min((int)g_tele.dI, 999), min((int)g_tele.dF, 999), min((int)g_tele.dD, 999));
  gfx->setCursor(4, 166); gfx->print(buf);
  gfx->setTextSize(1);
  gfx->setTextColor(COL_TENUE);
  gfx->setCursor(4, 186); gfx->print("distancias en mm (999 = libre)");

  // PWM motores con barras
  gfx->setTextSize(2);
  gfx->setTextColor(COL_TEXTO);
  sprintf(buf, "PWM I:%4d D:%4d", g_tele.pwmIzq, g_tele.pwmDer);
  gfx->setCursor(4, 204); gfx->print(buf);
  int bi = map(constrain(g_tele.pwmIzq, -255, 255), -255, 255, -55, 55);
  int bd = map(constrain(g_tele.pwmDer, -255, 255), -255, 255, -55, 55);
  gfx->drawRect(10, 230, 110, 14, COL_TENUE);
  gfx->drawRect(126, 230, 110, 14, COL_TENUE);
  if (bi >= 0) gfx->fillRect(65, 232, bi, 10, COL_OK);
  else         gfx->fillRect(65 + bi, 232, -bi, 10, COL_ALERTA);
  if (bd >= 0) gfx->fillRect(181, 232, bd, 10, COL_OK);
  else         gfx->fillRect(181 + bd, 232, -bd, 10, COL_ALERTA);

  // salud de sensores (MPU + VL53 + linea)
  gfx->setTextSize(1);
  gfx->setTextColor(COL_TENUE);
  gfx->setCursor(4, 248);
  char salud[48];
  sprintf(salud, "MPU:%s F:%s D:%s I:%s",
          (g_tele.flags & TF_MPU_OK)   ? "OK" : "--",
          (g_tele.flags & TF_VL53F_OK) ? "OK" : "--",
          (g_tele.flags & TF_VL53D_OK) ? "OK" : "--",
          (g_tele.flags & TF_VL53I_OK) ? "OK" : "--");
  gfx->print(salud);

  if (!(g_tele.flags & TF_CALIBRADA)) {
    gfx->setTextColor(COL_AMAR);
    gfx->setCursor(4, 258);
    gfx->print("LINEA SIN CALIBRAR: boton CALIB");
  }

  // indicador de obstaculo (lectura instantanea del sensor frontal)
  bool obst = g_tele.flags & TF_OBSTACULO;
  gfx->fillRoundRect(4, 262, 130, 18, 4, obst ? COL_ALERTA : COL_BOTON);
  gfx->setTextSize(1);
  gfx->setTextColor(obst ? COL_TEXTO : COL_TENUE);
  gfx->setCursor(12, 267);
  gfx->print(obst ? "! OBSTACULO !" : "sin obstaculo");

  gfx->setTextSize(1);
  gfx->setTextColor(COL_TENUE);
  sprintf(buf, "t robot: %lu ms", (unsigned long)g_tele.ms);
  gfx->setCursor(144, 267); gfx->print(buf);
}

static void _dibujarAjustes() {
  gfx->fillRect(0, 0, 240, 290, COL_FONDO);

  // encabezado con paginador (botones grandes, navegación circular)
  _boton(0, 0, 62, 28, "<", COL_ACENTO);
  _boton(178, 0, 62, 28, ">", COL_ACENTO);
  gfx->setTextSize(2);
  gfx->setTextColor(g_paramsValidos ? COL_TEXTO : COL_ALERTA);
  gfx->setCursor(78, 7);
  char buf[24];
  sprintf(buf, "PAG %d/%d", _pagParam + 1, N_PAGINAS);
  gfx->print(buf);

  // filas de parámetros
  for (int f = 0; f < POR_PAGINA; f++) {
    int i = _pagParam * POR_PAGINA + f;
    if (i >= N_PARAMS) break;
    int y = 32 + f * 36;
    gfx->setTextSize(1);
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(4, y + 2);
    gfx->print(TABLA[i].nombre);
    gfx->setTextSize(2);
    gfx->setTextColor(COL_AMAR);
    _formatear(i, buf);
    gfx->setCursor(4, y + 14);
    gfx->print(buf);
    _boton(150, y + 2, 40, 30, "-", COL_TEXTO);
    _boton(196, y + 2, 40, 30, "+", COL_TEXTO);
  }

  // botones de accion
  _boton(0, 252, 76, 34, "START", COL_OK);
  _boton(80, 252, 76, 34, "STOP", COL_ALERTA);
  _boton(160, 252, 80, 34, "GRABA", COL_ACENTO);
}

void uiDibujar() {
  if (millis() - _tUltDibujo < T_REFRESCO_MS) return;
  _tUltDibujo = millis();
  if (_pagina == 0) _dibujarTele();
  else if (_pagina == 1) _dibujarAjustes();
  else _dibujarPrueba();
  _tabs();
  gfx->flush();
}

void uiInit() {
  gfx->fillScreen(COL_FONDO);
  _tUltDibujo = 0;
  uiDibujar();
}

static void _prMot(int16_t izq, int16_t der) {
  _prHold = true; _prIzq = izq; _prDer = der;
  enlaceEnviarTest(TEST_MOTOR, izq, der);
  _tUltMot = millis();
}

void uiSoltar() {
  if (_prHold) {
    _prHold = false;
    enlaceEnviarTest(TEST_MOTOR, 0, 0);
  }
}

void uiMantener() {
  if (_prHold && millis() - _tUltMot > 80) {
    enlaceEnviarTest(TEST_MOTOR, _prIzq, _prDer);
    _tUltMot = millis();
  }
}

void uiTeclado() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c == 'E') { enlaceEnviarTest(TEST_ENTER, 0, 0); enlaceEnviarCmd(CMD_STOP); }
    else if (c == 'Q') { uiSoltar(); enlaceEnviarTest(TEST_EXIT, 0, 0); }
    else if (c == '1') _prSub = 0;
    else if (c == '2') _prSub = 1;
    else if (c == '3') _prSub = 2;
    else if (c == '4') _prSub = 3;
    else if (c == 'C') enlaceEnviarTest(TEST_GYRO_CERO, 0, 0);
    else if (c == 'W') _prMot(_prVel, _prVel);
    else if (c == 'S') _prMot(-_prVel, -_prVel);
    else if (c == 'A') _prMot(-_prVel, _prVel);
    else if (c == 'D') _prMot(_prVel, -_prVel);
    else if (c == ' ') uiSoltar();
    else if (c == '+' || c == '=') _prVel = constrain(_prVel + 10, 40, 255);
    else if (c == '-' || c == '_') _prVel = constrain(_prVel - 10, 40, 255);
    _tUltDibujo = 0;
  }
}

void uiTocar(int16_t x, int16_t y) {
  if (millis() - _tUltToque < T_REPETIR_MS) return;
  _tUltToque = millis();

  if (y >= 290) {
    uint8_t n = (x < 80) ? 0 : (x < 160) ? 1 : 2;
    if (n != _pagina) {
      if (_pagina == 2) uiSoltar();
      _pagina = n;
    }
    _tUltDibujo = 0;
    return;
  }

  if (_pagina == 2) {
    if (y < 28) {
      if (x < 72) { enlaceEnviarTest(TEST_ENTER, 0, 0); enlaceEnviarCmd(CMD_STOP); }
      else if (x < 146) { uiSoltar(); enlaceEnviarTest(TEST_EXIT, 0, 0); }
      else enlaceEnviarTest(TEST_GYRO_CERO, 0, 0);
      return;
    }
    if (y < 56) {
      int i = x / 60;
      if (i >= 0 && i < 4) { if (_prSub == 1) uiSoltar(); _prSub = i; }
      _tUltDibujo = 0;
      return;
    }
    if (_prSub == 1) {
      if (y >= 58 && y < 90) {
        if (x >= 80 && x < 120) _prVel = constrain(_prVel + 10, 40, 255);
        if (x >= 130 && x < 170) _prVel = constrain(_prVel - 10, 40, 255);
      }
      else if (x >= 80 && x < 160 && y >= 100 && y < 140) _prMot(_prVel, _prVel);
      else if (x < 70 && y >= 150 && y < 190) _prMot(-_prVel, _prVel);
      else if (x >= 85 && x < 155 && y >= 150 && y < 190) uiSoltar();
      else if (x >= 170 && y >= 150 && y < 190) _prMot(_prVel, -_prVel);
      else if (x >= 80 && x < 160 && y >= 200 && y < 240) _prMot(-_prVel, -_prVel);
      else if (x < 55 && y >= 100 && y < 140) _prMot(_prVel, 0);
      else if (x >= 185 && y >= 100 && y < 140) _prMot(0, _prVel);
    }
    return;
  }

  if (_pagina != 1) return;

  if (y < 32) {
    if (x < 110)      _pagParam = (_pagParam + N_PAGINAS - 1) % N_PAGINAS;
    else if (x > 130) _pagParam = (_pagParam + 1) % N_PAGINAS;
    _tUltDibujo = 0;
    return;
  }
  if (y >= 252 && y < 288) {
    if (x < 78)        enlaceEnviarCmd(CMD_START);
    else if (x < 158)  enlaceEnviarCmd(CMD_STOP);
    else               enlaceEnviarCmd(CMD_GUARDAR);
    return;
  }
  if (y >= 32 && y < 32 + POR_PAGINA * 36) {
    int f = (y - 32) / 36;
    int i = _pagParam * POR_PAGINA + f;
    if (i >= N_PARAMS) return;
    if (x >= 150 && x < 192) {
      _escribirParam(i, _leerParam(i) - TABLA[i].paso);
      enlaceEnviarSet();
      _tUltDibujo = 0;
    } else if (x >= 196) {
      _escribirParam(i, _leerParam(i) + TABLA[i].paso);
      enlaceEnviarSet();
      _tUltDibujo = 0;
    }
  }
}
