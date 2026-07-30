#pragma once

#include "config.h"

typedef void (*ButtonCallback)();

class ButtonHandler {
public:
  void begin();
  // Alle Aktionen werden beim Loslassen anhand der Haltedauer entschieden. Nur so
  // lassen sich 2,5s (Modus-Umschalter) und 6s (Setup) sauber trennen - ein im
  // Halten feuernder 2,5s-Trigger wuerde losgehen, bevor die 6s erreicht sind.
  void update(ButtonCallback onShortPress, ButtonCallback onModeSwitch, ButtonCallback onSetup);

private:
  bool buttonDown = false;
  unsigned long buttonDownAt = 0;
};
