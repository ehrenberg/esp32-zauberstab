#include "AppMode.h"

const char* modeName(AppMode mode) {
  switch (mode) {
    case MODE_BOOT: return "BOOT";
    case MODE_CALIBRATING: return "CALIBRATING";
    case MODE_IDLE: return "IDLE";
    case MODE_DISPLAY: return "DISPLAY";
    case MODE_SETUP: return "SETUP";
    case MODE_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}
