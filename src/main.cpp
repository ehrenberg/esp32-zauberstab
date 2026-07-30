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
#include "WandPatterns.h"
#include "PatternStore.h"
#include "PhotoStore.h"

Settings settings;
AppMode mode = MODE_BOOT;
LedController ledController;
MotionSensor motionSensor;
PovRenderer renderer(settings, ledController, motionSensor);
WebInterface web(settings, ledController, renderer, mode);
ButtonHandler button;

// Im Display-Modus per Taster geaenderte Auswahl: wird erst beim Verlassen
// gesichert, damit kein NVS-Write die POV-Schleife stoert.
bool settingsDirty = false;

void enterSetupMode();
void startDisplay();
void stopToIdle();
void flushSettings();
void handleShortPress();
void handleModeSwitch();
void handleSetup();

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  button.begin();
  loadSettings(settings);
  settings.wandMode = false;  // Start immer im Dreh-Modus (POV); Stab-Modus ist der zweite
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
  button.update(handleShortPress, handleModeSwitch, handleSetup);
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
    // Im Stab-Modus blaettert der Kurzdruck durch die Stab-Lichtspiele.
    if (settings.wandMode) {
      settings.wandPattern++;
      if (settings.wandPattern >= WandPatterns::COUNT) settings.wandPattern = 0;
      settingsDirty = true;
      return;
    }

    // Kurzdruck blaettert durch die eingebauten Muster. Der erste Druck aus
    // einem anderen Modus (Text/Zeichnung/Foto) wechselt nur zu den eingebauten
    // Mustern, ohne die Auswahl zu ueberspringen - vorher ging dabei still die
    // aktive Auswahl verloren.
    if (settings.patternMode != PATTERN_MODE_BUILTIN) {
      settings.patternMode = PATTERN_MODE_BUILTIN;
    } else {
      settings.selectedPattern++;
      if (settings.selectedPattern >= Patterns::COUNT) settings.selectedPattern = 0;
    }

    // Kein saveSettings() und kein delay() im laufenden Betrieb: der NVS-Write
    // blockiert die POV-Schleife sichtbar und nutzt den Flash unnoetig ab.
    // Gesichert wird beim Verlassen des Display-Modus.
    settingsDirty = true;
    renderer.reset();
    return;
  }

  if (mode == MODE_SETUP) {
    stopToIdle();
  }
}

void handleSetup() {
  enterSetupMode();
}

// 2,5s-Halten: zwischen Stab-Modus (Lichtspiele in der Hand) und Dreh-Modus
// (POV) umschalten. Funktioniert in jedem Betriebszustand.
void handleModeSwitch() {
  settings.wandMode = !settings.wandMode;
  ledController.showModeSwitch(settings.wandMode);
  Serial.print("Modus umgeschaltet -> ");
  Serial.println(settings.wandMode ? "Stab-Modus" : "Dreh-Modus (POV)");

  if (mode == MODE_DISPLAY) {
    // Kein NVS-Write in der laufenden Anzeige (blockiert die Schleife) - erst
    // beim Verlassen. Der Renderer nimmt den neuen Modus sofort auf.
    settingsDirty = true;
    renderer.reset();
    ledController.clear(true);
  } else {
    // In IDLE/SETUP ist ein Flash-Write unkritisch -> gleich sichern.
    saveSettings(settings);
    if (mode == MODE_IDLE) ledController.showIdle();
    else if (mode == MODE_SETUP) ledController.showSetup();
  }
}

void startDisplay() {
  web.stop();
  mode = MODE_DISPLAY;
  motionSensor.resetDiag();
  renderer.reset();
  ledController.clear(true);
  Serial.println("Display gestartet");
}

void flushSettings() {
  // Die automatische Gain-Kalibrierung schreibt angleGain im Sensor-Task; der
  // eingeregelte Wert waere sonst beim Ausschalten weg.
  if (motionSensor.gainChanged()) {
    motionSensor.clearGainChanged();
    settingsDirty = true;
  }
  if (!settingsDirty) return;
  settingsDirty = false;
  saveSettings(settings);
}

void stopToIdle() {
  motionSensor.saveDiag();
  flushSettings();
  mode = MODE_IDLE;
  renderer.reset();
  ledController.clear(true);
  ledController.showIdle();
  Serial.println("Display gestoppt");
}

void enterSetupMode() {
  motionSensor.saveDiag();  // Diagnose der gerade beendeten Sitzung sichern
  flushSettings();
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
