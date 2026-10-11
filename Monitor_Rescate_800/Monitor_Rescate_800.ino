// ============================================================
// Monitor_Rescate_800.ino — Monitor 800x480
// Pestañas: TELEMETRIA | AJUSTES | PRUEBA (giro/motores/línea/dist)
//
// Teclado por Serial USB (115200) en pestaña PRUEBA:
//   E entrar  Q salir  1..4 submodo  C yaw=0
//   W/A/S/D movimiento  espacio freno  +/- PWM
// ============================================================

#include "config_monitor800.h"
#include "comun_espnow.h"
#include "lcd.h"
#include "tactil.h"
#include "enlace.h"
#include "interfaz.h"

void setup() {
  Serial.begin(115200);
  Serial.println(F("Monitor 800 — TELE | AJUSTES | PRUEBA"));
  Serial.println(F("Teclado PRUEBA: E Q 1-4 C W A S D espacio +/-"));

  if (!lcdInit()) Serial.println(F("ERROR: panel RGB no inicia (revisar PSRAM y modelo)"));
  tactilInit();

  if (enlaceInit()) Serial.println(F("ESP-NOW listo (canal 1)"));
  else              Serial.println(F("ERROR: fallo ESP-NOW"));

  uiInit();
}

void loop() {
  uiTeclado();

  int16_t x, y;
  if (tactilPunto(x, y)) {
    uiTocar(x, y);
    uiMantener();
  } else {
    uiSoltar();
  }
  uiDibujar();
}
