<!-- Copilot / AI agent instructions for Battery_Capacity_Tester -->
# Battery_Capacity_Tester — Copilot Instructions

This file contains concise, actionable guidance for AI coding agents working in this repository. Focus on discoverable patterns and project-specific workflows so you can be productive immediately.

1) Big picture / architecture
- **Primary purpose:** An embedded ESP32-C3-based battery charge/discharge tester exposing a simple web UI and CSV download.
- **Major components:**
  - `src/hw.*` — Hardware abstraction (GPIO, ADC, charge/discharge interlock). Keep the interlock behavior (never enable charge and discharge simultaneously).
  - `src/state_machine.*` — Program and state machine that drives charge/discharge cycles. It depends on the `Hw` interface (passed by reference).
  - `src/ui_http.*` — Small HTTP server and web UI (uses `WebServer`) exposing endpoints: `/`, `/api/status`, `/api/control`, `/api/config`, `/download`.
  - `src/log_buffer.*` — In-memory CSV logging buffer; schema driven by `kLogSchema` in `src/config.h`.
  - `src/main.cpp` — Wi‑Fi startup (STA first, fallback to AP) and wiring of components.

2) Key files to inspect when changing behavior
- `platformio.ini` — build environment: `env:esp32c3`, `framework = arduino`, monitor/upload speeds.
- `src/config.h` — central compile-time settings: WiFi flags/credentials, `kLogSchema`, `kLogRamBytes`. Edit to change logging layout or WiFi defaults.
- `doc/readme.txt` — pin mappings (I2C, relays, optional LED/button) and board notes (ESP32‑C3 pinout image present).
- `src/hw.cpp` / `src/hw.h` — implement hardware specifics and ADC scaling. Respect `HwConfig` scales and offsets.
- `src/state_machine.*` — the decision logic for entering/exiting charge/discharge phases; use `sm_.command(...)` to trigger runs from UI.
- `src/ui_http.*` — minimal JSON parsing (string searches) is used; consider `ArduinoJson` if you need robust parsing.

3) Developer workflows (PlatformIO)
- Build: `pio run` (or use VS Code PlatformIO build task).
- Clean: `pio run -t clean`.
- Upload/flash: `pio run -t upload -e esp32c3` (may not need `-e` if only one env exists).
- Monitor serial: `pio device monitor -e esp32c3` (baud configured in `platformio.ini` `monitor_speed = 115200`).
- Quick sequence (Power cycle + upload):
```powershell
pio run -t clean; pio run -t upload -e esp32c3; pio device monitor -e esp32c3
```

4) Project-specific conventions & patterns
- Minimal dependencies: project uses Arduino framework and `WebServer` for HTTP; heavy JSON libs are intentionally avoided in UI code.
- Light-weight parsers: `UiHttp::handleConfig` and `handleControl` parse JSON using simple substring/indexing. If you change payloads, update these handlers and keep backward-compatibility with current keys (e.g. `"cycles":`, `"cmd":"start"`).
- Hardware interlocks: `Hw::allOff()`, `startCharge()`, and `startDischarge()` enforce disabling the opposite path first. Preserve this safety behavior when refactoring.
- Telemetry vs direct reads: `UiHttp::handleStatus()` prefers `StateMachine::getTelemetry()` but reads `hw_.readVoltage_V()` and `hw_.readCurrent_A()` directly for live values. When changing telemetry, update both producer (state machine) and consumer (UI).
- Log schema driven: `kLogSchema` in `src/config.h` defines CSV columns; `log_buffer.*` depends on this order. Modify schema only when also updating CSV serialization.

5) Integration points & external dependencies
- PlatformIO (local CLI or VS Code extension).
- Board: ESP32‑C3 (see `board = esp32-c3-devkitc-02` in `platformio.ini`).
- I²C sensor (INA219) expected on `GPIO8`/`GPIO9` per `doc/readme.txt`.

6) Safe change checklist (before pushing firmware)
- Confirm GPIO assignments in `doc/readme.txt` when modifying `HwConfig`.
- Keep or re-test interlock logic in `hw.cpp` to avoid enabling both charge and discharge.
- If you change logging schema (`kLogSchema`), regenerate/verify CSV header in `log_buffer.cpp`.
- If adding richer JSON, adopt `ArduinoJson` and update all minimal parsers in `ui_http.cpp`.

7) Small examples (where to change things)
- Disable WiFi (for offline testing): set `kWifiEnabled = false` in `src/config.h`.
- Change STA credentials: edit `kWifiStaSsid`/`kWifiStaPass` in `src/config.h` or implement runtime config.
- Add new control command: update `src/state_machine.h/cpp` to accept new `CommandType`, then handle it in `UiHttp::handleControl()`.

If anything above is unclear or you'd like the file to include more examples (e.g. sample `HwConfig` struct, telemetry layout, or a short test-run checklist), tell me which section to expand and I'll iterate.
