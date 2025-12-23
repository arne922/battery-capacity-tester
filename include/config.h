#pragma once
#include <stddef.h>
#include <stdint.h>
#include "log.h"
#include "hw.h"

#include "secrets.h" 

// =======================
// Logging configuration
// =======================

#define BT_LOG_LEVEL 5   // 1=E,2=W,3=I,4=D,5=V


// =======================
// Timing (long-running battery tests)
// =======================

// How often the core logic samples voltage/current
// (energy integration, stop conditions)
inline constexpr uint32_t kCoreSampleInterval_s = 15;//15;

// How often a row is stored into the log buffer (CSV resolution)
inline constexpr uint32_t kLogStoreInterval_s = 15*60;//15 * 60; // 15 minutes


// =======================
// WiFi configuration
// =======================

// Enable WiFi at all (set false for pure offline operation)
inline constexpr bool kWifiEnabled = true;

// Try to connect as STA first
inline constexpr bool kWifiUseSta = true;

// STA credentials (leave empty to skip STA)
// --> see secrets.h for your own SSID/PASSWORD
//inline constexpr const char* kWifiStaSsid = "your-ssid";
//inline constexpr const char* kWifiStaPass = "your-password";
  

// How long to wait for STA connection (ms)
inline constexpr uint32_t kWifiStaTimeoutMs = 15000; // 10 s

// AP fallback (always available if STA fails or is disabled)
inline constexpr const char* kWifiApSsid = "BatteryTester";
inline constexpr const char* kWifiApPass = "";   // empty = open AP
inline constexpr uint8_t     kWifiApChannel = 6;

// IP in AP mode 
inline constexpr uint8_t kWifiApIp[4]      = {192, 168, 0, 1};
inline constexpr uint8_t kWifiApGateway[4] = {192, 168, 0, 1};
inline constexpr uint8_t kWifiApSubnet[4]  = {255, 255, 255, 0};


// =======================
// measurement buffer
// =======================

// Log schema config (names + types define the row layout and CSV header).
enum class ColType : uint8_t { U8, U16, U32, F32 };

struct ColDef {
  const char* name;
  ColType type;
};

// Column order defines storage layout and CSV header.
inline constexpr ColDef kLogSchema[] = {
  {"Time_s",    ColType::U32},
  {"Cycle",     ColType::U16},
  {"Phase",     ColType::U8},
  {"Status",    ColType::U8},
  {"U_V",       ColType::F32},
  {"I_A",       ColType::F32},
  {"Ephase_Wh", ColType::F32},
};

inline constexpr size_t kLogSchemaCols = sizeof(kLogSchema) / sizeof(kLogSchema[0]);

// RAM budget for log buffer (adjust as needed).
inline constexpr size_t kLogRamBytes = 64 * 1024;


// =======================
//  HW config
// =======================

#define HW_USE_RELAIS          1  // 0 = off, 1 = on, can be switched off for testing
#define HW_USE_INA219          1  // 0 = off, 1 = on

// INA219 -------------------------------------------------------
inline constexpr uint8_t  kHwIna219Addr       = 0x40;
inline constexpr int      kHwInaI2cSdaPin     = 8;
inline constexpr int      kHwInaI2cSclPin     = 9;

// 0 = 32V/2A, 1 = 32V/1A, 2 = 16V/400mA
inline constexpr uint8_t  kHwInaCalPreset = 0;

// Outputs (Relais / MOSFET) -----------------------------------
inline constexpr int kHwChargePin    = 5;
inline constexpr int kHwDischargePin = 6;

inline constexpr bool kHwChargeActiveHigh    = false;
inline constexpr bool kHwDischargeActiveHigh = false;

// ADC fallback (optional) -------------------------------------
inline constexpr int kHwVoltageAdcPin = -1;
inline constexpr int kHwCurrentAdcPin = -1;

// Calibration -------------------------------------------------
inline constexpr float kHwVoltageScale  = 1.0f;
inline constexpr float kHwVoltageOffset = 0.0f;
inline constexpr float kHwCurrentScale  = 1.0f;
inline constexpr float kHwCurrentOffset = 0.0f;


// =======================
//  HW Simulation 
// =======================

#define HW_SIM_MEASUREMENTS    0  // 0 = off, 1 = on

// Simulation parameters --------------------------------------
inline constexpr float SIM_START_V  = 12.0f;   // start at 12 V

// Voltage gradients (V per second)
inline constexpr float kSimCharge_vps    = 0.01f;
inline constexpr float kSimDischarge_vps = 0.01f;

// Voltage limits (clamp)
inline constexpr float kSimVmin    = 9.0f;
inline constexpr float kSimVmax    = 14.6f;

// Dummy currents (A)
inline constexpr float kSimCurrentCharge_A    = 1.5f;
inline constexpr float kSimCurrentDischarge_A = 1.0f;
inline constexpr float kSimCurrentIdle_A      = 0.02f;

