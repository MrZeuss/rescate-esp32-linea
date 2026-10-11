// ============================================================
// Monitor_Rescate.ino — Waveshare ESP32-S3-Touch-LCD-2
// Pestañas: TELE | AJUS | PRUE (giro / motores / línea / dist)
// Teclado Serial: E Q 1-4 C W A S D espacio +/-
// ============================================================

#include "config_monitor.h"
#include "comun_espnow.h"
#include "lcd.h"
#include "tactil.h"
#include "enlace.h"
#include "interfaz.h"

void setup() {
  Serial.begin(115200);
  Serial.println(F("Monitor 2\" — TELE | AJUS | PRUE"));

  if (!lcdInit()) Serial.println(F("ERROR: LCD no responde"));
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
