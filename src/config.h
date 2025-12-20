#pragma once
#include <stddef.h>
#include <stdint.h>
#include "log.h"


// =======================
// Logging configuration
// =======================

// Global default log level
//#define CONFIG_LOG_MASTER_LEVEL BT_LOG_WARN
//#define LOG_LEVEL_GLOBAL  BT_LOG_INFO
//#define LOG_LEVEL_GLOBAL  BT_LOG_DEBUG
//#define LOG_LEVEL_GLOBAL  BT_LOG_VERBOSE

// Per-module overrides (comment out if not needed)
//#define LOG_SM_DEBUG
//#define LOG_CORE_DEBUG
//#define LOG_HTTP_DEBUG


// =======================
// Timing (long-running battery tests)
// =======================

// How often the core logic samples voltage/current
// (energy integration, stop conditions)
inline constexpr uint32_t kCoreSampleInterval_s = 1;//15;

// How often a row is stored into the log buffer (CSV resolution)
inline constexpr uint32_t kLogStoreInterval_s = 2;//15 * 60; // 15 minutes


// =======================
// WiFi configuration
// =======================

// Enable WiFi at all (set false for pure offline operation)
inline constexpr bool kWifiEnabled = true;

// Try to connect as STA first
inline constexpr bool kWifiUseSta = true;

// STA credentials (leave empty to skip STA)
inline constexpr const char* kWifiStaSsid = "ToLo2";//"your-ssid";
inline constexpr const char* kWifiStaPass = "einkaefer1200isteintollesauto124auch"; //your-password";

// How long to wait for STA connection (ms)
inline constexpr uint32_t kWifiStaTimeoutMs = 10000; // 10 s

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
  {"Time_ms",   ColType::U32},
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
//  HW Simulation 
// =======================

#define HW_SIM

// Start at 12V
static constexpr float SIM_START_V   = 12.0f;

// Gradienten (V per second)
static constexpr float SIM_CHG_VPS   = 0.01f;
static constexpr float SIM_DCHG_VPS  = 0.01f;

// Clamp (optional)
static constexpr float SIM_V_MIN     = 10.5f;
static constexpr float SIM_V_MAX     = 14.8f;

// Dummy-currents (A)
static constexpr float SIM_I_CHG_A   = +2.0f;
static constexpr float SIM_I_DCHG_A  = -2.0f;
static constexpr float SIM_I_IDLE_A  = 0.0f;
