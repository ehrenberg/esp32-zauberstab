#pragma once

#include "config.h"
#include "Settings.h"
#include "LedController.h"
#include "MotionSensor.h"

// PovRenderer liest nur noch den fertig integrierten Winkel aus dem
// MotionSensor (laeuft im eigenen Task) und kuemmert sich um Spaltenwahl,
// Mustererzeugung und LED-Ausgabe.
class PovRenderer {
public:
  PovRenderer(Settings& settings, LedController& leds, MotionSensor& sensor);
  void reset();
  void render();
  bool isRotationActive() const;
  uint8_t column() const;
  float rpm() const;
  uint32_t outputsPerSecond() const;
  uint8_t effectiveColumns() const;
  bool isLocked() const;
  // Telemetrie-Durchreichen fuer den DEV-Tab.
  float sampleHz() const;
  uint32_t i2cFails() const;
  uint32_t lockEvents() const;
  uint32_t rejects() const;
  float errAvgDeg() const;
  float errMaxDeg() const;
  float rpmMaxSession() const;
  float hzMinSession() const;
  void requestCalibration();
  bool isCalibrating() const;

private:
  Settings& settings;
  LedController& ledController;
  MotionSensor& sensor;

  float frameTimeUs = 200.0f;  // gemessene Dauer eines FastLED.show() (SK9822: ~100 us)
  uint8_t effectiveCols = 16;
  uint16_t currentColumn = 0xFFFF;
  bool lastFrameBlack = false;
  uint32_t lastShowAt = 0;
  uint32_t outputCounter = 0;
  uint32_t outputRate = 0;
  uint32_t lastOutputRateAt = 0;
  // Animationszeit, einmal pro Umdrehung uebernommen. Wuerde jede Spalte
  // millis() frisch lesen, liefe die Animation waehrend einer Umdrehung weiter
  // und das Bild erschiene in sich verdreht statt als bewegtes Ganzes.
  uint32_t animMs = 0;
  CRGB scratch[NUM_LEDS];
};
