#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "hw.h"

#if HW_USE_INA219
  #include <Wire.h>
  #include <Adafruit_INA219.h>
  static Adafruit_INA219 g_ina(kHwIna219Addr);
#endif

void Hw::begin() {

#if HW_USE_RELAIS
  if (kHwChargePin >= 0) pinMode(kHwChargePin, OUTPUT);
  if (kHwDischargePin >= 0) pinMode(kHwDischargePin, OUTPUT);
#endif

#if HW_USE_INA219
  initIna219_();
#else
  inaOk_ = false;
#endif

  allOff();
}


void Hw::allOff() {
  writeCharge(false);
  writeDischarge(false);
  chargeOn_ = false;
  dischargeOn_ = false;
}

void Hw::startCharge() {
  writeDischarge(false);
  dischargeOn_ = false;

  writeCharge(true);
  chargeOn_ = true;
}

void Hw::stopCharge() {
  writeCharge(false);
  chargeOn_ = false;
}

void Hw::startDischarge() {
  writeCharge(false);
  chargeOn_ = false;

  writeDischarge(true);
  dischargeOn_ = true;
}

void Hw::stopDischarge() {
  writeDischarge(false);
  dischargeOn_ = false;
}

void Hw::writeCharge(bool on) {

#if HW_USE_RELAIS
  if (kHwChargePin < 0) return;
  const bool level = kHwChargeActiveHigh ? on : !on;
  digitalWrite(kHwChargePin, level ? HIGH : LOW);
#endif
}

void Hw::writeDischarge(bool on) {
#if HW_USE_RELAIS
  if (kHwDischargePin < 0) return;
  const bool level = kHwDischargeActiveHigh ? on : !on;
  digitalWrite(kHwDischargePin, level ? HIGH : LOW);
#endif
}

float Hw::readVoltage_V() const {
#if HW_SIM_MEASUREMENTS
  return readVoltageSim_V();
#else
#if HW_USE_INA219
  if (inaOk_) return readVoltageIna_V_();
#endif
  if (kHwVoltageAdcPin < 0) return NAN;
  const float x = readAdcNormalized(kHwVoltageAdcPin);
  return x * kHwVoltageScale + kHwVoltageOffset;
#endif
}

float Hw::readCurrent_A() const {
#if HW_SIM_MEASUREMENTS
  return readCurrentSim_A();
#else
#if HW_USE_INA219
  if (inaOk_) return readCurrentIna_A_();
#endif
  if (kHwCurrentAdcPin < 0) return NAN;
  const float x = readAdcNormalized(kHwCurrentAdcPin);
  return x * kHwCurrentScale + kHwCurrentOffset;
#endif
}

float Hw::readAdcNormalized(int pin) const {
  const int raw = analogRead(pin);
  return (float)raw / 4095.0f;
}

#if HW_USE_INA219

void Hw::initIna219_() {
  // allow "Arduino default pins" pattern if you set SDA/SCL to -1
#if (INA_I2C_SDA >= 0) && (INA_I2C_SCL >= 0)
  Wire.begin(kHwInaI2cSdaPin, kHwInaI2cSclPin);
#else
  Wire.begin();
#endif

  inaOk_ = g_ina.begin();
  if (!inaOk_) return;

  switch (kHwInaCalPreset) {
    default:
    case 0: g_ina.setCalibration_32V_2A();    break;
    case 1: g_ina.setCalibration_32V_1A();    break;
    case 2: g_ina.setCalibration_16V_400mA(); break;
  }
}

float Hw::readVoltageIna_V_() const {
  return g_ina.getBusVoltage_V();
}

float Hw::readCurrentIna_A_() const {
  return g_ina.getCurrent_mA() / 1000.0f;
}

#endif // HW_USE_INA219

// ---------------------------
// HW_SIM_MEASUREMENTS
// ---------------------------
#if HW_SIM_MEASUREMENTS

float Hw::readVoltageSim_V() const {
  static float v = SIM_START_V;
  static uint32_t lastMs = 0;

  const uint32_t now = millis();
  if (lastMs == 0) lastMs = now;
  const float dt_s = (now - lastMs) / 1000.0f;
  lastMs = now;

  if (chargeOn_ && !dischargeOn_) {
    v += SIM_CHG_VPS * dt_s;
  } else if (dischargeOn_ && !chargeOn_) {
    v -= SIM_DCHG_VPS * dt_s;
  } else {
    // hold
  }

  if (v < SIM_V_MIN) v = SIM_V_MIN;
  if (v > SIM_V_MAX) v = SIM_V_MAX;
  return v;
}

float Hw::readCurrentSim_A() const {
  if (chargeOn_ && !dischargeOn_) return SIM_I_CHG_A;
  if (dischargeOn_ && !chargeOn_) return SIM_I_DCHG_A;
  return SIM_I_IDLE_A;
}

#endif // HW_SIM_MEASUREMENTS
