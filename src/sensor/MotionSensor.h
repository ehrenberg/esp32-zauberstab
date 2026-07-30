#pragma once

#include "config.h"
#include "Settings.h"
#include "LedController.h"
#include <Wire.h>

// MotionSensor laeuft in einem eigenen FreeRTOS-Task und integriert den
// Drehwinkel kontinuierlich (~1 kHz) mit exaktem micros()-dt. Dadurch laeuft
// die Winkelmessung auch waehrend des blockierenden FastLED.show() weiter.
// Optionaler Gravitations-Phase-Lock korrigiert die Drift einmal pro Umdrehung.
class MotionSensor {
public:
  bool begin();
  void calibrate(LedController& leds, const Settings& settings);
  void startTask(Settings* settings);
  void resetAngle();

  float angle() const;        // integrierter Winkel [0, 2π)
  float speedRad() const;     // |Drehgeschwindigkeit| rad/s (tiefpassgefiltert)
  float rpm() const;
  bool isRotating() const;
  bool isLocked() const;      // Phase-Lock greift gerade
  float gyroRad() const;      // letzter Rohwert rad/s (Debug)
  float signedRate() const;   // Drehrate im Winkelraum, mit Vorzeichen (rad/s)

  // ---- Stab-Modus-Telemetrie ------------------------------------------------
  // Nur aktiv, wenn cfg->wandMode gesetzt ist (dann liest der Task zusaetzlich
  // eine Beschleunigungsachse). Liefert Bewegungssignale fuer die Lichtspiele,
  // wenn der Stab gehalten statt gedreht wird.
  float wandEnergy() const;   // Betrag der Bewegung, 0..1
  float wandTilt() const;     // langsame Neigung entlang einer Achse, ~[-1,1]
  float wandShake() const;    // kurze, harte Bewegungsspitzen, ~0..1

  float offset() const;
  float sampleRateHz() const;
  uint32_t failedReads() const;
  uint32_t lockEvents() const;
  uint32_t rejects() const;
  float errAvgDeg() const;
  float errMaxDeg() const;
  float rpmMaxSession() const;
  float hzMinSession() const;
  uint32_t dropouts() const;  // Abbrueche der Drehungserkennung pro Sitzung
  void printFastStatus();

  // Nachkalibrierung im laufenden Betrieb. Der Gyro-Offset wird sonst nur beim
  // Booten bestimmt - wurde der Stab dabei bewegt, driftet das Bild die ganze
  // Sitzung. Die Messung laeuft im Sensor-Task selbst (kein zweiter I2C-Nutzer)
  // und startet erst, wenn der Stab ruht.
  void requestCalibration();
  bool isCalibrating() const;

  // true, sobald die automatische Gain-Kalibrierung den Wert veraendert hat -
  // dann muss beim Verlassen des Display-Modus gespeichert werden.
  bool gainChanged() const;
  void clearGainChanged();

  // Diagnose einer Schleuder-Sitzung (im RAM gesammelt, ins NVS gesichert).
  void resetDiag();         // beim Display-Start
  void saveDiag();          // beim Verlassen des Displays
  void loadDiag();          // beim Booten
  void printDiagSummary();  // 1x/s ausserhalb des Display-Modus

private:
  bool initDirect();
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegister(uint8_t reg, uint8_t& value);
  bool readWord(uint8_t reg, int16_t& value);
  void step();
  void phaseLockStep(uint32_t now);
  static void taskTrampoline(void* arg);

  Settings* cfg = nullptr;

  volatile float angleRad = 0.0f;
  volatile float speed = 0.0f;
  volatile float lastGyroRad = 0.0f;
  volatile float signedRateRad = 0.0f;
  volatile bool rotating = false;
  volatile bool locked = false;
  volatile bool resetRequested = false;

  // Nachkalibrierung (laeuft im Sensor-Task)
  volatile bool calibRequested = false;
  volatile bool calibBusy = false;
  volatile bool gainAdjusted = false;
  uint16_t calibCount = 0;
  float calibSum = 0.0f;

  float gyroOffset = 0.0f;
  uint32_t lastStepUs = 0;

  // Stab-Modus: langsame Neigung (EMA) und schnelle Abweichung (Shake) einer
  // Accel-Achse. Werden nur im Stab-Modus fortgeschrieben.
  volatile float wandTiltG = 0.0f;
  volatile float wandShakeG = 0.0f;

  // Rotations-Hysterese
  uint32_t aboveSinceUs = 0;
  uint32_t belowSinceUs = 0;

  // Phase-Lock
  float accelDC = 0.0f;
  bool dcInit = false;
  bool armed = false;
  uint32_t lastEventUs = 0;
  uint32_t lastLockOkUs = 0;

  // Statistik
  uint32_t samplesSinceRate = 0;
  uint32_t lastRateAt = 0;
  float sampleRate = 0.0f;
  uint32_t readFailures = 0;
  uint32_t lastSerialAt = 0;

  // Phase-Lock-Diagnose
  volatile uint32_t dbgLockEvents = 0;   // akzeptierte Lock-Ereignisse
  volatile uint32_t dbgRejects = 0;      // Schmitt feuerte, Abstand unplausibel
  volatile float dbgLastErrDeg = 0.0f;   // Winkelfehler bei letztem Lock (Grad)
  volatile float dbgErrAbsSum = 0.0f;    // Summe |Fehler| (fuer Mittelwert)
  volatile float dbgErrAbsMax = 0.0f;    // groesster |Fehler| der Sitzung
  volatile float dbgRpmMax = 0.0f;       // hoechste Drehzahl der Sitzung
  volatile float dbgHzMin = 0.0f;        // niedrigste Sample-Rate der Sitzung
  volatile uint32_t dbgDropouts = 0;     // wie oft die Drehung als "ruht" galt
  uint32_t stoppedSinceUs = 0;
  uint32_t lastDiagAt = 0;
};
