#include "Patterns.h"
#include "PatternStore.h"
#include "PhotoStore.h"
#include <math.h>

namespace {

constexpr float LINE_T = 0.03f;  // intrinsische Linienbreite (duenner = schaerfere Striche)

inline float rad(uint16_t i) { return static_cast<float>(i) / (NUM_LEDS - 1); }

// Alle Muster rechnen im Bildraum: X,Y sind kartesische Koordinaten (~[-1,1]),
// R der Abstand vom Bildzentrum. Im Vollkreis-Modus gilt (X,Y)=(r*cosθ, r*sinθ)
// und R=r -> exakt das alte Verhalten. Im positionierten Modus ("kleines Bild
// oben") ist der Ursprung ins Bildzentrum verschoben und skaliert, sodass
// dieselben Musterfunktionen ein kleines, stehendes Bild erzeugen.

// 0 Farbbalken (Kalibrierung): vier Sektoren + weisser Marker bei Spalte 0.
void pColorBars(CRGB* leds, uint8_t column, uint8_t columns) {
  const uint8_t q = (static_cast<uint16_t>(column) * 4) / columns;
  CRGB col = q == 0 ? CRGB::Red : q == 1 ? CRGB::Green : q == 2 ? CRGB::Blue : CRGB::Yellow;
  fill_solid(leds, NUM_LEDS, col);
  if (column == 0) fill_solid(leds, NUM_LEDS, CRGB::White);
}

void pVertical(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = (fabsf(X[i]) < LINE_T) ? CRGB(0, 0, 255) : CRGB::Black;
}

void pHorizontal(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = (fabsf(Y[i]) < LINE_T) ? CRGB(0, 255, 0) : CRGB::Black;
}

void pCross(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (fabsf(X[i]) < LINE_T) leds[i] = CRGB(255, 0, 0);
    else if (fabsf(Y[i]) < LINE_T) leds[i] = CRGB(0, 255, 0);
    else leds[i] = CRGB::Black;
  }
}

void pDiagonal(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (fabsf(X[i] - Y[i]) < LINE_T) leds[i] = CRGB(255, 0, 0);
    else if (fabsf(X[i] + Y[i]) < LINE_T) leds[i] = CRGB(0, 128, 255);
    else leds[i] = CRGB::Black;
  }
}

void pRing(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = (fabsf(R[i] - 0.65f) < 0.08f) ? CRGB(255, 180, 0) : CRGB::Black;
}

void pRectangle(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float ax = fabsf(X[i]), ay = fabsf(Y[i]);
    float m = ax > ay ? ax : ay;
    leds[i] = (m > 0.55f && m < 0.80f) ? CRGB(255, 0, 200) : CRGB::Black;
  }
}

void pChecker(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int gx = (int)floorf((X[i] + 1.0f) * 4);
    int gy = (int)floorf((Y[i] + 1.0f) * 4);
    leds[i] = ((gx + gy) & 1) ? CRGB(0, 200, 200) : CRGB::Black;
  }
}

void pDisc(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = (R[i] < 0.6f) ? CRGB(0, 90, 255) : CRGB::Black;
}

void pConcentric(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float r = R[i];
    if (fabsf(r - 0.3f) < 0.06f) leds[i] = CRGB::Red;
    else if (fabsf(r - 0.6f) < 0.06f) leds[i] = CRGB::Green;
    else if (fabsf(r - 0.9f) < 0.06f) leds[i] = CRGB::Blue;
    else leds[i] = CRGB::Black;
  }
}

void pStar(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float ang = atan2f(Y[i], X[i]);
    float edge = 0.5f + 0.4f * cosf(5.0f * ang);
    leds[i] = (R[i] <= edge) ? CRGB(255, 200, 0) : CRGB::Black;
  }
}

void pSpiral(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float ang = atan2f(Y[i], X[i]);
    float p = R[i] * 3.0f - ang / TWO_PI_F;
    p -= floorf(p);
    leds[i] = (p < 0.18f) ? CRGB(CHSV((uint8_t)(R[i] * 255), 255, 255)) : CRGB::Black;
  }
}

void pHeart(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float Xh = X[i] * 1.4f;
    float Yh = -Y[i] * 1.4f - 0.2f;
    float a = Xh * Xh + Yh * Yh - 1.0f;
    float f = a * a * a - Xh * Xh * Yh * Yh * Yh;
    leds[i] = (f < 0.0f) ? CRGB(255, 0, 60) : CRGB::Black;
  }
}

void pSmiley(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float r = R[i], x = X[i], y = Y[i];
    CRGB col = CRGB::Black;
    if (r < 0.92f) {
      col = CRGB(255, 210, 0);
      // Augen
      float e1 = (x - 0.32f) * (x - 0.32f) + (y - 0.30f) * (y - 0.30f);
      float e2 = (x + 0.32f) * (x + 0.32f) + (y - 0.30f) * (y - 0.30f);
      if (e1 < 0.02f || e2 < 0.02f) col = CRGB::Black;
      // Mund (Bogen unten)
      float md = sqrtf(x * x + (y + 0.05f) * (y + 0.05f));
      if (y < -0.05f && fabsf(md - 0.5f) < 0.09f) col = CRGB::Black;
    }
    leds[i] = col;
  }
}

void pArrow(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float x = X[i], y = Y[i];
    bool on = false;
    if (y > 0.25f && y < 0.75f && fabsf(x) < (0.75f - y)) on = true;       // Kopf
    else if (y <= 0.25f && y > -0.75f && fabsf(x) < 0.13f) on = true;       // Schaft
    leds[i] = on ? CRGB(0, 255, 80) : CRGB::Black;
  }
}

void pTriangle(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float x = X[i], y = Y[i];
    bool big = (y > -0.6f && y < 0.7f && fabsf(x) < (0.7f - y) * 0.7f);
    bool small = (y > -0.45f && y < 0.45f && fabsf(x) < (0.45f - y) * 0.7f);
    leds[i] = (big && !small) ? CRGB(0, 220, 255) : CRGB::Black;
  }
}

void pRainbowRings(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = CHSV((uint8_t)(R[i] * 255), 255, 255);
}

void pColorWheel(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float ang = atan2f(Y[i], X[i]);
    leds[i] = CHSV((uint8_t)(ang / TWO_PI_F * 255.0f), 255, 255);
  }
}

void pStripes(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int band = (int)floorf((X[i] + 1.0f) * 4.0f);
    leds[i] = (band & 1) ? CRGB(255, 60, 0) : CRGB(0, 60, 255);
  }
}

void pDots(CRGB* leds, const float* X, const float* Y, const float* R) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float gx = (X[i] + 1.0f) * 3.0f, gy = (Y[i] + 1.0f) * 3.0f;
    float dx = gx - floorf(gx) - 0.5f, dy = gy - floorf(gy) - 0.5f;
    leds[i] = (dx * dx + dy * dy < 0.10f) ? CRGB(255, 0, 220) : CRGB(10, 0, 20);
  }
}

}  // namespace

const char* Patterns::name(uint8_t p) {
  switch (p) {
    case 0: return "Farbbalken";
    case 1: return "Vertikale Linie";
    case 2: return "Horizontale Linie";
    case 3: return "Kreuz";
    case 4: return "Diagonale";
    case 5: return "Kreis";
    case 6: return "Rechteck";
    case 7: return "Schachbrett";
    case 8: return "Scheibe";
    case 9: return "Ringe";
    case 10: return "Stern";
    case 11: return "Spirale";
    case 12: return "Herz";
    case 13: return "Smiley";
    case 14: return "Pfeil";
    case 15: return "Dreieck";
    case 16: return "Regenbogen";
    case 17: return "Farbrad";
    case 18: return "Streifen";
    case 19: return "Punkte";
    default: return "?";
  }
}

void Patterns::render(CRGB* leds, const Settings& settings, float theta, uint8_t column, uint8_t columns) {
  if (settings.patternMode == PATTERN_MODE_CUSTOM) {
    PatternStore::renderCustom(leds, settings.customSlot, column, columns);
    return;
  }
  if (settings.patternMode == PATTERN_MODE_TEXT) {
    PatternStore::renderText(leds, column, columns);
    return;
  }
  if (settings.patternMode == PATTERN_MODE_IMAGE) {
    PhotoStore::renderImage(leds, settings.imageSlot, column, columns);
    return;
  }

  const float c = cosf(theta);
  const float s = sinf(theta);

  // Bildraum-Koordinaten pro LED aufbauen (einmal, dann von allen Mustern genutzt).
  float X[NUM_LEDS], Y[NUM_LEDS], R[NUM_LEDS];
  if (settings.imageMode) {
    const float ang = settings.imageAngleDeg * (TWO_PI_F / 360.0f);
    const float cx = (settings.imageRadius * 0.01f) * cosf(ang);
    const float cy = (settings.imageRadius * 0.01f) * sinf(ang);
    const float invK = 1.0f / (settings.imageScale * 0.01f);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      const float x = (rad(i) * c - cx) * invK;
      const float y = (rad(i) * s - cy) * invK;
      X[i] = x;
      Y[i] = y;
      R[i] = sqrtf(x * x + y * y);
    }
  } else {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      X[i] = rad(i) * c;
      Y[i] = rad(i) * s;
      R[i] = rad(i);
    }
  }

  switch (settings.selectedPattern) {
    case 0:  pColorBars(leds, column, columns); break;
    case 1:  pVertical(leds, X, Y, R); break;
    case 2:  pHorizontal(leds, X, Y, R); break;
    case 3:  pCross(leds, X, Y, R); break;
    case 4:  pDiagonal(leds, X, Y, R); break;
    case 5:  pRing(leds, X, Y, R); break;
    case 6:  pRectangle(leds, X, Y, R); break;
    case 7:  pChecker(leds, X, Y, R); break;
    case 8:  pDisc(leds, X, Y, R); break;
    case 9:  pConcentric(leds, X, Y, R); break;
    case 10: pStar(leds, X, Y, R); break;
    case 11: pSpiral(leds, X, Y, R); break;
    case 12: pHeart(leds, X, Y, R); break;
    case 13: pSmiley(leds, X, Y, R); break;
    case 14: pArrow(leds, X, Y, R); break;
    case 15: pTriangle(leds, X, Y, R); break;
    case 16: pRainbowRings(leds, X, Y, R); break;
    case 17: pColorWheel(leds, X, Y, R); break;
    case 18: pStripes(leds, X, Y, R); break;
    default: pDots(leds, X, Y, R); break;
  }

  // Positioniertes Bild: alles ausserhalb des Einheitskreises um das Bildzentrum
  // schwarz -> es steht ein klar begrenztes, stilles Bild an einer Stelle im Kreis.
  if (settings.imageMode) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (R[i] > 1.0f) leds[i] = CRGB::Black;
    }
  }
}
