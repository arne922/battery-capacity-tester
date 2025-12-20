#include "core.h"

static const char* TAG = "CORE"; // For BT_LOG*

// Keep implementation deliberately flat:
// - One main tick() with a few small inline blocks
// - Minimal helper usage (only phaseElapsed_s required by header)

uint32_t Core::phaseElapsed_s(uint32_t now_ms) const {
  if (phaseStartMs_ == 0) return 0;
  return (now_ms - phaseStartMs_) / 1000;
}

Core::Core(Hw& hw, StateMachine& sm)
  : hw_(hw), sm_(sm) {}

void Core::start() {
  runState_ = RunState::Running;
}

void Core::stop() {
  // Hard stop: outputs off, all runtime counters reset
  hw_.allOff();
  runState_ = RunState::Off;

  phase_ = Phase::Charge;
  phaseCount_ = 0;
  cycle1_ = 0;

  phaseStartMs_ = 0;
  lastEnergyMs_ = 0;
  phaseWh_ = 0.0f;

  aboveVStartMs_ = 0;
  waitStartMs_ = 0;
}

void Core::pause() {
  if (runState_ != RunState::Running) return;
  hw_.allOff();
  runState_ = RunState::Paused;
}

void Core::resume() {
  if (runState_ != RunState::Paused) return;
  runState_ = RunState::Running;
  // Hardware will be re-enabled in tick() based on current phase.
}

void Core::tick(uint32_t now_ms, const Telemetry& smTel) {
  // -------------------------------------------------------------------------
  // 1) Sync run state with StateMachine (Start/Stop detection)
  // -------------------------------------------------------------------------

  // Start detected: SM left Idle
  if (runState_ == RunState::Off && smTel.mode != Mode::Idle) {
    runState_ = RunState::Running;

    // Align to SM mode (Charge or Discharge)
    phase_ = (smTel.mode == Mode::Charge) ? Phase::Charge : Phase::Discharge;

    // Initialize per-phase accounting
    phaseStartMs_ = now_ms;
    lastEnergyMs_ = now_ms;
    phaseWh_ = 0.0f;

    // Reset charge-hold tracking
    aboveVStartMs_ = 0;

    // Cycle tracking: phaseCount comes from SM (counts completed active phases)
    phaseCount_ = smTel.phaseCount;
    cycle1_ = (phaseCount_ / 2) + 1;

    // Wait timer not active yet
    waitStartMs_ = 0;
  }

  // Stop detected: SM returned to Idle (Done/Stop/Error)
  if (runState_ != RunState::Off && smTel.mode == Mode::Idle) {
    stop();
    return;
  }

  // If not running, do nothing
  if (runState_ != RunState::Running) {
    return;
  }

  // -------------------------------------------------------------------------
  // 2) Read sensors (used for stop checks and energy integration)
  // -------------------------------------------------------------------------
  const float v = hw_.readVoltage_V();
  const float i = hw_.readCurrent_A();

  // -------------------------------------------------------------------------
  // 3) Energy integration (active phases only)
  // -------------------------------------------------------------------------
  if (phase_ == Phase::Charge || phase_ == Phase::Discharge) {
    if (lastEnergyMs_ == 0) lastEnergyMs_ = now_ms;

    const uint32_t dt_ms = now_ms - lastEnergyMs_;
    if (dt_ms > 0) {
      const float dt_s = (float)dt_ms / 1000.0f;
      phaseWh_ += (v * i) * (dt_s / 3600.0f);
      lastEnergyMs_ = now_ms;
    }
  }

  // -------------------------------------------------------------------------
  // 4) Phase logic (single switch, linear flow)
  // -------------------------------------------------------------------------

  if (phase_ == Phase::Charge) {
    // Charge stop condition:
    // voltage >= threshold continuously for chargeHoldAbove_s
    if (v >= cfg_.chargeStopVoltage_V) {
      if (aboveVStartMs_ == 0) aboveVStartMs_ = now_ms;
      const uint32_t held_ms = now_ms - aboveVStartMs_;
      if (held_ms >= cfg_.chargeHoldAbove_s * 1000UL) {
        // Tell SM to switch to the opposite mode
        sm_.notifyPhaseDone();

        // Update cycle counters (one active phase completed)
        phaseCount_++;
        cycle1_ = (phaseCount_ / 2) + 1;

        // Enter wait phase (charge -> discharge)
        hw_.allOff();
        phase_ = Phase::WaitChargeToDischarge;
        waitStartMs_ = now_ms;

        // Reset per-phase energy/timers for the next phase block
        phaseStartMs_ = now_ms;
        lastEnergyMs_ = now_ms;
        phaseWh_ = 0.0f;
        aboveVStartMs_ = 0;
      }
    } else {
      // Drop below threshold resets hold timer
      aboveVStartMs_ = 0;
    }

    return;
  }

  if (phase_ == Phase::Discharge) {
    // Discharge stop condition:
    // voltage <= dischargeStopVoltage_V
    if (v <= cfg_.dischargeStopVoltage_V) {
      sm_.notifyPhaseDone();

      phaseCount_++;
      cycle1_ = (phaseCount_ / 2) + 1;

      // Enter wait phase (discharge -> charge)
      hw_.allOff();
      phase_ = Phase::WaitDischargeToCharge;
      waitStartMs_ = now_ms;

      // Reset per-phase energy/timers
      phaseStartMs_ = now_ms;
      lastEnergyMs_ = now_ms;
      phaseWh_ = 0.0f;
      aboveVStartMs_ = 0;
    }

    return;
  }

  if (phase_ == Phase::WaitChargeToDischarge) {
    if (waitStartMs_ == 0) waitStartMs_ = now_ms;

    const uint32_t wait_ms = cfg_.waitChargeToDischarge_s * 1000UL;
    if ((now_ms - waitStartMs_) >= wait_ms) {
      // Start discharge after wait
      hw_.startDischarge();
      phase_ = Phase::Discharge;

      // Reset per-phase timers/energy for discharge
      phaseStartMs_ = now_ms;
      lastEnergyMs_ = now_ms;
      phaseWh_ = 0.0f;

      // No charge-hold tracking in discharge
      aboveVStartMs_ = 0;
      waitStartMs_ = 0;
    }

    return;
  }

  // Phase::WaitDischargeToCharge
  if (waitStartMs_ == 0) waitStartMs_ = now_ms;

  const uint32_t wait_ms = cfg_.waitDischargeToCharge_s * 1000UL;
  if ((now_ms - waitStartMs_) >= wait_ms) {
    // Start charge after wait
    hw_.startCharge();
    phase_ = Phase::Charge;

    // Reset per-phase timers/energy for charge
    phaseStartMs_ = now_ms;
    lastEnergyMs_ = now_ms;
    phaseWh_ = 0.0f;

    aboveVStartMs_ = 0;
    waitStartMs_ = 0;
  }
}
