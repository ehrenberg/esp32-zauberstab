#include "WebInterface.h"
#include "WebUI.h"
#include "Patterns.h"
#include "PatternStore.h"
#include "PhotoStore.h"

WebInterface::WebInterface(Settings& settings, LedController& leds, PovRenderer& renderer, AppMode& mode)
  : server(80), settings(settings), leds(leds), renderer(renderer), mode(mode) {}

void WebInterface::begin(ActionCallback startCallback, ActionCallback stopCallback) {
  if (serverActive) return;
  onStart = startCallback;
  onStop = stopCallback;

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Zauberstab", "zauberstab42");
  routes();
  server.begin();
  serverActive = true;
  lastStationCount = 0;

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void WebInterface::stop() {
  if (!serverActive) return;
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  serverActive = false;
}

void WebInterface::handle() {
  if (!serverActive) return;
  server.handleClient();

  // Neue WLAN-Verbindung am AP -> 2x kurz blinken als Quittung.
  uint8_t n = WiFi.softAPgetStationNum();
  if (n > lastStationCount) leds.showWifiConnected();
  lastStationCount = n;
}

bool WebInterface::active() const { return serverActive; }

void WebInterface::routes() {
  server.on("/", [this]() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/state", [this]() { handleState(); });
  server.on("/api/status", [this]() { handleStatus(); });
  server.on("/api/settings", HTTP_POST, [this]() { handleSettings(); });
  server.on("/api/select", HTTP_POST, [this]() { handleSelect(); });
  server.on("/api/text", HTTP_POST, [this]() { handleText(); });
  server.on("/api/draw", HTTP_GET, [this]() { handleDrawGet(); });
  server.on("/api/draw", HTTP_POST, [this]() { handleDrawPost(); });
  server.on("/api/photo", HTTP_GET, [this]() { handlePhotoGet(); });
  server.on("/api/photo", HTTP_POST, [this]() { handlePhotoPost(); });
  server.on("/api/clear", HTTP_POST, [this]() { handleClear(); });
  server.on("/api/frame", [this]() { handleFrame(); });
  server.on("/api/start", HTTP_POST, [this]() { handleStart(); });
  server.on("/api/stop", HTTP_POST, [this]() { handleStop(); });
  server.onNotFound([this]() { server.send_P(200, "text/html", INDEX_HTML); });
}

static String hexColor(const CRGB& c) {
  char b[8];
  snprintf(b, sizeof(b), "#%02X%02X%02X", c.r, c.g, c.b);
  return String(b);
}

static String jsonEscape(const String& in) {
  String o;
  o.reserve(in.length() + 4);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c >= 0x20) o += c;
  }
  return o;
}

String WebInterface::stateJson() {
  String j = "{";
  j += "\"mode\":\"" + String(modeName(mode)) + "\"";
  j += ",\"patternMode\":" + String(settings.patternMode);
  j += ",\"pattern\":" + String(settings.selectedPattern);
  j += ",\"customSlot\":" + String(settings.customSlot);
  j += ",\"textColor\":" + String(PatternStore::getTextColor());
  j += ",\"text\":\"" + jsonEscape(PatternStore::getText()) + "\"";
  j += ",\"imageSlot\":" + String(settings.imageSlot);
  j += ",\"imgCols\":" + String(IMG_COLS);
  j += ",\"imgRows\":" + String(IMG_ROWS);
  j += ",\"photoSlots\":[";
  for (uint8_t i = 0; i < PHOTO_SLOTS; i++) {
    if (i) j += ",";
    j += PhotoStore::hasPhoto(i) ? "1" : "0";
  }
  j += "]";

  j += ",\"settings\":{";
  j += "\"bright\":" + String(settings.brightness);
  j += ",\"columns\":" + String(settings.povColumns);
  j += ",\"blur\":" + String(settings.motionBlur);
  j += ",\"persist\":" + String(settings.angularPersistence);
  j += ",\"current\":" + String(settings.currentLimitMa);
  j += ",\"holdus\":" + String(settings.maxColumnHoldUs);
  j += ",\"gain\":" + String(settings.angleGain, 2);
  j += ",\"threshold\":" + String(settings.gyroThreshold, 2);
  j += ",\"axis\":" + String(settings.gyroAxis);
  j += ",\"invert\":" + String(settings.invertDirection ? 1 : 0);
  j += ",\"plock\":" + String(settings.phaseLock ? 1 : 0);
  j += ",\"imode\":" + String(settings.imageMode ? 1 : 0);
  j += ",\"iang\":" + String(settings.imageAngleDeg);
  j += ",\"irad\":" + String(settings.imageRadius);
  j += ",\"iscale\":" + String(settings.imageScale);
  j += "}";

  j += ",\"patterns\":[";
  for (uint8_t i = 0; i < Patterns::COUNT; i++) {
    if (i) j += ",";
    j += "\"" + String(Patterns::name(i)) + "\"";
  }
  j += "]";

  j += ",\"palette\":[";
  for (uint8_t i = 0; i < PALETTE_SIZE; i++) {
    if (i) j += ",";
    j += "\"" + hexColor(PatternStore::paletteColor(i)) + "\"";
  }
  j += "]}";
  return j;
}

void WebInterface::handleState() {
  server.send(200, "application/json", stateJson());
}

void WebInterface::handleStatus() {
  String j = "{";
  j += "\"mode\":\"" + String(modeName(mode)) + "\"";
  j += ",\"rpm\":" + String(renderer.rpm(), 1);
  j += ",\"outHz\":" + String(renderer.outputsPerSecond());
  j += ",\"effCols\":" + String(renderer.effectiveColumns());
  j += ",\"rotating\":" + String(renderer.isRotationActive() ? "true" : "false");
  j += ",\"locked\":" + String(renderer.isLocked() ? "true" : "false");
  j += ",\"sampleHz\":" + String(renderer.sampleHz(), 0);
  j += ",\"fails\":" + String(renderer.i2cFails());
  j += ",\"locks\":" + String(renderer.lockEvents());
  j += ",\"rej\":" + String(renderer.rejects());
  j += ",\"errAvg\":" + String(renderer.errAvgDeg(), 1);
  j += ",\"errMax\":" + String(renderer.errMaxDeg(), 1);
  j += ",\"rpmMax\":" + String(renderer.rpmMaxSession(), 1);
  j += ",\"hzMin\":" + String(renderer.hzMinSession(), 0);
  j += ",\"heap\":" + String(ESP.getFreeHeap());
  j += ",\"uptime\":" + String(millis() / 1000);
  j += "}";
  server.send(200, "application/json", j);
}

void WebInterface::handleSettings() {
  if (server.hasArg("bright")) settings.brightness = server.arg("bright").toInt();
  if (server.hasArg("columns")) settings.povColumns = server.arg("columns").toInt();
  if (server.hasArg("blur")) settings.motionBlur = server.arg("blur").toInt();
  if (server.hasArg("persist")) settings.angularPersistence = server.arg("persist").toInt();
  if (server.hasArg("current")) settings.currentLimitMa = server.arg("current").toInt();
  if (server.hasArg("holdus")) settings.maxColumnHoldUs = server.arg("holdus").toInt();
  if (server.hasArg("gain")) settings.angleGain = server.arg("gain").toFloat();
  if (server.hasArg("threshold")) settings.gyroThreshold = server.arg("threshold").toFloat();
  if (server.hasArg("axis")) settings.gyroAxis = server.arg("axis").toInt();
  if (server.hasArg("invert")) settings.invertDirection = server.arg("invert").toInt() != 0;
  if (server.hasArg("plock")) settings.phaseLock = server.arg("plock").toInt() != 0;
  if (server.hasArg("imode")) settings.imageMode = server.arg("imode").toInt() != 0;
  if (server.hasArg("iang")) settings.imageAngleDeg = server.arg("iang").toInt();
  if (server.hasArg("irad")) settings.imageRadius = server.arg("irad").toInt();
  if (server.hasArg("iscale")) settings.imageScale = server.arg("iscale").toInt();

  clampSettings(settings);
  saveSettings(settings);
  leds.apply(settings);
  server.send(200, "application/json", stateJson());
}

void WebInterface::handleSelect() {
  uint8_t m = server.hasArg("mode") ? server.arg("mode").toInt() : 0;
  uint8_t idx = server.hasArg("index") ? server.arg("index").toInt() : 0;
  settings.patternMode = m;
  if (m == PATTERN_MODE_BUILTIN) {
    settings.selectedPattern = idx;
  } else if (m == PATTERN_MODE_CUSTOM) {
    settings.customSlot = idx;
    PatternStore::loadSlot(idx);
  } else if (m == PATTERN_MODE_IMAGE) {
    settings.imageSlot = idx;
  }
  clampSettings(settings);
  saveSettings(settings);
  server.send(200, "application/json", "{\"ok\":1}");
}

void WebInterface::handleText() {
  String t = server.hasArg("text") ? server.arg("text") : "";
  uint8_t col = server.hasArg("color") ? server.arg("color").toInt() : settings.textColor;
  settings.textColor = col;
  PatternStore::setText(t, col);
  saveSettings(settings);
  server.send(200, "application/json", "{\"ok\":1}");
}

void WebInterface::handleDrawGet() {
  uint8_t slot = server.hasArg("slot") ? server.arg("slot").toInt() : 0;
  String data;
  PatternStore::getSlotBase64(slot, data);
  server.send(200, "application/json", "{\"data\":\"" + data + "\"}");
}

void WebInterface::handleDrawPost() {
  uint8_t slot = server.hasArg("slot") ? server.arg("slot").toInt() : 0;
  String body = server.arg("plain");
  bool ok = PatternStore::saveSlotBase64(slot, body);
  server.send(200, "application/json", ok ? "{\"ok\":1}" : "{\"ok\":0}");
}

void WebInterface::handlePhotoGet() {
  String j = "{\"slots\":[";
  for (uint8_t i = 0; i < PHOTO_SLOTS; i++) {
    if (i) j += ",";
    j += PhotoStore::hasPhoto(i) ? "1" : "0";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

void WebInterface::handlePhotoPost() {
  uint8_t slot = server.hasArg("slot") ? server.arg("slot").toInt() : 0;
  if (server.hasArg("clear")) {
    PhotoStore::clearPhoto(slot);
    server.send(200, "application/json", "{\"ok\":1}");
    return;
  }
  bool ok = PhotoStore::savePhotoBase64(slot, server.arg("plain"));
  server.send(200, "application/json", ok ? "{\"ok\":1}" : "{\"ok\":0}");
}

void WebInterface::handleClear() {
  uint8_t slot = server.hasArg("slot") ? server.arg("slot").toInt() : 0;
  PatternStore::clearSlot(slot);
  server.send(200, "application/json", "{\"ok\":1}");
}

void WebInterface::handleFrame() {
  static uint8_t buf[PREVIEW_COLS * NUM_LEDS * 3];
  size_t n = 0;
  for (uint8_t col = 0; col < PREVIEW_COLS; col++) {
    Patterns::render(scratch, settings, TWO_PI_F * col / PREVIEW_COLS, col, PREVIEW_COLS);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      buf[n++] = scratch[i].r;
      buf[n++] = scratch[i].g;
      buf[n++] = scratch[i].b;
    }
  }
  String data = PatternStore::base64Encode(buf, n);
  String j = "{\"cols\":" + String(PREVIEW_COLS) + ",\"leds\":" + String(NUM_LEDS) + ",\"data\":\"" + data + "\"}";
  server.send(200, "application/json", j);
}

void WebInterface::handleStart() {
  server.send(200, "application/json", "{\"ok\":1}");
  if (onStart) onStart();
}

void WebInterface::handleStop() {
  server.send(200, "application/json", "{\"ok\":1}");
  if (onStop) onStop();
}
