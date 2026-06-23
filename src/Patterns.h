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
  // theta = echter Drehwinkel (rad) fuer eingebaute Muster; column/columns
  // werden nur von Custom-/Text-Mustern (Bitmap-Spaltenindex) benutzt.
  static void render(CRGB* leds, const Settings& settings, float theta, uint8_t column, uint8_t columns);
};
