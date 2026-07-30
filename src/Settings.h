#pragma once

#include "config.h"

struct Settings {
  float gyroThreshold = 1.0f;
  uint8_t povColumns = 72;   // Winkelaufloesung; mehr = schaerfer, aber dunklere Striche
  uint8_t brightness = 100;
  uint16_t currentLimitMa = 1600;
  float angleGain = 1.0f;
  uint16_t maxColumnHoldUs = 3500;  // Sensor: max Haltezeit; Zeit-Modus: Dauer je Spalte
  uint8_t motionBlur = 0;
  uint8_t angularPersistence = 1;
  bool timeMode = false;   // Zeit-Modus: Spalten mit fester Taktung, Sensor ignoriert
  bool invertDirection = false;
  uint8_t gyroAxis = 1;       // 0 = X, 1 = Y, 2 = Z
  bool phaseLock = false;     // Schwerkraft-Lock saettigt ab ~2,4 U/s -> aus, reines Gyro ist kohaerenter
  bool autoGain = false;      // angleGain aus den Phase-Lock-Fehlern selbst einregeln (braucht phaseLock)

  // Musterauswahl
  uint8_t patternMode = PATTERN_MODE_BUILTIN;  // 0 builtin, 1 custom, 2 text
  uint8_t selectedPattern = 0;                 // eingebauter Index (0 = Komet)
  uint8_t customSlot = 0;                      // aktiver Zeichen-Slot
  uint8_t textColor = 2;                       // Palettenindex fuer Text
  uint8_t imageSlot = 0;                       // aktiver Foto-Slot

  // Positioniertes Bild ("kleines stilles Bild im Kreis" statt Vollkreis)
  bool imageMode = false;         // false: Motiv fuellt den ganzen Kreis (Standard), true: kleines Bild oben
  uint16_t imageAngleDeg = 270;   // Winkelposition des Bildzentrums (oben justieren)
  uint8_t imageRadius = 55;       // Abstand Bildzentrum vom Scheibenzentrum (%)
  uint8_t imageScale = 60;        // Bildgroesse (%)

  // Stab-Modus: Stab wird nicht gedreht, sondern gehalten. Der Streifen laeuft
  // dann als lineares, bewegungsreaktives Lichtspiel (WandPatterns) statt als POV.
  bool wandMode = false;          // true: Stab-Modus statt POV
  uint8_t wandPattern = 0;        // ausgewaehlter Stab-Effekt
};

void clampSettings(Settings& settings);
void loadSettings(Settings& settings);
void saveSettings(const Settings& settings);
void printSettings(const Settings& settings);
