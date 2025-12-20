#include "log.h"

#include "state_machine.h"
#include "hw.h"

static const char* TAG = "SM"; // For BT_LOG*

// Minimal hardware interface expected from hw.*
// The concrete implementation lives in hw.cpp
// class Hw {
// public:
//   void allOff();           // Disable charger and load
//   void startCharge();
//   void stopCharge();
//   void startDischarge();
//   void stopDischarge();
// };

StateMachine::StateMachine(Hw& hw)
  : hw_(hw) {
  enterIdle(IdleReason::Ready);
}

void StateMachine::setProgram(const Program& p) {
  p_ = p;
  if (p_.cycles < 1) {
    p_.cycles = 1;
  }
}

void StateMachine::command(CommandType c) {
  BT_LOGI(TAG, "cmd=%d", (int)c);
  switch (c) {
    case CommandType::Start:
      t_.phaseCount = 0;
      t_.completedCycles = 0;
      t_.idleReason = IdleReason::Ready;
      enterMode(p_.startMode);
      break;

    case CommandType::Stop:
      hw_.allOff();
      enterIdle(IdleReason::Stopped);
      break;

    case CommandType::ResetError:
      hw_.allOff();
      enterIdle(IdleReason::Ready);
      break;
  }
}

void StateMachine::tick() {
  // Intentionally empty.
  // Substates such as PRECHECK / RUN / FINISH
  // can be implemented here later if needed.
}

void StateMachine::notifyError() {
  BT_LOGE(TAG, "error -> shutdown");
  
  hw_.allOff();
  enterIdle(IdleReason::Error);
}

void StateMachine::notifyPhaseDone() {
  BT_LOGI(TAG, "phase done in mode=%d", (int)t_.mode);

  // Count completed phase
  t_.phaseCount++;
  t_.completedCycles = t_.phaseCount / 2;
  BT_LOGI(TAG, "phaseCount=%u completedCycles=%u", t_.phaseCount, t_.completedCycles);

  const Mode finishedMode = t_.mode;

  // Ensure current mode is safely stopped
  if (finishedMode == Mode::Charge) {
    hw_.stopCharge();
  } else if (finishedMode == Mode::Discharge) {
    hw_.stopDischarge();
  }

  // Decide whether the program should stop
  if (shouldStopAfterThisPhase(finishedMode, t_.phaseCount)) {
    hw_.allOff();
    enterIdle(IdleReason::Done);
    return;
  }

  // Otherwise switch to the opposite mode
  enterMode(opposite(finishedMode));
}

void StateMachine::enterIdle(IdleReason reason) {
  BT_LOGI(TAG, "enter Idle reason=%d", (int)reason);
  t_.mode = Mode::Idle;
  t_.idleReason = reason;
}

void StateMachine::enterMode(Mode mode) {
  BT_LOGI(TAG, "enter mode=%d", (int)mode);

  t_.mode = mode;
  t_.idleReason = IdleReason::Ready;

  if (mode == Mode::Charge) {
    hw_.startCharge();
  } else if (mode == Mode::Discharge) {
    hw_.startDischarge();
  } else {
    hw_.allOff();
  }
}

Mode StateMachine::opposite(Mode mode) const {
  if (mode == Mode::Charge) return Mode::Discharge;
  if (mode == Mode::Discharge) return Mode::Charge;
  return Mode::Idle;
}

bool StateMachine::shouldStopAfterThisPhase(
    Mode finishedMode,
    uint16_t phaseCountAfterIncrement) const {

  const uint16_t completed = phaseCountAfterIncrement / 2;
  return (completed >= p_.cycles) && (finishedMode == p_.stopMode);
}
