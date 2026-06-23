#pragma once

#include "Settings.h"
#include "AppMode.h"
#include "LedController.h"
#include "PovRenderer.h"
#include <WiFi.h>
#include <WebServer.h>

typedef void (*ActionCallback)();

class WebInterface {
public:
  WebInterface(Settings& settings, LedController& leds, PovRenderer& renderer, AppMode& mode);
  void begin(ActionCallback onStart, ActionCallback onStop);
  void stop();
  void handle();
  bool active() const;

private:
  WebServer server;
  Settings& settings;
  LedController& leds;
  PovRenderer& renderer;
  AppMode& mode;
  ActionCallback onStart = nullptr;
  ActionCallback onStop = nullptr;
  bool serverActive = false;
  uint8_t lastStationCount = 0;
  CRGB scratch[NUM_LEDS];

  void routes();
  void handleState();
  void handleStatus();
  void handleSettings();
  void handleSelect();
  void handleText();
  void handleDrawGet();
  void handleDrawPost();
  void handlePhotoGet();
  void handlePhotoPost();
  void handleClear();
  void handleFrame();
  void handleStart();
  void handleStop();

  String stateJson();
};
