#include <Arduino.h>
#include "config.h"
#include "Settings.h"
#include "AppMode.h"
#include "LedController.h"
#include "MotionSensor.h"
#include "PovRenderer.h"
#include "WebInterface.h"
#include "ButtonHandler.h"
#include "Patterns.h"
#include "PatternStore.h"
#include "PhotoStore.h"

Settings settings;
AppMode mode = MODE_BOOT;
LedController ledController;
MotionSensor motionSensor;
PovRenderer renderer(settings, ledController, motionSensor);
WebInterface web(settings, ledController, renderer, mode);
ButtonHandler button;

void enterSetupMode();
void startDisplay();
void stopToIdle();
void handleShortPress();
void handleLongPress();

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  button.begin();
  loadSettings(settings);
  PatternStore::begin(settings.customSlot);
  PhotoStore::begin();
#if POV_DEBUG_SERIAL
  printSettings(settings);
#endif

  ledController.begin(settings);
  // Kein weisser Boot-Blitz: spart die grosse Einschalt-Stromspitze (Brownout-Schutz)
  // und sieht ruhiger aus. Die Kalibrier-Animation ist die Startanzeige.

  if (!motionSensor.begin()) {
    mode = MODE_ERROR;
    Serial.println("MPU6050 nicht gefunden");
    return;
  }

  // Immer kalibrieren (Stab still halten), DANN den Sensor-Task starten -
  // so greift kein zweiter Task waehrend der Kalibrierung auf I2C zu.
  mode = MODE_CALIBRATING;
  motionSensor.calibrate(ledController, settings);
  motionSensor.startTask(&settings);
  motionSensor.loadDiag();  // Diagnose der letzten Schleuder-Sitzung bereithalten

  // POV-Loop pollt eng den Winkel; idle-Task wuerde sonst den WDT ausloesen.
  disableCore0WDT();

  if (digitalRead(BUTTON_PIN) == LOW) {
    enterSetupMode();
    return;
  }

  mode = MODE_IDLE;
  ledController.showIdle();
}

void loop() {
  button.update(handleShortPress, handleLongPress);
  web.handle();

  if (mode == MODE_ERROR) {
    ledController.renderErrorBlink();
    return;
  }

  if (mode == MODE_DISPLAY) {
    renderer.render();
#if POV_DEBUG_SERIAL
    motionSensor.printFastStatus();
#endif
    return;
  }

#if POV_DEBUG_SERIAL
  if (mode == MODE_IDLE || mode == MODE_SETUP) motionSensor.printDiagSummary();
#endif

  if (mode == MODE_SETUP) {
    ledController.renderSetupRainbow();
    return;
  }
}

void handleShortPress() {
  if (mode == MODE_IDLE) {
    startDisplay();
    return;
  }

  if (mode == MODE_DISPLAY) {
    // Kurzdruck blaettert durch die eingebauten Muster.
    settings.patternMode = PATTERN_MODE_BUILTIN;
    settings.selectedPattern++;
    if (settings.selectedPattern >= Patterns::COUNT) settings.selectedPattern = 0;

    saveSettings(settings);
    ledController.showPatternChange(settings.selectedPattern);
    renderer.reset();
    return;
  }

  if (mode == MODE_SETUP) {
    stopToIdle();
  }
}

void handleLongPress() {
  enterSetupMode();
}

void startDisplay() {
  web.stop();
  mode = MODE_DISPLAY;
  motionSensor.resetDiag();
  renderer.reset();
  ledController.clear(true);
  Serial.println("Display gestartet");
}

void stopToIdle() {
  motionSensor.saveDiag();
  mode = MODE_IDLE;
  renderer.reset();
  ledController.clear(true);
  ledController.showIdle();
  Serial.println("Display gestoppt");
}

void enterSetupMode() {
  motionSensor.saveDiag();  // Diagnose der gerade beendeten Sitzung sichern
#if POV_DEBUG_SERIAL
  printSettings(settings);
#endif
  mode = MODE_SETUP;
  renderer.reset();
  ledController.clear(true);
  web.begin(startDisplay, stopToIdle);
  ledController.showSetup();
  Serial.println("Setup Modus gestartet");
}
