#pragma once
// ============================================================
// prueba_ui.h — Pantalla de diagnóstico por módulos (800x480)
// Submodos: GIRO | MOTORES | LINEA | DIST
// Teclado USB/Serial (monitor abierto a 115200):
//   E entrar prueba   Q salir
//   1 giro  2 motores  3 linea  4 distancia
//   C yaw=0
//   W avanzar  S retroceder  A izq  D der  espacio freno
//   + / - velocidad PWM de prueba
// ============================================================
#include <Arduino.h>
#include "config_monitor800.h"
#include "comun_espnow.h"
#include "lcd.h"
#include "enlace.h"

enum : uint8_t { PR_GIRO = 0, PR_MOT = 1, PR_LIN = 2, PR_DIST = 3 };

static uint8_t _prSub = PR_GIRO;
static int16_t _prVel = 120;
static bool _prActivoUI = false;   // pedimos entrar/salir
static bool _prMotorHold = false;
static int16_t _prHoldIzq = 0, _prHoldDer = 0;
static unsigned long _tUltMotorCmd = 0;

static void _prEnviarMotor(int16_t izq, int16_t der) {
  enlaceEnviarTest(TEST_MOTOR, izq, der);
  _tUltMotorCmd = millis();
}

static void _prFreno() {
  _prMotorHold = false;
  _prHoldIzq = _prHoldDer = 0;
  _prEnviarMotor(0, 0);
}

void pruebaUiEntrar() {
  enlaceEnviarTest(TEST_ENTER, 0, 0);
  enlaceEnviarCmd(CMD_STOP);
  _prActivoUI = true;
  _prFreno();
}

void pruebaUiSalir() {
  _prFreno();
  enlaceEnviarTest(TEST_EXIT, 0, 0);
  _prActivoUI = false;
}

void pruebaUiSoltar() {
  if (_prSub == PR_MOT && _prMotorHold) _prFreno();
}

// Reenvía PWM mientras se mantiene el dedo / tecla
void pruebaUiMantener() {
  if (_prSub == PR_MOT && _prMotorHold &&
      (millis() - _tUltMotorCmd) > 80) {
    _prEnviarMotor(_prHoldIzq, _prHoldDer);
  }
}

static void _prBoton(int x, int y, int w, int h, const char *t,
                     uint16_t col, uint8_t tam = 2) {
  gfx->fillRoundRect(x, y, w, h, 6, COL_BOTON);
  gfx->drawRoundRect(x, y, w, h, 6, col);
  gfx->setTextColor(col);
  gfx->setTextSize(tam);
  int tw = (int)strlen(t) * 6 * tam;
  gfx->setCursor(x + (w - tw) / 2, y + (h - 8 * tam) / 2);
  gfx->print(t);
}

void pruebaUiEstatico() {
  gfx->fillScreen(COL_FONDO);
  gfx->setTextSize(2);
  gfx->setTextColor(COL_TENUE);
  gfx->setCursor(10, 12);
  gfx->print("MODO PRUEBA");

  _prBoton(500, 4, 90, 34, "ENTRAR", COL_OK);
  _prBoton(596, 4, 90, 34, "SALIR", COL_ALERTA);
  _prBoton(692, 4, 100, 34, "CERO Y", COL_ACENTO);

  // submodos
  const char *noms[] = {"GIRO", "MOTORES", "LINEA", "DIST"};
  for (int i = 0; i < 4; i++) {
    uint16_t c = (_prSub == i) ? COL_ACENTO : COL_TENUE;
    _prBoton(10 + i * 195, 48, 185, 40, noms[i], c, 2);
  }

  // pestañas principales (mismo layout que el resto)
  _prBoton(0, 428, 266, 52, "TELEMETRIA", COL_TENUE, 2);
  _prBoton(267, 428, 266, 52, "AJUSTES", COL_TENUE, 2);
  _prBoton(534, 428, 266, 52, "PRUEBA", COL_ACENTO, 2);
}

static void _prCabeceraEstado() {
  bool ok = enlaceHayTele();
  bool enPrueba = g_tele.flags & TF_MODO_PRUEBA;
  gfx->fillCircle(470, 20, 10, ok ? (enPrueba ? COL_AMAR : COL_OK) : COL_ALERTA);
  gfx->fillRect(180, 10, 300, 20, COL_FONDO);
  gfx->setTextSize(1);
  gfx->setTextColor(enPrueba ? COL_AMAR : COL_TENUE);
  gfx->setCursor(180, 14);
  if (!ok) gfx->print("sin senal del robot...");
  else if (enPrueba) gfx->print("ROBOT EN MODO PRUEBA");
  else gfx->print("pulse ENTRAR para activar prueba");
}

void pruebaUiDinamico() {
  char buf[48];
  _prCabeceraEstado();

  // zona de contenido
  gfx->fillRect(0, 95, 800, 325, COL_FONDO);

  if (_prSub == PR_GIRO) {
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(20, 110);
    gfx->print("Gire el carro a mano. Pulse CERO Y para yaw=0.");
    float yaw = g_tele.yaw10 / 10.0f;
    float pit = g_tele.pitch10 / 10.0f;
    gfx->setTextSize(8);
    gfx->setTextColor(COL_AMAR);
    sprintf(buf, "%6.1f", yaw);
    gfx->setCursor(40, 180);
    gfx->print(buf);
    gfx->setTextSize(3);
    gfx->setTextColor(COL_TEXTO);
    gfx->setCursor(40, 310);
    sprintf(buf, "yaw grados   pitch %.1f", pit);
    gfx->print(buf);
    gfx->setTextSize(2);
    gfx->setTextColor((g_tele.flags & TF_MPU_OK) ? COL_OK : COL_ALERTA);
    gfx->setCursor(40, 360);
    gfx->print((g_tele.flags & TF_MPU_OK) ? "MPU OK" : "MPU NO DETECTADO");
  }
  else if (_prSub == PR_MOT) {
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(20, 105);
    sprintf(buf, "PWM prueba: %d   (teclas W A S D + - espacio)", _prVel);
    gfx->print(buf);

    // pad:   [AVAN]
    //     [IZQ][FRE][DER]
    //        [RET]
    _prBoton(310, 140, 180, 70, "AVANZAR", COL_OK, 3);
    _prBoton(120, 230, 160, 70, "IZQ", COL_ACENTO, 3);
    _prBoton(310, 230, 180, 70, "FRENO", COL_ALERTA, 3);
    _prBoton(520, 230, 160, 70, "DER", COL_ACENTO, 3);
    _prBoton(310, 320, 180, 70, "RETRO", COL_AMAR, 3);

    // motores por separado
    _prBoton(20, 140, 90, 55, "I+", COL_TEXTO, 2);
    _prBoton(20, 205, 90, 55, "I-", COL_TEXTO, 2);
    _prBoton(690, 140, 90, 55, "D+", COL_TEXTO, 2);
    _prBoton(690, 205, 90, 55, "D-", COL_TEXTO, 2);

    gfx->setTextSize(2);
    gfx->setTextColor(COL_TEXTO);
    sprintf(buf, "PWM real I:%d  D:%d", g_tele.pwmIzq, g_tele.pwmDer);
    gfx->setCursor(200, 400);
    gfx->print(buf);
  }
  else if (_prSub == PR_LIN) {
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(20, 110);
    gfx->print("Pase el carro a mano sobre la linea");

    int n = 0;
    for (int i = 0; i < 5; i++) {
      bool negro = g_tele.patron & (1 << i);
      if (negro) n++;
      int cx = 100 + i * 140;
      gfx->fillCircle(cx, 220, 36, negro ? COL_TEXTO : COL_FONDO);
      gfx->drawCircle(cx, 220, 36, COL_TENUE);
      gfx->setTextSize(2);
      gfx->setTextColor(COL_ACENTO);
      sprintf(buf, "%d", g_tele.linea[i]);
      gfx->setCursor(cx - 24, 270);
      gfx->print(buf);
      gfx->setTextSize(1);
      gfx->setTextColor(COL_TENUE);
      sprintf(buf, "S%d", i);
      gfx->setCursor(cx - 8, 175);
      gfx->print(buf);
    }

    gfx->setTextSize(3);
    gfx->setTextColor(COL_AMAR);
    sprintf(buf, "posicion %d    en negro: %d", g_tele.posicion, n);
    gfx->setCursor(40, 340);
    gfx->print(buf);
  }
  else if (_prSub == PR_DIST) {
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TENUE);
    gfx->setCursor(20, 110);
    gfx->print("Acerque un obstaculo a cada sensor por separado");

    for (int k = 0; k < 3; k++) {
      int x = 20 + k * 260;
      const char *tit = (k == 0) ? "IZQUIERDO" : (k == 1) ? "FRONTAL" : "DERECHO";
      uint16_t mm = (k == 0) ? g_tele.dI : (k == 1) ? g_tele.dF : g_tele.dD;
      bool ok = (k == 0) ? (g_tele.flags & TF_VL53I_OK)
              : (k == 1) ? (g_tele.flags & TF_VL53F_OK)
                         : (g_tele.flags & TF_VL53D_OK);
      gfx->fillRoundRect(x, 160, 240, 200, 10, COL_BOTON);
      gfx->setTextSize(2);
      gfx->setTextColor(COL_TENUE);
      gfx->setCursor(x + 20, 180);
      gfx->print(tit);
      gfx->setTextSize(5);
      gfx->setTextColor(ok ? COL_AMAR : COL_ALERTA);
      if (!ok) {
        gfx->setCursor(x + 50, 250);
        gfx->print("---");
      } else if (mm >= 2000) {
        gfx->setCursor(x + 40, 250);
        gfx->print("libre");
      } else {
        sprintf(buf, "%4d", mm);
        gfx->setCursor(x + 30, 250);
        gfx->print(buf);
        gfx->setTextSize(2);
        gfx->setTextColor(COL_TENUE);
        gfx->setCursor(x + 90, 320);
        gfx->print("mm");
      }
    }
  }
}

bool pruebaUiTocar(int16_t x, int16_t y) {
  // cabecera ENTRAR / SALIR / CERO
  if (y < 42 && x >= 500) {
    if (x < 594) pruebaUiEntrar();
    else if (x < 690) pruebaUiSalir();
    else enlaceEnviarTest(TEST_GYRO_CERO, 0, 0);
    return true;
  }

  // submodos
  if (y >= 48 && y < 92) {
    int i = (x - 10) / 195;
    if (i >= 0 && i < 4) {
      if (_prSub == PR_MOT) _prFreno();
      _prSub = (uint8_t)i;
      pruebaUiEstatico();
    }
    return true;
  }

  if (_prSub != PR_MOT) return false;

  auto hold = [&](int16_t izq, int16_t der) {
    _prMotorHold = true;
    _prHoldIzq = izq;
    _prHoldDer = der;
    _prEnviarMotor(izq, der);
  };

  // pad
  if (x >= 310 && x < 490 && y >= 140 && y < 210) hold(_prVel, _prVel);
  else if (x >= 120 && x < 280 && y >= 230 && y < 300) hold(-_prVel, _prVel);
  else if (x >= 310 && x < 490 && y >= 230 && y < 300) _prFreno();
  else if (x >= 520 && x < 680 && y >= 230 && y < 300) hold(_prVel, -_prVel);
  else if (x >= 310 && x < 490 && y >= 320 && y < 390) hold(-_prVel, -_prVel);
  // solo izquierdo / derecho
  else if (x >= 20 && x < 110 && y >= 140 && y < 195) hold(_prVel, 0);
  else if (x >= 20 && x < 110 && y >= 205 && y < 260) hold(-_prVel, 0);
  else if (x >= 690 && x < 780 && y >= 140 && y < 195) hold(0, _prVel);
  else if (x >= 690 && x < 780 && y >= 205 && y < 260) hold(0, -_prVel);
  else return false;
  return true;
}

void pruebaUiTeclado() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c >= 'a' && c <= 'z') c -= 32;

    switch (c) {
      case 'E': pruebaUiEntrar(); break;
      case 'Q': pruebaUiSalir(); break;
      case '1': _prSub = PR_GIRO; pruebaUiEstatico(); break;
      case '2': _prSub = PR_MOT;  pruebaUiEstatico(); break;
      case '3': _prSub = PR_LIN;  pruebaUiEstatico(); break;
      case '4': _prSub = PR_DIST; pruebaUiEstatico(); break;
      case 'C': enlaceEnviarTest(TEST_GYRO_CERO, 0, 0); break;
      case '+': case '=':
        _prVel = constrain(_prVel + 10, 40, 255); break;
      case '-': case '_':
        _prVel = constrain(_prVel - 10, 40, 255); break;
      case 'W':
        _prMotorHold = true; _prHoldIzq = _prVel; _prHoldDer = _prVel;
        _prEnviarMotor(_prVel, _prVel); break;
      case 'S':
        _prMotorHold = true; _prHoldIzq = -_prVel; _prHoldDer = -_prVel;
        _prEnviarMotor(-_prVel, -_prVel); break;
      case 'A':
        _prMotorHold = true; _prHoldIzq = -_prVel; _prHoldDer = _prVel;
        _prEnviarMotor(-_prVel, _prVel); break;
      case 'D':
        _prMotorHold = true; _prHoldIzq = _prVel; _prHoldDer = -_prVel;
        _prEnviarMotor(_prVel, -_prVel); break;
      case ' ':
        _prFreno(); break;
    }
  }
}
