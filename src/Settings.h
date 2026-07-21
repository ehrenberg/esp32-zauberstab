#pragma once

#include "config.h"

struct Settings {
  float gyroThreshold = 1.0f;
  uint8_t povColumns = 46;  // SK9822 hat Reserve; mehr Spalten = schaerfer, aber dunklere POV-Striche
  uint8_t brightness = 100;
  uint16_t currentLimitMa = 1600;
  float angleGain = 1.0f;
  uint16_t maxColumnHoldUs = 6000;
  uint8_t motionBlur = 0;
  uint8_t angularPersistence = 1;
  bool invertDirection = false;
  uint8_t gyroAxis = 1;       // 0 = X, 1 = Y, 2 = Z
  bool phaseLock = false;     // Schwerkraft-Lock saettigt ab ~2,4 U/s -> aus, reines Gyro ist kohaerenter

  // Musterauswahl
  uint8_t patternMode = PATTERN_MODE_BUILTIN;  // 0 builtin, 1 custom, 2 text
  uint8_t selectedPattern = 0;                 // eingebauter Index (0 = Komet)
  uint8_t customSlot = 0;                      // aktiver Zeichen-Slot
  uint8_t textColor = 2;                       // Palettenindex fuer Text
  uint8_t imageSlot = 0;                       // aktiver Foto-Slot

  // Positioniertes Bild ("kleines stilles Bild im Kreis" statt Vollkreis)
  bool imageMode = true;          // true: Bild an einer Stelle, false: ganzer Kreis
  uint16_t imageAngleDeg = 270;   // Winkelposition des Bildzentrums (oben justieren)
  uint8_t imageRadius = 55;       // Abstand Bildzentrum vom Scheibenzentrum (%)
  uint8_t imageScale = 40;        // Bildgroesse (%)
};

void clampSettings(Settings& settings);
void loadSettings(Settings& settings);
void saveSettings(const Settings& settings);
void printSettings(const Settings& settings);
