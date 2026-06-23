#include "PatternStore.h"
#include <Preferences.h>

namespace PatternStore {

// ---- Farbpalette ---------------------------------------------------------
static const CRGB PALETTE[PALETTE_SIZE] = {
  CRGB(0, 0, 0),      CRGB(255, 255, 255), CRGB(255, 0, 0),   CRGB(0, 255, 0),
  CRGB(0, 0, 255),    CRGB(255, 255, 0),   CRGB(0, 255, 255), CRGB(255, 0, 255),
  CRGB(255, 120, 0),  CRGB(150, 0, 255),   CRGB(255, 80, 160),CRGB(160, 255, 0),
  CRGB(0, 200, 160),  CRGB(0, 160, 255),   CRGB(255, 200, 0), CRGB(120, 120, 120)
};

CRGB paletteColor(uint8_t index) { return PALETTE[index & 0x0F]; }

// ---- 5x7 Font (zeilenbasiert, Bit4 = linke Spalte) -----------------------
static const char FONT_CHARS[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.!?-:";
static const uint8_t FONT[][7] = {
  {0,0,0,0,0,0,0},                                              // space
  {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110},    // 0
  {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110},    // 1
  {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111},    // 2
  {0b11111,0b00010,0b00100,0b00010,0b00001,0b10001,0b01110},    // 3
  {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010},    // 4
  {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110},    // 5
  {0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110},    // 6
  {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000},    // 7
  {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110},    // 8
  {0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100},    // 9
  {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001},    // A
  {0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110},    // B
  {0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110},    // C
  {0b11100,0b10010,0b10001,0b10001,0b10001,0b10010,0b11100},    // D
  {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111},    // E
  {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000},    // F
  {0b01110,0b10001,0b10000,0b10111,0b10001,0b10001,0b01111},    // G
  {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001},    // H
  {0b01110,0b00100,0b00100,0b00100,0b00100,0b00100,0b01110},    // I
  {0b00111,0b00010,0b00010,0b00010,0b00010,0b10010,0b01100},    // J
  {0b10001,0b10010,0b10100,0b11000,0b10100,0b10010,0b10001},    // K
  {0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111},    // L
  {0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001},    // M
  {0b10001,0b11001,0b10101,0b10011,0b10001,0b10001,0b10001},    // N
  {0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110},    // O
  {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000},    // P
  {0b01110,0b10001,0b10001,0b10001,0b10101,0b10010,0b01101},    // Q
  {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001},    // R
  {0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110},    // S
  {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100},    // T
  {0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110},    // U
  {0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100},    // V
  {0b10001,0b10001,0b10001,0b10101,0b10101,0b11011,0b10001},    // W
  {0b10001,0b10001,0b01010,0b00100,0b01010,0b10001,0b10001},    // X
  {0b10001,0b10001,0b01010,0b00100,0b00100,0b00100,0b00100},    // Y
  {0b11111,0b00001,0b00010,0b00100,0b01000,0b10000,0b11111},    // Z
  {0,0,0,0,0,0b00110,0b00110},                                  // .
  {0b00100,0b00100,0b00100,0b00100,0b00100,0,0b00100},          // !
  {0b01110,0b10001,0b00001,0b00010,0b00100,0,0b00100},          // ?
  {0,0,0,0b11111,0,0,0},                                        // -
  {0,0b00110,0b00110,0,0b00110,0b00110,0}                       // :
};

static const uint8_t* glyphFor(char c) {
  if (c >= 'a' && c <= 'z') c -= 32;
  for (uint8_t i = 0; FONT_CHARS[i] != '\0'; i++) {
    if (FONT_CHARS[i] == c) return FONT[i];
  }
  return FONT[0];  // space
}

// ---- Zustand -------------------------------------------------------------
static uint8_t cache[CUSTOM_BYTES];
static int cachedSlot = -1;
static String textStr = "POV";
static uint8_t textColorIndex = 2;

static void slotKey(uint8_t slot, char* out) {
  out[0] = 's';
  out[1] = '0' + slot;
  out[2] = '\0';
}

// ---- Base64 --------------------------------------------------------------
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64Encode(const uint8_t* data, size_t len) {
  String out;
  out.reserve(((len + 2) / 3) * 4 + 1);
  size_t i = 0;
  while (i + 3 <= len) {
    uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
    out += B64[(n >> 18) & 63];
    out += B64[(n >> 12) & 63];
    out += B64[(n >> 6) & 63];
    out += B64[n & 63];
    i += 3;
  }
  size_t rem = len - i;
  if (rem == 1) {
    uint32_t n = data[i] << 16;
    out += B64[(n >> 18) & 63];
    out += B64[(n >> 12) & 63];
    out += '=';
    out += '=';
  } else if (rem == 2) {
    uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
    out += B64[(n >> 18) & 63];
    out += B64[(n >> 12) & 63];
    out += B64[(n >> 6) & 63];
    out += '=';
  }
  return out;
}

static int b64Value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

size_t base64Decode(const String& in, uint8_t* out, size_t maxOut) {
  uint32_t buf = 0;
  int bits = 0;
  size_t n = 0;
  for (size_t i = 0; i < in.length(); i++) {
    int v = b64Value(in[i]);
    if (v < 0) continue;  // '=' und Whitespace ueberspringen
    buf = (buf << 6) | v;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      if (n < maxOut) out[n++] = (buf >> bits) & 0xFF;
    }
  }
  return n;
}

// ---- Slots ---------------------------------------------------------------
void loadSlot(uint8_t slot) {
  if (slot >= CUSTOM_SLOTS) return;
  char key[3];
  slotKey(slot, key);
  Preferences p;
  p.begin("draw", true);
  size_t got = p.getBytes(key, cache, CUSTOM_BYTES);
  p.end();
  if (got != CUSTOM_BYTES) memset(cache, 0, CUSTOM_BYTES);
  cachedSlot = slot;
}

static void ensureCached(uint8_t slot) {
  if (cachedSlot != static_cast<int>(slot)) loadSlot(slot);
}

void getSlotBase64(uint8_t slot, String& out) {
  ensureCached(slot);
  out = base64Encode(cache, CUSTOM_BYTES);
}

bool saveSlotBase64(uint8_t slot, const String& b64) {
  if (slot >= CUSTOM_SLOTS) return false;
  static uint8_t buf[CUSTOM_BYTES];
  size_t n = base64Decode(b64, buf, CUSTOM_BYTES);
  if (n != CUSTOM_BYTES) return false;
  char key[3];
  slotKey(slot, key);
  Preferences p;
  p.begin("draw", false);
  p.putBytes(key, buf, CUSTOM_BYTES);
  p.end();
  if (cachedSlot == static_cast<int>(slot)) memcpy(cache, buf, CUSTOM_BYTES);
  return true;
}

void clearSlot(uint8_t slot) {
  if (slot >= CUSTOM_SLOTS) return;
  static uint8_t buf[CUSTOM_BYTES];
  memset(buf, 0, CUSTOM_BYTES);
  char key[3];
  slotKey(slot, key);
  Preferences p;
  p.begin("draw", false);
  p.putBytes(key, buf, CUSTOM_BYTES);
  p.end();
  if (cachedSlot == static_cast<int>(slot)) memset(cache, 0, CUSTOM_BYTES);
}

void renderCustom(CRGB* leds, uint8_t slot, uint8_t column, uint8_t columns) {
  ensureCached(slot);
  uint16_t cx = static_cast<uint16_t>(column) * CUSTOM_COLS / columns;
  if (cx >= CUSTOM_COLS) cx = CUSTOM_COLS - 1;
  uint16_t base = cx * NUM_LEDS;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint16_t cell = base + i;
    uint8_t byte = cache[cell >> 1];
    uint8_t nibble = (cell & 1) ? (byte & 0x0F) : (byte >> 4);
    leds[i] = PALETTE[nibble];
  }
}

// ---- Text ----------------------------------------------------------------
void setText(const String& text, uint8_t colorIndex) {
  textStr = text.substring(0, TEXT_MAX);
  textColorIndex = colorIndex & 0x0F;
  Preferences p;
  p.begin("draw", false);
  p.putString("text", textStr);
  p.putUChar("tcol", textColorIndex);
  p.end();
}

const String& getText() { return textStr; }
uint8_t getTextColor() { return textColorIndex; }

void renderText(CRGB* leds, uint8_t column, uint8_t columns) {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  const uint8_t len = textStr.length();
  if (len == 0) return;

  const uint16_t totalCols = static_cast<uint16_t>(len) * 6;  // 5 + 1 Luecke
  uint16_t x = static_cast<uint16_t>(column) * totalCols / columns;
  if (x >= totalCols) x = totalCols - 1;

  const uint8_t charIndex = x / 6;
  const uint8_t colInChar = x % 6;
  if (charIndex >= len || colInChar >= 5) return;  // Luecke zwischen Zeichen

  const uint8_t* g = glyphFor(textStr[charIndex]);
  const CRGB color = PALETTE[textColorIndex];

  const uint8_t ledsPerRow = NUM_LEDS / 8;            // ~8
  const uint8_t offset = (NUM_LEDS - ledsPerRow * 7) / 2;

  for (uint8_t r = 0; r < 7; r++) {
    if (g[r] & (1 << (4 - colInChar))) {
      uint8_t start = offset + r * ledsPerRow;
      for (uint8_t k = 0; k < ledsPerRow; k++) {
        uint8_t led = start + k;
        if (led < NUM_LEDS) leds[led] = color;
      }
    }
  }
}

// ---- Init ----------------------------------------------------------------
void begin(uint8_t activeSlot) {
  Preferences p;
  p.begin("draw", true);
  if (p.isKey("text")) textStr = p.getString("text", textStr);
  textColorIndex = p.getUChar("tcol", textColorIndex);
  p.end();
  loadSlot(activeSlot < CUSTOM_SLOTS ? activeSlot : 0);
}

}  // namespace PatternStore
