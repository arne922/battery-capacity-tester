#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "log.h"
#include <ESPmDNS.h>


#include "config.h"
#include "hw.h"
#include "state_machine.h"
#include "ui_http.h"
#include "log_buffer.h"
#include "core.h"

static const char* TAG = "Main"; // For BT_LOG*
static const char* TAG_WIFI = "WIFI";

// ---------------------------------------------------------------------------
// WiFi start (STA first, AP fallback)
static void startWifi() {
  if (!kWifiEnabled) {
    BT_LOGI(TAG_WIFI, "WiFi disabled");
    return;
  }

  // --- STA ---
  if (kWifiUseSta && kWifiStaSsid[0] != '\0') {
    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiStaSsid, kWifiStaPass);

    BT_LOGI(TAG_WIFI, "Connecting to WiFi (STA)");
    Serial.println("#Connecting to WiFi (STA)");
    const uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED &&
           (uint32_t)(millis() - t0) < kWifiStaTimeoutMs) {
      delay(300);
    }

    if (WiFi.status() == WL_CONNECTED) {
      BT_LOGI(TAG_WIFI, "STA connected, IP: %s",
               WiFi.localIP().toString().c_str());
      Serial.print("#STA connected, IP: ");      
      Serial.println(WiFi.localIP().toString());

      if (MDNS.begin("batterytester")) {
        BT_LOGI(TAG_WIFI, "mDNS started: http://batterytester.local");
        Serial.println("#mDNS started: http://batterytester.local");
      } else {
        BT_LOGE(TAG_WIFI, "mDNS setup failed");
        Serial.println("#mDNS setup failed");
      }
      return;
    }

    BT_LOGW(TAG_WIFI, "STA failed");
    Serial.println("#STA failed");

  }

  // --- AP fallback ---
  WiFi.mode(WIFI_AP);

  IPAddress ip(kWifiApIp[0], kWifiApIp[1], kWifiApIp[2], kWifiApIp[3]);
  IPAddress gw(kWifiApGateway[0], kWifiApGateway[1],
               kWifiApGateway[2], kWifiApGateway[3]);
  IPAddress sn(kWifiApSubnet[0], kWifiApSubnet[1],
               kWifiApSubnet[2], kWifiApSubnet[3]);

  if (!WiFi.softAPConfig(ip, gw, sn)) {
    BT_LOGE(TAG_WIFI, "AP IP config failed");
  }

  WiFi.softAP(kWifiApSsid, kWifiApPass, kWifiApChannel);

  BT_LOGI(TAG_WIFI, "AP started, IP: %s",
           WiFi.softAPIP().toString().c_str());

  if (MDNS.begin("batterytester")) {
    BT_LOGI(TAG_WIFI, "mDNS started (AP): http://batterytester.local");
  }
}


// ---------------------------------------------------------------------------
// Global objects (explicit wiring)

// Hardware
static HwConfig g_hwCfg;  // default values from struct

static void initHwConfig() {
  g_hwCfg.pinChargeEnable = 26;
  g_hwCfg.pinDischargeEnable = 27;
  g_hwCfg.activeHighCharge = true;
  g_hwCfg.activeHighDischarge = true;
  g_hwCfg.pinAdcVoltage = 34;
  g_hwCfg.pinAdcCurrent = 35;
  g_hwCfg.vScale = 20.0f;
  g_hwCfg.iScale = 1.0f;
}

static Hw g_hw(g_hwCfg);


// State machine
static StateMachine g_sm(g_hw);

// Compute core
static CoreConfig g_coreCfg;
static Core g_core(g_hw, g_sm);

// Log buffer (schema-driven, typed)
static uint8_t g_logMem[kLogRamBytes];
static LogBuffer g_log(g_logMem, sizeof(g_logMem), kLogSchema, kLogSchemaCols);

// HTTP UI
static WebServer g_server(80);
static UiHttp g_ui(g_server, g_sm, g_core, g_hw, g_log);

// // Sampling interval for log rows (fast sampling)
// static const uint32_t kSampleIntervalMs = 5000;
// static uint32_t g_lastSampleMs = 0;
static uint32_t lastCoreSampleMs = 0;
static uint32_t lastLogStoreMs  = 0;



// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(4000);   // important for USB CDC stability
  Serial.println("#System starting...");

  // set log levels

  // Foreign code: keep quiet (ERROR only)
  //esp_log_level_set("*", BT_LOG_ERROR);
  //esp_log_level_set("*", BT_LOG_VERBOSE);

  // Project modules: global level
  // esp_log_level_set("WIFI", LOG_LEVEL_GLOBAL);   // TAG_WIFI in main.cpp :contentReference[oaicite:2]{index=2}
  // esp_log_level_set("HTTP", LOG_LEVEL_GLOBAL);   // TAG in ui_http.cpp is "HTTP" :contentReference[oaicite:3]{index=3}
  // esp_log_level_set("SM",   LOG_LEVEL_GLOBAL);
  // esp_log_level_set("CORE", LOG_LEVEL_GLOBAL);
  // esp_log_level_set("HW",   LOG_LEVEL_GLOBAL);
  // esp_log_level_set("LOG",  LOG_LEVEL_GLOBAL);

  // Per-module overrides via compile-time switches
#ifdef LOG_SM_DEBUG
  esp_log_level_set("SM", BT_LOG_DEBUG);
#endif

#ifdef LOG_CORE_DEBUG
  esp_log_level_set("CORE", BT_LOG_DEBUG);
#endif

#ifdef LOG_HTTP_DEBUG
  esp_log_level_set("HTTP", BT_LOG_DEBUG);
#endif

  ESP_EARLY_LOGI(TAG, "System Starting");

  initHwConfig();
  g_hw.begin();

  // Apply core config (later this will come from UI)
  g_core.setConfig(g_coreCfg);

  startWifi();

  g_ui.begin();

  Serial.println("#System ready");
}

void loop() {

  const uint32_t now = millis();

  // Serve HTTP
  g_ui.tick();

  // Core sampling (e.g. 15 s)
  if (now - lastCoreSampleMs >= kCoreSampleInterval_s * 1000UL) {
    lastCoreSampleMs = now;
    //BT_LOGV(TAG, "Core tick at %lu ms", lastCoreSampleMs);

    // SM orchestration
    g_sm.tick();

    // Compute core (stop rules, waits, energy integration)
    const auto tel = g_sm.getTelemetry();
    g_core.tick(now, tel);  
  }




  // Periodic data log row (content-free buffer: we push already computed values)
  if (now - lastLogStoreMs >= kLogStoreInterval_s * 1000UL) {
    lastLogStoreMs = now;
    //BT_LOGV(TAG, "Log store at %lu ms", lastLogStoreMs);

    // Map runtime values to schema order (config.h).
    ColValue row[kLogSchemaCols];

    row[0].u32 = now;                                   // Time_ms
    row[1].u16 = g_core.cycleIndex1Based();              // Cycle
    row[2].u8  = (uint8_t)g_core.phase();                // Phase
    row[3].u8  = (uint8_t)g_core.runState();             // Status
    row[4].f32 = g_hw.readVoltage_V();                   // U_V
    row[5].f32 = g_hw.readCurrent_A();                   // I_A
    row[6].f32 = g_core.phaseEnergy_Wh();                // Ephase_Wh

    g_log.store(row, kLogSchemaCols);
  }
}
