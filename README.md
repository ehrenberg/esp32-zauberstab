# Zauberstab — POV-Poi-Stab

Ein Persistence-of-Vision-Stab auf Basis eines Seeed XIAO ESP32-C3. Beim Schleudern
zeichnet der 65-LED-Streifen ein stehendes Bild in die Luft. Muster, Text,
eigene Zeichnungen und Handyfotos lassen sich über eine eingebaute Weboberfläche
einstellen — ohne App, ohne Internet.

## Hardware

| Teil | Anschluss | Hinweis |
|---|---|---|
| Seeed XIAO ESP32-C3 | — | |
| SK9822 / APA102 LED-Strip, 65 LEDs | DI → GPIO5, CI → GPIO4 | getakteter SPI-Strip, kein WS2812B |
| MPU6050 (Gyro + Accel) | I2C → GPIO6/7 | Standardadresse 0x68 |
| Taster | GPIO10 gegen GND | interner Pull-up |

Der SPI-Takt steht bewusst auf **8 MHz**. Bei 24 MHz koppelten die Takt-/Datenleitungen
so stark in den I2C-Bus des MPU6050 ein, dass tausende Sensor-Reads fehlschlugen und
der Winkel unbrauchbar wurde. 8 MHz sind immer noch ~6× schneller als der alte
WS2812B-Aufbau. Nur erhöhen, wenn `I2C-Fehler` im DEV-Tab sauber bleibt.

## Bauen und flashen

```bash
pio run              # bauen
pio run -t upload    # flashen
pio device monitor   # Serielle Ausgabe (115200 Baud)
```

## Bedienung

Drei Modi, gesteuert über den einen Taster:

| Zustand | Kurzdruck | Langdruck (2 s) |
|---|---|---|
| **IDLE** (3 blaue LEDs) | Anzeige starten | Setup |
| **DISPLAY** (Schleudern) | nächstes eingebautes Muster | Setup |
| **SETUP** (Regenbogen) | zurück zu IDLE | — |

Beim Booten kalibriert sich der Gyro — **Stab dabei still halten**. Ein Komet läuft
währenddessen über den Streifen. Wird der Taster beim Einschalten gehalten, geht es
direkt in den Setup-Modus.

### Weboberfläche

Im Setup-Modus spannt der Stab einen WLAN-Accesspoint auf:

- **SSID:** `Zauberstab`
- **Passwort:** `zauberstab42`
- **Adresse:** http://192.168.4.1

Beim Verbinden blinkt der Stab zweimal cyan. Die Oberfläche hat sechs Reiter:

- **Muster** — 20 eingebaute Muster mit Rundvorschau. `⚡` markiert Muster, die viele
  LEDs gleichzeitig leuchten lassen; das Stromlimit dimmt dann global herunter.
- **Malen** — vier Zeichen-Slots, 48 × 65 Pixel, 16-Farben-Palette.
- **Text** — bis 32 Zeichen, 5×7-Font (Großbuchstaben, Ziffern, `. ! ? - :`).
- **Foto** — vier Foto-Slots. Das Bild wird im Browser rund zugeschnitten und in ein
  Polarraster umgerechnet, erst dann hochgeladen.
- **Setup** — Helligkeit, Auflösung, Bildposition, Sensorparameter, Nachkalibrierung.
- **DEV** — Live-Telemetrie und Diagnose der letzten Schleuder-Sitzung.

**Beim Start der Anzeige wird das WLAN abgeschaltet**, damit die volle Rechenzeit in
die POV-Schleife geht. Zurück ins Setup nur über langen Tastendruck.

## Die zwei Anzeigearten

**Vollkreis** (`Bild oben` = aus): Das Muster füllt die ganze Kreisscheibe. Die
Winkelauflösung ist der Regler *POV-Spalten*.

**Positioniertes Bild** (`Bild oben` = ein, Standard): Das Muster erscheint als kleines,
stehendes Bild an einer Stelle des Kreises — einstellbar über *Bildwinkel* (270° = oben),
*Bildhöhe* und *Bildgröße*. Außerhalb des Bildfensters bleibt der Stab dunkel. Weil das
Bild nur einen schmalen Winkelbereich belegt, rendert der Stab dort adaptiv mit bis zu
480 Feinschritten statt mit festen Spalten.

Der Modus gilt nur für eingebaute Muster; Text, Zeichnungen und Fotos laufen immer
über den Vollkreis.

## Wichtige Einstellungen

| Regler | Bedeutung |
|---|---|
| **POV-Spalten** (8–192) | Winkelauflösung im Vollkreis. Mehr = feiner, aber jede Spalte steht kürzer und wirkt dunkler. Text, Zeichnungen und Fotos heben den Wert automatisch auf ihren eigenen Bedarf an. |
| **Angle Gain** | Trimmt die Winkelmessung. Erscheint das Bild mehrfach → zu hoch. Wandert es → zu niedrig. |
| **Gyro-Achse** | Muss zur Einbaulage des Sensors passen. |
| **Stromlimit** | Schützt Akku und Spannungslage. Zu niedrig → FastLED dimmt global. |
| **Phase-Lock** | Schwerkraft-basierte Driftkorrektur. Funktioniert nur unter ~2,4 U/s; beim schnellen Schleudern sättigt der Beschleunigungssensor. Im Zweifel aus lassen. |

## Kalibrieren

Der Gyro-Nullpunkt bestimmt, ob das Bild steht oder langsam wandert. Er wird beim
Booten gemessen — wurde der Stab dabei bewegt, driftet das Bild die ganze Sitzung.
Über **Setup → Sensor neu kalibrieren** lässt er sich jederzeit neu bestimmen; die
Messung läuft im Sensor-Task und startet erst, wenn der Stab wirklich ruht.

Bleibt ein Rest, hilft *Angle Gain* in kleinen Schritten (0,005).

## Architektur

```
MPU6050 ──► MotionSensor ──► PovRenderer ──► Patterns ──► LedController ──► SK9822
            (FreeRTOS-Task,   (Spaltenwahl,   (65 LEDs      (FastLED)
             ~1 kHz, Prio 2)   Adaptivrate)    je Winkel)
```

| Datei | Aufgabe |
|---|---|
| `main.cpp` | Modus-Zustandsautomat, Tastenlogik |
| `config.h` | Pins, Sensorregister, Timing — dokumentiert die Messungen hinter den Werten |
| `MotionSensor` | Gyro-Integration zum Drehwinkel, Phase-Lock, Telemetrie |
| `PovRenderer` | Winkelquantisierung, adaptive Spaltenzahl, Bildfenster |
| `Patterns` | 20 eingebaute Muster im kartesischen Bildraum |
| `PatternStore` | Zeichen-Slots und Text-Font (NVS) |
| `PhotoStore` | Foto-Slots als RGB565-Polarbilder (LittleFS) |
| `WebInterface` / `WebUI.h` | HTTP-API und eingebettete Single-Page-App |

Der Sensor läuft in einem eigenen Task mit höherer Priorität als die POV-Schleife —
so wird der Winkel auch während `FastLED.show()` weitergeführt. Der Core-0-Watchdog
ist deshalb abgeschaltet.

### Muster erweitern

Eine Musterfunktion bekommt die LED-Koordinaten im Bildraum und die Animationszeit:

```cpp
void pMeins(CRGB* leds, const Ctx& c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // c.X[i], c.Y[i] ~ [-1,1], c.R[i] = Abstand zur Bildmitte, c.t = Sekunden
    leds[i] = (fabsf(c.R[i] - 0.5f) < 0.06f) ? CRGB::Cyan : CRGB::Black;
  }
}
```

Dann in `Patterns::name()` und dem `switch` in `Patterns::render()` eintragen und
`PATTERN_BUILTIN_COUNT` in `config.h` anpassen.

`c.t` wird **einmal pro Umdrehung** gelatcht. Würde jede Spalte die aktuelle Zeit
lesen, liefe die Animation innerhalb einer Umdrehung weiter und das Bild erschiene
in sich verdreht statt als bewegtes Ganzes.

Für POV gilt: dünne helle Formen auf Schwarz lesen sich gut, Flächenmuster nicht.
Wer viele LEDs gleichzeitig leuchten lässt, sollte das Muster in `Patterns::isHeavy()`
markieren.

## Fehlersuche

| Symptom | Ursache |
|---|---|
| Bild wandert langsam | Gyro-Drift → neu kalibrieren, dann *Angle Gain* trimmen |
| Bild erscheint doppelt | *Angle Gain* zu hoch |
| Text/Bild spiegelverkehrt | *Richtung invertieren* |
| Bild flackert oder ist flau | Stromlimit greift → Helligkeit runter oder Muster ohne `⚡` |
| Rot und Blau vertauscht | Kanalordnung in `LedController::begin()` von `BGR` auf `RGB`/`GRB` |
| Stab blinkt rot | MPU6050 nicht gefunden — Verkabelung GPIO6/7 prüfen |
| Viele `I2C-Fehler` im DEV-Tab | Einkopplung vom LED-Strip → `LED_DATA_RATE_MHZ` senken |
| Text unleserlich | Zu wenig Spalten bei niedriger Drehzahl — schneller schleudern |
