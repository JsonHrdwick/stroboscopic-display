#include "web_ui.h"
#include "config.h"
#include "drive.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
static unsigned long  s_last_web_cmd_ms = 0;
static const unsigned long WEB_OVERRIDE_TTL_MS = 10000;

// ── HTML page (served from flash) ─────────────────────────────────────────────
static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Strobe Display</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: monospace; background: #111; color: #eee;
         display: flex; flex-direction: column; align-items: center;
         padding: 20px; gap: 18px; }
  h1 { font-size: 1.4em; letter-spacing: 2px; color: #0df; margin-bottom: 4px; }
  .card { background: #1e1e1e; border: 1px solid #333; border-radius: 8px;
          padding: 16px; width: 100%; max-width: 420px; }
  .row { display: flex; align-items: center; gap: 10px; margin-bottom: 10px; }
  label { width: 120px; font-size: .85em; color: #aaa; flex-shrink: 0; }
  input[type=range] { flex: 1; accent-color: #0df; }
  .val { width: 60px; text-align: right; font-size: .9em; color: #0df; }
  .modes { display: flex; gap: 8px; flex-wrap: wrap; }
  button { padding: 8px 14px; border: 1px solid #444; border-radius: 6px;
           background: #2a2a2a; color: #ccc; cursor: pointer; font-family: monospace; }
  button.active { background: #0df; color: #000; border-color: #0df; }
  #status { font-size: .75em; color: #666; margin-top: 4px; }
  .delta-row { display: none; }
  .delta-row.visible { display: flex; }
</style>
</head>
<body>
<h1>⚡ STROBE DISPLAY</h1>

<div class="card">
  <div class="row">
    <label>Frequency</label>
    <input type="range" id="freq" min="5" max="120" step="0.5" value="30">
    <span class="val" id="freq_v">30.0 Hz</span>
  </div>
  <div class="row">
    <label>Phase</label>
    <input type="range" id="phase" min="0" max="359" step="1" value="0">
    <span class="val" id="phase_v">0°</span>
  </div>
  <div class="row">
    <label>Flash width</label>
    <input type="range" id="pulse" min="5" max="50" step="1" value="15">
    <span class="val" id="pulse_v">1500 µs</span>
  </div>
  <div class="row delta-row" id="delta_row">
    <label>Drift speed</label>
    <input type="range" id="delta" min="0.05" max="5" step="0.05" value="0.5">
    <span class="val" id="delta_v">0.50 Hz</span>
  </div>
</div>

<div class="card">
  <div style="font-size:.8em;color:#aaa;margin-bottom:10px;">MODE</div>
  <div class="modes">
    <button id="m_sync"    onclick="setMode('sync')">SYNC</button>
    <button id="m_slowmo"  onclick="setMode('slowmo')">SLOW MO</button>
    <button id="m_emoff"   onclick="setMode('emoff')">EM OFF</button>
    <button id="m_sweep"   onclick="setMode('sweep')">SWEEP</button>
  </div>
</div>

<div class="card">
  <div style="font-size:.8em;color:#aaa;margin-bottom:6px;">STATUS</div>
  <div id="status">connecting…</div>
</div>

<script>
const $ = id => document.getElementById(id);
let mode = 'sync';

function fmt(id, val, suffix) {
  $(id).textContent = val + suffix;
}

$('freq').oninput  = () => { fmt('freq_v',  parseFloat($('freq').value).toFixed(1), ' Hz'); send(); };
$('phase').oninput = () => { fmt('phase_v', $('phase').value, '°'); send(); };
$('pulse').oninput = () => { fmt('pulse_v', $('pulse').value * 100, ' µs'); send(); };
$('delta').oninput = () => { fmt('delta_v', parseFloat($('delta').value).toFixed(2), ' Hz'); send(); };

function setMode(m) {
  mode = m;
  ['sync','slowmo','emoff','sweep'].forEach(k =>
    $('m_' + k).classList.toggle('active', k === m)
  );
  $('delta_row').classList.toggle('visible', m === 'slowmo');
  // Phase slider irrelevant in slow-mo / sweep
  $('phase').disabled = (m === 'slowmo' || m === 'sweep');
  send();
}

function send() {
  fetch('/api/set', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({
      freq:  parseFloat($('freq').value),
      phase: parseInt($('phase').value),
      pulse: parseInt($('pulse').value),
      delta: parseFloat($('delta').value),
      mode:  mode
    })
  });
}

async function poll() {
  try {
    const r = await fetch('/api/status');
    const d = await r.json();
    $('status').textContent =
      `drive: ${d.freq.toFixed(1)} Hz  |  phase: ${d.phase.toFixed(0)}°  |  mode: ${d.mode}`;
    // Sync sliders on first load
    if (!poll.synced) {
      $('freq').value  = d.freq;
      $('phase').value = d.phase;
      $('pulse').value = d.pulse;
      fmt('freq_v',  d.freq.toFixed(1), ' Hz');
      fmt('phase_v', d.phase.toFixed(0), '°');
      fmt('pulse_v', d.pulse * 100, ' µs');
      setMode(d.mode);
      poll.synced = true;
    }
  } catch(e) {
    $('status').textContent = 'offline';
  }
}

setMode('sync');
setInterval(poll, 1500);
poll();
</script>
</body>
</html>
)rawhtml";

// ── Mode string helpers ────────────────────────────────────────────────────────
static const char* mode_to_str(DriveMode m) {
    switch (m) {
        case DriveMode::SYNC:    return "sync";
        case DriveMode::SLOW_MO: return "slowmo";
        case DriveMode::EM_OFF:  return "emoff";
        case DriveMode::SWEEP:   return "sweep";
    }
    return "sync";
}

static DriveMode str_to_mode(const char* s) {
    if (strcmp(s, "slowmo") == 0) return DriveMode::SLOW_MO;
    if (strcmp(s, "emoff")  == 0) return DriveMode::EM_OFF;
    if (strcmp(s, "sweep")  == 0) return DriveMode::SWEEP;
    return DriveMode::SYNC;
}

// ── Public ────────────────────────────────────────────────────────────────────
void web_ui_init() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, strlen(WIFI_PASS) ? WIFI_PASS : nullptr);
    Serial.printf("[wifi] AP: %s  IP: %s\n", WIFI_SSID, WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", INDEX_HTML);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        char buf[128];
        snprintf(buf, sizeof(buf),
            "{\"freq\":%.2f,\"phase\":%.1f,\"pulse\":%d,\"mode\":\"%s\"}",
            drive_get_freq(), drive_get_phase(),
            (int)drive_get_led_pulse(), mode_to_str(drive_get_mode()));
        req->send(200, "application/json", buf);
    });

    // POST /api/set  body: {"freq":30,"phase":90,"pulse":15,"delta":0.5,"mode":"sync"}
    AsyncCallbackJsonWebHandler* set_handler =
        new AsyncCallbackJsonWebHandler("/api/set",
            [](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!json.is<JsonObject>()) { req->send(400); return; }
                JsonObject obj = json.as<JsonObject>();

                if (obj["freq"].is<float>())  drive_set_freq(obj["freq"].as<float>());
                if (obj["phase"].is<float>()) drive_set_phase(obj["phase"].as<float>());
                if (obj["pulse"].is<int>())   drive_set_led_pulse(obj["pulse"].as<int>());
                if (obj["delta"].is<float>()) drive_set_slow_mo_delta(obj["delta"].as<float>());
                if (obj["mode"].is<const char*>())
                    drive_set_mode(str_to_mode(obj["mode"].as<const char*>()));

                s_last_web_cmd_ms = millis();
                req->send(200, "application/json", "{\"ok\":true}");
            });
    server.addHandler(set_handler);

    server.begin();
}

void web_ui_poll() {
    // Nothing needed — AsyncWebServer runs on its own tasks.
}

bool web_ui_has_override() {
    return (millis() - s_last_web_cmd_ms) < WEB_OVERRIDE_TTL_MS;
}
