#pragma once

#include "config.h"
#include "Settings.h"

// Mustererzeugung im column x radius Raum. Eingebaute Muster werden echt
// kartesisch gerendert (r = LED-Index, theta = Spaltenwinkel); Custom- und
// Text-Muster liegen im PatternStore.
class Patterns {
public:
  static constexpr uint8_t COUNT = PATTERN_BUILTIN_COUNT;
  static const char* name(uint8_t pattern);
  // true = Muster leuchtet einen grossen Teil des Streifens gleichzeitig. Solche
  // Muster ziehen viel Strom; FastLEDs Limiter dimmt dann global herunter und
  // das Bild wirkt flau. Die Web-UI markiert sie.
  static bool isHeavy(uint8_t pattern);
  // Wieviele Winkelspalten der aktive Modus mindestens braucht, damit keine
  // Bildspalte verschluckt wird (Text-Glyphen, Zeichengitter, Foto-Spalten).
  // 0 = keine Vorgabe (eingebaute Muster sind stetig in theta).
  static uint16_t nativeColumns(const Settings& settings);
  // theta = echter Drehwinkel (rad) fuer eingebaute Muster; column/columns
  // werden nur von Custom-/Text-Mustern (Bitmap-Spaltenindex) benutzt.
  // tMs = Animationszeit, pro Umdrehung gelatcht (sonst schert das Bild).
  static void render(CRGB* leds, const Settings& settings, float theta, uint8_t column, uint8_t columns, uint32_t tMs);
};
