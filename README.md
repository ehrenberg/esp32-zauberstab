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

Gesteuert über den einen Taster — die Aktion entscheidet sich beim Loslassen anhand der
Haltedauer:

| Zustand | Kurzdruck | 2 s halten | 4 s halten |
|---|---|---|---|
| **IDLE** (3 blaue LEDs) | Anzeige starten | Modus wechseln¹ | Setup |
| **DISPLAY** (Schleudern) | nächstes Muster / Lichtspiel | Modus wechseln¹ | Setup |
| **SETUP** (Regenbogen) | zurück zu IDLE | Modus wechseln¹ | — |

¹ Wechselt zwischen **Dreh-Modus** (POV, Stab schleudern) und **Stab-Modus** (Lichtspiele in
der Hand). Quittung: grüner Wisch = Stab-Modus, blauer Wisch = Dreh-Modus. Der Modus lässt
sich auch im Setup-Reiter der Weboberfläche umschalten.

Beim Booten kalibriert sich der Gyro — **Stab dabei still halten**. Ein Komet läuft
währenddessen über den Streifen. Wird der Taster beim Einschalten gehalten, geht es
direkt in den Setup-Modus.

### Weboberfläche

Im Setup-Modus spannt der Stab einen WLAN-Accesspoint auf:

- **SSID:** `Zauberstab`
- **Passwort:** `zauberstab42`
- **Adresse:** http://192.168.4.1

Beim Verbinden blinkt der Stab zweimal cyan. Die Oberfläche hat sechs Reiter:

- **Muster** — 10 eingebaute Motive mit Rundvorschau: *Funken, Stern, Herzen, Nyan Cat,
  Strichmänner, Feuerstrahlen, Smiley, Peace, ANTIFA, Spirale*. Erkennbare Figuren im
  kartesischen Bildraum. (`⚡` markiert die flächig gefüllten *Stern, Nyan Cat* und *Smiley*,
  die das Stromlimit global herunterdimmen können.) Breite Motive (ANTIFA, Nyan Cat) lesen
  sich im Vollkreis am besten, runde (Stern, Smiley, Peace, Herzen) auch im Bild-Modus.
- **Malen** — vier Zeichen-Slots, 48 × 65 Pixel, 16-Farben-Palette.
- **Text** — bis 32 Zeichen, 5×7-Font (Großbuchstaben, Ziffern, `. ! ? - :`).
- **Foto** — vier Foto-Slots. Das Bild wird im Browser rund zugeschnitten und in ein
  Polarraster umgerechnet, erst dann hochgeladen.
- **Setup** — Stab-Modus, Helligkeit, Auflösung, Bildposition, Sensorparameter, Nachkalibrierung.
- **DEV** — Live-Telemetrie und Diagnose der letzten Schleuder-Sitzung.

**Beim Start der Anzeige wird das WLAN abgeschaltet**, damit die volle Rechenzeit in
die POV-Schleife geht. Zurück ins Setup nur über langen Tastendruck.

## Die zwei Anzeigearten

**Vollkreis** (`Bild oben` = aus, Standard): Das Muster füllt die ganze Kreisscheibe. Die
Winkelauflösung ist der Regler *Auflösung (Spalten)*.

**Positioniertes Bild** (`Bild oben` = ein): Das Muster erscheint als kleines,
stehendes Bild an einer Stelle des Kreises — einstellbar über *Bildwinkel* (270° = oben),
*Bildhöhe* und *Bildgröße*. Außerhalb des Bildfensters bleibt der Stab dunkel. Weil das
Bild nur einen schmalen Winkelbereich belegt, rendert der Stab dort adaptiv mit bis zu
480 Feinschritten statt mit festen Spalten.

Der Modus gilt nur für eingebaute Muster; Text, Zeichnungen und Fotos laufen immer
über den Vollkreis.

## Stab-Modus (nicht drehen)

Neben POV gibt es den **Stab-Modus**: der Stab wird *nicht* geschleudert, sondern in
der Hand gehalten und geführt. Statt eines Luftbildes laufen dann bewegungsreaktive
Lichtspiele direkt über den Streifen — ideal zum ruhigen Spielen oder als Glowstick.
Einschalten im Reiter **Setup → Stab-Modus**, das Lichtspiel dort per Auswahl oder im
laufenden Betrieb per Taster wechseln.

Die acht Lichtspiele sind bewusst *kinetisch* — wenige helle Punkte in Bewegung statt
flächiger Füllungen — und reagieren auf Wedeln, Neigen und Schütteln:

| Lichtspiel | Idee | reagiert auf |
|---|---|---|
| **Kollision** | zwei Punkte laufen aufeinander zu, treffen sich mit Blitz | Wedeln → Tempo |
| **Pong** | ein Ball bounct zwischen den Enden | Neigen → rollt bergab; Schütteln → Kick |
| **Jagd** | blauer Punkt jagt orangen im Kreis, blitzt beim Einholen | Schwung → Richtung/Tempo |
| **Komet** | heller Kopf mit Schweif saust auf und ab | Wedeln → Geschwindigkeit |
| **Funken** | einzelne Funken springen | Schütteln → zündet Schwälle |
| **Regen** | Tropfen fallen der Schwerkraft nach, blitzen beim Aufschlag | Neigen → Fallrichtung/-tempo |
| **Magnet** | mehrere Punkte werden zu einem Punkt gezogen, dann stieben sie auseinander | Neigen → Sammelpunkt |
| **Pegel** | VU-Balken (grün→rot) mit Spitzenpunkt | Bewegung → Länge |

Zum Aufhängen als POV-Bild den Stab-Modus einfach wieder ausschalten.

## Wichtige Einstellungen

| Regler | Bedeutung |
|---|---|
| **Auflösung (Spalten)** (16–200) | Winkelauflösung im Vollkreis. Mehr = feiner, aber jede Spalte steht kürzer und wirkt dunkler. Text, Zeichnungen und Fotos heben den Wert automatisch auf ihren eigenen Bedarf an. |
| **Angle Gain** | Trimmt die Winkelmessung. Erscheint das Bild mehrfach → zu hoch. Wandert es → zu niedrig. |
| **Gyro-Achse** | Muss zur Einbaulage des Sensors passen. |
| **Stromlimit** | Schützt Akku und Spannungslage. Zu niedrig → FastLED dimmt global. |
| **Bildgröße** (15–100 %) | Durchmesser des positionierten Bildes. **Kleiner ist nicht schärfer:** der Bildraum wird gestaucht, unter ~15 % ist das Bild schmaler als ~19 LED-Abstände und Formen lassen sich nicht mehr auflösen. Wirkt das Bild grob → vergrößern. |
| **Phase-Lock** | Schwerkraft-basierte Driftkorrektur. Funktioniert nur unter ~2,4 U/s; beim schnellen Schleudern sättigt der Beschleunigungssensor. Im Zweifel aus lassen. |

## Kalibrieren

Der Gyro-Nullpunkt bestimmt, ob das Bild steht oder langsam wandert. Er wird beim
Booten gemessen — wurde der Stab dabei bewegt, driftet das Bild die ganze Sitzung.
Über **Setup → Sensor neu kalibrieren** lässt er sich jederzeit neu bestimmen; die
Messung läuft im Sensor-Task und startet erst, wenn der Stab wirklich ruht.

### Angle Gain

Der Nullpunkt ist nur die halbe Miete — der zweite Fehler ist die *Skala*. Die
MPU6050-Empfindlichkeit streut fertigungsbedingt um ±3 %, was bis zu ~11° Wanderung
pro Umdrehung verursacht. Dafür ist *Angle Gain* da.

**Manuell:** Wandert das Bild entgegen der Drehrichtung um den Bruchteil *f* einer
Umdrehung, *Angle Gain* mit `(1 − f)` multiplizieren; wandert es mit, mit `(1 + f)`.
Ein Zehntel Kreis rückwärts pro Umdrehung → Gain × 0,9.

**Automatisch:** **DEV → Gain selbst einregeln**. Der Phase-Lock liefert über die
Schwerkraft einmal pro Umdrehung einen absoluten Winkelbezug. Der dabei verbleibende
Fehler `err` hängt direkt am Skalenfehler `G`:

```
pro Umdrehung läuft der Winkel um (G−1)·2π davon,
der PLL zieht mit Faktor P zurück
  ⇒ eingeschwungen gilt  err = −(G−1)·2π / P
  ⇒ der Fixpunkt err = 0 ist exakt G = 1
```

Weil der Fixpunkt nicht davon abhängt, wie genau `P` bekannt ist, genügt ein langsamer
Integrator mit richtigem Vorzeichen. Die Regelung braucht **eingeschalteten Phase-Lock**
und **0,6–3 U/s** (ruhiges Drehen von Hand, kein Schleudern) und konvergiert in rund
40 Umdrehungen. Außerhalb des Drehzahlfensters friert sie ein und behält den gelernten
Wert; gespeichert wird beim Verlassen des Display-Modus. Der DEV-Tab zeigt an, warum
sie gerade nicht greift.

Einmal eingeregelt kann der Schalter aus bleiben — der Gain ist sensorindividuell und
ändert sich nicht mehr.

## Architektur

```
MPU6050 ──► MotionSensor ──► PovRenderer ──► Patterns ──► LedController ──► SK9822
            (FreeRTOS-Task,   (Spaltenwahl,   (65 LEDs      (FastLED)
             ~1 kHz, Prio 2)   Adaptivrate)    je Winkel)
```

Der Quellcode liegt nach Modulen in Unterordnern unter `src/` (die Header werden
über Include-Pfade in `platformio.ini` per Basisname gefunden, kein Pfad im `#include`):

| Datei | Aufgabe |
|---|---|
| `src/main.cpp` | Modus-Zustandsautomat, Tastenlogik |
| `src/core/config.h` | Pins, Sensorregister, Timing — dokumentiert die Messungen hinter den Werten |
| `src/core/Settings` · `AppMode` | Einstellungen (NVS) und Betriebszustand |
| `src/sensor/MotionSensor` | Gyro-Integration zum Drehwinkel, Phase-Lock, Telemetrie |
| `src/sensor/ButtonHandler` | Taster-Entprellung und Haltedauer-Logik |
| `src/render/PovRenderer` | Winkelquantisierung, feste Spaltenzahl, Zeit-Modus, Stab-Modus |
| `src/render/Patterns` | 10 eingebaute Motive im kartesischen Bildraum |
| `src/render/WandPatterns` | 8 kinetische Lichtspiele für den Stab-Modus (linear, kein POV) |
| `src/render/LedController` | FastLED-Ausgabe (SK9822, BGR) |
| `src/store/PatternStore` | Zeichen-Slots und Text-Font (NVS) |
| `src/store/PhotoStore` | Foto-Slots als RGB565-Polarbilder (LittleFS) |
| `src/web/WebInterface` · `WebUI.h` | HTTP-API und eingebettete Single-Page-App |

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

Für POV am handgeschleuderten Stab gilt: **viele helle Einzelpunkte auf Schwarz** lesen
sich am besten. Stehende Vollbilder und feine Geometrie verwaschen, weil der Winkel leicht
driftet und das Stromlimit Flächen dimmt. Die eingebauten Muster arbeiten deshalb
partikelbasiert — jeder Bildpunkt wird über einen Hash aus `i`, `c.column` und einem
Zeit-Bucket an-/ausgeknipst (`rng()`), was jede Spalte unabhängig und damit drift-fest
macht. Wer viele LEDs gleichzeitig leuchten lässt, sollte das Muster in `Patterns::isHeavy()`
markieren (nur ein UI-Hinweis; die Helligkeit begrenzt ohnehin immer das Stromlimit).

## Fehlersuche

| Symptom | Ursache |
|---|---|
| Bild wandert langsam | Skalenfehler → *Angle Gain* trimmen oder Auto-Kalibrierung (DEV) |
| Bild wandert entgegen der Drehrichtung | *Angle Gain* zu hoch |
| Bild erscheint doppelt | *Angle Gain* deutlich zu hoch |
| Form nur halb sichtbar (z. B. halbes Kreuz) | Bildgröße zu klein → vergrößern; im DEV-Tab *Auslastung* prüfen |
| Text/Bild spiegelverkehrt | *Richtung invertieren* |
| Bild flackert oder ist flau | Stromlimit greift → Helligkeit runter oder Muster ohne `⚡` |
| Rot und Blau vertauscht | Kanalordnung in `LedController::begin()` von `BGR` auf `RGB`/`GRB` |
| Stab blinkt rot | MPU6050 nicht gefunden — Verkabelung GPIO6/7 prüfen |
| Viele `I2C-Fehler` im DEV-Tab | Einkopplung vom LED-Strip → `LED_DATA_RATE_MHZ` senken |
| Text unleserlich | Zu wenig Spalten bei niedriger Drehzahl — schneller schleudern |
