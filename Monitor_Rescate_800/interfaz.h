#pragma once
// ============================================================
// interfaz.h — DOS PANTALLAS con pestañas (800x480)
//
//  Pestaña TELEMETRIA: todos los datos del carro en tiempo real
//    a pantalla completa: estado gigante, barra de posición,
//    5 sensores grandes, obstáculo, giroscopio, láseres y PWM.
//  Pestaña AJUSTES: los 16 parámetros en 2 columnas x 8 filas
//    con botones [-]/[+] grandes (efecto INMEDIATO en el robot).
//  En AMBAS: START / STOP / GRABAR siempre visibles arriba.
//
// ANTI-PARPADEO: lo estático de cada pestaña se dibuja una vez
// al entrar; el refresco solo repinta las zonas de valores.
// API: uiInit(), uiTocar(x,y), uiDibujar()
// ============================================================
#include <Arduino.h>
#include "config_monitor800.h"
#include "comun_espnow.h"
#include "lcd.h"
#include "enlace.h"
#include "prueba_ui.h"

// ---------------- Tabla de parámetros ----------------
enum TipoParam : uint8_t { TP_U8, TP_F, TP_U16, TP_I8 };
struct DefParam {
  const char *nombre;
  TipoParam   tipo;
  void       *ptr;
  float       paso, minimo, maximo;
};

static DefParam TABLA[] = {
  {"Vel base",     TP_U8,  &g_params.velBase,      5, 40, 255},
  {"Vel max",      TP_U8,  &g_params.velMax,       5, 60, 255},
  {"Vel giro",     TP_U8,  &g_params.velGiro,      5, 60, 255},
  {"Vel subida",   TP_U8,  &g_params.velSubida,    5, 80, 255},
  {"Vel bajada",   TP_U8,  &g_params.velBajada,    5, 30, 200},
  {"Vel evasion",  TP_U8,  &g_params.velEvasion,   5, 60, 255},
  {"Vel recuper",  TP_U8,  &g_params.velRecuperar, 5, 50, 200},
  {"Vel cruce",    TP_U8,  &g_params.velCruce,     5, 60, 255},
  {"Kp",           TP_F,   &g_params.kp,       0.005f, 0, 1},
  {"Kd",           TP_F,   &g_params.kd,        0.05f, 0, 10},
  {"Dist obstac",  TP_U16, &g_params.distObstaculo, 10, 50, 500},
  {"t ev lateral", TP_U16, &g_params.tEvLateral,    50, 100, 3000},
  {"t ev paralelo",TP_U16, &g_params.tEvParalelo,   50, 100, 4000},
  {"t ev buscar",  TP_U16, &g_params.tEvBuscar,     50, 500, 6000},
  {"Trim izq %",   TP_I8,  &g_params.trimIzq,        1, -30, 30},
  {"Trim der %",   TP_I8,  &g_params.trimDer,        1, -30, 30},
};
static const int N_PARAMS = sizeof(TABLA) / sizeof(TABLA[0]);

// Geometría de la grilla de AJUSTES (2 columnas x 8 filas)
#define AJ_X0    10
#define AJ_COLW 395
#define AJ_Y0    48
#define AJ_ROWH  47

static uint8_t _pagina = 0;                 // 0=TELE, 1=AJUSTES
static unsigned long _tUltDibujo = 0, _tUltToque = 0;

// Cachés: repintar SOLO lo que cambió. Menos escritura a la PSRAM
// = menos competencia con el DMA del panel = imagen SIN temblor.
static PaqueteTele _prevTele;
static Parametros  _prevParams;
static bool _prevOk = false, _prevVer = false, _prevValidos = false;
static int  _prevPx = -1;
static uint16_t _prevLinea[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static bool _forzarRedibujo = true;

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

static void _boton(int x, int y, int w, int h, const char *txt,
                   uint16_t col, uint8_t tam = 2) {
  gfx->fillRoundRect(x, y, w, h, 6, COL_BOTON);
  gfx->drawRoundRect(x, y, w, h, 6, col);
  gfx->setTextColor(col);
  gfx->setTextSize(tam);
  int tw = strlen(txt) * 6 * tam;
  gfx->setCursor(x + (w - tw) / 2, y + (h - 8 * tam) / 2);
  gfx->print(txt);
}

// ---------- elementos comunes: cabecera y pestañas ----------
static void _cabeceraEstatica(const char *titulo) {
  gfx->setTextSize(2);
  gfx->setTextColor(COL_TENUE);
  gfx->setCursor(10, 12);
  gfx->print(titulo);
  _boton(500, 4, 90, 34, "START", COL_OK);
  _boton(596, 4, 90, 34, "STOP", COL_ALERTA);
  _boton(692, 4, 100, 34, "GRABAR", COL_ACENTO);
}

static void _tabsEstaticas() {
  _boton(0, 428, 266, 52, "TELEMETRIA", _pagina == 0 ? COL_ACENTO : COL_TENUE, 2);
  _boton(267, 428, 266, 52, "AJUSTES",   _pagina == 1 ? COL_ACENTO : COL_TENUE, 2);
  _boton(534, 428, 266, 52, "PRUEBA",    _pagina == 2 ? COL_ACENTO : COL_TENUE, 2);
}

static void _cabeceraDinamica() {
  bool ok = enlaceHayTele();
  if (!_forzarRedibujo && ok == _prevOk && g_versionErronea == _prevVer) return;
  _prevOk = ok; _prevVer = g_versionErronea;
  gfx->fillCircle(470, 20, 10, ok ? COL_OK : COL_ALERTA);
  gfx->fillRect(180, 12, 280, 16, COL_FONDO);
  gfx->setTextSize(1);
  if (g_versionErronea) {
    gfx->setTextColor(COL_ALERTA);
    gfx->setCursor(180, 16);
    gfx->print("PROTOCOLO DISTINTO: resubir ambos");
  } else if (!ok) {
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(180, 16);
    gfx->print("sin senal del robot...");
  }
}

// ================= PANTALLA TELEMETRIA =================
static void _teleEstatico() {
  gfx->fillScreen(COL_FONDO);
  _cabeceraEstatica("TELEMETRIA");
  _tabsEstaticas();

  // marco de la barra de posición de línea
  gfx->drawRect(20, 130, 760, 36, COL_TENUE);
  gfx->drawFastVLine(400, 130, 36, COL_TENUE);

  // aros de los 5 sensores
  for (int i = 0; i < 5; i++)
    gfx->drawCircle(120 + i * 140, 228, 26, COL_TENUE);
  gfx->setTextSize(1);
  gfx->setTextColor(COL_TENUE);
  gfx->setCursor(20, 169);
  gfx->print("ADC crudo 0-4095 por sensor (negro = valor alto)");

  // etiquetas de la fila inferior
  gfx->setTextSize(2);
  gfx->setTextColor(COL_TENUE);
  gfx->setCursor(20, 330);  gfx->print("giroscopio");
  gfx->setCursor(290, 330); gfx->print("laser (mm)");
  gfx->setCursor(530, 330); gfx->print("PWM motores");

  // marcos de barras PWM
  gfx->drawRect(530, 390, 120, 26, COL_TENUE);
  gfx->drawRect(660, 390, 120, 26, COL_TENUE);
  gfx->drawFastVLine(590, 390, 26, COL_TENUE);
  gfx->drawFastVLine(720, 390, 26, COL_TENUE);
}

static void _teleDinamico() {
  char buf[40];
  bool ok = enlaceHayTele();
  bool F = _forzarRedibujo;
  _cabeceraDinamica();

  // estado gigante
  if (F || g_tele.estado != _prevTele.estado || ok != _prevOk) {
  gfx->fillRect(20, 48, 460, 60, COL_FONDO);
  gfx->setTextSize(7);
  gfx->setTextColor(ok ? COL_AMAR : COL_TENUE);
  gfx->setCursor(20, 50);
  gfx->print(NOMBRES_ESTADO[g_tele.estado < 10 ? g_tele.estado : 0]);
  }

  // aguja de posición (repintar solo si se movió)
  int px = map(constrain(g_tele.posicion, -2000, 2000), -2000, 2000, 26, 774);
  if (F || px != _prevPx) {
    gfx->fillRect(21, 131, 758, 34, COL_FONDO);
    gfx->drawFastVLine(400, 130, 36, COL_TENUE);
    gfx->fillRect(px - 5, 134, 10, 28, COL_ACENTO);
    _prevPx = px;
  }

  // sensores
  if (F || g_tele.patron != _prevTele.patron) {
    for (int i = 0; i < 5; i++) {
      bool negro = g_tele.patron & (1 << i);
      gfx->fillCircle(120 + i * 140, 228, 22, negro ? COL_TEXTO : COL_FONDO);
    }
  }

  // valor ADC crudo de cada sensor (con umbral anti-jitter de 8 cuentas)
  for (int i = 0; i < 5; i++) {
    if (!F && abs((int)g_tele.linea[i] - (int)_prevLinea[i]) <= 8) continue;
    _prevLinea[i] = g_tele.linea[i];
    int cx = 120 + i * 140;
    gfx->fillRect(cx - 34, 180, 68, 18, COL_FONDO);
    gfx->setTextSize(2);
    gfx->setTextColor(COL_ACENTO);
    char b2[8];
    sprintf(b2, "%4d", g_tele.linea[i]);
    gfx->setCursor(cx - 24, 181);
    gfx->print(b2);
  }

  // obstáculo + aviso de calibración
  bool obst = g_tele.flags & TF_OBSTACULO;
  if (F || (g_tele.flags & TF_OBSTACULO) != (_prevTele.flags & TF_OBSTACULO)) {
  gfx->fillRoundRect(20, 272, 250, 42, 8, obst ? COL_ALERTA : COL_BOTON);
  gfx->setTextSize(2);
  gfx->setTextColor(obst ? COL_TEXTO : COL_TENUE);
  gfx->setCursor(40, 286);
  gfx->print(obst ? "! OBSTACULO !" : "sin obstaculo");
  }

  uint8_t saludMask = TF_CALIBRADA | TF_MPU_OK | TF_VL53F_OK | TF_VL53D_OK | TF_VL53I_OK;
  if (F || (g_tele.flags & saludMask) != (_prevTele.flags & saludMask) || ok != _prevOk) {
    gfx->fillRect(290, 276, 500, 40, COL_FONDO);
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(290, 278);
    char salud[40];
    sprintf(salud, "MPU:%s  VL53 F:%s D:%s I:%s",
            (g_tele.flags & TF_MPU_OK)   ? "OK" : "--",
            (g_tele.flags & TF_VL53F_OK) ? "OK" : "--",
            (g_tele.flags & TF_VL53D_OK) ? "OK" : "--",
            (g_tele.flags & TF_VL53I_OK) ? "OK" : "--");
    gfx->print(salud);
    if (ok && !(g_tele.flags & TF_CALIBRADA)) {
      gfx->setTextColor(COL_AMAR);
      gfx->setCursor(290, 300);
      gfx->print("LINEA SIN CALIBRAR (btn CALIB)");
    }
  }

  gfx->setTextColor(COL_TEXTO);

  // giroscopio (dos líneas)
  if (F || g_tele.yaw10 != _prevTele.yaw10 || g_tele.pitch10 != _prevTele.pitch10) {
  gfx->fillRect(20, 352, 250, 64, COL_FONDO);
  gfx->setTextSize(3);
  sprintf(buf, "Y%7.1f", g_tele.yaw10 / 10.0f);
  gfx->setCursor(20, 354); gfx->print(buf);
  sprintf(buf, "P%7.1f", g_tele.pitch10 / 10.0f);
  gfx->setCursor(20, 388); gfx->print(buf);
  }

  // láseres: "---" = sensor NO inicializado (desconectado/falla),
  //          "999" = sensor vivo midiendo fuera de rango (libre)
  uint8_t vlMask = TF_VL53F_OK | TF_VL53D_OK | TF_VL53I_OK;
  if (F || g_tele.dF != _prevTele.dF || g_tele.dD != _prevTele.dD ||
      g_tele.dI != _prevTele.dI ||
      (g_tele.flags & vlMask) != (_prevTele.flags & vlMask)) {
    gfx->setTextSize(3);
    gfx->setTextColor(COL_TEXTO);
    gfx->fillRect(290, 352, 230, 64, COL_FONDO);
    if (g_tele.flags & TF_VL53F_OK) sprintf(buf, "F %3d", min((int)g_tele.dF, 999));
    else                            strcpy(buf, "F ---");
    gfx->setCursor(290, 354); gfx->print(buf);
    gfx->setTextSize(2);
    char bi[10], bd[10];
    if (g_tele.flags & TF_VL53I_OK) sprintf(bi, "I %3d", min((int)g_tele.dI, 999));
    else                            strcpy(bi, "I ---");
    if (g_tele.flags & TF_VL53D_OK) sprintf(bd, "D %3d", min((int)g_tele.dD, 999));
    else                            strcpy(bd, "D ---");
    sprintf(buf, "%s  %s", bi, bd);
    gfx->setCursor(290, 394); gfx->print(buf);
  }

  // PWM: números y barras
  if (F || g_tele.pwmIzq != _prevTele.pwmIzq || g_tele.pwmDer != _prevTele.pwmDer) {
  gfx->setTextColor(COL_TEXTO);
  gfx->fillRect(530, 352, 250, 34, COL_FONDO);
  gfx->setTextSize(2);
  sprintf(buf, "I%5d  D%5d", g_tele.pwmIzq, g_tele.pwmDer);
  gfx->setCursor(530, 358); gfx->print(buf);

  gfx->fillRect(531, 391, 118, 24, COL_FONDO);
  gfx->fillRect(661, 391, 118, 24, COL_FONDO);
  gfx->drawFastVLine(590, 390, 26, COL_TENUE);
  gfx->drawFastVLine(720, 390, 26, COL_TENUE);
  int bi = map(constrain(g_tele.pwmIzq, -255, 255), -255, 255, -58, 58);
  int bd = map(constrain(g_tele.pwmDer, -255, 255), -255, 255, -58, 58);
  if (bi >= 0) gfx->fillRect(590, 394, bi, 18, COL_OK);
  else         gfx->fillRect(590 + bi, 394, -bi, 18, COL_ALERTA);
  if (bd >= 0) gfx->fillRect(720, 394, bd, 18, COL_OK);
  else         gfx->fillRect(720 + bd, 394, -bd, 18, COL_ALERTA);
  }

  _prevTele = g_tele;
  _forzarRedibujo = false;
}

// ================= PANTALLA AJUSTES =================
static void _ajustesEstatico() {
  gfx->fillScreen(COL_FONDO);
  _cabeceraEstatica("AJUSTES");
  _tabsEstaticas();

  for (int i = 0; i < N_PARAMS; i++) {
    int x0 = AJ_X0 + (i / 8) * AJ_COLW;
    int y0 = AJ_Y0 + (i % 8) * AJ_ROWH;
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(x0, y0 + 12);
    gfx->print(TABLA[i].nombre);
    _boton(x0 + 252, y0 + 4, 60, 38, "-", COL_TEXTO, 3);
    _boton(x0 + 320, y0 + 4, 60, 38, "+", COL_TEXTO, 3);
  }
}

static void _ajustesDinamico() {
  char buf[20];
  _cabeceraDinamica();
  if (!_forzarRedibujo &&
      memcmp(&g_params, &_prevParams, sizeof(Parametros)) == 0 &&
      g_paramsValidos == _prevValidos) return;
  _prevParams = g_params;
  _prevValidos = g_paramsValidos;
  _forzarRedibujo = false;
  for (int i = 0; i < N_PARAMS; i++) {
    int x0 = AJ_X0 + (i / 8) * AJ_COLW;
    int y0 = AJ_Y0 + (i % 8) * AJ_ROWH;
    gfx->fillRect(x0 + 148, y0 + 6, 100, 30, COL_FONDO);
    gfx->setTextSize(3);
    gfx->setTextColor(g_paramsValidos ? COL_AMAR : COL_TENUE);
    _formatear(i, buf);
    gfx->setCursor(x0 + 148, y0 + 10);
    gfx->print(buf);
  }
}

// ================= API =================
void uiInit() {
  _pagina = 0;
  _forzarRedibujo = true;
  _prevPx = -1;
  _teleEstatico();
  _tUltDibujo = 0;
}

void uiDibujar() {
  if (millis() - _tUltDibujo < T_REFRESCO_MS) return;
  _tUltDibujo = millis();
  if (_pagina == 0) _teleDinamico();
  else if (_pagina == 1) _ajustesDinamico();
  else {
    pruebaUiDinamico();
    // redibuja pestañas por si el fill las tapó
  }
}

void uiTocar(int16_t x, int16_t y) {
  if (millis() - _tUltToque < T_REPETIR_MS) return;
  _tUltToque = millis();
  Serial.printf("toque x=%d y=%d\n", x, y);

  // pestañas
  if (y >= 428) {
    uint8_t nueva = (x < 267) ? 0 : (x < 534) ? 1 : 2;
    if (nueva != _pagina) {
      if (_pagina == 2) pruebaUiSoltar();  // frena si había motor en hold
      _pagina = nueva;
      _forzarRedibujo = true;
      _prevPx = -1;
      if (_pagina == 0) _teleEstatico();
      else if (_pagina == 1) _ajustesEstatico();
      else pruebaUiEstatico();
      _tUltDibujo = 0;
    }
    return;
  }

  if (_pagina == 2) {
    pruebaUiTocar(x, y);
    return;
  }

  // botones de cabecera (TELE / AJUSTES)
  if (y < 42 && x >= 500) {
    if (x < 594)       enlaceEnviarCmd(CMD_START);
    else if (x < 690)  enlaceEnviarCmd(CMD_STOP);
    else               enlaceEnviarCmd(CMD_GUARDAR);
    return;
  }

  if (_pagina == 1 && y >= AJ_Y0 && y < AJ_Y0 + 8 * AJ_ROWH) {
    int col = (x - AJ_X0) / AJ_COLW;
    if (col < 0 || col > 1) return;
    int fila = (y - AJ_Y0) / AJ_ROWH;
    int i = col * 8 + fila;
    if (i >= N_PARAMS) return;
    int rx = x - (AJ_X0 + col * AJ_COLW);
    if (rx >= 248 && rx < 316) {
      _escribirParam(i, _leerParam(i) - TABLA[i].paso);
      enlaceEnviarSet();
    } else if (rx >= 316 && rx < 384) {
      _escribirParam(i, _leerParam(i) + TABLA[i].paso);
      enlaceEnviarSet();
    }
  }
}

void uiSoltar() { pruebaUiSoltar(); }
void uiTeclado() { pruebaUiTeclado(); }
void uiMantener() { if (_pagina == 2) pruebaUiMantener(); }
