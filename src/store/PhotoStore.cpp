#include "PhotoStore.h"
#include "PatternStore.h"  // base64Encode/base64Decode
#include <LittleFS.h>

namespace PhotoStore {

static uint8_t imgCache[IMG_BYTES];
static int cachedImgSlot = -1;
static bool fsReady = false;

static void slotPath(uint8_t slot, char* out, size_t n) {
  snprintf(out, n, "/img%u.565", static_cast<unsigned>(slot));
}

void begin() {
  fsReady = LittleFS.begin(true);  // true = bei Bedarf formatieren
  if (!fsReady) Serial.println("LittleFS mount fehlgeschlagen");
}

bool hasPhoto(uint8_t slot) {
  if (!fsReady || slot >= PHOTO_SLOTS) return false;
  char path[16];
  slotPath(slot, path, sizeof(path));
  return LittleFS.exists(path);
}

static void loadSlot(uint8_t slot) {
  memset(imgCache, 0, IMG_BYTES);
  cachedImgSlot = slot;
  if (!fsReady || slot >= PHOTO_SLOTS) return;
  char path[16];
  slotPath(slot, path, sizeof(path));
  File f = LittleFS.open(path, "r");
  if (!f) return;
  f.read(imgCache, IMG_BYTES);
  f.close();
}

static void ensureCached(uint8_t slot) {
  if (cachedImgSlot != static_cast<int>(slot)) loadSlot(slot);
}

bool savePhotoBase64(uint8_t slot, const String& b64) {
  if (!fsReady || slot >= PHOTO_SLOTS) return false;
  static uint8_t buf[IMG_BYTES];
  size_t n = PatternStore::base64Decode(b64, buf, IMG_BYTES);
  if (n != IMG_BYTES) return false;

  char path[16];
  slotPath(slot, path, sizeof(path));
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  size_t written = f.write(buf, IMG_BYTES);
  f.close();
  if (written != IMG_BYTES) return false;

  if (cachedImgSlot == static_cast<int>(slot)) memcpy(imgCache, buf, IMG_BYTES);
  return true;
}

void getPhotoBase64(uint8_t slot, String& out) {
  ensureCached(slot);
  out = PatternStore::base64Encode(imgCache, IMG_BYTES);
}

void clearPhoto(uint8_t slot) {
  if (!fsReady || slot >= PHOTO_SLOTS) return;
  char path[16];
  slotPath(slot, path, sizeof(path));
  LittleFS.remove(path);
  if (cachedImgSlot == static_cast<int>(slot)) {
    memset(imgCache, 0, IMG_BYTES);
  }
}

void renderImage(CRGB* leds, uint8_t slot, uint8_t column, uint8_t columns) {
  ensureCached(slot);
  // Spalte auf das Polar-Gitter abbilden (wie renderCustom).
  uint16_t cx = static_cast<uint16_t>(column) * IMG_COLS / columns;
  if (cx >= IMG_COLS) cx = IMG_COLS - 1;
  const uint8_t* p = imgCache + static_cast<uint32_t>(cx) * IMG_ROWS * 2;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint16_t v = (static_cast<uint16_t>(p[i * 2]) << 8) | p[i * 2 + 1];
    uint8_t r = (v >> 11) & 0x1F;
    uint8_t g = (v >> 5) & 0x3F;
    uint8_t b = v & 0x1F;
    leds[i] = CRGB((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2));
  }
}

}  // namespace PhotoStore
