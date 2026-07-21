#pragma once

#include <Arduino.h>

#ifndef FASTLED_ALLOW_INTERRUPTS
#define FASTLED_ALLOW_INTERRUPTS 0
#endif

#ifndef FASTLED_INTERRUPT_RETRY_COUNT
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#endif

#ifndef POV_DEBUG_SERIAL
#define POV_DEBUG_SERIAL 0
#endif

#include <FastLED.h>

// SK9822 (APA102-kompatibel): getakteter SPI-Strip mit separater Daten- und
// Taktleitung. DI -> GPIO5, CI -> GPIO4. Anders als der alte WS2812B braucht
// SK9822 kein bit-genaues 800-kHz-Timing -> show() ist ~20x kuerzer.
constexpr uint8_t LED_DATA_PIN = 5;   // DI
constexpr uint8_t LED_CLOCK_PIN = 4;  // CI
// SK9822-SPI-Takt. 24 MHz auf GPIO4/5 koppelte massiv Stoerungen in den I2C-Bus
// des MPU6050 (GPIO6/7) -> tausende fehlgeschlagene Sensor-Reads, Winkel wurde
// Muell. 8 MHz ist immer noch ~6x schneller als der alte WS2812B (show ~260 us,
// reicht fuer hunderte Spalten) und stoert den Sensor nicht mehr. Nur erhoehen,
// wenn die Diagnose (fails) sauber bleibt.
constexpr uint32_t LED_DATA_RATE_MHZ = 8;
constexpr uint8_t BUTTON_PIN = 10;
constexpr uint16_t NUM_LEDS = 65;
constexpr uint32_t SERIAL_BAUD = 921600;

constexpr uint16_t SHORT_PRESS_MIN_MS = 50;
constexpr uint16_t SHORT_PRESS_MAX_MS = 600;
constexpr uint16_t LONG_PRESS_MS = 2000;

constexpr float TWO_PI_F = 6.28318530718f;
constexpr float PI_F = 3.14159265359f;

// Positioniertes Bild: maximale Winkelschritte pro Umdrehung (Bildschaerfe).
// Real durch Drehzahl & SK9822-show()-Dauer begrenzt (adaptiv). Hoch = scharf.
constexpr uint16_t POV_MAX_FINE_STEPS = 480;

constexpr uint8_t MPU_ADDR = 0x68;
constexpr uint8_t MPU_REG_SMPLRT_DIV = 0x19;
constexpr uint8_t MPU_REG_CONFIG = 0x1A;
constexpr uint8_t MPU_REG_GYRO_CONFIG = 0x1B;
constexpr uint8_t MPU_REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t MPU_REG_FIFO_EN = 0x23;
constexpr uint8_t MPU_REG_INT_ENABLE = 0x38;
constexpr uint8_t MPU_REG_GYRO_XOUT_H = 0x43;
constexpr uint8_t MPU_REG_GYRO_YOUT_H = 0x45;
constexpr uint8_t MPU_REG_GYRO_ZOUT_H = 0x47;
constexpr uint8_t MPU_REG_PWR = 0x6B;
constexpr uint8_t MPU_REG_WHO_AM_I = 0x75;

// MPU6050 auf 100 kHz: Telemetrie zeigte massenhafte I2C-Fehler bei aktiven LEDs
// (Strom-/Stoereinkopplung am Stab). 100 kHz hat deutlich mehr Rausch- und
// Kapazitaetsreserve als 400 kHz; ~500 Hz Samplerate reichen bei langsamem Stab.
constexpr uint32_t MPU_I2C_CLOCK = 100000;
constexpr uint8_t MPU_I2C_TIMEOUT_MS = 1;  // kuerzer -> ein Fehlversuch blockiert die Sample-Rate weniger

// Digitaler Tiefpass (DLPF_CFG). Bei nur ~1 kHz Polling eines langsam gedrehten
// Stabes wuerde der ungefilterte 256-Hz-Pfad (0x00) hochfrequentes Rauschen
// aliasen, das in den integrierten Winkel driftet. 0x03 = 44 Hz Gyro / 5 ms
// Verzoegerung: glaettet Gyro UND Accel (sauberer Phase-Lock), behaelt aber die
// Signaldynamik eines 1-3 U/s-Stabes. 0x02 (98 Hz, ~2,8 ms) statt 0x03, weil
// die kuerzere Gruppenlaufzeit den drehzahlabhaengigen Phasenversatz des
// Phase-Lock kleiner haelt -> stehendes Bild wandert weniger bei Tempowechsel.
constexpr uint8_t MPU_DLPF_CFG = 0x02;

// Vollausschlag ±2000 °/s (FS_SEL=3). Ein geschleuderter Stab erreicht
// 5-20 U/s = 1800-7200 °/s. ±500 °/s wuerde bereits bei 1,4 U/s saettigen!
constexpr uint8_t MPU_GYRO_FS_VALUE = 0x18;  // FS_SEL = 3 -> ±2000 °/s
constexpr float GYRO_SCALE_2000 = 16.4f;     // LSB pro °/s bei ±2000 °/s

// Register-Adresse des Gyro-High-Bytes je Achse (0=X, 1=Y, 2=Z).
constexpr uint8_t GYRO_AXIS_REG[3] = {
  MPU_REG_GYRO_XOUT_H, MPU_REG_GYRO_YOUT_H, MPU_REG_GYRO_ZOUT_H
};

// Beschleunigungssensor (fuer Gravitations-Phase-Lock).
constexpr uint8_t MPU_REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t ACCEL_AXIS_REG[3] = { 0x3B, 0x3D, 0x3F };
constexpr uint8_t MPU_ACCEL_FS_VALUE = 0x18;  // ±16 g: bei 30-40cm-Stab saettigt ±8g schon ab ~2,4 U/s

// Sensor-Task.
constexpr uint32_t SENSOR_TASK_STACK = 4096;
constexpr uint8_t SENSOR_TASK_PRIORITY = 2;   // ueber loopTask (1) -> sampelt auch waehrend show()
constexpr float SENSOR_SPEED_ALPHA = 0.10f;   // Tiefpass fuer Drehgeschwindigkeit
constexpr uint32_t ROT_ON_DELAY_US = 20000;   // Hysterese: so lange ueber Schwelle -> aktiv
constexpr uint32_t ROT_OFF_DELAY_US = 150000; // so lange unter Schwelle -> ruht

// Gravitations-Phase-Lock.
constexpr float PHASE_LOCK_PLL_GAIN = 0.45f;     // Korrektur pro Umdrehung (hoeher = haelt Bild fester)
constexpr int16_t PHASE_LOCK_AC_THRESHOLD = 600; // Schmitt-Schwelle, raw LSB (~0,3 g @ ±16 g)
constexpr float PHASE_LOCK_DC_ALPHA = 0.001f;     // EMA entfernt Zentripetal-DC (~1 s)
constexpr float PHASE_LOCK_MIN_RAD = 4.0f;        // ~0,64 U/s (greift schon bei langsamem Stab)
constexpr float PHASE_LOCK_MAX_RAD = 20.0f;       // ~3,2 U/s: darueber saettigt auch ±16g -> Lock aus, reines Gyro
constexpr float PHASE_LOCK_REF = 0.0f;            // Zielwinkel an der Phasenmarke

constexpr uint32_t SENSOR_SERIAL_INTERVAL_US = 50000;
// Samplezahl der Nachkalibrierung im Betrieb (bei ~1 kHz Task ca. 1,5 s).
constexpr uint16_t CALIB_SAMPLES = 1500;

// ---- Muster-System -------------------------------------------------------
constexpr uint8_t PATTERN_BUILTIN_COUNT = 20;   // eingebaute Muster
constexpr uint8_t PALETTE_SIZE = 16;            // Farbpalette (Editor + Text)
constexpr uint8_t CUSTOM_SLOTS = 4;             // eigene Zeichen-Slots
constexpr uint8_t CUSTOM_COLS = 48;             // Spalten im Zeichengitter
constexpr uint16_t CUSTOM_CELLS = static_cast<uint16_t>(CUSTOM_COLS) * NUM_LEDS;
constexpr uint16_t CUSTOM_BYTES = (CUSTOM_CELLS + 1) / 2;  // 4-Bit gepackt
constexpr uint8_t TEXT_MAX = 32;                // max Zeichen im Text

// Web-Vorschau: im Vollkreis-Modus rendert sie genau povColumns Spalten ueber
// 360 Grad (also exakt das, was der Stab zeigt). Im positionierten Bild-Modus
// deckt das Bild nur ein schmales Winkelfenster ab - dort waeren gleichmaessig
// verteilte Spalten viel zu grob (10 von 40 traefen das Bild). Deshalb wird
// dann nur das Fenster abgetastet, dafuer mit PREVIEW_FINE_COLS Schritten -
// vergleichbar fein wie die adaptiven Schritte des Renderers.
constexpr uint8_t PREVIEW_FINE_COLS = 120;
constexpr uint16_t PREVIEW_MAX_COLS = 192;      // Puffergroesse (>= povColumns-Max und PREVIEW_FINE_COLS)
constexpr uint32_t PREVIEW_BYTES = static_cast<uint32_t>(PREVIEW_MAX_COLS) * NUM_LEDS * 2;  // RGB565

constexpr uint8_t PATTERN_MODE_BUILTIN = 0;
constexpr uint8_t PATTERN_MODE_CUSTOM = 1;
constexpr uint8_t PATTERN_MODE_TEXT = 2;
constexpr uint8_t PATTERN_MODE_IMAGE = 3;

// ---- Foto-System (vom Handy hochgeladene Bilder) -------------------------
constexpr uint8_t PHOTO_SLOTS = 4;             // Foto-Speicherplaetze (LittleFS)
constexpr uint16_t IMG_COLS = 72;              // Winkelspalten des Polar-Bildes
constexpr uint16_t IMG_ROWS = NUM_LEDS;        // eine Reihe pro LED (65)
constexpr uint32_t IMG_BYTES = static_cast<uint32_t>(IMG_COLS) * IMG_ROWS * 2;  // RGB565
