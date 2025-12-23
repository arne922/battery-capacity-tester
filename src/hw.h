#pragma once
#include <math.h>

class Hw {
public:
  void begin();

  void allOff();
  void startCharge();
  void stopCharge();
  void startDischarge();
  void stopDischarge();

  float readVoltage_V() const;
  float readCurrent_A() const;

  bool isChargeOn() const { return chargeOn_; }
  bool isDischargeOn() const { return dischargeOn_; }

private:
  bool chargeOn_ = false;
  bool dischargeOn_ = false;

  // INA219 runtime status (exists even if INA is compiled out)
  bool inaOk_ = false;

  void writeCharge(bool on);
  void writeDischarge(bool on);
  float readAdcNormalized(int pin) const;

  // Declared always, defined only if enabled in hw.cpp
  void initIna219_();
  float readVoltageIna_V_() const;
  float readCurrentIna_A_() const;

  // Declared always, defined only if HW_SIM_MEASUREMENTS in hw.cpp
  float readVoltageSim_V() const;
  float readCurrentSim_A() const;
};
