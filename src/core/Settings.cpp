#include "Settings.h"
#include <Preferences.h>

void clampSettings(Settings& settings) {
  if (settings.gyroThreshold < 0.05f) settings.gyroThreshold = 0.05f;
  if (settings.gyroThreshold > 20.0f) settings.gyroThreshold = 20.0f;
  if (settings.povColumns < 8) settings.povColumns = 8;
  // SK9822 schafft locker >300 show()/s -> Spalten sind nicht mehr durch den
  // Strip, sondern durch die Winkelaufloesung begrenzt. Der Renderer drosselt
  // bei niedriger Drehzahl ohnehin adaptiv herunter. Die alte 64er-Grenze stammte
  // vom WS2812B und war zu niedrig fuer Text (bis 192 Spalten) und Fotos (72).
  // Obergrenze der POV-Spalten ist der uint8_t-Bereich (255) - der Renderer und
  // der Custom-/Text-Spaltenindex sind ohnehin auf 255 gedeckelt.
  if (settings.brightness < 1) settings.brightness = 1;
  if (settings.brightness > 100) settings.brightness = 100;
  if (settings.currentLimitMa < 300) settings.currentLimitMa = 300;
  if (settings.currentLimitMa > 2000) settings.currentLimitMa = 2000;
  // angleGain-Clamp bleibt bewusst weit (0,2-3,0): die Auto-Gain-Regelung darf
  // ausserhalb des manuellen UI-Fensters (0,5-1,5) einregeln, ohne beschnitten zu werden.
  if (settings.angleGain < 0.2f) settings.angleGain = 0.2f;
  if (settings.angleGain > 3.0f) settings.angleGain = 3.0f;
  if (settings.maxColumnHoldUs < 200) settings.maxColumnHoldUs = 200;
  if (settings.maxColumnHoldUs > 15000) settings.maxColumnHoldUs = 15000;
  if (settings.motionBlur > 50) settings.motionBlur = 50;
  if (settings.angularPersistence < 1) settings.angularPersistence = 1;
  if (settings.angularPersistence > 6) settings.angularPersistence = 6;
  if (settings.gyroAxis > 2) settings.gyroAxis = 1;
  if (settings.patternMode > PATTERN_MODE_IMAGE) settings.patternMode = PATTERN_MODE_BUILTIN;
  if (settings.selectedPattern >= PATTERN_BUILTIN_COUNT) settings.selectedPattern = 0;
  if (settings.customSlot >= CUSTOM_SLOTS) settings.customSlot = 0;
  if (settings.imageSlot >= PHOTO_SLOTS) settings.imageSlot = 0;
  if (settings.textColor >= PALETTE_SIZE) settings.textColor = 2;
  if (settings.imageAngleDeg > 359) settings.imageAngleDeg %= 360;
  if (settings.imageRadius > 100) settings.imageRadius = 100;
  // Unter ~15 % ist das Bild schmaler als etwa 19 LED-Abstaende - Formen lassen
  // sich dann nicht mehr aufloesen, egal wie fein die Winkelschritte sind.
  if (settings.imageScale < 15) settings.imageScale = 15;
  if (settings.imageScale > 100) settings.imageScale = 100;
  if (settings.wandPattern >= WAND_PATTERN_COUNT) settings.wandPattern = 0;
}

void loadSettings(Settings& settings) {
  Preferences preferences;
  preferences.begin("stab", false);
  settings.gyroThreshold = preferences.getFloat("threshold", settings.gyroThreshold);
  settings.povColumns = preferences.getUChar("columns", settings.povColumns);
  settings.brightness = preferences.getUChar("bright", settings.brightness);
  settings.currentLimitMa = preferences.getUShort("current", settings.currentLimitMa);
  settings.angleGain = preferences.getFloat("gain", settings.angleGain);
  settings.maxColumnHoldUs = preferences.getUShort("holdus", settings.maxColumnHoldUs);
  settings.motionBlur = preferences.getUChar("blur", settings.motionBlur);
  settings.angularPersistence = preferences.getUChar("persist", settings.angularPersistence);
  settings.timeMode = preferences.getBool("tmode", settings.timeMode);
  settings.invertDirection = preferences.getBool("invert", settings.invertDirection);
  settings.gyroAxis = preferences.getUChar("axis", settings.gyroAxis);
  settings.phaseLock = preferences.getBool("plock", settings.phaseLock);
  settings.autoGain = preferences.getBool("again", settings.autoGain);
  settings.patternMode = preferences.getUChar("pmode", settings.patternMode);
  settings.selectedPattern = preferences.getUChar("pattern", settings.selectedPattern);
  settings.customSlot = preferences.getUChar("cslot", settings.customSlot);
  settings.textColor = preferences.getUChar("tcol", settings.textColor);
  settings.imageSlot = preferences.getUChar("islot", settings.imageSlot);
  settings.imageMode = preferences.getBool("imode", settings.imageMode);
  settings.imageAngleDeg = preferences.getUShort("iang", settings.imageAngleDeg);
  settings.imageRadius = preferences.getUChar("irad", settings.imageRadius);
  settings.imageScale = preferences.getUChar("iscale", settings.imageScale);
  settings.wandMode = preferences.getBool("wmode", settings.wandMode);
  settings.wandPattern = preferences.getUChar("wpat", settings.wandPattern);
  preferences.end();
  clampSettings(settings);
}

void printSettings(const Settings& settings) {
  Serial.print("SETTINGS gain=");
  Serial.print(settings.angleGain, 3);
  Serial.print(" cols=");
  Serial.print(settings.povColumns);
  Serial.print(" bright=");
  Serial.print(settings.brightness);
  Serial.print(" cur=");
  Serial.print(settings.currentLimitMa);
  Serial.print(" thr=");
  Serial.print(settings.gyroThreshold, 2);
  Serial.print(" hold=");
  Serial.print(settings.maxColumnHoldUs);
  Serial.print(" blur=");
  Serial.print(settings.motionBlur);
  Serial.print(" persist=");
  Serial.print(settings.angularPersistence);
  Serial.print(" axis=");
  Serial.print(settings.gyroAxis);
  Serial.print(" invert=");
  Serial.print(settings.invertDirection ? 1 : 0);
  Serial.print(" plock=");
  Serial.print(settings.phaseLock ? 1 : 0);
  Serial.print(" pat=");
  Serial.print(settings.selectedPattern);
  Serial.print(" pmode=");
  Serial.print(settings.patternMode);
  Serial.print(" imode=");
  Serial.print(settings.imageMode ? 1 : 0);
  Serial.print(" iang=");
  Serial.print(settings.imageAngleDeg);
  Serial.print(" irad=");
  Serial.print(settings.imageRadius);
  Serial.print(" iscale=");
  Serial.println(settings.imageScale);
}

void saveSettings(const Settings& settings) {
  Preferences preferences;
  preferences.begin("stab", false);
  preferences.putFloat("threshold", settings.gyroThreshold);
  preferences.putUChar("columns", settings.povColumns);
  preferences.putUChar("bright", settings.brightness);
  preferences.putUShort("current", settings.currentLimitMa);
  preferences.putFloat("gain", settings.angleGain);
  preferences.putUShort("holdus", settings.maxColumnHoldUs);
  preferences.putUChar("blur", settings.motionBlur);
  preferences.putUChar("persist", settings.angularPersistence);
  preferences.putBool("tmode", settings.timeMode);
  preferences.putBool("invert", settings.invertDirection);
  preferences.putUChar("axis", settings.gyroAxis);
  preferences.putBool("plock", settings.phaseLock);
  preferences.putBool("again", settings.autoGain);
  preferences.putUChar("pmode", settings.patternMode);
  preferences.putUChar("pattern", settings.selectedPattern);
  preferences.putUChar("cslot", settings.customSlot);
  preferences.putUChar("tcol", settings.textColor);
  preferences.putUChar("islot", settings.imageSlot);
  preferences.putBool("imode", settings.imageMode);
  preferences.putUShort("iang", settings.imageAngleDeg);
  preferences.putUChar("irad", settings.imageRadius);
  preferences.putUChar("iscale", settings.imageScale);
  preferences.putBool("wmode", settings.wandMode);
  preferences.putUChar("wpat", settings.wandPattern);
  preferences.end();
}
