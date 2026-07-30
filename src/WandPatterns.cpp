#include "WandPatterns.h"
#include <FastLED.h>
#include <math.h>

namespace {

// Gebuendelte Eingaben eines Frames. t ist eine intern mitlaufende Zeit (nur fuer
// Farbwanderung/Flacker), dt die Frame-Dauer fuer die Bewegungsintegration.
// first = true beim ersten Frame nach einem Effektwechsel -> Zustand neu setzen.
struct In {
  float t;
  float dt;
  float energy;
  float swing;
  float tilt;
  float shake;
  bool first;
};

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float pos(uint16_t i) { return static_cast<float>(i) / (NUM_LEDS - 1); }

inline void blendMax(CRGB& dst, const CRGB& src) {
  if (src.r > dst.r) dst.r = src.r;
  if (src.g > dst.g) dst.g = src.g;
  if (src.b > dst.b) dst.b = src.b;
}

// Weicher Leuchtpunkt an Position p (0..1) mit linearem Abfall ueber radius LEDs.
// Additiv (Max-Blend), damit sich mehrere Punkte sauber ueberlagern.
void addDot(CRGB* leds, float p, const CRGB& col, float radius) {
  if (radius < 0.5f) radius = 0.5f;
  const float ctr = p * (NUM_LEDS - 1);
  int lo = static_cast<int>(floorf(ctr - radius));
  int hi = static_cast<int>(ceilf(ctr + radius));
  if (lo < 0) lo = 0;
  if (hi > NUM_LEDS - 1) hi = NUM_LEDS - 1;
  for (int i = lo; i <= hi; i++) {
    const float d = fabsf(i - ctr) / radius;
    if (d > 1.0f) continue;
    CRGB c = col;
    c.nscale8_video(static_cast<uint8_t>((1.0f - d) * 255.0f));
    blendMax(leds[i], c);
  }
}

// ===========================================================================
//  Stab-Effekte - kinetisch und verspielt. Wenige helle Punkte in Bewegung,
//  die aufeinander zulaufen, kollidieren, jagen oder unter "Schwerkraft"
//  bouncen. Der Streifen wird direkt betrachtet, nicht geschleudert.
// ===========================================================================

// 0 Kollision: zwei Punkte laufen von beiden Enden aufeinander zu, treffen sich
// mit einem hellen Blitz in der Mitte und laufen wieder auseinander. Wedeln
// beschleunigt das Ganze.
void eKollision(CRGB* leds, const In& in) {
  static float ph;
  if (in.first) ph = 0.0f;
  ph += in.dt * (1.2f + in.energy * 4.5f);
  const float u = 0.5f - 0.5f * cosf(ph);          // 0 (Enden) .. 1 (Mitte)
  const uint8_t hue = static_cast<uint8_t>(in.t * 20.0f);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  addDot(leds, u * 0.5f, CHSV(hue, 255, 255), 1.7f);
  addDot(leds, 1.0f - u * 0.5f, CHSV(hue + 128, 255, 255), 1.7f);
  const float flash = powf(u, 8.0f);               // scharf nur beim Treffen
  if (flash > 0.02f) {
    CRGB w = CRGB::White; w.nscale8_video(static_cast<uint8_t>(flash * 255.0f));
    addDot(leds, 0.5f, w, 2.0f + flash * 6.0f);
  }
}

// 1 Pong: ein Ball springt zwischen den Enden. Die NEIGUNG wirkt als Schwerkraft
// - kippt man den Stab, rollt der Ball bergab und prellt am Ende auf (Blitz).
void ePong(CRGB* leds, const In& in) {
  static float p, v, flash;
  if (in.first) { p = 0.5f; v = 0.6f; flash = 0.0f; }
  v += in.tilt * 2.6f * in.dt;                     // Schwerkraft aus Neigung
  if (in.shake > 0.3f) v += (v >= 0.0f ? 1.0f : -1.0f) * in.shake * 0.5f;
  p += v * in.dt;
  if (p < 0.0f) { p = -p; v = fabsf(v) * 0.92f; if (v < 0.4f) v = 0.4f + in.energy * 1.5f; flash = 1.0f; }
  if (p > 1.0f) { p = 2.0f - p; v = -fabsf(v) * 0.92f; if (v > -0.4f) v = -(0.4f + in.energy * 1.5f); flash = 1.0f; }
  if (v > 3.5f) v = 3.5f; else if (v < -3.5f) v = -3.5f;
  flash -= in.dt * 3.0f; if (flash < 0.0f) flash = 0.0f;

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  const uint8_t hue = static_cast<uint8_t>(150.0f - clamp01(fabsf(v) * 0.4f) * 130.0f);  // schnell = rot
  const float tail = -0.06f * (v >= 0.0f ? 1.0f : -1.0f);
  addDot(leds, p + tail, CHSV(hue, 255, 110), 1.2f);
  addDot(leds, p, CHSV(hue, 255, 255), 1.8f);
  if (flash > 0.0f) {
    CRGB w = CRGB::White; w.nscale8_video(static_cast<uint8_t>(flash * 255.0f));
    addDot(leds, clamp01(p), w, 2.0f + flash * 5.0f);
  }
}

// 2 Jagd: zwei Punkte laufen im Kreis (Streifen als Schleife), der blaue jagt den
// orangen und blitzt auf, wenn er ihn einholt. Schwung gibt die Richtung.
void eJagd(CRGB* leds, const In& in) {
  static float a, b;
  if (in.first) { a = 0.0f; b = 0.3f; }
  const float sp = 0.35f + in.energy * 1.8f;
  const float dir = in.swing >= 0.0f ? 1.0f : -1.0f;
  a += in.dt * sp * dir;
  b += in.dt * sp * 1.18f * dir;                   // Jaeger minimal schneller
  const float pa = a - floorf(a);
  const float pb = b - floorf(b);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  addDot(leds, pa - 0.04f * dir, CHSV(20, 255, 110), 1.1f);
  addDot(leds, pb - 0.04f * dir, CHSV(160, 255, 110), 1.1f);
  addDot(leds, pa, CHSV(20, 255, 255), 1.6f);      // orange, verfolgt
  addDot(leds, pb, CHSV(160, 255, 255), 1.6f);     // blau, jagt
  float d = fabsf(pa - pb); if (d > 0.5f) d = 1.0f - d;
  if (d < 0.05f) {
    CRGB w = CRGB::White; w.nscale8_video(static_cast<uint8_t>((1.0f - d / 0.05f) * 255.0f));
    addDot(leds, pb, w, 2.4f);
  }
}

// 3 Komet: ein heller Kopf mit langem Schweif saust den Stab auf und ab. Wedeln
// macht ihn schneller.
void eKomet(CRGB* leds, const In& in) {
  static float ph;
  if (in.first) ph = 0.0f;
  ph += in.dt * (1.0f + in.energy * 3.5f);
  const float u = 0.5f - 0.5f * cosf(ph);
  const float dir = sinf(ph) >= 0.0f ? 1.0f : -1.0f;
  const uint8_t hue = static_cast<uint8_t>(in.t * 16.0f);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  constexpr int TAIL = 11;
  for (int k = TAIL; k >= 1; k--) {
    const float tp = u - dir * k * 0.028f;
    const float v = 1.0f - static_cast<float>(k) / TAIL;
    addDot(leds, tp, CHSV(hue, 255, static_cast<uint8_t>(v * v * 220.0f)), 1.2f);
  }
  addDot(leds, u, CHSV(hue, 220, 255), 1.7f);
}

// 4 Funken: einzelne helle Funken springen ueber den Stab. Ein Schuetteln zuendet
// einen ganzen Schwall.
void eFunken(CRGB* leds, const In& in) {
  const uint32_t bucket = static_cast<uint32_t>(in.t * 14.0f);
  const uint8_t baseHue = static_cast<uint8_t>(in.t * 26.0f);
  const float density = 0.02f + clamp01(in.shake + in.energy * 0.4f) * 0.16f;
  const uint8_t thr = static_cast<uint8_t>(density * 255.0f);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint32_t h = (i * 2654435761u) ^ (bucket * 2246822519u);
    h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    if ((h & 0xFF) >= thr) { leds[i] = CRGB::Black; continue; }
    leds[i] = CHSV(baseHue + static_cast<uint8_t>((h >> 9) & 0x2F), 205, 255);
  }
}

// 5 Regen: Tropfen fallen der Schwerkraft nach (Neigung bestimmt, wo "unten" ist)
// und blitzen beim Aufschlag. Staerkeres Kippen laesst sie schneller fallen.
void eRegen(CRGB* leds, const In& in) {
  struct Drop { float p; uint8_t hue; bool live; };
  static Drop dr[7];
  static float spawn;
  if (in.first) { for (auto& d : dr) d.live = false; spawn = 0.0f; }

  const bool downTop = in.tilt >= 0.0f;            // faellt Richtung Index 0 oder NUM_LEDS?
  const float g = 0.35f + fabsf(in.tilt) * 1.8f + in.energy * 0.5f;
  const float ddir = downTop ? 1.0f : -1.0f;       // +: Richtung Spitze, -: Richtung Griff

  spawn -= in.dt;
  if (spawn <= 0.0f) {
    for (auto& d : dr) {
      if (!d.live) { d.live = true; d.p = downTop ? 0.0f : 1.0f; d.hue = random8(); break; }
    }
    spawn = 0.22f - in.energy * 0.12f; if (spawn < 0.05f) spawn = 0.05f;
  }

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (auto& d : dr) {
    if (!d.live) continue;
    d.p += ddir * g * in.dt;
    if (d.p < 0.0f || d.p > 1.0f) {                // aufgeschlagen -> heller Blitz, dann weg
      addDot(leds, clamp01(d.p), CRGB::White, 2.0f);
      d.live = false;
      continue;
    }
    addDot(leds, d.p - ddir * 0.05f, CHSV(d.hue, 220, 90), 1.0f);  // kurzer Schweif nach oben
    addDot(leds, d.p, CHSV(d.hue, 220, 255), 1.4f);
  }
}

// 6 Magnet: mehrere Punkte werden zu einem Sammelpunkt gezogen (per Neigung
// verschiebbar), treffen sich mit einem Blitz und stieben wieder auseinander -
// das "aufeinander zu"-Spiel.
void eMagnet(CRGB* leds, const In& in) {
  static float ph;
  if (in.first) ph = 0.0f;
  ph += in.dt * (0.8f + in.energy * 2.2f);
  const float u = 0.5f - 0.5f * cosf(ph);          // 0 verteilt .. 1 gesammelt
  float focus = 0.5f + in.tilt * 0.45f;
  focus = clamp01(focus);
  constexpr int N = 5;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int k = 0; k < N; k++) {
    const float home = (k + 0.5f) / N;
    const float p = home + (focus - home) * u;
    addDot(leds, p, CHSV(static_cast<uint8_t>(k * (256 / N) + in.t * 12.0f), 255, 255), 1.5f);
  }
  const float flash = powf(u, 10.0f);
  if (flash > 0.02f) {
    CRGB w = CRGB::White; w.nscale8_video(static_cast<uint8_t>(flash * 255.0f));
    addDot(leds, focus, w, 2.0f + flash * 6.0f);
  }
}

// 7 Pegel: ein VU-Balken vom Griff aufwaerts, dessen Laenge der Bewegung folgt -
// gruen (ruhig) ueber gelb bis rot (wildes Wedeln), mit hellem Spitzenpunkt.
void ePegel(CRGB* leds, const In& in) {
  const float level = clamp01(in.energy + in.shake * 0.5f);
  const float lit = level * NUM_LEDS;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (i >= lit) { leds[i] = CRGB::Black; continue; }
    leds[i] = CHSV(static_cast<uint8_t>(96.0f * (1.0f - pos(i))), 255, 255);
  }
  const int peak = static_cast<int>(lit);
  if (peak > 0 && peak <= NUM_LEDS) leds[peak - 1] = CRGB::White;
}

}  // namespace

const char* WandPatterns::name(uint8_t effect) {
  switch (effect) {
    case 0:  return "Kollision";
    case 1:  return "Pong";
    case 2:  return "Jagd";
    case 3:  return "Komet";
    case 4:  return "Funken";
    case 5:  return "Regen";
    case 6:  return "Magnet";
    case 7:  return "Pegel";
    default: return "?";
  }
}

void WandPatterns::render(CRGB* leds, uint8_t effect, float dt,
                          float energy, float swing, float tilt, float shake) {
  static float gT = 0.0f;
  static uint8_t prev = 255;
  gT += dt;
  In in{gT, dt, energy, swing, tilt, shake, effect != prev};
  prev = effect;
  switch (effect) {
    case 0:  eKollision(leds, in); break;
    case 1:  ePong(leds, in); break;
    case 2:  eJagd(leds, in); break;
    case 3:  eKomet(leds, in); break;
    case 4:  eFunken(leds, in); break;
    case 5:  eRegen(leds, in); break;
    case 6:  eMagnet(leds, in); break;
    case 7:  ePegel(leds, in); break;
    default: eKollision(leds, in); break;
  }
}
