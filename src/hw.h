#pragma once
#include <stdint.h>

#include "config.h"

// Minimal hardware access layer.
// This module owns pin access, ADC reads, and safe switching rules.

struct HwConfig {
  // GPIO outputs (set to a valid GPIO number)
  int pinChargeEnable = -1;     // Charger enable (relay/MOSFET gate)
  int pinDischargeEnable = -1;  // Load enable (relay/MOSFET gate)

  // Output polarity
  bool activeHighCharge = true;
  bool activeHighDischarge = true;

  // ADC inputs (set to a valid ADC-capable pin)
  int pinAdcVoltage = -1;       // Battery voltage divider -> ADC
  int pinAdcCurrent = -1;       // Current sense -> ADC (or unused)
  int pinAdcTemp    = -1;       // Temperature -> ADC (or unused)

  // ADC calibration / scaling
  float vScale = 1.0f;          // ADC units -> Volts (includes divider ratio)
  float vOffset = 0.0f;

  float iScale = 1.0f;          // ADC units -> Amps
  float iOffset = 0.0f;

  float tScale = 1.0f;          // ADC units -> °C (optional, if used)
  float tOffset = 0.0f;
};

class Hw {
public:
  explicit Hw(const HwConfig& cfg);

  // Call once from setup()
  void begin();

  // Safe outputs
  void allOff();
  void startCharge();
  void stopCharge();
  void startDischarge();
  void stopDischarge();

  // Measurements (optional pins may be -1 -> returns NAN)
  float readVoltage_V() const;
  float readCurrent_A() const;
  float readTemp_C() const;

  // Quick status helpers
  bool isChargeOn() const { return chargeOn_; }
  bool isDischargeOn() const { return dischargeOn_; }

private:
  HwConfig cfg_;
  bool chargeOn_ = false;
  bool dischargeOn_ = false;

  void writeCharge(bool on);
  void writeDischarge(bool on);

  // Raw ADC read mapped to a normalized float (0..1) by default.
  // You can later replace this with a better calibration if needed.
  float readAdcNormalized(int pin) const;

#ifdef HW_SIM
  float readVoltageSim_V() const;
  float readCurrentSim_A() const;
#endif

};
