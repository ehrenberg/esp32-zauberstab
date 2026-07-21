#include "MotionSensor.h"
#include <math.h>
#include <Preferences.h>

bool MotionSensor::begin() {
  Wire.begin();
  Wire.setClock(MPU_I2C_CLOCK);
  Wire.setTimeOut(MPU_I2C_TIMEOUT_MS);
  delay(50);

  for (int i = 0; i < 10; i++) {
    if (initDirect()) {
      lastRateAt = micros();
      return true;
    }
    delay(80);
  }

  return false;
}

void MotionSensor::calibrate(LedController& leds, const Settings& settings) {
  constexpr int samples = 3000;
  const uint8_t reg = GYRO_AXIS_REG[settings.gyroAxis > 2 ? 1 : settings.gyroAxis];
  float sum = 0.0f;
  int validSamples = 0;

  for (int i = 0; i < samples; i++) {
    int16_t raw;
    if (readWord(reg, raw)) {
      sum += (raw / GYRO_SCALE_2000) * PI / 180.0f;
      validSamples++;
    }
    if ((i & 31) == 0) leds.showCalibrationProgress(i, samples);
    delayMicroseconds(125);
  }

  gyroOffset = validSamples > 0 ? sum / validSamples : 0.0f;
  leds.showCalibrationDone();

  Serial.print("Gyro Offset (Achse ");
  Serial.print(settings.gyroAxis);
  Serial.print("): ");
  Serial.println(gyroOffset, 6);
}

void MotionSensor::startTask(Settings* settings) {
  cfg = settings;
  lastStepUs = micros();
  xTaskCreate(taskTrampoline, "sensor", SENSOR_TASK_STACK, this, SENSOR_TASK_PRIORITY, nullptr);
}

void MotionSensor::taskTrampoline(void* arg) {
  MotionSensor* self = static_cast<MotionSensor*>(arg);
  for (;;) {
    self->step();
    vTaskDelay(1);  // ~1 kHz, gibt loopTask und idle Zeit
  }
}

void MotionSensor::resetAngle() {
  resetRequested = true;
}

void MotionSensor::requestCalibration() {
  calibCount = 0;
  calibSum = 0.0f;
  calibBusy = false;
  calibRequested = true;
}

bool MotionSensor::isCalibrating() const { return calibRequested; }

void MotionSensor::step() {
  const uint32_t now = micros();
  uint32_t dtUs = now - lastStepUs;
  lastStepUs = now;

  int16_t gRaw;
  const uint8_t reg = GYRO_AXIS_REG[cfg->gyroAxis > 2 ? 1 : cfg->gyroAxis];
  if (!readWord(reg, gRaw)) {
    readFailures++;
    return;
  }

  const float gyroRad = (gRaw / GYRO_SCALE_2000) * (PI / 180.0f) - gyroOffset;
  lastGyroRad = gyroRad;
  const float aspeed = fabsf(gyroRad);
  speed += (aspeed - speed) * SENSOR_SPEED_ALPHA;

  const float rpmNow = speed * 9.549296586f;
  if (rpmNow > dbgRpmMax) dbgRpmMax = rpmNow;

  // Samplerate-Statistik
  samplesSinceRate++;
  if (now - lastRateAt >= 250000) {
    sampleRate = samplesSinceRate * 1000000.0f / (now - lastRateAt);
    samplesSinceRate = 0;
    lastRateAt = now;
    if (rotating && (dbgHzMin == 0.0f || sampleRate < dbgHzMin)) dbgHzMin = sampleRate;
  }

  // Rotation mit Zeit-Hysterese
  if (aspeed > cfg->gyroThreshold) {
    belowSinceUs = 0;
    if (!rotating) {
      if (aboveSinceUs == 0) aboveSinceUs = now;
      else if (now - aboveSinceUs > ROT_ON_DELAY_US) rotating = true;
    }
  } else {
    aboveSinceUs = 0;
    if (rotating) {
      if (belowSinceUs == 0) belowSinceUs = now;
      else if (now - belowSinceUs > ROT_OFF_DELAY_US) rotating = false;
    }
  }

  if (resetRequested) {
    angleRad = 0.0f;
    resetRequested = false;
    armed = false;
    locked = false;
  }

  if (!rotating) {
    angleRad = 0.0f;

    // Angeforderte Nachkalibrierung: nur im Stillstand mitteln. Der Wert ist
    // bereits offsetbereinigt, der Mittelwert also der verbliebene Restbias.
    if (calibRequested) {
      if (!calibBusy) { calibBusy = true; calibCount = 0; calibSum = 0.0f; }
      if (aspeed < 0.35f) {
        calibSum += gyroRad;
        if (++calibCount >= CALIB_SAMPLES) {
          gyroOffset += calibSum / calibCount;
          calibRequested = false;
          calibBusy = false;
          Serial.print("Nachkalibriert, Offset=");
          Serial.println(gyroOffset, 6);
        }
      } else {
        calibCount = 0;  // Bewegung erkannt -> Messung neu beginnen
        calibSum = 0.0f;
      }
      return;
    }

    // Restbias im Stillstand langsam nachfuehren -> weniger Gyro-Drift pro Session.
    if (aspeed < 0.05f) gyroOffset += gyroRad * 0.002f;
    return;
  }

  // Erster gueltiger Schritt nach Pause: nicht ueber riesiges dt integrieren.
  if (dtUs > 100000) return;
  const float dt = dtUs * 1e-6f;

  const float dir = cfg->invertDirection ? -1.0f : 1.0f;
  float a = angleRad + gyroRad * dir * dt * cfg->angleGain;
  if (a >= TWO_PI_F) a = fmodf(a, TWO_PI_F);
  else if (a < 0.0f) { a = fmodf(a, TWO_PI_F); if (a < 0.0f) a += TWO_PI_F; }
  angleRad = a;

  if (cfg->phaseLock) phaseLockStep(now);
  else locked = false;
}

void MotionSensor::phaseLockStep(uint32_t now) {
  const uint8_t axis = (cfg->gyroAxis + 1) % 3;  // garantiert senkrecht zur Drehachse
  int16_t aRaw;
  if (!readWord(ACCEL_AXIS_REG[axis], aRaw)) return;

  if (!dcInit) { accelDC = aRaw; dcInit = true; }
  const float ac = aRaw - accelDC;
  accelDC += (aRaw - accelDC) * PHASE_LOCK_DC_ALPHA;

  // Nur im sinnvollen Drehzahlfenster (sonst Accel-Saettigung / zu langsam).
  if (speed < PHASE_LOCK_MIN_RAD || speed > PHASE_LOCK_MAX_RAD) {
    armed = false;
    if (now - lastLockOkUs > 600000) locked = false;
    return;
  }

  // Schmitt-Trigger: unter -H scharf, ueber +H ausloesen -> 1 Ereignis/Umdrehung.
  if (ac < -PHASE_LOCK_AC_THRESHOLD) armed = true;
  if (armed && ac > PHASE_LOCK_AC_THRESHOLD) {
    armed = false;
    const uint32_t interval = now - lastEventUs;
    const float revPeriodUs = (TWO_PI_F / speed) * 1e6f;
    lastEventUs = now;

    // Plausibilitaet: Ereignisabstand ~ eine Umdrehung. Weit gefasst, damit der
    // Lock auch bei Tempowechsel (Handschleudern) erhalten bleibt.
    if (interval > revPeriodUs * 0.5f && interval < revPeriodUs * 1.7f) {
      float err = PHASE_LOCK_REF - angleRad;
      if (err > PI) err -= TWO_PI_F;
      else if (err < -PI) err += TWO_PI_F;

      dbgLastErrDeg = err * 57.2957795f;
      dbgLockEvents++;
      const float ae = fabsf(dbgLastErrDeg);
      dbgErrAbsSum += ae;
      if (ae > dbgErrAbsMax) dbgErrAbsMax = ae;

      float a = angleRad + PHASE_LOCK_PLL_GAIN * err;
      if (a < 0.0f) a += TWO_PI_F;
      else if (a >= TWO_PI_F) a -= TWO_PI_F;
      angleRad = a;

      lastLockOkUs = now;
      locked = true;
    } else {
      dbgRejects++;
    }
  }

  if (now - lastLockOkUs > 600000) locked = false;
}

float MotionSensor::angle() const { return angleRad; }
float MotionSensor::speedRad() const { return speed; }
float MotionSensor::rpm() const { return speed * 9.549296586f; }
bool MotionSensor::isRotating() const { return rotating; }
bool MotionSensor::isLocked() const { return locked; }
float MotionSensor::gyroRad() const { return lastGyroRad; }
float MotionSensor::offset() const { return gyroOffset; }
float MotionSensor::sampleRateHz() const { return sampleRate; }
uint32_t MotionSensor::failedReads() const { return readFailures; }
uint32_t MotionSensor::lockEvents() const { return dbgLockEvents; }
uint32_t MotionSensor::rejects() const { return dbgRejects; }
float MotionSensor::errAvgDeg() const { return dbgLockEvents > 0 ? dbgErrAbsSum / dbgLockEvents : 0.0f; }
float MotionSensor::errMaxDeg() const { return dbgErrAbsMax; }
float MotionSensor::rpmMaxSession() const { return dbgRpmMax; }
float MotionSensor::hzMinSession() const { return dbgHzMin; }

void MotionSensor::printFastStatus() {
  uint32_t now = micros();
  if (now - lastSerialAt < SENSOR_SERIAL_INTERVAL_US) return;
  lastSerialAt = now;

  Serial.print("ROT=");
  Serial.print(rotating ? 1 : 0);
  Serial.print(" RPM=");
  Serial.print(rpm(), 1);
  Serial.print(" HZ=");
  Serial.print(sampleRate, 0);
  Serial.print(" LOCK=");
  Serial.print(locked ? 1 : 0);
  Serial.print(" locks=");
  Serial.print(dbgLockEvents);
  Serial.print(" rej=");
  Serial.print(dbgRejects);
  Serial.print(" errDeg=");
  Serial.print(dbgLastErrDeg, 1);
  Serial.print(" FAIL=");
  Serial.println(readFailures);
}

void MotionSensor::resetDiag() {
  dbgLockEvents = 0;
  dbgRejects = 0;
  dbgErrAbsSum = 0.0f;
  dbgErrAbsMax = 0.0f;
  dbgRpmMax = 0.0f;
  dbgHzMin = 0.0f;
  readFailures = 0;
}

void MotionSensor::saveDiag() {
  Preferences p;
  p.begin("diagp", false);
  p.putUInt("locks", dbgLockEvents);
  p.putUInt("rej", dbgRejects);
  p.putFloat("esum", dbgErrAbsSum);
  p.putFloat("emax", dbgErrAbsMax);
  p.putFloat("rpm", dbgRpmMax);
  p.putFloat("hz", dbgHzMin);
  p.putUInt("fail", readFailures);
  p.end();
}

void MotionSensor::loadDiag() {
  Preferences p;
  p.begin("diagp", true);
  dbgLockEvents = p.getUInt("locks", 0);
  dbgRejects = p.getUInt("rej", 0);
  dbgErrAbsSum = p.getFloat("esum", 0.0f);
  dbgErrAbsMax = p.getFloat("emax", 0.0f);
  dbgRpmMax = p.getFloat("rpm", 0.0f);
  dbgHzMin = p.getFloat("hz", 0.0f);
  readFailures = p.getUInt("fail", 0);
  p.end();
}

void MotionSensor::printDiagSummary() {
  uint32_t nowMs = millis();
  if (nowMs - lastDiagAt < 1000) return;
  lastDiagAt = nowMs;

  const float errAvg = dbgLockEvents > 0 ? dbgErrAbsSum / dbgLockEvents : 0.0f;
  Serial.print("DIAG letzte Sitzung -> locks=");
  Serial.print(dbgLockEvents);
  Serial.print(" rej=");
  Serial.print(dbgRejects);
  Serial.print(" errAvg=");
  Serial.print(errAvg, 1);
  Serial.print(" errMax=");
  Serial.print(dbgErrAbsMax, 1);
  Serial.print(" rpmMax=");
  Serial.print(dbgRpmMax, 1);
  Serial.print(" hzMin=");
  Serial.print(dbgHzMin, 0);
  Serial.print(" fails=");
  Serial.println(readFailures);
}

bool MotionSensor::initDirect() {
  uint8_t who = 0;
  if (!readRegister(MPU_REG_WHO_AM_I, who)) return false;
  if (who != 0x68) return false;

  if (!writeRegister(MPU_REG_PWR, 0x01)) return false;
  delay(20);

  if (!writeRegister(MPU_REG_CONFIG, MPU_DLPF_CFG)) return false;  // DLPF gegen Rauschen/Aliasing
  if (!writeRegister(MPU_REG_SMPLRT_DIV, 0x00)) return false;
  if (!writeRegister(MPU_REG_GYRO_CONFIG, MPU_GYRO_FS_VALUE)) return false;   // ±2000 °/s
  if (!writeRegister(MPU_REG_ACCEL_CONFIG, MPU_ACCEL_FS_VALUE)) return false; // ±8 g
  if (!writeRegister(MPU_REG_FIFO_EN, 0x00)) return false;
  if (!writeRegister(MPU_REG_INT_ENABLE, 0x00)) return false;

  return true;
}

bool MotionSensor::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool MotionSensor::readRegister(uint8_t reg, uint8_t& value) {
  for (uint8_t attempt = 0; attempt < 2; attempt++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) == 0 &&
        Wire.requestFrom(MPU_ADDR, static_cast<uint8_t>(1)) == 1) {
      value = Wire.read();
      return true;
    }
    delayMicroseconds(50);
  }
  return false;
}

bool MotionSensor::readWord(uint8_t reg, int16_t& value) {
  // Ein Wiederholversuch faengt vereinzelte Stoerungen ab, ohne die Samplerate
  // bei Dauerfehlern zu stark zu bremsen.
  for (uint8_t attempt = 0; attempt < 2; attempt++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) == 0 &&
        Wire.requestFrom(MPU_ADDR, static_cast<uint8_t>(2)) == 2) {
      value = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
      return true;
    }
    delayMicroseconds(50);
  }
  return false;
}
