# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Persistence-of-Vision-Stab auf einem Seeed XIAO ESP32-C3. Der Code ist durchgehend
deutsch kommentiert; neue Kommentare und Commit-Messages ebenfalls auf Deutsch halten.
Die `README.md` erklärt Bedienung, Kalibrierung und Fehlersuche aus Nutzersicht — diese
Datei ergänzt die nicht offensichtlichen Implementierungsentscheidungen.

## Bauen, flashen, debuggen

```bash
pio run                 # bauen
pio run -t upload       # flashen (Upload 921600 Baud)
pio device monitor      # serielle Ausgabe (Monitor 115200 Baud)
```

Es gibt **keine Tests** — validiert wird auf der Hardware über den seriellen Monitor
und den **DEV-Tab** der Weboberfläche (Live-Telemetrie). `platformio.ini` setzt
`-DPOV_DEBUG_SERIAL=1`; das schaltet in `main.cpp`/`MotionSensor` die Serial-Telemetrie
scharf. Für Auslieferung auf 0 setzen — die Prints kosten Rechenzeit in der POV-Schleife.

Zielboard: `seeed_xiao_esp32c3`, Arduino-Framework. Kern-Libs: FastLED, Adafruit MPU6050.

## Datenfluss und Task-Modell

```
MPU6050 ─► MotionSensor ─► PovRenderer ─► Patterns ─► LedController ─► SK9822
           (eigener Task,    (Spaltenwahl,  (65 LEDs     (FastLED, BGR)
            ~1 kHz, Prio 2)   Lead, Fenster) je Winkel)
```

Der **Sensor läuft in einem eigenen FreeRTOS-Task mit Priorität 2** (über loopTask=1).
Das ist die zentrale Architekturentscheidung: `FastLED.show()` blockiert, aber der
Winkel wird währenddessen weiterintegriert. Der `PovRenderer` liest nur den fertigen
Winkel — er integriert nicht selbst. Wegen des eng pollenden POV-Loops ist der
**Core-0-Watchdog abgeschaltet** (`disableCore0WDT()` in `setup()`).

Zustandsautomat (`AppMode`) in `main.cpp`: BOOT → CALIBRATING → IDLE ⇄ DISPLAY,
jederzeit → SETUP (4 s halten). Tasterlogik komplett in `main.cpp`. Der Taster wertet
**auf Loslassen** aus (`ButtonHandler`): Kurzdruck, 2 s = Stab-/Dreh-Modus umschalten
(`handleModeSwitch`, kippt `settings.wandMode`), 4 s = Setup. Auf Loslassen ausgewertet,
weil ein im Halten feuernder 2-s-Trigger sonst losginge, bevor die 4 s erreicht sind.

## Nicht offensichtliche Invarianten (hier brechen Änderungen leicht das Bild)

- **Einmal-pro-Umdrehung-Latching.** Sowohl die Spaltenzahl (`gateStepsLatched` in
  `PovRenderer`) als auch die Animationszeit (`animMs`, an `Patterns::render` als `tMs`)
  werden nur beim Umdrehungswechsel neu gesetzt. Würde man sie pro Spalte aktualisieren,
  springt das Raster mitten in der Umdrehung (Spalten doppelt/verschluckt) bzw. die
  Animation läuft *innerhalb* einer Umdrehung weiter und das Bild erscheint in sich
  verdreht. Umdrehungswechsel = `rawAngle < lastRawAngle - PI` bzw. `nextColumn < currentColumn`.

- **Lead-Kompensation.** Zwischen Winkel-Lesen und Leuchten vergeht eine ganze Frame-Zeit
  (Muster rechnen + show); der C3 hat **keine Hardware-FPU**, atan2f/sqrtf über 65 LEDs
  dominieren. Deshalb `angle += signedRate * frameTimeUs * 1.5e-6`. `frameTimeUs` ist ein
  Tiefpass über die **gemessene Gesamt-Frame-Dauer** (nicht nur show()). Wer nur show()
  misst, plant zu viele Spalten ein und verschmiert das Bild. Gedeckelt auf `POV_MAX_LEAD_RAD`.

- **I2C-Fehler ziehen `lastStepUs` NICHT nach.** Bei fehlgeschlagenem Read kehrt `step()`
  ohne dt-Update zurück, damit der nächste erfolgreiche Schritt das verpasste Intervall
  nachintegriert. Sonst summieren sich tausende Fehler zu systematisch zu kleinem Winkel
  → Bild wandert.

- **Reset des Winkels auf 0 nur nach echtem Stillstand** (`ROT_RESET_AFTER_US`, 1,5 s),
  nicht bei jedem kurzen Unterschreiten der Schwelle. Beim Handdrehen fällt die Momentan-
  geschwindigkeit regelmäßig kurz ab; ein Reset würde die Phase zerstören.

- **Settings werden im DISPLAY-Modus NICHT nach NVS geschrieben.** Ein `saveSettings()`
  blockiert die POV-Schleife sichtbar und nutzt den Flash ab. Änderungen (Musterwechsel
  per Taster, Auto-Gain) setzen `settingsDirty` bzw. `gainChanged()`; `flushSettings()`
  schreibt erst beim Verlassen des Display-Modus.

- **WLAN wird beim Display-Start abgeschaltet** (`web.stop()` in `startDisplay()`), damit
  die volle Rechenzeit in die POV-Schleife geht.

## Die zwei Render-Pfade

`positioned = imageMode && patternMode == BUILTIN`:

- **Vollkreis** (positioned=false, oder Custom/Text/Foto): `gateSteps` = `povColumns`,
  hochgezogen auf `Patterns::nativeColumns()` (Text/Zeichnung/Foto haben eigenen Bedarf),
  gedeckelt auf `revPeriodUs / frameTimeUs` (was in der verfügbaren Zeit machbar ist).

- **Positioniertes Bild** (positioned=true): Muster erscheint nur im Winkelfenster um
  `imageAngleDeg`. Fensterhalbbreite `windowHalf = asin(scale/radius) + 0,25`. Außerhalb
  wird nur Schwarz ausgegeben (fast kostenlos), also steht das ganze Zeitbudget dem
  Fenster zur Verfügung → adaptive Feinauflösung bis `POV_MAX_FINE_STEPS` (480).

Eingebaute Muster (`Patterns.cpp`) sind stetig in `theta` (echter Drehwinkel).
Custom/Text/Foto nutzen einen **Spaltenindex** (`custCol`/`columns`) statt eines Winkels —
Bitmaps aus `PatternStore` (NVS) bzw. `PhotoStore` (LittleFS, RGB565-Polarbilder).

## Der dritte Pfad: Stab-Modus (kein POV)

`settings.wandMode` schaltet auf ein **lineares** Lichtspiel um: der Stab wird gehalten,
nicht gedreht. `PovRenderer::render()` zweigt ganz oben nach `renderWand()` ab (noch vor
der `isRotating`-Prüfung) und ruft mit fester Bildrate (~120 fps) `WandPatterns::render()`.
Die Effekte sind 1D über die 65 LEDs und reagieren auf drei Bewegungssignale, die der
**Sensor-Task nur bei `wandMode` liefert** (sonst kostet der zusätzliche Accel-Read I2C):
`wandTilt` (langsame EMA einer Accel-Achse = Neigung), `wandShake` (schnelle Abweichung
davon) und `wandEnergy` (aus `speed` + Shake). Der Accel-Read liegt in `step()` vor den
`!rotating`-Returns, läuft also unabhängig vom Drehzustand. Neue Stab-Effekte: Funktion in
`WandPatterns.cpp`, in `name()`/`switch` eintragen, `WAND_PATTERN_COUNT` in `config.h`
erhöhen. Der Taster blättert im Display-Modus durch `wandPattern` statt `selectedPattern`;
das 2,5-s-Halten kippt `wandMode` (in DISPLAY deferred gespeichert, sonst sofort ins NVS).

## Kalibrierung: Offset vs. Gain (zwei getrennte Fehler)

- **Gyro-Offset** (Nullpunkt): beim Booten gemessen (`calibrate()`, Stab still), im Betrieb
  nachführbar (`requestCalibration()` läuft im Sensor-Task, nur im Stillstand). Falscher
  Offset → Bild wandert.
- **Angle Gain** (Skala): MPU6050-Empfindlichkeit streut ±3 %. `angleGain` multipliziert
  in die Integration. **Auto-Gain** (`autoGain`, braucht `phaseLock`): der Phase-Lock liefert
  über die Schwerkraft einmal/Umdrehung einen absoluten Bezug; der Restfehler `err` folgt
  `err = -(G-1)·2π/P`, Fixpunkt `err=0 ⇔ G=1`. Deshalb genügt ein langsamer Integrator
  mit richtigem Vorzeichen (`AUTOGAIN_RATE`), unabhängig von der genauen PLL-Verstärkung.
  Phase-Lock/Auto-Gain funktionieren nur bei **0,6–3,2 U/s** (`PHASE_LOCK_MIN/MAX_RAD`);
  darüber sättigt der Beschleunigungssensor.

## Wo Konstanten dokumentiert sind

`config.h` ist die zentrale Referenz — jeder Timing-/Sensorwert trägt die Messung, die
ihn begründet. Besonders heikel:

- `LED_DATA_RATE_MHZ = 8`: bei 24 MHz koppelte der SK9822-Takt (GPIO4/5) massiv in den
  I2C-Bus des MPU6050 (GPIO6/7) → tausende Fehl-Reads. Nur erhöhen, wenn `fails` im
  DEV-Tab sauber bleibt.
- `MPU_I2C_CLOCK = 100000` (bewusst niedrig, mehr Störreserve), `MPU_GYRO_FS_VALUE`
  ±2000 °/s (geschleuderter Stab erreicht 1800–7200 °/s), `MPU_DLPF_CFG = 0x02` (98 Hz,
  Kompromiss aus Anti-Aliasing und kleiner Gruppenlaufzeit für den Phase-Lock).
- LED-Kanalordnung ist **BGR** (in `LedController::begin()`) — bei vertauschtem Rot/Blau dort ändern.

## Ein Muster hinzufügen

1. Funktion `void pMeins(CRGB* leds, const Ctx& c)` in `Patterns.cpp` (c.X/Y/R ~[-1,1],
   c.t = Sekunden, einmal/Umdrehung gelatcht).
2. In `Patterns::name()` und den `switch` in `Patterns::render()` eintragen.
3. `PATTERN_BUILTIN_COUNT` in `config.h` erhöhen.
4. Flächige/helle Muster in `Patterns::isHeavy()` markieren (Stromlimit dimmt sonst global).
   Für POV lesen sich dünne helle Formen auf Schwarz am besten.
