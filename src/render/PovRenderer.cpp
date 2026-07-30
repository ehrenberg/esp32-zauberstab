#include "PovRenderer.h"
#include "Patterns.h"
#include "WandPatterns.h"
#include <math.h>

PovRenderer::PovRenderer(Settings& settings, LedController& leds, MotionSensor& sensor)
  : settings(settings), ledController(leds), sensor(sensor) {}

void PovRenderer::reset() {
  // Startwert fuer die Frame-Dauer (Muster rechnen + show). Der Tiefpass zieht
  // ihn nach den ersten Frames auf den echten Wert. Bewusst eher zu hoch als zu
  // niedrig: zu niedrig plant der Renderer Spalten ein, die er nie schafft.
  frameTimeUs = 800.0f;
  gateValid = false;
  lastRawAngle = 0.0f;
  covValid = false;
  covMin = covMax = covWindow = 0.0f;
  effectiveCols = settings.povColumns;
  currentColumn = 0xFFFF;
  lastFrameBlack = false;
  lastShowAt = 0;
  outputCounter = 0;
  outputRate = 0;
  lastOutputRateAt = millis();
  animMs = millis();
  sensor.resetAngle();
}

void PovRenderer::render() {
  // Stab-Modus: der Stab wird nicht gedreht. Kein Winkel, keine Spaltenlogik -
  // der Streifen laeuft als lineares, bewegungsreaktives Lichtspiel.
  if (settings.wandMode) { renderWand(); return; }

  // Zeit-Modus: Open-Loop, Spalten mit fester Taktung, Sensor ignoriert.
  if (settings.timeMode) { renderTimed(); return; }

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
  const float rawAngle = sensor.angle();

  // ---- Lead-Kompensation ---------------------------------------------------
  // Zwischen "Winkel gelesen" und "LEDs leuchten" vergeht eine volle Frame-Zeit
  // (Muster rechnen + show). Der ESP32-C3 hat keine Hardware-FPU, das Rechnen
  // von 65 LEDs mit atan2f/sqrtf dominiert diese Zeit deutlich. Ohne Vorhalt
  // haengt das Bild um omega*T hinterher - und zwar drehzahlabhaengig, was sich
  // wie Drift anfuehlt. Das angezeigte Frame steht von T bis 2T nach dem Lesen,
  // die Bildmitte liegt also bei 1,5*T.
  float lead = sensor.signedRate() * (frameTimeUs * 1.5e-6f);
  if (lead > POV_MAX_LEAD_RAD) lead = POV_MAX_LEAD_RAD;
  else if (lead < -POV_MAX_LEAD_RAD) lead = -POV_MAX_LEAD_RAD;

  float angle = rawAngle + lead;
  if (angle >= TWO_PI_F) angle -= TWO_PI_F;
  else if (angle < 0.0f) angle += TWO_PI_F;

  // Positioniertes Bild nur fuer eingebaute Muster (Custom/Text bleiben Vollkreis).
  const bool positioned = settings.imageMode && settings.patternMode == PATTERN_MODE_BUILTIN;

  // ---- Bildfenster bestimmen ----------------------------------------------
  float windowHalf = PI_F;
  float d = 0.0f;
  if (positioned) {
    const float imgAng = settings.imageAngleDeg * (TWO_PI_F / 360.0f);
    d = angle - imgAng;
    while (d > PI_F) d -= TWO_PI_F;
    while (d < -PI_F) d += TWO_PI_F;

    const float rr = settings.imageRadius * 0.01f;
    const float k = settings.imageScale * 0.01f;
    // Winkelhalbbreite, unter der das Bild ueberhaupt LEDs treffen kann.
    windowHalf = (rr <= k) ? PI_F : (asinf(k / rr) + 0.25f);
  }

  // ---- Winkelaufloesung ----------------------------------------------------
  // Die Schrittzahl wird nur einmal pro Umdrehung neu bestimmt. Vorher lief sie
  // aus einer staendig zappelnden Schaetzung heraus bei JEDEM Aufruf neu - damit
  // aendert sich das Raster mitten in der Umdrehung, der Spaltenindex springt
  // nicht mehr monoton und Spalten werden doppelt gezeichnet oder verschluckt.
  // Genau das verschmiert das Bild.
  if (!gateValid || rawAngle < lastRawAngle - PI_F) {
    const float revPeriodUs = (speed > 0.01f) ? (TWO_PI_F / speed) * 1e6f : 1e9f;

    if (positioned) {
      // Ausserhalb des Fensters kostet ein Frame fast nichts (nur schwarz), das
      // Zeitbudget steht also praktisch komplett dem Fenster zur Verfuegung.
      // Frueher wurde es ueber die ganze Umdrehung verteilt - das verschenkte
      // Schaerfe im Verhaeltnis Vollkreis/Fenster.
      const float frac = (windowHalf * 2.0f) / TWO_PI_F;
      const float budget = revPeriodUs / frameTimeUs;
      float steps = (frac > 0.01f) ? budget / frac : budget;
      if (steps < 32.0f) steps = 32.0f;
      if (steps > POV_MAX_FINE_STEPS) steps = POV_MAX_FINE_STEPS;
      gateStepsLatched = static_cast<uint16_t>(steps);
    } else {
      // Feste Spaltenzahl (nicht mehr an die zappelnde Drehzahlschaetzung
      // gedeckelt) - kein "Atmen" mehr zwischen Umdrehungen.
      uint16_t g = settings.povColumns;
      const uint16_t need = Patterns::nativeColumns(settings);
      if (need > g) g = need;
      if (g > 255) g = 255;
      gateStepsLatched = g;
    }
    gateValid = true;
  }
  lastRawAngle = rawAngle;

  const uint16_t gateSteps = gateStepsLatched;
  effectiveCols = gateSteps > 255 ? 255 : static_cast<uint8_t>(gateSteps);

  // ---- Bild-Modus: ausserhalb des Bildfensters nur schwarz, kein Rechenaufwand ----
  if (positioned) {
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

  // Umdrehungswechsel (Spaltenindex springt zurueck) -> Animationszeit nachziehen.
  if (currentColumn == 0xFFFF || nextColumn < currentColumn) animMs = millis();

  // Diagnose: welcher Winkelbereich des Bildfensters wird wirklich gezeichnet?
  // Deckt das nicht symmetrisch +/-windowHalf ab, fehlt das Bild dort wirklich -
  // dann liegt es an der Winkelkette, nicht an der Mustermathematik.
  if (positioned) {
    if (!covValid) { covMin = d; covMax = d; covValid = true; }
    else {
      if (d < covMin) covMin = d;
      if (d > covMax) covMax = d;
    }
    covWindow = windowHalf;
  }

  currentColumn = nextColumn;
  lastShowAt = now;
  lastFrameBlack = false;
  const uint32_t frameStart = micros();

  if (settings.motionBlur > 0) fadeToBlackBy(leds, NUM_LEDS, settings.motionBlur);
  else fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Spaltenindex fuer Custom-/Text-Bitmaps (die einen Index, keinen Winkel nutzen).
  // Im Vollkreis entspricht das Bitmap-Raster jetzt 1:1 dem Winkelraster - vorher
  // wurde hier auf povColumns zurueckgerechnet und der eben gewonnene Zugewinn
  // an Spalten wieder weggeworfen. Im positionierten Bild-Modus laeuft gateSteps
  // bis POV_MAX_FINE_STEPS, das muss auf den uint8_t-Index gedeckelt werden.
  uint16_t custColumns16 = positioned ? settings.povColumns : gateSteps;
  if (custColumns16 > 255) custColumns16 = 255;
  if (custColumns16 < 1) custColumns16 = 1;
  const uint8_t custColumns = static_cast<uint8_t>(custColumns16);
  uint8_t custCol = static_cast<uint8_t>((static_cast<uint32_t>(nextColumn) * custColumns) / gateSteps);
  if (custCol >= custColumns) custCol = custColumns - 1;

  // Winkelbreite: Bild um +/- persistence feine Schritte verdicken (Max-Blend).
  const uint8_t persistence = settings.angularPersistence;
  const uint8_t halfP = persistence >> 1;
  const float angStep = TWO_PI_F / gateSteps;

  // Inhalt an der Spalten-Mitte rendern, nicht am verrauschten Momentanwinkel
  // -> Sensorrauschen verzerrt das Bild nicht mehr, es steht innerlich starr.
  const float thetaColumn = (TWO_PI_F * nextColumn) / gateSteps;

  for (uint8_t p = 0; p < persistence; p++) {
    const float th = thetaColumn + (static_cast<int>(p) - static_cast<int>(halfP)) * angStep;
    Patterns::render(scratch, settings, th, custCol, custColumns, animMs);
    for (uint16_t j = 0; j < NUM_LEDS; j++) {
      if (scratch[j].r > leds[j].r) leds[j].r = scratch[j].r;
      if (scratch[j].g > leds[j].g) leds[j].g = scratch[j].g;
      if (scratch[j].b > leds[j].b) leds[j].b = scratch[j].b;
    }
  }

  ledController.show();

  // Dauer des GANZEN Frames messen, nicht nur die von show(). Vorher speiste
  // allein die show()-Zeit (~100 us) die Schrittzahl-Schaetzung, waehrend das
  // Rechnen der Muster auf dem FPU-losen C3 ein Vielfaches davon braucht. Die
  // Folge: der Renderer plante ein Vielfaches der Spalten ein, die er real
  // ausgeben kann, kam nie hinterher und das Bild verschmierte.
  const uint32_t frameDur = micros() - frameStart;
  frameTimeUs += (static_cast<float>(frameDur) - frameTimeUs) * 0.1f;
  outputCounter++;

  const uint32_t rateNow = millis();
  const uint32_t elapsed = rateNow - lastOutputRateAt;
  if (elapsed >= 1000) {
    outputRate = outputCounter * 1000UL / elapsed;
    outputCounter = 0;
    lastOutputRateAt = rateNow;
  }
}

void PovRenderer::renderWand() {
  // Feste Bildrate ~120 fps: gleichmaessig weich, aber nicht schneller als noetig
  // (SK9822-show ist billig, aber die Motion-Signale aendern sich langsamer).
  const uint32_t now = micros();
  const uint32_t elapsed = now - lastWandUs;
  if (elapsed < 8000) return;
  // dt fuer die Bewegungsintegration; nach einer Pause (erster Frame) deckeln,
  // damit Baelle/Tropfen nicht in einem Riesenschritt aus dem Bild springen.
  float dt = elapsed * 1e-6f;
  if (dt > 0.05f) dt = 0.05f;
  lastWandUs = now;

  CRGB* leds = ledController.buffer();
  const float energy = sensor.wandEnergy();
  const float swing = sensor.signedRate();
  const float tilt = sensor.wandTilt();
  const float shake = sensor.wandShake();
  WandPatterns::render(leds, settings.wandPattern, dt, energy, swing, tilt, shake);
  ledController.show();
}

void PovRenderer::renderTimed() {
  // Spalten mit fester Taktung (maxColumnHoldUs je Spalte) ohne Sensor.
  const uint32_t now = micros();
  if (now - lastShowAt < settings.maxColumnHoldUs) return;
  lastShowAt = now;

  CRGB* leds = ledController.buffer();

  uint16_t nCols = settings.povColumns;
  const uint16_t need = Patterns::nativeColumns(settings);
  if (need > nCols) nCols = need;
  if (nCols > 255) nCols = 255;
  if (nCols < 1) nCols = 1;
  effectiveCols = static_cast<uint8_t>(nCols);

  uint16_t col = (currentColumn == 0xFFFF) ? 0 : static_cast<uint16_t>(currentColumn) + 1;
  if (col >= nCols) col = 0;
  if (col == 0) animMs = millis();   // Umlauf -> Animationszeit nachziehen
  currentColumn = col;

  if (settings.motionBlur > 0) fadeToBlackBy(leds, NUM_LEDS, settings.motionBlur);
  else fill_solid(leds, NUM_LEDS, CRGB::Black);

  const uint8_t custColumns = static_cast<uint8_t>(nCols);
  const uint8_t custCol = static_cast<uint8_t>(col);
  const float thetaColumn = (TWO_PI_F * col) / nCols;
  const uint8_t persistence = settings.angularPersistence;
  const uint8_t halfP = persistence >> 1;
  const float angStep = TWO_PI_F / nCols;

  for (uint8_t p = 0; p < persistence; p++) {
    const float th = thetaColumn + (static_cast<int>(p) - static_cast<int>(halfP)) * angStep;
    Patterns::render(scratch, settings, th, custCol, custColumns, animMs);
    for (uint16_t j = 0; j < NUM_LEDS; j++) {
      if (scratch[j].r > leds[j].r) leds[j].r = scratch[j].r;
      if (scratch[j].g > leds[j].g) leds[j].g = scratch[j].g;
      if (scratch[j].b > leds[j].b) leds[j].b = scratch[j].b;
    }
  }

  ledController.show();

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
uint32_t PovRenderer::frameUs() const { return static_cast<uint32_t>(frameTimeUs); }
float PovRenderer::coverMinDeg() const { return covMin * 57.2957795f; }
float PovRenderer::coverMaxDeg() const { return covMax * 57.2957795f; }
float PovRenderer::coverWindowDeg() const { return covWindow * 57.2957795f; }
uint32_t PovRenderer::dropouts() const { return sensor.dropouts(); }
bool PovRenderer::isLocked() const { return sensor.isLocked(); }
float PovRenderer::sampleHz() const { return sensor.sampleRateHz(); }
uint32_t PovRenderer::i2cFails() const { return sensor.failedReads(); }
uint32_t PovRenderer::lockEvents() const { return sensor.lockEvents(); }
uint32_t PovRenderer::rejects() const { return sensor.rejects(); }
float PovRenderer::errAvgDeg() const { return sensor.errAvgDeg(); }
float PovRenderer::errMaxDeg() const { return sensor.errMaxDeg(); }
float PovRenderer::rpmMaxSession() const { return sensor.rpmMaxSession(); }
float PovRenderer::hzMinSession() const { return sensor.hzMinSession(); }
void PovRenderer::requestCalibration() { sensor.requestCalibration(); }
bool PovRenderer::isCalibrating() const { return sensor.isCalibrating(); }
