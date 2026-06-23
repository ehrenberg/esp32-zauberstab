#pragma once

#include "config.h"

// Vom Handy hochgeladene Fotos: clientseitig in ein Polar-RGB565-Gitter
// (IMG_COLS x NUM_LEDS) umgerechnet, hier nur gespeichert (LittleFS) und als
// Vollscheiben-POV-Bild gerendert. Laeuft parallel zu den 4-Bit-Zeichenslots.
namespace PhotoStore {

void begin();                                          // LittleFS mounten (format-on-fail)
bool savePhotoBase64(uint8_t slot, const String& b64); // RGB565-Gitter ablegen
void getPhotoBase64(uint8_t slot, String& out);
bool hasPhoto(uint8_t slot);
void clearPhoto(uint8_t slot);
void renderImage(CRGB* leds, uint8_t slot, uint8_t column, uint8_t columns);

}  // namespace PhotoStore
