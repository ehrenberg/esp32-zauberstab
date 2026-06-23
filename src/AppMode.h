#pragma once

enum AppMode {
  MODE_BOOT,
  MODE_CALIBRATING,
  MODE_IDLE,
  MODE_DISPLAY,
  MODE_SETUP,
  MODE_ERROR
};

const char* modeName(AppMode mode);
