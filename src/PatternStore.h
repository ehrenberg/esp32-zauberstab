#pragma once

#include "config.h"

// Persistente Musterdaten: Farbpalette, eigene Zeichen-Slots (NVS) und Text.
// Eingebaute (prozedurale) Muster liegen in Patterns.cpp.
namespace PatternStore {

void begin(uint8_t activeSlot);             // Palette init + aktiven Slot + Text laden
CRGB paletteColor(uint8_t index);          // Palettenfarbe (0..PALETTE_SIZE-1)

// Eigene Zeichen-Slots (Bitmap im NVS, 4-Bit Palettenindizes)
void loadSlot(uint8_t slot);               // Slot in den RAM-Cache laden
void getSlotBase64(uint8_t slot, String& out);
bool saveSlotBase64(uint8_t slot, const String& b64);
void clearSlot(uint8_t slot);
void renderCustom(CRGB* leds, uint8_t slot, uint8_t column, uint8_t columns);

// Text
void setText(const String& text, uint8_t colorIndex);
const String& getText();
uint8_t getTextColor();
void renderText(CRGB* leds, uint8_t column, uint8_t columns);

// Base64 (auch von WebInterface fuer Frame-Vorschau und PhotoStore genutzt)
String base64Encode(const uint8_t* data, size_t len);
size_t base64Decode(const String& in, uint8_t* out, size_t maxOut);

}  // namespace PatternStore
