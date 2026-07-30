#include "Patterns.h"
#include "PatternStore.h"
#include "PhotoStore.h"
#include <math.h>

namespace {

inline float rad(uint16_t i) { return static_cast<float>(i) / (NUM_LEDS - 1); }
inline float frac(float x) { return x - floorf(x); }

// Winkeldifferenz auf [-PI, PI] falten.
inline float wrapPi(float a) {
  while (a > PI_F) a -= TWO_PI_F;
  while (a < -PI_F) a += TWO_PI_F;
  return a;
}

// Radiant -> Farbton-Byte, faltet beliebige Winkel sauber auf einen Umlauf.
inline uint8_t hueWheel(float a) {
  float f = a * (1.0f / TWO_PI_F);
  f -= floorf(f);
  return static_cast<uint8_t>(f * 256.0f) & 0xFF;
}

// Deterministischer Hash (LED-Index, Spalte, Zeit-Bucket) - Kern der Partikel-Muster.
inline uint32_t rng(uint32_t a, uint32_t b, uint32_t d) {
  uint32_t h = a * 2654435761u ^ b * 2246822519u ^ d * 3266489917u;
  h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
  return h;
}
inline float rngf(uint32_t a, uint32_t b, uint32_t d) {
  return (rng(a, b, d) & 0xFFFFu) * (1.0f / 65535.0f);
}

// Max-Blend (fuer sich ueberlagernde Partikel wie mehrere Meteore).
inline void blendMax(CRGB& dst, const CRGB& src) {
  if (src.r > dst.r) dst.r = src.r;
  if (src.g > dst.g) dst.g = src.g;
  if (src.b > dst.b) dst.b = src.b;
}

// Bildraum je LED: X,Y kartesisch ~[-1,1], R Abstand zum Zentrum, t = Zeit (s).
struct Ctx {
  const float* X;
  const float* Y;
  const float* R;
  float t;
  uint8_t column;
  uint8_t columns;
  float theta;      // Spaltenwinkel (Vollkreis: fuer alle LEDs gleich -> kein atan2f)
  bool positioned;
};

inline float angleAt(const Ctx& c, uint16_t i) {
  return c.positioned ? atan2f(c.Y[i], c.X[i]) : c.theta;
}

// Partikel-POV-Set: viele helle Einzelpunkte auf Schwarz, jede Spalte unabhaengig.

// 0 Funken: spaerliche helle Funken, enge Farbfamilie.
void pSparkle(CRGB* leds, const Ctx& c) {
  const uint32_t bucket = static_cast<uint32_t>(c.t * 11.0f);
  const uint8_t baseHue = static_cast<uint8_t>(c.t * 30.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint32_t h = rng(i, c.column, bucket);
    if ((h & 0x1F) != 0) { leds[i] = CRGB::Black; continue; }   // ~1 von 32
    leds[i] = CHSV(baseHue + static_cast<uint8_t>((h >> 8) & 0x3F), 210, 255);
  }
}

// 1 Konfetti: wie Funken, aber jeder Funke eine kraeftige Zufallsfarbe.
void pConfetti(CRGB* leds, const Ctx& c) {
  const uint32_t bucket = static_cast<uint32_t>(c.t * 9.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint32_t h = rng(i, c.column, bucket);
    if ((h & 0x1F) != 0) { leds[i] = CRGB::Black; continue; }   // ~1 von 32
    leds[i] = CHSV(static_cast<uint8_t>(h >> 9), 255, 255);      // volle Zufallsfarbe
  }
}

// 2 Regen: pro Spalte ein fallender Tropfen mit Schweif -> Matrix-Regen.
void pRain(CRGB* leds, const Ctx& c) {
  const uint32_t hc = rng(c.column, 777u, 3u);
  const float speed = 0.30f + (hc & 0xFF) * (1.0f / 255.0f) * 0.55f;  // R-Einheiten/s
  const float phase = ((hc >> 8) & 0xFF) * (1.0f / 255.0f);
  const float head = frac(c.t * speed + phase);                 // Kopf-Radius 0..1
  const float trail = 0.40f;
  const uint8_t hue = 112 + static_cast<uint8_t>((hc >> 16) & 0x1F);  // Cyan/Gruen-Familie
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    const float d = head - r;                                   // >0: hinter dem Kopf
    if (d < 0.0f || d > trail) { leds[i] = CRGB::Black; continue; }
    float v = 1.0f - d / trail;
    v *= v;                                                      // heller Kopf, schneller Auslauf
    leds[i] = CHSV(hue, 220, static_cast<uint8_t>(v * 255.0f));
  }
}

// 3 Feuer: flackernde Glut, heiss am Zentrum, Rot->Gelb je Hitze. Flaechig (isHeavy).
void pFire(CRGB* leds, const Ctx& c) {
  const uint32_t bucket = static_cast<uint32_t>(c.t * 20.0f);   // schnelles Flackern
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    const float n = rngf(i, c.column, bucket);
    float heat = (1.0f - r) * (0.35f + 0.65f * n) * 1.7f;       // Zentrum am heissesten
    if (heat < 0.18f) { leds[i] = CRGB::Black; continue; }
    if (heat > 1.0f) heat = 1.0f;
    const uint8_t hue = static_cast<uint8_t>(heat * 40.0f);     // 0 rot .. 40 gelb
    const uint8_t sat = static_cast<uint8_t>(255 - heat * 60.0f);  // heisse Spitzen weisslicher
    leds[i] = CHSV(hue, sat, static_cast<uint8_t>(heat * 255.0f));
  }
}

// 4 Meteore: wenige helle Sternschnuppen mit Schweif schiessen radial nach aussen.
void pMeteors(CRGB* leds, const Ctx& c) {
  constexpr uint8_t K = 5;
  const float W = 0.13f;    // Winkelhalbbreite
  const float tail = 0.45f; // Schweiflaenge (R-Einheiten)
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    const float ang = angleAt(c, i);
    CRGB out = CRGB::Black;
    for (uint8_t k = 0; k < K; k++) {
      const float angK = k * (TWO_PI_F / K) + c.t * 0.12f;
      const float headR = frac(c.t * 0.75f + k * 0.37f) * 1.3f; // 0..1,3: Luecke = Aufblitzen
      const float dA = fabsf(wrapPi(ang - angK));
      if (dA > W) continue;
      const float along = headR - r;                            // >0: hinter dem Kopf (nach innen)
      if (along < 0.0f || along > tail) continue;
      float v = (1.0f - along / tail) * (1.0f - dA / W);
      v *= v;
      blendMax(out, CRGB(CHSV(static_cast<uint8_t>(k * 51 + c.t * 8.0f), 255,
                              static_cast<uint8_t>(v * 255.0f))));
    }
    leds[i] = out;
  }
}

// 5 Plasma: dichteres Funkeln, Farben aus einem fliessenden Feld (Radius+Winkel+Zeit).
void pPlasma(CRGB* leds, const Ctx& c) {
  const uint32_t bucket = static_cast<uint32_t>(c.t * 10.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint32_t h = rng(i, c.column, bucket);
    if ((h & 0x0F) != 0) { leds[i] = CRGB::Black; continue; }   // ~1 von 16 (dichter)
    const float field = sinf(c.R[i] * 4.0f + c.t * 1.3f) + sinf(angleAt(c, i) * 3.0f - c.t * 0.8f);
    leds[i] = CHSV(hueWheel(field), 255, 255);
  }
}

// 6 Sterne: ruhiger Sternenhimmel, feste LED-Auswahl blendet weich weissblau auf/ab.
void pStars(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint32_t h = rng(i, c.column, 5u);                    // feste Sternauswahl (stabil je Umlauf)
    if ((h & 0x0F) >= 2u) { leds[i] = CRGB::Black; continue; }  // ~1 von 8 ist ein Stern
    const float phase = ((h >> 8) & 0xFF) * (TWO_PI_F / 255.0f);
    float tw = 0.5f + 0.5f * sinf(c.t * 2.2f + phase);
    tw *= tw;                                                   // weiches Auf- und Abblenden
    const uint8_t hue = 150 + static_cast<uint8_t>((h >> 16) & 0x1F);
    leds[i] = CHSV(hue, 110, static_cast<uint8_t>(tw * 255.0f));
  }
}

// 7 Blitze: selten blitzt eine ganze Spalte als greller Strich an Zufallsstelle auf.
void pFlash(CRGB* leds, const Ctx& c) {
  const uint32_t bucket = static_cast<uint32_t>(c.t * 13.0f);
  const uint32_t hc = rng(c.column, bucket, 99u);
  if ((hc & 0x07) != 0u) { fill_solid(leds, NUM_LEDS, CRGB::Black); return; }  // ~1 von 8 Spalten
  const float center = ((hc >> 8) & 0xFF) * (1.0f / 255.0f);
  const float len = 0.14f + ((hc >> 16) & 0x3F) * (1.0f / 63.0f) * 0.28f;
  const uint8_t hue = 150 + static_cast<uint8_t>((hc >> 24) & 0x1F);           // Blauweiss
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    const float d = fabsf(r - center);
    if (d > len) { leds[i] = CRGB::Black; continue; }
    const float v = 1.0f - d / len;
    leds[i] = CHSV(hue, static_cast<uint8_t>(50 + 60 * (1.0f - v)), static_cast<uint8_t>(v * 255.0f));
  }
}

// 8 Wirbel: helle Funken nur entlang einer langsam drehenden Doppelspirale.
void pSwirl(CRGB* leds, const Ctx& c) {
  constexpr uint8_t N = 2;
  const float twist = 4.0f;
  const float spin = c.t * 1.4f;
  const float seg = TWO_PI_F / N;
  const float W = 0.30f;                                        // Naehe zum Spiralarm
  const uint32_t bucket = static_cast<uint32_t>(c.t * 14.0f);
  const uint8_t baseHue = static_cast<uint8_t>(c.t * 20.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f || r < 0.05f) { leds[i] = CRGB::Black; continue; }
    float m = (angleAt(c, i) - spin - r * twist) / seg;
    m -= floorf(m);
    const float d = (m < 0.5f ? m : 1.0f - m) * seg;
    if (d > W) { leds[i] = CRGB::Black; continue; }
    const uint32_t h = rng(i, c.column, bucket);
    if ((h & 0x03) != 0) { leds[i] = CRGB::Black; continue; }   // 1 von 4 auf dem Arm leuchtet
    leds[i] = CHSV(baseHue + static_cast<uint8_t>(r * 80.0f), 255, 255);
  }
}

}  // namespace

const char* Patterns::name(uint8_t p) {
  switch (p) {
    case 0:  return "Funken";
    case 1:  return "Konfetti";
    case 2:  return "Regen";
    case 3:  return "Feuer";
    case 4:  return "Meteore";
    case 5:  return "Plasma";
    case 6:  return "Sterne";
    case 7:  return "Blitze";
    case 8:  return "Wirbel";
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
  // Nur Feuer leuchtet einen groesseren Teil des Streifens gleichzeitig und zieht
  // damit Strom (FastLEDs Limiter dimmt dann global; die Web-UI markiert es). Alle
  // uebrigen Muster sind spaerliche Funken/Partikel und unkritisch.
  return p == 3;
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

  const Ctx ctx{X, Y, R, tMs * 0.001f, column, columns, theta, settings.imageMode};

  switch (settings.selectedPattern) {
    case 0:  pSparkle(leds, ctx); break;
    case 1:  pConfetti(leds, ctx); break;
    case 2:  pRain(leds, ctx); break;
    case 3:  pFire(leds, ctx); break;
    case 4:  pMeteors(leds, ctx); break;
    case 5:  pPlasma(leds, ctx); break;
    case 6:  pStars(leds, ctx); break;
    case 7:  pFlash(leds, ctx); break;
    case 8:  pSwirl(leds, ctx); break;
    default: pSparkle(leds, ctx); break;
  }

  // Positioniertes Bild: alles ausserhalb des Einheitskreises um das Bildzentrum
  // schwarz -> es steht ein klar begrenztes Bild an einer Stelle im Kreis.
  if (settings.imageMode) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (R[i] > 1.0f) leds[i] = CRGB::Black;
    }
  }
}
