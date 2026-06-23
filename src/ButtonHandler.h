#pragma once

#include "config.h"

typedef void (*ButtonCallback)();

class ButtonHandler {
public:
  void begin();
  void update(ButtonCallback onShortPress, ButtonCallback onLongPress);

private:
  bool buttonDown = false;
  bool longPressHandled = false;
  unsigned long buttonDownAt = 0;
};
