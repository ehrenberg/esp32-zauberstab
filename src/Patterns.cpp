#include "Patterns.h"
#include "PatternStore.h"
#include "PhotoStore.h"
#include <math.h>

namespace {

constexpr float LINE_T = 0.03f;  // intrinsische Linienbreite (duenner = schaerfere Striche)

inline float rad(uint16_t i) { return static_cast<float>(i) / (NUM_LEDS - 1); }

// Winkeldifferenz auf [-PI, PI] falten.
inline float wrapPi(float a) {
  while (a > PI_F) a -= TWO_PI_F;
  while (a < -PI_F) a += TWO_PI_F;
  return a;
}

// Alle Muster rechnen im Bildraum: X,Y sind kartesische Koordinaten (~[-1,1]),
// R der Abstand vom Bildzentrum. Im Vollkreis-Modus gilt (X,Y)=(r*cosθ, r*sinθ)
// und R=r. Im positionierten Modus ("kleines Bild oben") ist der Ursprung ins
// Bildzentrum verschoben und skaliert, sodass dieselben Musterfunktionen ein
// kleines, stehendes Bild erzeugen.
//
// t ist die Animationszeit in Sekunden. Sie wird vom Renderer einmal pro
// Umdrehung gelatcht - wuerde jede Spalte ihre eigene Zeit sehen, liefe die
// Animation innerhalb einer Umdrehung weiter und das Bild wuerde schraeg
// verzerrt ("geschert") statt sich als Ganzes zu bewegen.
struct Ctx {
  const float* X;
  const float* Y;
  const float* R;
  float t;
  uint8_t column;
  uint8_t columns;
};

// ---- Animierte Muster (POV-tauglich: wenige LEDs gleichzeitig hell) --------

// Leuchtpunkt kreist mit ausblendendem Schweif - der klassische Poi-Look.
void pComet(CRGB* leds, const Ctx& c) {
  const float head = c.t * 2.2f;          // Kopfposition (rad)
  const float ringR = 0.72f;              // Bahnradius
  const float tail = 2.6f;                // Schweiflaenge (rad)
  const uint8_t hue = static_cast<uint8_t>(c.t * 24.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float ang = atan2f(c.Y[i], c.X[i]);
    float d = head - ang;
    while (d < 0.0f) d += TWO_PI_F;
    while (d >= TWO_PI_F) d -= TWO_PI_F;
    const float radial = fabsf(c.R[i] - ringR);
    if (d > tail || radial > 0.14f) { leds[i] = CRGB::Black; continue; }
    float v = (1.0f - d / tail);
    v *= v * (1.0f - radial / 0.14f);
    leds[i] = CHSV(hue + static_cast<uint8_t>(d * 14.0f), 230, static_cast<uint8_t>(v * 255.0f));
  }
}

// Zwei gegenlaeufig verschraubte Straenge - dreht sich langsam mit.
void pHelix(CRGB* leds, const Ctx& c) {
  const float twist = 4.0f;
  const float phase = c.t * 1.8f;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    const float ang = atan2f(c.Y[i], c.X[i]);
    const float base = r * twist + phase;
    CRGB out = CRGB::Black;
    for (uint8_t k = 0; k < 2; k++) {
      const float d = fabsf(wrapPi(ang - (base + k * PI_F)));
      if (d < 0.16f) {
        const uint8_t v = static_cast<uint8_t>((1.0f - d / 0.16f) * 255.0f);
        out = CHSV(static_cast<uint8_t>(r * 90.0f + c.t * 20.0f + k * 128), 255, v);
      }
    }
    leds[i] = out;
  }
}

// Ring, dessen Durchmesser atmet; Farbe wandert durch den Farbkreis.
void pPulse(CRGB* leds, const Ctx& c) {
  const float target = 0.40f + 0.34f * sinf(c.t * 2.0f);
  const uint8_t hue = static_cast<uint8_t>(c.t * 30.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float d = fabsf(c.R[i] - target);
    if (d > 0.09f) { leds[i] = CRGB::Black; continue; }
    leds[i] = CHSV(hue, 255, static_cast<uint8_t>((1.0f - d / 0.09f) * 255.0f));
  }
}

// Sechs duenne Speichen, die sich drehen - der stromsparende Ersatz fuers Farbrad.
void pSpokes(CRGB* leds, const Ctx& c) {
  constexpr uint8_t N = 6;
  const float phase = c.t * 1.4f;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f || r < 0.12f) { leds[i] = CRGB::Black; continue; }
    const float seg = TWO_PI_F / N;
    // Auf [0, N) normieren - atan2f liefert negative Winkel, floorf davon ergaebe
    // einen negativen Index.
    float m = (atan2f(c.Y[i], c.X[i]) - phase) / seg;
    m -= floorf(m / N) * N;                      // m in [0, N)
    const float frac = m - floorf(m);            // Lage zwischen zwei Speichen
    // Abstand zur naechstgelegenen Speiche (Speichen liegen bei ganzzahligem m).
    const float d = (frac < 0.5f ? frac : 1.0f - frac) * seg;
    if (d > 0.10f) { leds[i] = CRGB::Black; continue; }
    const uint8_t k = static_cast<uint8_t>(frac < 0.5f ? m : m + 1.0f) % N;
    leds[i] = CHSV(static_cast<uint8_t>(k * (256 / N)), 255,
                   static_cast<uint8_t>((1.0f - d / 0.10f) * 255.0f));
  }
}

// Zufaellige Funken mit kurzer Standzeit. Sehr wenig Strom, wirkt im Dunkeln stark.
void pSparkle(CRGB* leds, const Ctx& c) {
  const uint32_t bucket = static_cast<uint32_t>(c.t * 12.0f);  // ~12 Wechsel/s
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint32_t h = (i * 2654435761u) ^ (c.column * 40503u) ^ (bucket * 2246822519u);
    h ^= h >> 13;
    h *= 3266489917u;
    h ^= h >> 16;
    if ((h & 0x3F) != 0) { leds[i] = CRGB::Black; continue; }  // ~1 von 64
    leds[i] = CHSV(static_cast<uint8_t>(h >> 8), 200, 255);
  }
}

// Radiale Welle, die nach aussen laeuft (schmale helle Kaemme).
void pWave(CRGB* leds, const Ctx& c) {
  const uint8_t hue = static_cast<uint8_t>(c.t * 18.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    float s = sinf(r * 9.0f - c.t * 5.0f);
    if (s <= 0.0f) { leds[i] = CRGB::Black; continue; }
    s = s * s;      // schmaler
    s = s * s * s;  // sehr schmaler Kamm -> duenne Ringe statt Flaeche
    leds[i] = CHSV(hue + static_cast<uint8_t>(r * 60.0f), 255, static_cast<uint8_t>(s * 255.0f));
  }
}

// ---- Formen (statisch bzw. dezent animiert) -------------------------------

void pSpiral(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float ang = atan2f(c.Y[i], c.X[i]);
    float p = c.R[i] * 3.0f - ang / TWO_PI_F + c.t * 0.5f;
    p -= floorf(p);
    leds[i] = (p < 0.18f) ? CRGB(CHSV(static_cast<uint8_t>(c.R[i] * 255), 255, 255)) : CRGB::Black;
  }
}

// Herz mit Herzschlag (zwei schnelle Schlaege, dann Pause).
void pHeart(CRGB* leds, const Ctx& c) {
  float beat = sinf(c.t * 6.0f);
  beat = beat > 0.0f ? beat * beat : 0.0f;
  const float k = 1.4f / (1.0f + 0.12f * beat);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float Xh = c.X[i] * k;
    const float Yh = -c.Y[i] * k - 0.2f;
    const float a = Xh * Xh + Yh * Yh - 1.0f;
    const float f = a * a * a - Xh * Xh * Yh * Yh * Yh;
    leds[i] = (f < 0.0f) ? CRGB(255, 0, 60) : CRGB::Black;
  }
}

void pSmiley(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i], x = c.X[i], y = c.Y[i];
    CRGB col = CRGB::Black;
    if (r < 0.92f) {
      col = CRGB(255, 210, 0);
      const float e1 = (x - 0.32f) * (x - 0.32f) + (y - 0.30f) * (y - 0.30f);
      const float e2 = (x + 0.32f) * (x + 0.32f) + (y - 0.30f) * (y - 0.30f);
      if (e1 < 0.02f || e2 < 0.02f) col = CRGB::Black;
      const float md = sqrtf(x * x + (y + 0.05f) * (y + 0.05f));
      if (y < -0.05f && fabsf(md - 0.5f) < 0.09f) col = CRGB::Black;
    }
    leds[i] = col;
  }
}

// Stern als Umriss (statt gefuellt) - viel weniger Strom, schaerferes Bild.
void pStar(CRGB* leds, const Ctx& c) {
  const float spin = c.t * 0.7f;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float ang = atan2f(c.Y[i], c.X[i]) - spin;
    const float edge = 0.5f + 0.4f * cosf(5.0f * ang);
    leds[i] = (fabsf(c.R[i] - edge) < 0.07f) ? CRGB(255, 200, 0) : CRGB::Black;
  }
}

void pArrow(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float x = c.X[i], y = c.Y[i];
    bool on = false;
    if (y > 0.25f && y < 0.75f && fabsf(x) < (0.75f - y)) on = true;   // Kopf
    else if (y <= 0.25f && y > -0.75f && fabsf(x) < 0.13f) on = true;  // Schaft
    leds[i] = on ? CRGB(0, 255, 80) : CRGB::Black;
  }
}

void pTriangle(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float x = c.X[i], y = c.Y[i];
    const bool big = (y > -0.6f && y < 0.7f && fabsf(x) < (0.7f - y) * 0.7f);
    const bool small = (y > -0.45f && y < 0.45f && fabsf(x) < (0.45f - y) * 0.7f);
    leds[i] = (big && !small) ? CRGB(0, 220, 255) : CRGB::Black;
  }
}

void pCross(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (fabsf(c.X[i]) < LINE_T) leds[i] = CRGB(255, 0, 0);
    else if (fabsf(c.Y[i]) < LINE_T) leds[i] = CRGB(0, 255, 0);
    else leds[i] = CRGB::Black;
  }
}

void pDiagonal(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (fabsf(c.X[i] - c.Y[i]) < LINE_T) leds[i] = CRGB(255, 0, 0);
    else if (fabsf(c.X[i] + c.Y[i]) < LINE_T) leds[i] = CRGB(0, 128, 255);
    else leds[i] = CRGB::Black;
  }
}

void pVertical(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = (fabsf(c.X[i]) < LINE_T) ? CRGB(0, 0, 255) : CRGB::Black;
}

void pHorizontal(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = (fabsf(c.Y[i]) < LINE_T) ? CRGB(0, 255, 0) : CRGB::Black;
}

void pRing(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = (fabsf(c.R[i] - 0.65f) < 0.08f) ? CRGB(255, 180, 0) : CRGB::Black;
}

void pRectangle(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float ax = fabsf(c.X[i]), ay = fabsf(c.Y[i]);
    const float m = ax > ay ? ax : ay;
    leds[i] = (m > 0.55f && m < 0.80f) ? CRGB(255, 0, 200) : CRGB::Black;
  }
}

void pConcentric(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (fabsf(r - 0.3f) < 0.06f) leds[i] = CRGB::Red;
    else if (fabsf(r - 0.6f) < 0.06f) leds[i] = CRGB::Green;
    else if (fabsf(r - 0.9f) < 0.06f) leds[i] = CRGB::Blue;
    else leds[i] = CRGB::Black;
  }
}

// Kalibriermuster: vier Sektoren + weisser Marker bei Spalte 0. Hilft beim
// Einstellen von Angle Gain und Richtung, ist als Show-Muster ungeeignet.
void pColorBars(CRGB* leds, const Ctx& c) {
  const uint8_t q = (static_cast<uint16_t>(c.column) * 4) / (c.columns ? c.columns : 1);
  const CRGB col = q == 0 ? CRGB::Red : q == 1 ? CRGB::Green : q == 2 ? CRGB::Blue : CRGB::Yellow;
  fill_solid(leds, NUM_LEDS, col);
  if (c.column == 0) fill_solid(leds, NUM_LEDS, CRGB::White);
}

}  // namespace

const char* Patterns::name(uint8_t p) {
  switch (p) {
    case 0:  return "Komet";
    case 1:  return "Doppelhelix";
    case 2:  return "Pulsring";
    case 3:  return "Speichen";
    case 4:  return "Funken";
    case 5:  return "Welle";
    case 6:  return "Spirale";
    case 7:  return "Herz";
    case 8:  return "Smiley";
    case 9:  return "Stern";
    case 10: return "Pfeil";
    case 11: return "Dreieck";
    case 12: return "Kreuz";
    case 13: return "Diagonale";
    case 14: return "Vertikale Linie";
    case 15: return "Horizontale Linie";
    case 16: return "Kreis";
    case 17: return "Rechteck";
    case 18: return "Ringe";
    case 19: return "Farbbalken (Kalib.)";
    default: return "?";
  }
}

uint16_t Patterns::nativeColumns(const Settings& settings) {
  switch (settings.patternMode) {
    case PATTERN_MODE_TEXT:   return PatternStore::textColumns();
    case PATTERN_MODE_CUSTOM: return CUSTOM_COLS;
    case PATTERN_MODE_IMAGE:  return IMG_COLS;
    default:                  return 0;
  }
}

bool Patterns::isHeavy(uint8_t p) {
  // Smiley fuellt eine gelbe Scheibe, Farbbalken den ganzen Streifen.
  return p == 8 || p == 19;
}

void Patterns::render(CRGB* leds, const Settings& settings, float theta,
                      uint8_t column, uint8_t columns, uint32_t tMs) {
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

  const Ctx ctx{X, Y, R, tMs * 0.001f, column, columns};

  switch (settings.selectedPattern) {
    case 0:  pComet(leds, ctx); break;
    case 1:  pHelix(leds, ctx); break;
    case 2:  pPulse(leds, ctx); break;
    case 3:  pSpokes(leds, ctx); break;
    case 4:  pSparkle(leds, ctx); break;
    case 5:  pWave(leds, ctx); break;
    case 6:  pSpiral(leds, ctx); break;
    case 7:  pHeart(leds, ctx); break;
    case 8:  pSmiley(leds, ctx); break;
    case 9:  pStar(leds, ctx); break;
    case 10: pArrow(leds, ctx); break;
    case 11: pTriangle(leds, ctx); break;
    case 12: pCross(leds, ctx); break;
    case 13: pDiagonal(leds, ctx); break;
    case 14: pVertical(leds, ctx); break;
    case 15: pHorizontal(leds, ctx); break;
    case 16: pRing(leds, ctx); break;
    case 17: pRectangle(leds, ctx); break;
    case 18: pConcentric(leds, ctx); break;
    default: pColorBars(leds, ctx); break;
  }

  // Positioniertes Bild: alles ausserhalb des Einheitskreises um das Bildzentrum
  // schwarz -> es steht ein klar begrenztes, stilles Bild an einer Stelle im Kreis.
  if (settings.imageMode) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (R[i] > 1.0f) leds[i] = CRGB::Black;
    }
  }
}
