#include "ButtonHandler.h"

void ButtonHandler::begin() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void ButtonHandler::update(ButtonCallback onShortPress, ButtonCallback onModeSwitch, ButtonCallback onSetup) {
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  unsigned long now = millis();

  if (pressed && !buttonDown) {
    buttonDown = true;
    buttonDownAt = now;
  }

  if (!pressed && buttonDown) {
    unsigned long duration = now - buttonDownAt;
    buttonDown = false;

    // Laengste zutreffende Schwelle gewinnt (auf Loslassen ausgewertet).
    if (duration >= SETUP_PRESS_MS) {
      if (onSetup) onSetup();
    } else if (duration >= MODE_SWITCH_PRESS_MS) {
      if (onModeSwitch) onModeSwitch();
    } else if (duration >= SHORT_PRESS_MIN_MS && duration <= SHORT_PRESS_MAX_MS) {
      if (onShortPress) onShortPress();
    }
  }
}
