#include "PovRenderer.h"
#include "Patterns.h"
#include <math.h>

PovRenderer::PovRenderer(Settings& settings, LedController& leds, MotionSensor& sensor)
  : settings(settings), ledController(leds), sensor(sensor) {}

void PovRenderer::reset() {
  // SK9822 schiebt 65 LEDs in ~100 us heraus (WS2812B brauchte ~2200 us).
  // Konservativer Startwert, den der gemessene Tiefpass schnell nachzieht.
  frameTimeUs = 200.0f;
  effectiveCols = settings.povColumns;
  currentColumn = 0xFFFF;
  lastFrameBlack = false;
  lastShowAt = 0;
  outputCounter = 0;
  outputRate = 0;
  lastOutputRateAt = millis();
  sensor.resetAngle();
}

void PovRenderer::render() {
  const uint32_t now = micros();
  CRGB* leds = ledController.buffer();

  if (!sensor.isRotating()) {
    currentColumn = 0xFFFF;
    fadeToBlackBy(leds, NUM_LEDS, 200);
    if (now - lastShowAt >= settings.maxColumnHoldUs) {
      lastShowAt = now;
      ledController.show();
    }
    return;
  }

  const float speed = sensor.speedRad();
  const float angle = sensor.angle();

  // Maximal darstellbare Winkelschritte pro Umdrehung bei aktueller Drehzahl
  // (1 show() pro Schritt). Speist sowohl Vollkreis als auch Bild-Modus.
  const float revPeriodUs = (speed > 0.01f) ? (TWO_PI_F / speed) * 1e6f : 1e9f;
  const uint16_t maxCols = static_cast<uint16_t>(revPeriodUs / frameTimeUs);

  // Positioniertes Bild nur fuer eingebaute Muster (Custom/Text bleiben Vollkreis).
  const bool positioned = settings.imageMode && settings.patternMode == PATTERN_MODE_BUILTIN;

  // Winkelaufloesung: Vollkreis nutzt povColumns; das positionierte Bild rendert
  // am ECHTEN Winkel mit so vielen feinen Schritten, wie Drehzahl & SK9822
  // zulassen -> scharfes kleines Bild statt grober Spalten.
  uint16_t gateSteps;
  if (positioned) {
    gateSteps = maxCols;
    if (gateSteps < 32) gateSteps = 32;
    if (gateSteps > POV_MAX_FINE_STEPS) gateSteps = POV_MAX_FINE_STEPS;
  } else {
    gateSteps = settings.povColumns;
    if (maxCols < gateSteps) gateSteps = maxCols < 4 ? 4 : maxCols;
  }
  effectiveCols = gateSteps > 255 ? 255 : static_cast<uint8_t>(gateSteps);

  // ---- Bild-Modus: ausserhalb des Bildfensters nur schwarz, kein Rechenaufwand ----
  if (positioned) {
    const float imgAng = settings.imageAngleDeg * (TWO_PI_F / 360.0f);
    float d = angle - imgAng;
    while (d > PI_F) d -= TWO_PI_F;
    while (d < -PI_F) d += TWO_PI_F;

    const float rr = settings.imageRadius * 0.01f;
    const float k = settings.imageScale * 0.01f;
    // Winkelhalbbreite, unter der das Bild ueberhaupt LEDs treffen kann.
    const float windowHalf = (rr <= k) ? PI_F : (asinf(k / rr) + 0.25f);

    if (fabsf(d) > windowHalf) {
      currentColumn = 0xFFFF;
      const bool forced = now - lastShowAt >= settings.maxColumnHoldUs;
      if (!lastFrameBlack || forced) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        ledController.show();
        lastShowAt = now;
        lastFrameBlack = true;
      }
      return;
    }
  }

  // ---- Winkel quantisieren (Trigger fuer Neuzeichnen) ----
  uint16_t nextColumn = static_cast<uint16_t>((angle * gateSteps) / TWO_PI_F);
  if (nextColumn >= gateSteps) nextColumn = gateSteps - 1;

  const bool columnChanged = nextColumn != currentColumn;
  const bool forced = now - lastShowAt >= settings.maxColumnHoldUs;
  if (!columnChanged && !forced) return;

  currentColumn = nextColumn;
  lastShowAt = now;
  lastFrameBlack = false;

  if (settings.motionBlur > 0) fadeToBlackBy(leds, NUM_LEDS, settings.motionBlur);
  else fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Spaltenindex fuer Custom-/Text-Bitmaps (die einen Index, keinen Winkel nutzen).
  const uint8_t custColumns = settings.povColumns;
  uint8_t custCol = static_cast<uint8_t>((static_cast<uint32_t>(nextColumn) * custColumns) / gateSteps);
  if (custCol >= custColumns) custCol = custColumns - 1;

  // Winkelbreite: Bild um +/- persistence feine Schritte verdicken (Max-Blend).
  const uint8_t persistence = settings.angularPersistence;
  const uint8_t halfP = persistence >> 1;
  const float angStep = TWO_PI_F / gateSteps;

  for (uint8_t p = 0; p < persistence; p++) {
    const float th = angle + (static_cast<int>(p) - static_cast<int>(halfP)) * angStep;
    Patterns::render(scratch, settings, th, custCol, custColumns);
    for (uint16_t j = 0; j < NUM_LEDS; j++) {
      if (scratch[j].r > leds[j].r) leds[j].r = scratch[j].r;
      if (scratch[j].g > leds[j].g) leds[j].g = scratch[j].g;
      if (scratch[j].b > leds[j].b) leds[j].b = scratch[j].b;
    }
  }

  // Echte show()-Dauer messen -> speist adaptive Schrittzahl.
  const uint32_t showStart = micros();
  ledController.show();
  const uint32_t showDur = micros() - showStart;
  frameTimeUs += (static_cast<float>(showDur) - frameTimeUs) * 0.1f;
  outputCounter++;

  const uint32_t rateNow = millis();
  const uint32_t elapsed = rateNow - lastOutputRateAt;
  if (elapsed >= 1000) {
    outputRate = outputCounter * 1000UL / elapsed;
    outputCounter = 0;
    lastOutputRateAt = rateNow;
  }
}

bool PovRenderer::isRotationActive() const { return sensor.isRotating(); }
uint8_t PovRenderer::column() const { return static_cast<uint8_t>(currentColumn); }
float PovRenderer::rpm() const { return sensor.rpm(); }
uint32_t PovRenderer::outputsPerSecond() const { return outputRate; }
uint8_t PovRenderer::effectiveColumns() const { return effectiveCols; }
bool PovRenderer::isLocked() const { return sensor.isLocked(); }
float PovRenderer::sampleHz() const { return sensor.sampleRateHz(); }
uint32_t PovRenderer::i2cFails() const { return sensor.failedReads(); }
uint32_t PovRenderer::lockEvents() const { return sensor.lockEvents(); }
uint32_t PovRenderer::rejects() const { return sensor.rejects(); }
float PovRenderer::errAvgDeg() const { return sensor.errAvgDeg(); }
float PovRenderer::errMaxDeg() const { return sensor.errMaxDeg(); }
float PovRenderer::rpmMaxSession() const { return sensor.rpmMaxSession(); }
float PovRenderer::hzMinSession() const { return sensor.hzMinSession(); }
