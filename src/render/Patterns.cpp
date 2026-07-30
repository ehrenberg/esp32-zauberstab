#include "Patterns.h"
#include "PatternStore.h"
#include "PhotoStore.h"
#include <math.h>

namespace {

inline float rad(uint16_t i) { return static_cast<float>(i) / (NUM_LEDS - 1); }

// Radiant -> Farbton-Byte, faltet beliebige Winkel auf einen Umlauf.
inline uint8_t hueWheel(float a) {
  float f = a * (1.0f / TWO_PI_F);
  f -= floorf(f);
  return static_cast<uint8_t>(f * 256.0f) & 0xFF;
}

// Deterministischer Hash (LED-Index, Spalte, Zeit-Bucket) fuer Funken/Flackern.
inline uint32_t rng(uint32_t a, uint32_t b, uint32_t d) {
  uint32_t h = a * 2654435761u ^ b * 2246822519u ^ d * 3266489917u;
  h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
  return h;
}
inline float rngf(uint32_t a, uint32_t b, uint32_t d) {
  return (rng(a, b, d) & 0xFFFFu) * (1.0f / 65535.0f);
}

// Abstand Punkt->Strecke (fuer Strichfiguren und Peace-Zeichen).
inline float segDist(float px, float py, float ax, float ay, float bx, float by) {
  const float vx = bx - ax, vy = by - ay;
  const float wx = px - ax, wy = py - ay;
  float t = (vx * wx + vy * wy) / (vx * vx + vy * vy + 1e-6f);
  if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
  const float dx = ax + t * vx - px, dy = ay + t * vy - py;
  return sqrtf(dx * dx + dy * dy);
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

// Herzkurve: <0 im Inneren. lx,ly = lokale Koordinaten (~[-1.2,1.2]).
inline bool insideHeart(float lx, float ly) {
  const float a = lx * lx + ly * ly - 1.0f;
  return (a * a * a - lx * lx * ly * ly * ly) <= 0.0f;
}

// 5x7-Glyphen fuer "ANTIFA" (MSB = linke Spalte).
const uint8_t GLYPH_A[7] = {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
const uint8_t GLYPH_N[7] = {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001};
const uint8_t GLYPH_T[7] = {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
const uint8_t GLYPH_I[7] = {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111};
const uint8_t GLYPH_F[7] = {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000};

// Nyan-Cat-Bitmap, 26x13. Zeichen -> Farbe in nyanColor().
const char* const NYAN[13] = {
  "                          ",
  "                     G  G ",
  "            tttttttttGGGGG",
  "rrrrrrrrrrrrtppppppptGkGkG",
  "ooooooooooootppppppptGGGGG",
  "yyyyyyyyyyyytppppppptpGkGp",
  "ggggggggggggtppppppptGGGGG",
  "bbbbbbbbbbbbtppppppptGGGGG",
  "vvvvvvvvvvvvttttttttt     ",
  "            G G G G G     ",
  "                          ",
  "                          ",
  "                          ",
};

inline CRGB nyanColor(char ch) {
  switch (ch) {
    case 'r': return CRGB(255, 0, 0);
    case 'o': return CRGB(255, 100, 0);
    case 'y': return CRGB(255, 210, 0);
    case 'g': return CRGB(0, 210, 40);
    case 'b': return CRGB(0, 110, 255);
    case 'v': return CRGB(150, 40, 255);
    case 't': return CRGB(240, 180, 120);
    case 'p': return CRGB(255, 110, 170);
    case 'G': return CRGB(150, 150, 165);
    default:  return CRGB::Black;   // ' ' und 'k' (Augen/Mund = Loecher)
  }
}

// Char-Bitmap an kartesischer Position abtasten (Seitenverhaeltnis erhalten).
inline char sampleChar(const char* const* rows, uint8_t W, uint8_t H,
                       float x, float y, float hx, float hy) {
  if (x < -hx || x > hx || y < -hy || y > hy) return ' ';
  int u = static_cast<int>((hx - x) / (2.0f * hx) * W);   // horizontal gespiegelt (Leserichtung)
  int v = static_cast<int>((hy - y) / (2.0f * hy) * H);
  if (u < 0) u = 0; else if (u >= W) u = W - 1;
  if (v < 0) v = 0; else if (v >= H) v = H - 1;
  return rows[v][u];
}

// ============================================================================
//  Bild-POV-Set: echte, erkennbare Figuren im kartesischen Bildraum.
// ============================================================================

// 0 Funken: spaerliche helle Funken, enge Farbfamilie.
void pSparkle(CRGB* leds, const Ctx& c) {
  const uint32_t bucket = static_cast<uint32_t>(c.t * 11.0f);
  const uint8_t baseHue = static_cast<uint8_t>(c.t * 30.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint32_t h = rng(i, c.column, bucket);
    if ((h & 0x1F) != 0) { leds[i] = CRGB::Black; continue; }
    leds[i] = CHSV(baseHue + static_cast<uint8_t>((h >> 8) & 0x3F), 210, 255);
  }
}

// 1 Bunter Stern: gefuellter Fuenfzack, Farbe laeuft rund herum.
void pStar(CRGB* leds, const Ctx& c) {
  constexpr uint8_t P = 5;
  const float rot = c.t * 0.3f;
  const float rIn = 0.40f, rOut = 0.98f;
  const uint8_t drift = static_cast<uint8_t>(c.t * 20.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    const float a = angleAt(c, i);
    float u = (a - rot) * (P / TWO_PI_F);
    u -= floorf(u);
    const float tri = 1.0f - fabsf(2.0f * u - 1.0f);
    const float edge = rIn + (rOut - rIn) * tri;
    leds[i] = (r <= edge) ? CRGB(CHSV(hueWheel(a) + drift, 255, 255)) : CRGB::Black;
  }
}

// 2 Fuenf rote Herzen: ein Kranz aus fuenf Herzen, langsam drehend.
void pHearts(CRGB* leds, const Ctx& c) {
  constexpr uint8_t K = 5;
  const float rot = c.t * 0.3f;
  const float ringR = 0.62f;
  const float size = 0.30f;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (c.R[i] > 1.05f) { leds[i] = CRGB::Black; continue; }
    const float x = c.X[i], y = c.Y[i];
    bool on = false;
    for (uint8_t k = 0; k < K; k++) {
      const float ak = k * (TWO_PI_F / K) + rot;
      const float lx = (x - cosf(ak) * ringR) / size;
      const float ly = (y - sinf(ak) * ringR) / size;
      if (fabsf(lx) > 1.4f || fabsf(ly) > 1.4f) continue;
      if (insideHeart(lx, ly)) { on = true; break; }
    }
    leds[i] = on ? CRGB(230, 0, 30) : CRGB::Black;
  }
}

// 3 Nyan Cat: Pixel-Sprite mit Regenbogenschweif (Bitmap, kartesisch abgetastet).
void pNyan(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const char ch = sampleChar(NYAN, 26, 13, c.X[i], c.Y[i], 0.95f, 0.475f);
    leds[i] = nyanColor(ch);
  }
}

// 4 Strichfiguren: zwei grosse laufende Strichmaennchen, Arme/Beine schwingen.
void pStickmen(CRGB* leds, const Ctx& c) {
  const float ph = c.t * 4.0f;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (c.R[i] > 1.1f) { leds[i] = CRGB::Black; continue; }
    const float x = c.X[i], y = c.Y[i];
    CRGB col = CRGB::Black;
    for (uint8_t k = 0; k < 2; k++) {
      const float cx = (k == 0) ? -0.42f : 0.42f;
      const float s = 0.95f;
      const float p = (k == 0) ? ph : ph + PI_F;
      const float lx = x - cx, ly = y;
      if (fabsf(lx) > 0.5f) continue;
      bool hit = false;
      const float hy = 0.34f * s;
      if (lx * lx + (ly - hy) * (ly - hy) < (0.12f * s) * (0.12f * s)) hit = true;   // Kopf
      if (!hit && segDist(lx, ly, 0, 0.22f * s, 0, -0.02f * s) < 0.05f) hit = true;  // Rumpf
      const float sw = 0.13f * s * sinf(p);
      if (!hit && segDist(lx, ly, 0, 0.18f * s, -0.22f * s, 0.04f * s + sw) < 0.045f) hit = true;  // Arme
      if (!hit && segDist(lx, ly, 0, 0.18f * s, 0.22f * s, 0.04f * s - sw) < 0.045f) hit = true;
      if (!hit && segDist(lx, ly, 0, -0.02f * s, -0.16f * s + sw, -0.42f * s) < 0.05f) hit = true; // Beine
      if (!hit && segDist(lx, ly, 0, -0.02f * s, 0.16f * s - sw, -0.42f * s) < 0.05f) hit = true;
      if (hit) { col = (k == 0) ? CRGB(0, 200, 255) : CRGB(255, 80, 200); break; }
    }
    leds[i] = col;
  }
}

// 5 Feuerstrahlen: flackernde Flammen schiessen aus der Mitte nach aussen.
void pFireRays(CRGB* leds, const Ctx& c) {
  constexpr uint8_t N = 9;
  const float seg = TWO_PI_F / N;
  const float rot = c.t * 0.15f;
  const uint32_t bucket = static_cast<uint32_t>(c.t * 16.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f) { leds[i] = CRGB::Black; continue; }
    const float a = angleAt(c, i) - rot;
    float m = a / seg;
    const float ray = floorf(m + 0.5f);
    m -= floorf(m);
    const float d = (m < 0.5f ? m : 1.0f - m) * seg;
    const float halfW = 0.22f;
    if (d > halfW) { leds[i] = CRGB::Black; continue; }
    const float len = 0.6f + 0.4f * rngf(static_cast<uint32_t>(ray) + 1u, bucket, 7u);
    if (r > len) { leds[i] = CRGB::Black; continue; }
    const float fl = 0.6f + 0.4f * rngf(static_cast<uint32_t>(ray) + 1u, bucket, 3u);
    float v = (1.0f - d / halfW) * fl * (1.0f - 0.25f * r);
    if (v > 1.0f) v = 1.0f;
    const uint8_t hue = static_cast<uint8_t>((1.0f - r) * 40.0f);  // Mitte gelb, Spitze rot
    leds[i] = CHSV(hue, 255, static_cast<uint8_t>(v * 255.0f));
  }
}

// 6 Smiley: gefuelltes gelbes Gesicht mit Augen und laechelndem Mund.
void pSmiley(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float x = c.X[i], y = c.Y[i], r = c.R[i];
    if (r > 0.92f) { leds[i] = CRGB::Black; continue; }
    const float exl = x + 0.34f, exr = x - 0.34f, ey = y - 0.28f;
    if (exl * exl + ey * ey < 0.12f * 0.12f || exr * exr + ey * ey < 0.12f * 0.12f) {
      leds[i] = CRGB::Black; continue;                                   // Augen
    }
    if (fabsf(r - 0.55f) < 0.09f && y < -0.02f) { leds[i] = CRGB::Black; continue; }  // Mund
    leds[i] = CRGB(255, 210, 0);
  }
}

// 7 Peace-Zeichen: Ring mit senkrechter Linie und zwei unteren Diagonalen.
void pPeace(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float x = c.X[i], y = c.Y[i], r = c.R[i];
    bool on = false;
    if (fabsf(r - 0.82f) < 0.08f) on = true;                            // Ring
    else if (fabsf(x) < 0.07f && r < 0.86f) on = true;                  // senkrechte Linie
    else if (y < 0.03f && segDist(x, y, 0, 0, -0.58f, -0.58f) < 0.07f) on = true;   // Diagonalen
    else if (y < 0.03f && segDist(x, y, 0, 0, 0.58f, -0.58f) < 0.07f) on = true;
    leds[i] = on ? CRGB(235, 235, 245) : CRGB::Black;
  }
}

// 8 ANTIFA: der Schriftzug, Buchstaben abwechselnd rot und weiss.
void pAntifa(CRGB* leds, const Ctx& c) {
  const uint8_t* G[6] = {GLYPH_A, GLYPH_N, GLYPH_T, GLYPH_I, GLYPH_F, GLYPH_A};
  const float hx = 0.95f, hy = 0.19f;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float x = c.X[i], y = c.Y[i];
    if (x < -hx || x > hx || y < -hy || y > hy) { leds[i] = CRGB::Black; continue; }
    const int u = static_cast<int>((hx - x) / (2.0f * hx) * 35.0f);   // horizontal gespiegelt (Leserichtung)
    const int v = static_cast<int>((hy - y) / (2.0f * hy) * 7.0f);
    if (u < 0 || u > 34 || v < 0 || v > 6) { leds[i] = CRGB::Black; continue; }
    const int letter = u / 6, lx = u % 6;                              // 6 = 5 Glyph + 1 Luecke
    if (letter > 5 || lx >= 5 || !(G[letter][v] & (1 << (4 - lx)))) {
      leds[i] = CRGB::Black; continue;
    }
    leds[i] = (letter & 1) ? CRGB(255, 255, 255) : CRGB(255, 0, 0);
  }
}

// 9 Weisse Spirale: ein breiter, weisser Spiralarm dreht langsam.
void pSpiral(CRGB* leds, const Ctx& c) {
  const float twist = 4.0f;
  const float spin = c.t * 0.8f;
  const float W = 0.36f;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const float r = c.R[i];
    if (r > 1.0f || r < 0.03f) { leds[i] = CRGB::Black; continue; }
    float ph = angleAt(c, i) - spin - r * twist;
    ph -= TWO_PI_F * floorf(ph / TWO_PI_F + 0.5f);   // auf [-PI,PI] falten
    const float d = fabsf(ph);
    if (d > W) { leds[i] = CRGB::Black; continue; }
    const uint8_t v = static_cast<uint8_t>((1.0f - 0.55f * (d / W)) * 255.0f);
    leds[i] = CRGB(v, v, v);
  }
}

}  // namespace

const char* Patterns::name(uint8_t p) {
  switch (p) {
    case 0:  return "Funken";
    case 1:  return "Stern";
    case 2:  return "Herzen";
    case 3:  return "Nyan Cat";
    case 4:  return "Strichmaenner";
    case 5:  return "Feuerstrahlen";
    case 6:  return "Smiley";
    case 7:  return "Peace";
    case 8:  return "ANTIFA";
    case 9:  return "Spirale";
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
  // Flaechig gefuellte Bilder ziehen viel Strom (Stern, Nyan Cat, Smiley).
  return p == 1 || p == 3 || p == 6;
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
    case 1:  pStar(leds, ctx); break;
    case 2:  pHearts(leds, ctx); break;
    case 3:  pNyan(leds, ctx); break;
    case 4:  pStickmen(leds, ctx); break;
    case 5:  pFireRays(leds, ctx); break;
    case 6:  pSmiley(leds, ctx); break;
    case 7:  pPeace(leds, ctx); break;
    case 8:  pAntifa(leds, ctx); break;
    case 9:  pSpiral(leds, ctx); break;
    default: pSparkle(leds, ctx); break;
  }

  // Positioniertes Bild: alles ausserhalb des Einheitskreises schwarz.
  if (settings.imageMode) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (R[i] > 1.0f) leds[i] = CRGB::Black;
    }
  }
}
