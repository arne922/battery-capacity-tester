#pragma once
#include <stdint.h>
#include "state_machine.h"
#include "hw.h"

// Runtime run state (what UI shows as On/Off/Pause).
enum class RunState : uint8_t { Off = 0, Running = 1, Paused = 2 };

// Detailed phase inside a cycle (includes waits).
enum class Phase : uint8_t {
  Charge = 0,
  WaitChargeToDischarge = 1,
  Discharge = 2,
  WaitDischargeToCharge = 3
};

// Parameters for phase stop conditions and waits.
// Move these into config.h as soon as you want them configurable via UI.
struct CoreConfig {
  float chargeStopVoltage_V = 14.5f;
  uint32_t chargeHoldAbove_s = 3 * 3600;     // time above threshold

  uint32_t waitChargeToDischarge_s = 10;
  float dischargeStopVoltage_V = 12.2f;
  uint32_t waitDischargeToCharge_s = 10;
};

// The "compute core":
// - evaluates stop criteria
// - handles wait phases
// - integrates energy per phase
// - triggers SM transitions via notifyPhaseDone()
class Core {
public:
  Core(Hw& hw, StateMachine& sm);

  void setConfig(const CoreConfig& cfg) { cfg_ = cfg; }
  CoreConfig getConfig() const { return cfg_; }
  
  // Optional direct control (UI can call these later).
  void start();
  void stop();
  void pause();
  void resume();

  // Must be called regularly.
  // Uses telemetry to detect Start/Stop when UI still controls the state machine directly.
  void tick(uint32_t now_ms, const Telemetry& smTel);

  // Outputs for UI/logging
  RunState runState() const { return runState_; }
  Phase phase() const { return phase_; }
  uint16_t cycleIndex1Based() const { return cycle1_; }      // 1..N (0 if Off)
  float phaseEnergy_Wh() const { return phaseWh_; }
  uint32_t phaseElapsed_s(uint32_t now_ms) const;

private:
  Hw& hw_;
  StateMachine& sm_;
  CoreConfig cfg_;

  RunState runState_ = RunState::Off;
  Phase phase_ = Phase::Charge;

  // We track cycles ourselves for logging/UI.
  uint16_t phaseCount_ = 0;  // counts Charge/Discharge completions (not waits)
  uint16_t cycle1_ = 0;      // 1-based cycle number for human display

  // Timing/energy integration
  uint32_t phaseStartMs_ = 0;
  uint32_t lastEnergyMs_ = 0;
  float phaseWh_ = 0.0f;

  // Charge stop: hold time above voltage threshold
  uint32_t aboveVStartMs_ = 0;

  // Wait phase timing
  uint32_t waitStartMs_ = 0;

  // Internal helpers
  void syncFromStateMachine_(uint32_t now_ms, const Telemetry& smTel);
  void enterPhase_(uint32_t now_ms, Phase p);

  void updateEnergy_(uint32_t now_ms);
  bool checkChargeDone_(uint32_t now_ms, float voltage_V) ;
  bool checkDischargeDone_(float voltage_V) const;
  bool checkWaitDone_(uint32_t now_ms, uint32_t wait_s) const;

  void finishActiveModeAndEnterWait_(uint32_t now_ms, Phase waitPhase);
  void startNextActiveModeAfterWait_();
};
