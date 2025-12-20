#include "ui_http.h"
#include <Arduino.h>
#include <WebServer.h>
#include "log.h"

#include "state_machine.h"
#include "hw.h"
#include "log_buffer.h"
#include "core.h"


static const char* TAG = "HTTP"; // For BT_LOG*


static const char* kHtml = R"HTML(
<!doctype html><html><head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Battery Tester</title>
<style>
body{font-family:system-ui,Arial;margin:16px;max-width:920px}
fieldset{margin:12px 0;padding:12px}
label{display:block;margin:6px 0}
input,select,button{font-size:16px;padding:6px;margin-left:6px}
pre{background:#f5f5f5;padding:10px;overflow:auto}
.row{display:flex;gap:12px;flex-wrap:wrap}
.card{border:1px solid #ddd;border-radius:10px;padding:12px;min-width:260px}
</style>
</head><body>
<h2>Battery Tester</h2>

<!-- Run control -->
<fieldset>
<legend>Run control</legend>
<button onclick="ctrl('start')">Start</button>
<button onclick="ctrl('pause')">Pause</button>
<button onclick="ctrl('resume')">Resume</button>
<button onclick="ctrl('stop')">Stop</button>
<button onclick="location.href='/download'">Download</button>
</fieldset>

<!-- Status -->
<fieldset>
<legend>Status</legend>
<div id="status">loading...</div>
</fieldset>

<!-- Program -->
<fieldset>
<legend>Program</legend>

<label>Cycles
  <input id="cycles" type="number" min="1" value="1"/>
</label>

<label>Start mode
  <select id="startMode">
    <option value="charge">Charge</option>
    <option value="discharge">Discharge</option>
  </select>
</label>

<label>Stop mode
  <select id="stopMode">
    <option value="charge">Charge</option>
    <option value="discharge">Discharge</option>
  </select>
</label>

<div class="row">
  <div class="card">
    <b>Charge stop</b>
    <label>Voltage (V)
      <input id="chgV" type="number" step="0.1" value="14.5"/>
    </label>
    <label>Hold time (h)
      <input id="chgHoldH" type="number" step="0.1" value="3"/>
    </label>
  </div>

  <div class="card">
    <b>Wait charge → discharge</b>
    <label>Time (s)
      <input id="wCD" type="number" value="10"/>
    </label>
  </div>

  <div class="card">
    <b>Discharge stop</b>
    <label>Voltage (V)
      <input id="dsgV" type="number" step="0.1" value="12.2"/>
    </label>
  </div>

  <div class="card">
    <b>Wait discharge → charge</b>
    <label>Time (s)
      <input id="wDC" type="number" value="10"/>
    </label>
  </div>
</div>

<button onclick="saveConfig()">Save config</button>
</fieldset>

<script>
async function api(path, obj){
  const r = await fetch(path, {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(obj)
  });
  return await r.text();
}

function modeVal(id){
  return document.getElementById(id).value;
}

async function loadConfig(){
  try{
    const r = await fetch('/api/config');
    const c = await r.json();

    if (c.cycles != null) document.getElementById('cycles').value = c.cycles;
    if (c.startMode) document.getElementById('startMode').value = c.startMode;
    if (c.stopMode) document.getElementById('stopMode').value = c.stopMode;

    if (c.chargeStopVoltage_V != null) document.getElementById('chgV').value = c.chargeStopVoltage_V;
    if (c.chargeStopHold_s != null) document.getElementById('chgHoldH').value = (Number(c.chargeStopHold_s) / 3600);
    if (c.waitChargeToDischarge_s != null) document.getElementById('wCD').value = c.waitChargeToDischarge_s;

    if (c.dischargeStopVoltage_V != null) document.getElementById('dsgV').value = c.dischargeStopVoltage_V;
    if (c.waitDischargeToCharge_s != null) document.getElementById('wDC').value = c.waitDischargeToCharge_s;

  } catch(e){}
}

async function saveConfig(){
  const cfg = {
    cycles: Number(document.getElementById('cycles').value),
    startMode: modeVal('startMode'),
    stopMode: modeVal('stopMode'),
    chargeStopVoltage_V: Number(document.getElementById('chgV').value),
    chargeStopHold_s: Math.round(Number(document.getElementById('chgHoldH').value) * 3600),
    waitChargeToDischarge_s: Number(document.getElementById('wCD').value),
    dischargeStopVoltage_V: Number(document.getElementById('dsgV').value),
    waitDischargeToCharge_s: Number(document.getElementById('wDC').value)
  };
  await api('/api/config', cfg);
}

async function ctrl(cmd){
  await api('/api/control', {cmd});
}

function esc(x){
  return String(x)
    .replaceAll("&","&amp;")
    .replaceAll("<","&lt;")
    .replaceAll(">","&gt;");
}

async function refresh(){
  try{
    const r = await fetch('/api/status');
    const s = await r.json();

    const modeTxt = ["Idle","Charge","Discharge"][s.mode] ?? s.mode;
    const idleTxt = ["Ready","Done","Error","Stopped"][s.idleReason] ?? s.idleReason;

    document.getElementById('status').innerHTML = `
      <div class="row">
        <div class="card"><b>Mode</b><div>${esc(modeTxt)}</div></div>
        <div class="card"><b>Cycles</b><div>${esc(s.completedCycles)}</div></div>
        <div class="card"><b>Voltage</b><div>${Number(s.voltage_V).toFixed(2)} V</div></div>

        <div class="card"><b>Idle Reas.</b><div>${esc(idleTxt)}</div></div>
        <div class="card"><b>Phase C.</b><div>${esc(s.phaseCount)}</div></div>
        <div class="card"><b>Current</b><div>${Number(s.current_A).toFixed(2)} A</div></div>
      </div>
    `;
  } catch(e){
    document.getElementById('status').textContent =
      'Status error: ' + e;
  }
}

loadConfig();
setInterval(refresh, 1000);
refresh();
</script>

</body></html>
)HTML";


UiHttp::UiHttp(WebServer& server, StateMachine& sm, Core& core, Hw& hw, LogBuffer& log)
  : server_(server), sm_(sm), core_(core), hw_(hw), log_(log) {}


void UiHttp::begin() {
  setupRoutes();
  server_.begin();
}

void UiHttp::tick() {
  server_.handleClient();
}

void UiHttp::setupRoutes() {
  server_.on("/", HTTP_GET, [this](){ handleRoot(); });

  server_.on("/api/status", HTTP_GET, [this](){ handleStatus(); });
  server_.on("/api/control", HTTP_POST, [this](){ handleControl(); });
  server_.on("/api/config",  HTTP_POST, [this](){ handleConfig(); });
  server_.on("/api/config",  HTTP_GET,  [this](){ handleGetConfig(); });

  server_.on("/download", HTTP_GET, [this](){ handleDownload(); });

  server_.onNotFound([this]() {
  // Common browser requests (avoid noisy error logs)
  if (server_.uri() == "/favicon.ico" ||
      server_.uri() == "/apple-touch-icon.png" ||
      server_.uri() == "/apple-touch-icon-precomposed.png") {
    server_.send(204); // No Content
    return;
  }

  // Optional: return a readable 404 for everything else
  BT_LOGW(TAG, "404 %s", server_.uri().c_str());
  server_.send(404, "text/plain", "Not found");
});

}

void UiHttp::handleRoot() {
  server_.send(200, "text/html; charset=utf-8", kHtml);
}

void UiHttp::handleStatus() {
  // Keep this endpoint dumb: just serialize current telemetry.
  const auto t = sm_.getTelemetry();

  // If you want live voltage/current here, you can also read hw_ directly,
  // but ideally telemetry already contains those values.
  const float v = hw_.readVoltage_V();
  const float i = hw_.readCurrent_A();

  String json = "{";
  json += "\"mode\":" + String((int)t.mode) + ",";
  json += "\"idleReason\":" + String((int)t.idleReason) + ",";
  json += "\"phaseCount\":" + String(t.phaseCount) + ",";
  json += "\"completedCycles\":" + String(t.completedCycles) + ",";
  json += "\"voltage_V\":" + String(v, 3) + ",";
  json += "\"current_A\":" + String(i, 3);
  json += "}";

  server_.send(200, "application/json; charset=utf-8", json);
}

void UiHttp::handleControl() {
  // Minimal parser: looks for cmd substring.
  String body;
  if (!readJsonBody(server_, body)) {
    BT_LOGW(TAG, "POST /api/control missing body");
    server_.send(400, "text/plain", "Missing body");
    return;
  }

  BT_LOGI(TAG, "POST /api/control body=%s", body.c_str());
  if (body.indexOf("\"cmd\":\"start\"") >= 0) {
    sm_.command(CommandType::Start);
  } else if (body.indexOf("\"cmd\":\"stop\"") >= 0) {
    sm_.command(CommandType::Stop);
  } else if (body.indexOf("\"cmd\":\"pause\"") >= 0) {
    // Add CommandType::Pause later in state_machine.h
    // sm_.command(CommandType::Pause);
  } else if (body.indexOf("\"cmd\":\"resume\"") >= 0) {
    // Add CommandType::Resume later in state_machine.h
    // sm_.command(CommandType::Resume);
  }

  server_.send(200, "text/plain", "OK");
}


bool UiHttp::extractNumber(const String& body, const char* key, long& out) {
  String pat = String("\"") + key + "\":";
  int idx = body.indexOf(pat);
  if (idx < 0) return false;
  idx += pat.length();

  // Skip spaces
  while (idx < (int)body.length() && body[idx] == ' ') idx++;

  // Read until non-number-ish
  int end = idx;
  while (end < (int)body.length()) {
    char c = body[end];
    if (!(c == '-' || c == '+' || c == '.' || (c >= '0' && c <= '9') || c == 'e' || c == 'E')) break;
    end++;
  }

  out = body.substring(idx, end).toInt();
  return true;
}

bool UiHttp::extractNumber(const String& body, const char* key, float& out) {
  String pat = String("\"") + key + "\":";
  int idx = body.indexOf(pat);
  if (idx < 0) return false;
  idx += pat.length();

  while (idx < (int)body.length() && body[idx] == ' ') idx++;

  int end = idx;
  while (end < (int)body.length()) {
    char c = body[end];
    if (!(c == '-' || c == '+' || c == '.' || (c >= '0' && c <= '9') || c == 'e' || c == 'E')) break;
    end++;
  }

  out = body.substring(idx, end).toFloat();
  return true;
}

void UiHttp::handleConfig() {
  String body;
  if (!readJsonBody(server_, body)) {
    BT_LOGW(TAG, "POST /api/config missing body");
    server_.send(400, "text/plain", "Missing body");
    return;
  }

  BT_LOGI(TAG, "POST /api/config body=%s", body.c_str());

  // --- Program (StateMachine) ------------------------------------------------
  Program p = sm_.getProgram();

  long ltmp;
  if (extractNumber(body, "cycles", ltmp)) {
    if (ltmp < 1) ltmp = 1;
    if (ltmp > 65535) ltmp = 65535;
    p.cycles = (uint16_t)ltmp;
  }

  if (body.indexOf("\"startMode\":\"discharge\"") >= 0) p.startMode = Mode::Discharge;
  if (body.indexOf("\"startMode\":\"charge\"") >= 0)    p.startMode = Mode::Charge;

  if (body.indexOf("\"stopMode\":\"discharge\"") >= 0) p.stopMode = Mode::Discharge;
  if (body.indexOf("\"stopMode\":\"charge\"") >= 0)    p.stopMode = Mode::Charge;

  sm_.setProgram(p);

  // --- CoreConfig (thresholds + waits) --------------------------------------
  CoreConfig cfg = core_.getConfig();

  float ftmp;
  if (extractNumber(body, "chargeStopVoltage_V", ftmp)) {
    cfg.chargeStopVoltage_V = ftmp;
  }

  if (extractNumber(body, "dischargeStopVoltage_V", ftmp)) {
    cfg.dischargeStopVoltage_V = ftmp;
  }

  if (extractNumber(body, "chargeStopHold_s", ltmp)) {
    if (ltmp < 0) ltmp = 0;
    cfg.chargeHoldAbove_s = (uint32_t)ltmp;
  }

  if (extractNumber(body, "waitChargeToDischarge_s", ltmp)) {
    if (ltmp < 0) ltmp = 0;
    cfg.waitChargeToDischarge_s = (uint32_t)ltmp;
  }

  if (extractNumber(body, "waitDischargeToCharge_s", ltmp)) {
    if (ltmp < 0) ltmp = 0;
    cfg.waitDischargeToCharge_s = (uint32_t)ltmp;
  }

  core_.setConfig(cfg);

  server_.send(200, "text/plain", "OK");
}

void UiHttp::handleGetConfig() {
  Program p = sm_.getProgram();
  CoreConfig cfg = core_.getConfig();

  const char* sm = (p.startMode == Mode::Discharge) ? "discharge" : "charge";
  const char* em = (p.stopMode  == Mode::Discharge) ? "discharge" : "charge";

  String json = "{";
  json += "\"cycles\":" + String((int)p.cycles) + ",";
  json += "\"startMode\":\"" + String(sm) + "\",";
  json += "\"stopMode\":\""  + String(em) + "\",";

  json += "\"chargeStopVoltage_V\":" + String(cfg.chargeStopVoltage_V, 3) + ",";
  json += "\"chargeStopHold_s\":" + String((uint32_t)cfg.chargeHoldAbove_s) + ",";
  json += "\"waitChargeToDischarge_s\":" + String((uint32_t)cfg.waitChargeToDischarge_s) + ",";
  json += "\"dischargeStopVoltage_V\":" + String(cfg.dischargeStopVoltage_V, 3) + ",";
  json += "\"waitDischargeToCharge_s\":" + String((uint32_t)cfg.waitDischargeToCharge_s);

  json += "}";

  server_.send(200, "application/json; charset=utf-8", json);
}


void UiHttp::handleDownload() {
  BT_LOGI(TAG, "Download log requested");

  // ---- Pass 1: count exact CSV byte length -------------------------------
  struct CountingPrint : public Print {
    size_t n = 0;
    size_t write(uint8_t) override { n++; return 1; }
    size_t write(const uint8_t* /*buf*/, size_t len) override { n += len; return len; }
  };

  CountingPrint counter;
  log_.printCsv(counter);
  const size_t totalBytes = counter.n;

  // ---- HTTP headers (known Content-Length, no chunked) --------------------
  server_.setContentLength(totalBytes);
  server_.sendHeader("Content-Disposition", "attachment; filename=\"battery_log.csv\"");
  server_.sendHeader("Connection", "close");
  server_.send(200, "text/csv; charset=utf-8", "");

  // ---- Pass 2: stream actual CSV -----------------------------------------
  WiFiClient c = server_.client();
  log_.printCsv(c);

  // Optional: explicitly close after sending
  c.stop();
}


bool UiHttp::readJsonBody(WebServer& s, String& out) {
  if (!s.hasArg("plain")) return false;
  out = s.arg("plain");
  return out.length() > 0;
}

