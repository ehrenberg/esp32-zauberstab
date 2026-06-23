#pragma once

#include "config.h"
#include "Settings.h"

class LedController {
public:
  void begin(const Settings& settings);
  void apply(const Settings& settings);
  void clear(bool showNow = false);
  void show();
  void showIdle();
  void showSetup();
  void showPatternChange(uint8_t pattern);
  void showCalibrationProgress(int sample, int samples);
  void showCalibrationDone();
  void renderSetupBlink();
  void renderSetupRainbow();
  void renderErrorBlink();
  void showWifiConnected();
  CRGB* buffer();

private:
  CRGB leds[NUM_LEDS];
  unsigned long lastBlinkAt = 0;
  bool blinkOn = true;
  uint8_t setupHue = 0;
};
