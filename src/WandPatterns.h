#pragma once

#include "config.h"

// Stab-Modus: der Stab wird NICHT gedreht, sondern in der Hand gehalten. Der
// 65-LED-Streifen wird dann direkt (nicht als POV) angezeigt und laeuft als
// lineares Lichtspiel. Die Effekte sind bewusst KINETISCH (wenige, wandernde
// Punkte, die kollidieren/jagen/bouncen) statt flaechig - das wirkt spielerisch.
// Sie integrieren ihre Bewegung selbst ueber dt und reagieren auf vier Signale
// des MotionSensor:
//   energy      - Betrag der Bewegung, 0..1 (Handgelenk-Schwung)
//   swingSigned - Drehrate mit Vorzeichen (rad/s), gibt Richtung
//   tilt        - langsame Neigung entlang einer Achse, ~[-1,1] (Schwerkraft)
//   shake       - kurze, harte Bewegungsspitzen, ~0..1 (Rucke/Schlaege)
// dtSec ist die Zeit seit dem letzten Frame - damit laufen Physik/Animation
// unabhaengig von der Bildrate gleich schnell. Beim Effektwechsel wird der
// interne Zustand automatisch neu gesetzt.
class WandPatterns {
public:
  static constexpr uint8_t COUNT = WAND_PATTERN_COUNT;
  static const char* name(uint8_t effect);
  static void render(CRGB* leds, uint8_t effect, float dtSec,
                     float energy, float swingSigned, float tilt, float shake);
};
