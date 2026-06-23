#include "ButtonHandler.h"

void ButtonHandler::begin() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void ButtonHandler::update(ButtonCallback onShortPress, ButtonCallback onLongPress) {
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  unsigned long now = millis();

  if (pressed && !buttonDown) {
    buttonDown = true;
    longPressHandled = false;
    buttonDownAt = now;
  }

  if (pressed && buttonDown && !longPressHandled && now - buttonDownAt >= LONG_PRESS_MS) {
    longPressHandled = true;
    if (onLongPress) onLongPress();
  }

  if (!pressed && buttonDown) {
    unsigned long duration = now - buttonDownAt;
    buttonDown = false;

    if (!longPressHandled && duration >= SHORT_PRESS_MIN_MS && duration <= SHORT_PRESS_MAX_MS) {
      if (onShortPress) onShortPress();
    }
  }
}
