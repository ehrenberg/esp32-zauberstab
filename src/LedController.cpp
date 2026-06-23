#include "LedController.h"

void LedController::begin(const Settings& settings) {
  // SK9822 = getakteter SPI-Strip (DI + CI). BGR ist die uebliche Kanalordnung
  // der SK9822/APA102-Familie; falls Rot/Blau vertauscht erscheinen, hier auf
  // RGB bzw. GRB aendern. Der hohe SPI-Takt ist der eigentliche Performance-
  // Gewinn gegenueber dem alten WS2812B.
  FastLED.addLeds<SK9822, LED_DATA_PIN, LED_CLOCK_PIN, BGR, DATA_RATE_MHZ(LED_DATA_RATE_MHZ)>(leds, NUM_LEDS);
  // POV: temporales Dithering aus -> sonst flackern die Spalten ungleich hell,
  // weil jeder Frame eine andere Winkelposition trifft.
  FastLED.setDither(DISABLE_DITHER);
  apply(settings);
  clear(true);
}

void LedController::apply(const Settings& settings) {
  FastLED.setBrightness(settings.brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, settings.currentLimitMa);
}

void LedController::clear(bool showNow) {
  FastLED.clear();
  if (showNow) FastLED.show();
}

void LedController::show() {
  FastLED.show();
}

void LedController::showIdle() {
  clear(false);

  leds[0] = CRGB::Blue;
  leds[1] = CRGB::Blue;
  leds[2] = CRGB::Blue;
  FastLED.show();
}

void LedController::showSetup() {
  fill_solid(leds, NUM_LEDS, CRGB::Purple);
  FastLED.show();
}

void LedController::showPatternChange(uint8_t pattern) {
  // Kurze Quittung: pattern+1 gruene LEDs vom Anfang.
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  uint8_t count = pattern + 1;
  if (count > NUM_LEDS) count = NUM_LEDS;
  for (uint8_t i = 0; i < count; i++) leds[i] = CRGB(0, 180, 0);
  FastLED.show();
  delay(180);
  clear(true);
}

void LedController::showCalibrationProgress(int sample, int samples) {
  // Eleganter Komet mit Schweif laeuft ueber den Stab, Farbe wandert mit dem
  // Fortschritt von Violett -> Cyan -> Gruen. Nur wenige LEDs an -> wenig Strom.
  const float p = samples > 1 ? static_cast<float>(sample) / (samples - 1) : 1.0f;
  const int head = static_cast<int>(p * (NUM_LEDS - 1));
  const uint8_t hue = static_cast<uint8_t>(192 - p * 96);  // 192=violett .. 96=gruen

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int t = 0; t < 8; t++) {
    const int idx = head - t;
    if (idx < 0) break;
    const uint8_t v = static_cast<uint8_t>(220 - t * 26);  // Schweif faded aus
    leds[idx] = CHSV(hue, 230, v);
  }
  FastLED.show();
}

void LedController::showCalibrationDone() {
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(300);
  clear(true);
}

void LedController::renderSetupBlink() {
  unsigned long now = millis();
  if (now - lastBlinkAt < 500) return;
  lastBlinkAt = now;
  blinkOn = !blinkOn;
  fill_solid(leds, NUM_LEDS, blinkOn ? CRGB::Purple : CRGB::Black);
  FastLED.show();
}

void LedController::renderSetupRainbow() {
  // Fliessender Regenbogen ueber den ganzen Streifen (~30 fps).
  unsigned long now = millis();
  if (now - lastBlinkAt < 30) return;
  lastBlinkAt = now;
  setupHue += 2;
  fill_rainbow(leds, NUM_LEDS, setupHue, 6);
  FastLED.show();
}

void LedController::showWifiConnected() {
  // Quittung wenn sich ein Geraet mit dem AP verbindet: 2x kurz cyan blinken.
  for (uint8_t k = 0; k < 2; k++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, 150, 250));
    FastLED.show();
    delay(90);
    FastLED.clear();
    FastLED.show();
    delay(90);
  }
}

void LedController::renderErrorBlink() {
  unsigned long now = millis();
  if (now - lastBlinkAt < 250) return;
  lastBlinkAt = now;
  blinkOn = !blinkOn;
  fill_solid(leds, NUM_LEDS, blinkOn ? CRGB::Red : CRGB::Black);
  FastLED.show();
}

CRGB* LedController::buffer() {
  return leds;
}
