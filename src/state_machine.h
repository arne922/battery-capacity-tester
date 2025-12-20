#pragma once
#include <stdint.h>

class Hw; // Forward declaration (implemented in hw.*)

// Top-level modes exposed to UI / telemetry
enum class Mode : uint8_t {
  Idle,
  Charge,
  Discharge
};

// Program configuration coming from UI
struct Program {
  uint16_t cycles = 1;          // Number of full cycles (>= 1)
  Mode startMode = Mode::Charge;
  Mode stopMode  = Mode::Discharge;
};

// Reason why the system is in Idle
enum class IdleReason : uint8_t {
  Ready,    // Idle and ready to start
  Done,     // Program finished normally
  Error,    // Error / safety shutdown
  Stopped  // User requested stop
};

// Commands sent from UI to state machine
enum class CommandType : uint8_t {
  Start,
  Stop,
  ResetError
};

// Telemetry data exposed to UI / HTTP / MQTT
struct Telemetry {
  Mode mode = Mode::Idle;
  IdleReason idleReason = IdleReason::Ready;
  uint16_t phaseCount = 0;        // Counts every phase (Charge or Discharge)
  uint16_t completedCycles = 0;   // phaseCount / 2
};

class StateMachine {
public:
  explicit StateMachine(Hw& hw);

  // Set program parameters (should be validated by UI)
  void setProgram(const Program& p);

  // Handle external commands (UI / HTTP)
  void command(CommandType c);

  // Must be called regularly from main loop
  void tick();

  // Read-only access for UI / server
  Telemetry getTelemetry() const { return t_; }
  Program   getProgram()   const { return p_; }

  // Events triggered by internal logic or safety checks
  void notifyPhaseDone();   // Current charge/discharge phase finished
  void notifyError();       // Safety or hardware error detected

private:
  Hw& hw_;
  Program p_;
  Telemetry t_;

  // Internal helpers
  void enterIdle(IdleReason reason);
  void enterMode(Mode mode);
  bool shouldStopAfterThisPhase(Mode finishedMode,
                                uint16_t phaseCountAfterIncrement) const;
  Mode opposite(Mode mode) const;
};
