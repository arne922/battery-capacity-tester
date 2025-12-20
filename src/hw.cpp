#include <Arduino.h>
#include <math.h>

#include "hw.h"


Hw::Hw(const HwConfig& cfg)
  : cfg_(cfg) {}

void Hw::begin() {
  // Configure GPIOs
  if (cfg_.pinChargeEnable >= 0) {
    pinMode(cfg_.pinChargeEnable, OUTPUT);
  }
  if (cfg_.pinDischargeEnable >= 0) {
    pinMode(cfg_.pinDischargeEnable, OUTPUT);
  }

  // Ensure fail-safe state on boot
  allOff();
}

void Hw::allOff() {
  // Enforce interlock: never allow both paths at once
  writeCharge(false);
  writeDischarge(false);
  chargeOn_ = false;
  dischargeOn_ = false;
}

void Hw::startCharge() {
  // Interlock: disable discharge before enabling charge
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
  // Interlock: disable charge before enabling discharge
  writeCharge(false);
  chargeOn_ = false;

  writeDischarge(true);
  dischargeOn_ = true;
}

void Hw::stopDischarge() {
  writeDischarge(false);
  dischargeOn_ = false;
}

float Hw::readVoltage_V() const {
#ifdef HW_SIM
  return readVoltageSim_V();
#else
  if (cfg_.pinAdcVoltage < 0) return NAN;
  const float x = readAdcNormalized(cfg_.pinAdcVoltage);
  return (x * cfg_.vScale) + cfg_.vOffset;
#endif
}

float Hw::readCurrent_A() const {
#ifdef HW_SIM
  return readCurrentSim_A();
#else
  if (cfg_.pinAdcCurrent < 0) return NAN;
  const float x = readAdcNormalized(cfg_.pinAdcCurrent);
  return (x * cfg_.iScale) + cfg_.iOffset;
#endif
}

float Hw::readTemp_C() const {
  if (cfg_.pinAdcTemp < 0) return NAN;
  const float x = readAdcNormalized(cfg_.pinAdcTemp);
  return (x * cfg_.tScale) + cfg_.tOffset;
}

void Hw::writeCharge(bool on) {
  if (cfg_.pinChargeEnable < 0) return;
  const bool level = cfg_.activeHighCharge ? on : !on;
  digitalWrite(cfg_.pinChargeEnable, level ? HIGH : LOW);
}

void Hw::writeDischarge(bool on) {
  if (cfg_.pinDischargeEnable < 0) return;
  const bool level = cfg_.activeHighDischarge ? on : !on;
  digitalWrite(cfg_.pinDischargeEnable, level ? HIGH : LOW);
}

float Hw::readAdcNormalized(int pin) const {
  // Arduino analogRead() on ESP32 returns 0..4095 by default (12-bit),
  // but configuration may differ. This keeps the abstraction small.
  const int raw = analogRead(pin);
  return (float)raw / 4095.0f;
}

#ifdef HW_SIM

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
    // wait -> hold current voltage (no change)
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
#endif
