#pragma once
#include <Arduino.h>

static const char PORTAL_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>CYD HA Dashboard Portal</title>
<style>
  :root {
    --bg-dark: #0f172a;
    --card-bg: #1e293b;
    --card-border: #334155;
    --primary: #38bdf8;
    --primary-hover: #0284c7;
    --success: #10b981;
    --warning: #f59e0b;
    --danger: #ef4444;
    --text: #f8fafc;
    --text-muted: #94a3b8;
    --input-bg: #0f172a;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; }
  body { background: var(--bg-dark); color: var(--text); padding: 16px; min-height: 100vh; }
  .container { max-width: 720px; margin: 0 auto; }
  .header { display: flex; align-items: center; justify-content: space-between; padding-bottom: 12px; border-bottom: 1px solid var(--card-border); margin-bottom: 16px; }
  .header h1 { font-size: 20px; font-weight: 700; color: var(--primary); }
  .badge { font-size: 11px; padding: 3px 8px; border-radius: 9999px; background: rgba(56, 189, 248, 0.15); color: var(--primary); font-weight: 600; border: 1px solid rgba(56, 189, 248, 0.3); }
  .tabs { display: flex; gap: 8px; margin-bottom: 16px; background: #141f36; padding: 4px; border-radius: 8px; border: 1px solid var(--card-border); }
  .tab-btn { flex: 1; padding: 8px 12px; border: none; background: transparent; color: var(--text-muted); font-size: 13px; font-weight: 600; border-radius: 6px; cursor: pointer; transition: all 0.2s; }
  .tab-btn.active { background: var(--primary); color: #0f172a; }
  .tab-content { display: none; }
  .tab-content.active { display: block; }
  .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 16px; margin-bottom: 16px; }
  .card-title { font-size: 15px; font-weight: 600; color: var(--text); margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center; }
  label { display: block; font-size: 12px; font-weight: 600; color: var(--text-muted); margin-bottom: 4px; margin-top: 10px; }
  label:first-child { margin-top: 0; }
  input, select { width: 100%; padding: 9px 12px; background: var(--input-bg); border: 1px solid var(--card-border); border-radius: 6px; color: var(--text); font-size: 14px; outline: none; transition: border-color 0.2s; }
  input[type="color"] { padding: 0; -webkit-appearance: none; appearance: none; border: none; cursor: pointer; border-radius: 6px; }
  input[type="color"]::-webkit-color-swatch-wrapper { padding: 0; border-radius: 6px; }
  input[type="color"]::-webkit-color-swatch { border: 1px solid rgba(255,255,255,0.25); border-radius: 6px; }
  input[type="color"]::-moz-color-swatch { border: 1px solid rgba(255,255,255,0.25); border-radius: 6px; }
  input:focus, select:focus { border-color: var(--primary); }
  .btn { display: inline-flex; align-items: center; justify-content: center; gap: 6px; padding: 9px 16px; border: none; border-radius: 6px; font-size: 14px; font-weight: 600; cursor: pointer; transition: opacity 0.2s, background-color 0.2s; }
  .btn-primary { background: var(--primary); color: #0f172a; width: 100%; }
  .btn-primary:hover { background: var(--primary-hover); }
  .btn-secondary { background: #334155; color: var(--text); }
  .btn-secondary:hover { background: #475569; }
  .btn-success { background: var(--success); color: #0f172a; }
  .btn-danger { background: rgba(239, 68, 68, 0.2); color: var(--danger); border: 1px solid rgba(239, 68, 68, 0.4); }
  .btn-danger:hover { background: rgba(239, 68, 68, 0.3); }
  .btn-sm { padding: 6px 10px; font-size: 12px; }
  .page-nav { display: flex; gap: 8px; align-items: center; margin-bottom: 14px; }
  .page-nav select { flex: 1; }
  .grid-2x2 { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 14px; }
  .grid-3x2 { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; margin-bottom: 14px; }
  @media (max-width: 600px) { .grid-3x2 { grid-template-columns: 1fr 1fr; } }
  @media (max-width: 480px) { .grid-2x2, .grid-3x2 { grid-template-columns: 1fr; } }
  .slot-card { background: #141f36; border: 1px solid #293548; border-radius: 8px; padding: 12px; position: relative; }
  .slot-header { font-size: 11px; font-weight: 700; color: var(--primary); text-transform: uppercase; margin-bottom: 6px; display: flex; justify-content: space-between; align-items: center; }
  .slot-icon-badge { font-size: 11px; color: var(--warning); }
  .action-bar { display: flex; gap: 10px; }
  .alert { padding: 10px 14px; border-radius: 6px; font-size: 13px; font-weight: 500; margin-bottom: 12px; display: none; }
  .alert-success { background: rgba(16, 185, 129, 0.15); border: 1px solid var(--success); color: var(--success); }
  .alert-error { background: rgba(239, 68, 68, 0.15); border: 1px solid var(--danger); color: var(--danger); }
  .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
  .stat-card { background: #141f36; padding: 10px; border-radius: 8px; border: 1px solid var(--card-border); }
  .stat-label { font-size: 11px; color: var(--text-muted); }
  .stat-val { font-size: 15px; font-weight: 700; color: var(--text); margin-top: 2px; }
  .hint { font-size: 11px; color: var(--text-muted); margin-top: 3px; }
  .hidden { display: none !important; }
  .search-wrap { position: relative; margin-bottom: 6px; }
  .search-results { position: absolute; top: 100%; left: 0; right: 0; background: #1e293b; border: 1px solid var(--primary); border-radius: 6px; max-height: 200px; overflow-y: auto; z-index: 100; box-shadow: 0 6px 16px rgba(0,0,0,0.6); margin-top: 3px; }
  .search-item { padding: 8px 10px; border-bottom: 1px solid #334155; cursor: pointer; display: flex; justify-content: space-between; align-items: center; }
  .search-item:hover { background: #334155; }
  .search-item:last-child { border-bottom: none; }
  .search-name { font-size: 13px; font-weight: 600; color: var(--text); }
  .search-id { font-size: 11px; color: var(--text-muted); }
  .search-tag { font-size: 10px; padding: 2px 6px; border-radius: 4px; background: rgba(56, 189, 248, 0.2); color: var(--primary); white-space: nowrap; }
  @keyframes spin { 100% { transform: rotate(360deg); } }
  .spinning { animation: spin 1s linear infinite; display: inline-block; }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <div>
      <h1>CYD HA Dashboard</h1>
      <div style="font-size:12px;color:var(--text-muted);"><span id="header-hw-model">%HARDWARE_MODEL%</span> &bull; Web Portal</div>
    </div>
    <div style="display:flex;align-items:center;gap:8px;flex-wrap:wrap;justify-content:flex-end;">
      <span class="badge" id="ha-badge">HA: Checking...</span>
      <button id="btn-sync" class="btn btn-secondary btn-sm" onclick="syncEntities(true)" title="Fetch latest entities from Home Assistant" style="gap:5px;">
        <span id="sync-icon">&#x21bb;</span>
        <span id="sync-text">Sync Entities</span>
      </button>
      <span id="sync-badge" style="font-size:11px;color:var(--text-muted);white-space:nowrap;">Not synced</span>
    </div>
  </div>

  <div class="tabs">
    <button class="tab-btn active" onclick="switchTab('dashboard')">&#128241; Dashboard</button>
    <button class="tab-btn" onclick="switchTab('settings')">&#9881; Settings</button>
    <button class="tab-btn" onclick="switchTab('system')">&#128295; System</button>
  </div>

  <div id="alert-box" class="alert"></div>

  <!-- TAB 1: DASHBOARD EDITOR -->
  <div id="tab-dashboard" class="tab-content active">
    <div class="card">
      <div class="card-title">
        <span>Active Page</span>
        <div style="display:flex;gap:6px;">
          <button class="btn btn-secondary btn-sm" onclick="addPage()">+ Add Page</button>
          <button class="btn btn-secondary btn-sm" onclick="renamePage()">Rename</button>
          <button class="btn btn-danger btn-sm" onclick="deletePage()">Delete</button>
        </div>
      </div>

      <div class="page-nav">
        <select id="page-select" onchange="onPageChange()"></select>
      </div>

      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:14px;background:#141f36;padding:10px 12px;border-radius:8px;border:1px solid var(--card-border);">
        <label style="margin:0;font-size:13px;color:var(--text);">Grid Layout Size</label>
        <select id="page-grid" style="width:auto;min-width:180px;padding:6px 10px;font-size:13px;" onchange="onGridChange()">
          <option value="2x2">2x2 Grid (4 Tiles - Large)</option>
          <option value="3x2">3x2 Grid (6 Tiles - Compact)</option>
        </select>
      </div>

      <div id="slots-container">
        <!-- Grid Slots Generated by JS -->
      </div>

      <div class="action-bar" style="margin-top:16px;">
        <button class="btn btn-primary" onclick="saveDashboardConfig()">&#128190; Save &amp; Apply to CYD</button>
      </div>
    </div>
  </div>

  <!-- TAB 2: SETTINGS (WI-FI & HA) -->
  <div id="tab-settings" class="tab-content">
    <div class="card">
      <div class="card-title">Wi-Fi Connection</div>
      <form method="POST" action="/save" onsubmit="prepareSubmit()">
        <label>Wi-Fi Network (2.4 GHz)</label>
        <div style="display:flex;gap:8px;">
          <select id="ssid_select" onchange="onSsidChange()">
            <option value="">Scanning networks...</option>
          </select>
          <button type="button" class="btn btn-secondary btn-sm" onclick="scanWifi()">&#8635; Scan</button>
        </div>
        <div id="custom_ssid_wrap" style="display:none;margin-top:8px;">
          <input id="custom_ssid" placeholder="Enter hidden SSID manually">
        </div>
        <input type="hidden" name="wifi_ssid" id="wifi_ssid">

        <label>Wi-Fi Password</label>
        <input name="wifi_password" type="password" autocomplete="off" placeholder="Leave blank to keep existing password">

        <div class="card-title" style="margin-top:20px;border-top:1px solid var(--card-border);padding-top:16px;">Home Assistant Settings</div>
        <label>Home Assistant Base URL</label>
        <input name="ha_url" value="%HA_URL%" placeholder="http://192.168.1.100:8123">
        <div class="hint">Must be reachable over your local Wi-Fi.</div>

        <label>Long-Lived Access Token</label>
        <input name="ha_token" type="password" autocomplete="off" placeholder="%HA_TOKEN_PLACEHOLDER%">
        <div class="hint">Secret stored securely in NVS. Leave blank to keep existing token.</div>

        <label>Default Entity ID (Fallback)</label>
        <input name="ha_entity" value="%HA_ENTITY%" placeholder="switch.living_room_light">

        <button type="submit" class="btn btn-primary" style="margin-top:16px;">Save &amp; Reconnect</button>
      </form>
    </div>
  </div>

  <!-- TAB 3: SYSTEM & TOOLS -->
  <div id="tab-system" class="tab-content">
    <div class="card">
      <div class="card-title">Device Status</div>
      <div class="stat-grid">
        <div class="stat-card">
          <div class="stat-label">Hardware Model</div>
          <div class="stat-val" id="stat-hw" style="font-size:12px;font-weight:600;">%HARDWARE_MODEL%</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Device IP</div>
          <div class="stat-val" id="stat-ip">--</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Wi-Fi RSSI</div>
          <div class="stat-val" id="stat-rssi">--</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Free RAM</div>
          <div class="stat-val" id="stat-heap">--</div>
          <div class="stat-label" id="stat-min-heap" style="font-size:10px;margin-top:2px;color:var(--text-muted);">--</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Uptime</div>
          <div class="stat-val" id="stat-uptime">--</div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">&#127912; Dashboard Theme &amp; Mode</div>
      <div style="display:grid;grid-template-columns:1fr;gap:12px;">
        <div>
          <label style="font-size:12px;margin-bottom:6px;">Select CYD Screen Color Scheme</label>
          <select id="theme-select" onchange="updateThemeUI()">
            <optgroup label="Home Assistant (Official)">
              <option value="ha_dark">Home Assistant Dark (Cyan &amp; Charcoal)</option>
              <option value="ha_light">Home Assistant Light (Blue &amp; Clean White)</option>
            </optgroup>
            <optgroup label="Slate">
              <option value="slate_dark">Slate Dark (Deep Navy &amp; Emerald)</option>
              <option value="slate_light">Slate Light (Soft Gray &amp; Forest)</option>
            </optgroup>
            <optgroup label="Midnight OLED">
              <option value="oled_dark">Midnight OLED Dark (True Black &amp; Indigo)</option>
              <option value="oled_light">Midnight Monokai Light (High Contrast White)</option>
            </optgroup>
            <optgroup label="Forest">
              <option value="forest_dark">Forest Pine Dark (Deep Green &amp; Mint)</option>
              <option value="forest_light">Forest Sage Light (Soft Sage &amp; Emerald)</option>
            </optgroup>
            <optgroup label="Amber Cyberpunk">
              <option value="amber_dark">Amber Cyberpunk Dark (Warm Carbon &amp; Gold)</option>
              <option value="amber_light">Amber Sand Light (Warm Cream &amp; Amber)</option>
            </optgroup>
          </select>
        </div>
      </div>
      <button class="btn btn-primary" style="margin-top:14px;width:100%;" onclick="saveThemeConfig()">&#128190; Save Theme</button>
    </div>

    <div class="card">
      <div class="card-title">&#128161; Display &amp; Backlight Power</div>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;">
        <div>
          <label style="font-size:12px;margin-bottom:4px;">Active Brightness: <span id="val-active-bright">100%</span></label>
          <input type="range" id="disp-active-bright" min="10" max="100" value="100" oninput="updateDispUI()" style="width:100%;">
        </div>
        <div>
          <label style="font-size:12px;margin-bottom:4px;">Dim Brightness: <span id="val-dim-bright">15%</span></label>
          <input type="range" id="disp-dim-bright" min="0" max="50" value="15" oninput="updateDispUI()" style="width:100%;">
        </div>
        <div>
          <label style="font-size:12px;margin-bottom:4px;">Dim Timeout (Idle)</label>
          <select id="disp-dim-timeout" onchange="updateDispUI()">
            <option value="0">Disabled (Always Active)</option>
            <option value="15">15 seconds</option>
            <option value="30" selected>30 seconds</option>
            <option value="60">1 minute</option>
            <option value="120">2 minutes</option>
            <option value="300">5 minutes</option>
          </select>
        </div>
        <div>
          <label style="font-size:12px;margin-bottom:4px;">Screen Sleep (Backlight Off)</label>
          <select id="disp-sleep-timeout" onchange="updateDispUI()">
            <option value="0">Disabled (Never Off)</option>
            <option value="30">30 seconds</option>
            <option value="60" selected>1 minute</option>
            <option value="120">2 minutes</option>
            <option value="300">5 minutes</option>
            <option value="600">10 minutes</option>
          </select>
        </div>
      </div>
      <button class="btn btn-primary" style="margin-top:14px;width:100%;" onclick="saveDisplayConfig()">&#128190; Save Display Settings</button>
    </div>

    <div class="card">
      <div class="card-title">&#128640; Firmware &amp; GitHub Updates</div>
      <div style="font-size:12px;margin-bottom:8px;color:var(--text-muted);">
        Installed Version: <b id="stat-version" style="color:var(--primary);">v0.1.0</b> &bull; Repository: <b>demonkazuya/CYD-HA-Dashboard</b>
      </div>

      <div id="ota-status-box" style="background:#141f36;padding:10px;border-radius:6px;border:1px solid var(--card-border);margin-bottom:12px;font-size:12px;display:none;">
        <div id="ota-status-text">Checking for updates...</div>
        <div id="ota-progress-wrap" style="display:none;margin-top:8px;background:#0b1329;border-radius:4px;overflow:hidden;height:12px;">
          <div id="ota-progress-bar" style="width:0%;height:100%;background:var(--success);transition:width 0.2s;"></div>
        </div>
      </div>

      <div style="display:flex;flex-direction:column;gap:8px;">
        <button type="button" class="btn btn-secondary" id="btn-check-ota" onclick="checkGitHubRelease()" style="width:100%;">&#128260; Check for GitHub Updates</button>
        <button type="button" class="btn btn-primary" id="btn-install-ota" onclick="installGitHubRelease()" style="width:100%;display:none;">&#9889; Install Update Now</button>

        <div style="border-top:1px solid var(--card-border);padding-top:10px;margin-top:4px;">
          <label style="font-size:11px;color:var(--text-muted);display:block;margin-bottom:4px;">Manual Firmware (.bin) Upload</label>
          <input type="file" id="manual-ota-file" accept=".bin" style="display:none;" onchange="uploadManualOta(this)">
          <button type="button" class="btn btn-secondary" onclick="document.getElementById('manual-ota-file').click()" style="width:100%;">&#128194; Upload Local .bin File</button>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Maintenance &amp; Calibration</div>
      <div style="display:flex;flex-direction:column;gap:10px;">
        <form method="POST" action="/calibrate" onsubmit="return confirm('Recalibrate touchscreen? Device will reboot into 2-point calibration mode.')">
          <button type="submit" class="btn btn-secondary" style="width:100%;">&#127919; Recalibrate Screen Touch</button>
        </form>

        <div style="display:flex;gap:8px;">
          <button type="button" class="btn btn-secondary" style="flex:1;" onclick="downloadConfig()">&#128190; Export Config</button>
          <input type="file" id="import-config-file" accept=".json,application/json" onchange="uploadConfigFile(this)" style="display:none;">
          <button type="button" class="btn btn-secondary" style="flex:1;" onclick="document.getElementById('import-config-file').click()">&#128229; Import Config</button>
        </div>

        <form method="POST" action="/api/reboot" onsubmit="return confirm('Restart device now?')">
          <button type="submit" class="btn btn-danger" style="width:100%;">&#8635; Restart Device</button>
        </form>
      </div>
    </div>
  </div>
</div>

<script>
var currentConfig = { pages: [] };
var activePageIdx = 0;
var cachedEntities = [];
var isSyncing = false;

function loadCachedEntities() {
  var demoItems = [
    { id: "light.cyd_demo_rgb", name: "[Demo] RGB Dimmer Light", icon: "mdi:lightbulb", type: "toggle" },
    { id: "fan.cyd_demo_fan", name: "[Demo] Ceiling Fan Speed", icon: "mdi:fan", type: "toggle" },
    { id: "cover.cyd_demo_blinds", name: "[Demo] Window Blinds", icon: "mdi:blinds", type: "toggle" },
    { id: "scene.cyd_demo_movie", name: "[Demo] Movie Night Scene", icon: "mdi:play", type: "toggle" },
    { id: "climate.cyd_demo_thermostat", name: "[Demo] Living Room Thermostat", icon: "mdi:thermostat", type: "toggle" }
  ];
  try {
    var raw = localStorage.getItem('cyd_ha_entities');
    var ts = localStorage.getItem('cyd_ha_synced_at');
    if (raw) {
      cachedEntities = JSON.parse(raw);
      var hasDemo = cachedEntities.some(function(e) { return e.id === "light.cyd_demo_rgb"; });
      if (!hasDemo) {
        cachedEntities = demoItems.concat(cachedEntities);
      }
      updateSyncStatusUI(cachedEntities.length, ts ? parseInt(ts) : null, false, false);
      return true;
    } else {
      cachedEntities = demoItems;
      return true;
    }
  } catch(e) {
    console.error('Error loading cached entities', e);
    cachedEntities = demoItems;
  }
  return false;
}

function updateSyncStatusUI(count, timestamp, syncing, error) {
  var icon = document.getElementById('sync-icon');
  var text = document.getElementById('sync-text');
  var badge = document.getElementById('sync-badge');
  var btn = document.getElementById('btn-sync');

  if (syncing) {
    if (icon) icon.classList.add('spinning');
    if (text) text.textContent = 'Syncing...';
    if (btn) btn.disabled = true;
    if (badge) {
      badge.textContent = 'Fetching from HA...';
      badge.style.color = 'var(--primary)';
    }
  } else {
    if (icon) icon.classList.remove('spinning');
    if (text) text.textContent = 'Sync Entities';
    if (btn) btn.disabled = false;
    if (error) {
      if (badge) {
        badge.innerHTML = '&#9888; Sync failed';
        badge.style.color = 'var(--danger)';
      }
    } else if (count > 0) {
      var timeAgo = 'just now';
      if (timestamp) {
        var diffMin = Math.round((Date.now() - timestamp) / 60000);
        timeAgo = (diffMin <= 1) ? 'just now' : (diffMin + 'm ago');
      }
      if (badge) {
        badge.innerHTML = '<span style="color:var(--success);">&#10004;</span> Synced (' + count + ' &bull; ' + timeAgo + ')';
        badge.style.color = 'var(--text-muted)';
      }
    } else {
      if (badge) {
        badge.textContent = 'Not synced';
        badge.style.color = 'var(--text-muted)';
      }
    }
  }
}

function syncEntities(manual) {
  if (isSyncing) return;
  isSyncing = true;
  updateSyncStatusUI(cachedEntities.length, null, true, false);

  fetch('/api/entities')
    .then(function(r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.text();
    })
    .then(function(text) {
      var clean = text.replace(/,\s*\]/, ']');
      var list = JSON.parse(clean);
      var demoItems = [
        { id: "light.cyd_demo_rgb", name: "[Demo] RGB Dimmer Light", icon: "mdi:lightbulb", type: "toggle", rgb: true },
        { id: "fan.cyd_demo_fan", name: "[Demo] Ceiling Fan Speed", icon: "mdi:fan", type: "toggle", rgb: false },
        { id: "cover.cyd_demo_blinds", name: "[Demo] Window Blinds", icon: "mdi:blinds", type: "toggle", rgb: false },
        { id: "scene.cyd_demo_movie", name: "[Demo] Movie Night Scene", icon: "mdi:play", type: "toggle", rgb: false },
        { id: "climate.cyd_demo_thermostat", name: "[Demo] Living Room Thermostat", icon: "mdi:thermostat", type: "toggle", rgb: false }
      ];
      var hasDemo = list.some(function(e) { return e.id === "light.cyd_demo_rgb"; });
      if (!hasDemo) {
        list = demoItems.concat(list);
      }
      cachedEntities = list;
      var now = Date.now();
      try {
        localStorage.setItem('cyd_ha_entities', JSON.stringify(list));
        localStorage.setItem('cyd_ha_synced_at', now.toString());
      } catch(e) {}
      isSyncing = false;
      updateSyncStatusUI(list.length, now, false, false);
      showAlert('Synced ' + list.length + ' entities from Home Assistant!', false);
    })
    .catch(function(err) {
      console.error('Entity sync error:', err);
      isSyncing = false;
      updateSyncStatusUI(cachedEntities.length, null, false, true);
      if (manual) {
        showAlert('Entity sync failed. Verify HA settings in Settings tab.', true);
      }
    });
}

function switchTab(name) {
  document.querySelectorAll('.tab-btn').forEach(function(b){ b.classList.remove('active'); });
  document.querySelectorAll('.tab-content').forEach(function(c){ c.classList.remove('active'); });
  event.target.classList.add('active');
  document.getElementById('tab-' + name).classList.add('active');
  if (name === 'system') loadStatus();
}

function showAlert(msg, isError) {
  var b = document.getElementById('alert-box');
  b.className = 'alert ' + (isError ? 'alert-error' : 'alert-success');
  b.textContent = msg;
  b.style.display = 'block';
  setTimeout(function(){ b.style.display = 'none'; }, 4000);
}

function updateDispUI() {
  var ab = document.getElementById('disp-active-bright');
  var db = document.getElementById('disp-dim-bright');
  var dt = document.getElementById('disp-dim-timeout');
  var st = document.getElementById('disp-sleep-timeout');
  if (!ab || !db || !dt || !st) return;

  var a = ab.value;
  var d = db.value;
  document.getElementById('val-active-bright').textContent = a + '%';
  document.getElementById('val-dim-bright').textContent = d + '%';

  if (!currentConfig.display) currentConfig.display = {};
  currentConfig.display.brightness = Math.round(a * 2.55);
  currentConfig.display.dim_brightness = Math.round(d * 2.55);
  currentConfig.display.dim_timeout = parseInt(dt.value);
  currentConfig.display.sleep_timeout = parseInt(st.value);
}

function saveDisplayConfig() {
  updateDispUI();
  fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(currentConfig)
  })
  .then(function(r){ return r.json(); })
  .then(function(res){
    showAlert('Display settings saved! Applied live on CYD.', false);
  })
  .catch(function(err){
    showAlert('Error saving display settings.', true);
  });
}

function updateThemeUI() {
  var ts = document.getElementById('theme-select');
  if (ts) {
    currentConfig.theme = ts.value;
  }
}

function saveThemeConfig() {
  updateThemeUI();
  fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(currentConfig)
  })
  .then(function(r){ return r.json(); })
  .then(function(res){
    showAlert('Theme updated! Applied live on CYD.', false);
  })
  .catch(function(err){
    showAlert('Error saving theme.', true);
  });
}

function loadDashboardConfig() {
  fetch('/api/config')
    .then(function(r){ return r.json(); })
    .then(function(cfg){
      currentConfig = cfg;
      if (!currentConfig.pages || currentConfig.pages.length === 0) {
        currentConfig.pages = [{ id: 'p1', name: 'Page 1', grid: '2x2', widgets: [] }];
      }
      if (currentConfig.color_presets && Array.isArray(currentConfig.color_presets)) {
        for (var c = 0; c < 6; c++) {
          var el = document.getElementById('preset-color-' + c);
          if (el && currentConfig.color_presets[c]) {
            el.value = currentConfig.color_presets[c];
          }
        }
      }

      if (!currentConfig.display) {
        currentConfig.display = { brightness: 255, dim_brightness: 40, dim_timeout: 30, sleep_timeout: 60 };
      }
      var ab = document.getElementById('disp-active-bright');
      var db = document.getElementById('disp-dim-bright');
      var dt = document.getElementById('disp-dim-timeout');
      var st = document.getElementById('disp-sleep-timeout');
      if (ab) ab.value = Math.round((currentConfig.display.brightness || 255) / 255 * 100);
      if (db) db.value = Math.round((currentConfig.display.dim_brightness || 40) / 255 * 100);
      if (dt) dt.value = currentConfig.display.dim_timeout !== undefined ? currentConfig.display.dim_timeout : 30;
      if (st) st.value = currentConfig.display.sleep_timeout !== undefined ? currentConfig.display.sleep_timeout : 60;
      updateDispUI();

      var ts = document.getElementById('theme-select');
      if (ts) ts.value = currentConfig.theme || 'ha_dark';

      renderPageSelect();
      renderSlots();
    })
    .catch(function(err){
      console.error('Config fetch failed:', err);
    });
}

function renderPageSelect() {
  var sel = document.getElementById('page-select');
  sel.innerHTML = '';
  currentConfig.pages.forEach(function(p, i){
    var opt = document.createElement('option');
    opt.value = i;
    opt.textContent = (i + 1) + '. ' + p.name;
    if (i === activePageIdx) opt.selected = true;
    sel.appendChild(opt);
  });
  var p = currentConfig.pages[activePageIdx];
  if (p) {
    document.getElementById('page-grid').value = p.grid || '2x2';
  }
}

function onPageChange() {
  syncCurrentSlotsToMemory();
  activePageIdx = parseInt(document.getElementById('page-select').value) || 0;
  var p = currentConfig.pages[activePageIdx];
  if (p) {
    document.getElementById('page-grid').value = p.grid || '2x2';
  }
  renderSlots();
}

function onGridChange() {
  var p = currentConfig.pages[activePageIdx];
  if (p) {
    p.grid = document.getElementById('page-grid').value;
    renderSlots();
  }
}

function addPage() {
  var name = prompt('Enter new page name:');
  if (!name || !name.trim()) return;
  syncCurrentSlotsToMemory();
  var newId = 'page_' + Date.now();
  currentConfig.pages.push({ id: newId, name: name.trim(), grid: '2x2', widgets: [] });
  activePageIdx = currentConfig.pages.length - 1;
  renderPageSelect();
  renderSlots();
}

function renamePage() {
  var p = currentConfig.pages[activePageIdx];
  if (!p) return;
  var name = prompt('Rename page:', p.name);
  if (!name || !name.trim()) return;
  p.name = name.trim();
  renderPageSelect();
}

function deletePage() {
  if (currentConfig.pages.length <= 1) {
    alert('Dashboard must have at least one page.');
    return;
  }
  if (!confirm('Delete page "' + currentConfig.pages[activePageIdx].name + '"?')) return;
  currentConfig.pages.splice(activePageIdx, 1);
  if (activePageIdx >= currentConfig.pages.length) activePageIdx = currentConfig.pages.length - 1;
  renderPageSelect();
  renderSlots();
}

function autoDetectType(entityId) {
  if (!entityId) return 'toggle';
  var id = entityId.trim().toLowerCase();
  if (id.startsWith('binary_sensor.')) return 'binary_sensor';
  if (id.startsWith('sensor.') || id.startsWith('weather.') || id.startsWith('number.')) return 'sensor';
  return 'toggle'; // light, switch, fan, scene, script, input_boolean, etc.
}

function isEntityRgb(entityId) {
  if (!entityId) return false;
  var id = entityId.trim().toLowerCase();
  if (id === 'light.cyd_demo_rgb') return true;
  if (!id.startsWith('light.')) return false;
  if (cachedEntities && cachedEntities.length > 0) {
    for (var i = 0; i < cachedEntities.length; i++) {
      if (cachedEntities[i].id.toLowerCase() === id) {
        return cachedEntities[i].rgb === true;
      }
    }
  }
  return false;
}

function updateSlotRgbVisibility(slotIdx, entityId) {
  var wrap = document.getElementById('slot-rgb-wrap-' + slotIdx);
  if (!wrap) return;
  if (isEntityRgb(entityId)) {
    wrap.classList.remove('hidden');
  } else {
    wrap.classList.add('hidden');
  }
}

function onEntityInput(slotIdx, val) {
  var inType = document.getElementById('slot-type-' + slotIdx);
  if (inType && val) {
    inType.value = autoDetectType(val);
  }
  updateSlotRgbVisibility(slotIdx, val);
}

function syncCurrentSlotsToMemory() {
  var p = currentConfig.pages[activePageIdx];
  if (!p) return;
  p.grid = document.getElementById('page-grid').value;
  var maxSlots = (p.grid === '3x2') ? 6 : 4;
  p.widgets = [];
  for (var i = 0; i < maxSlots; i++) {
    var label = (document.getElementById('slot-label-' + i) || {}).value || '';
    var type = (document.getElementById('slot-type-' + i) || {}).value || 'toggle';
    var entity = (document.getElementById('slot-entity-' + i) || {}).value || '';
    var icon = (document.getElementById('slot-icon-' + i) || {}).value || '';
    if (entity.trim()) {
      var widgetObj = {
        id: 'w_' + i,
        slot: i,
        label: label.trim() || entity.trim(),
        type: type,
        icon: icon.trim(),
        entity_id: entity.trim()
      };
      var wrap = document.getElementById('slot-rgb-wrap-' + i);
      if (wrap && !wrap.classList.contains('hidden')) {
        var presets = [];
        for (var c = 0; c < 6; c++) {
          var cp = document.getElementById('slot-color-' + i + '-' + c);
          if (cp) presets.push(cp.value);
        }
        if (presets.length === 6) {
          widgetObj.color_presets = presets;
        }
      }
      p.widgets.push(widgetObj);
    }
  }
}

function renderSlots() {
  var p = currentConfig.pages[activePageIdx] || { widgets: [], grid: '2x2' };
  var is3x2 = (p.grid === '3x2');
  var numSlots = is3x2 ? 6 : 4;
  var container = document.getElementById('slots-container');
  container.className = is3x2 ? 'grid-3x2' : 'grid-2x2';
  container.innerHTML = '';

  var slotNames = is3x2 ?
    ['Slot 1 (Row 1 Col 1)', 'Slot 2 (Row 1 Col 2)', 'Slot 3 (Row 1 Col 3)',
     'Slot 4 (Row 2 Col 1)', 'Slot 5 (Row 2 Col 2)', 'Slot 6 (Row 2 Col 3)'] :
    ['Slot 1 (Top-Left)', 'Slot 2 (Top-Right)', 'Slot 3 (Bottom-Left)', 'Slot 4 (Bottom-Right)'];

  var globalDefs = currentConfig.color_presets && currentConfig.color_presets.length === 6 ?
    currentConfig.color_presets : ['#FFF1E0', '#EF4444', '#F59E0B', '#10B981', '#38BDF8', '#A855F7'];

  for (var i = 0; i < numSlots; i++) {
    var existing = null;
    if (p.widgets) {
      for (var w = 0; w < p.widgets.length; w++) {
        if (p.widgets[w].slot === i) { existing = p.widgets[w]; break; }
      }
    }

    var entId = existing ? existing.entity_id : '';
    var isRgb = isEntityRgb(entId);
    var slotPresets = (existing && existing.color_presets && existing.color_presets.length === 6) ?
      existing.color_presets : globalDefs;

    var rgbPaletteHtml =
      '<div id="slot-rgb-wrap-' + i + '" class="' + (isRgb ? '' : 'hidden') + '" style="margin-top:10px;padding:8px;background:#0f172a;border-radius:6px;border:1px solid var(--card-border);">' +
        '<div style="font-size:11px;font-weight:600;color:var(--text);margin-bottom:6px;">&#127912; Custom RGB Presets (Entity specific)</div>' +
        '<div style="display:grid;grid-template-columns:repeat(6, 1fr);gap:6px;">' +
          '<input type="color" id="slot-color-' + i + '-0" value="' + slotPresets[0] + '" style="width:100%;height:34px;" title="Preset 1">' +
          '<input type="color" id="slot-color-' + i + '-1" value="' + slotPresets[1] + '" style="width:100%;height:34px;" title="Preset 2">' +
          '<input type="color" id="slot-color-' + i + '-2" value="' + slotPresets[2] + '" style="width:100%;height:34px;" title="Preset 3">' +
          '<input type="color" id="slot-color-' + i + '-3" value="' + slotPresets[3] + '" style="width:100%;height:34px;" title="Preset 4">' +
          '<input type="color" id="slot-color-' + i + '-4" value="' + slotPresets[4] + '" style="width:100%;height:34px;" title="Preset 5">' +
          '<input type="color" id="slot-color-' + i + '-5" value="' + slotPresets[5] + '" style="width:100%;height:34px;" title="Preset 6">' +
        '</div>' +
      '</div>';

    var card = document.createElement('div');
    card.className = 'slot-card';

    card.innerHTML =
      '<div class="slot-header"><span>' + slotNames[i] + '</span><span id="slot-badge-' + i + '" class="slot-icon-badge">' + (existing && existing.icon ? existing.icon : '') + '</span></div>' +
      '<label>Search HA Entity <span style="font-size:10px;color:var(--text-muted);font-weight:normal;">(Cached)</span></label>' +
      '<div class="search-wrap">' +
        '<input id="slot-search-' + i + '" placeholder="&#128269; Search by name or entity ID..." oninput="onSearchInput(' + i + ', this.value)" autocomplete="off">' +
        '<div id="slot-results-' + i + '" class="search-results hidden"></div>' +
      '</div>' +
      '<label>Friendly Label</label>' +
      '<input id="slot-label-' + i + '" value="' + (existing ? existing.label : '') + '" placeholder="e.g. Ceiling Light">' +
      '<label>Widget Type <span style="font-size:10px;color:var(--text-muted);font-weight:normal;">(Auto-detected)</span></label>' +
      '<select id="slot-type-' + i + '">' +
        '<option value="toggle"' + (existing && existing.type === 'toggle' ? ' selected' : '') + '>Toggle / Action (Light / Switch / Fan / Scene / Script)</option>' +
        '<option value="sensor"' + (existing && existing.type === 'sensor' ? ' selected' : '') + '>Sensor (Temperature / Value)</option>' +
        '<option value="binary_sensor"' + (existing && existing.type === 'binary_sensor' ? ' selected' : '') + '>Binary Sensor (Motion / Door)</option>' +
      '</select>' +
      '<label>Entity ID</label>' +
      '<input id="slot-entity-' + i + '" value="' + entId + '" placeholder="e.g. switch.kitchen" oninput="onEntityInput(' + i + ', this.value)">' +
      '<input type="hidden" id="slot-icon-' + i + '" value="' + (existing && existing.icon ? existing.icon : '') + '">' +
      rgbPaletteHtml;

    container.appendChild(card);
  }
}

function onSearchInput(slotIdx, query) {
  var resultsBox = document.getElementById('slot-results-' + slotIdx);
  if (!query || query.trim().length === 0) {
    resultsBox.classList.add('hidden');
    resultsBox.innerHTML = '';
    return;
  }

  var q = query.trim().toLowerCase();

  if (!cachedEntities || cachedEntities.length === 0) {
    resultsBox.innerHTML =
      '<div style="padding:10px;font-size:12px;color:var(--text-muted);text-align:center;">' +
        'Entities not synced yet.<br>' +
        '<button type="button" class="btn btn-secondary btn-sm" style="margin-top:6px;" onclick="syncEntities(true)">&#x21bb; Sync from HA</button>' +
      '</div>';
    resultsBox.classList.remove('hidden');
    return;
  }

  var matches = [];
  for (var i = 0; i < cachedEntities.length; i++) {
    var item = cachedEntities[i];
    var id = (item.id || '').toLowerCase();
    var name = (item.name || '').toLowerCase();
    if (id.indexOf(q) >= 0 || name.indexOf(q) >= 0) {
      matches.push(item);
      if (matches.length >= 35) break;
    }
  }

  if (matches.length === 0) {
    resultsBox.innerHTML = '<div style="padding:10px;font-size:12px;color:var(--text-muted);text-align:center;">No matching entities in cache.<br><button type="button" class="btn btn-secondary btn-sm" style="margin-top:6px;" onclick="syncEntities(true)">&#x21bb; Refresh Cache</button></div>';
    resultsBox.classList.remove('hidden');
    return;
  }

  resultsBox.innerHTML = '';
  matches.forEach(function(item){
    var div = document.createElement('div');
    div.className = 'search-item';
    div.innerHTML =
      '<div>' +
        '<div class="search-name">' + (item.name || item.id) + '</div>' +
        '<div class="search-id">' + item.id + (item.icon ? (' &bull; ' + item.icon) : '') + '</div>' +
      '</div>' +
      '<span class="search-tag">' + item.type + '</span>';
    div.onclick = function() {
      selectSearchedEntity(slotIdx, item);
    };
    resultsBox.appendChild(div);
  });
  resultsBox.classList.remove('hidden');
}

function selectSearchedEntity(slotIdx, item) {
  document.getElementById('slot-entity-' + slotIdx).value = item.id;
  document.getElementById('slot-label-' + slotIdx).value = item.name || item.id;
  document.getElementById('slot-type-' + slotIdx).value = item.type || autoDetectType(item.id);
  document.getElementById('slot-icon-' + slotIdx).value = item.icon || '';
  var badge = document.getElementById('slot-badge-' + slotIdx);
  if (badge) { badge.innerText = item.icon || ''; }
  document.getElementById('slot-search-' + slotIdx).value = '';
  document.getElementById('slot-results-' + slotIdx).classList.add('hidden');
  updateSlotRgbVisibility(slotIdx, item.id);
}

function saveDashboardConfig() {
  syncCurrentSlotsToMemory();
  fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(currentConfig)
  })
  .then(function(r){ return r.json(); })
  .then(function(res){
    showAlert('Saved! Dashboard updated live on CYD display.', false);
  })
  .catch(function(err){
    showAlert('Error saving config to device.', true);
  });
}

function loadStatus() {
  fetch('/api/status')
    .then(function(r){ return r.json(); })
    .then(function(d){
      document.getElementById('stat-ip').textContent = d.ip || '--';
      document.getElementById('stat-rssi').textContent = (d.wifi_rssi || '--') + ' dBm';
      if (d.hardware_model) {
        var hwEl = document.getElementById('stat-hw');
        if (hwEl) hwEl.textContent = d.hardware_model;
        var hdrHw = document.getElementById('header-hw-model');
        if (hdrHw) hdrHw.textContent = d.hardware_model;
      }
      document.getElementById('stat-heap').textContent = Math.round((d.free_heap || 0) / 1024) + ' KB';
      var minEl = document.getElementById('stat-min-heap');
      if (minEl && d.min_free_heap) {
        minEl.textContent = 'Lowest: ' + Math.round(d.min_free_heap / 1024) + ' KB';
      }
      var m = Math.floor((d.uptime_s || 0) / 60);
      document.getElementById('stat-uptime').textContent = m + 'm';
      var vEl = document.getElementById('stat-version');
      if (vEl && d.version) {
        vEl.textContent = 'v' + d.version;
      }
      var badge = document.getElementById('ha-badge');
      if (d.ha_connected) {
        badge.textContent = 'HA: Connected';
        badge.style.color = '#10b981';
      } else {
        badge.textContent = 'HA: Disconnected';
        badge.style.color = '#ef4444';
      }
    });
}

function downloadConfig() {
  window.open('/api/config?download=1', '_blank');
}

function uploadConfigFile(input) {
  var file = input.files && input.files[0];
  if (!file) return;

  var reader = new FileReader();
  reader.onload = function(e) {
    try {
      var json = JSON.parse(e.target.result);
      if (!json.pages || !Array.isArray(json.pages)) {
        showAlert('Invalid config: Missing "pages" list.', true);
        input.value = '';
        return;
      }

      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(json)
      })
      .then(function(r) { return r.json(); })
      .then(function(res) {
        if (res.success) {
          showAlert('Configuration restored! Applied live to CYD.', false);
          loadDashboardConfig();
        } else {
          showAlert('Error: ' + (res.error || 'Failed to restore config'), true);
        }
        input.value = '';
      })
      .catch(function(err) {
        showAlert('Network error restoring configuration.', true);
        input.value = '';
      });
    } catch(err) {
      showAlert('Invalid JSON file format: ' + err.message, true);
      input.value = '';
    }
  };
  reader.readAsText(file);
}

var latestOtaData = null;

function checkGitHubRelease() {
  var box = document.getElementById('ota-status-box');
  var txt = document.getElementById('ota-status-text');
  var btnInstall = document.getElementById('btn-install-ota');
  box.style.display = 'block';
  txt.innerHTML = 'Querying GitHub releases for <b>demonkazuya/CYD-HA-Dashboard</b>...';
  btnInstall.style.display = 'none';

  fetch('https://raw.githubusercontent.com/demonkazuya/CYD-HA-Dashboard/main/ota.json')
    .then(function(r) {
      if (!r.ok) {
        return fetch('https://api.github.com/repos/demonkazuya/CYD-HA-Dashboard/releases/latest')
          .then(function(res) {
            if (!res.ok) throw new Error('No release tag found on GitHub yet.');
            return res.json().then(function(gh) {
              var asset = gh.assets && gh.assets.find(function(a){ return a.name.endsWith('.bin') && !a.name.includes('factory'); });
              return {
                version: gh.tag_name.replace(/^v/, ''),
                url: asset ? asset.browser_download_url : ''
              };
            });
          });
      }
      return r.json();
    })
    .then(function(data) {
      latestOtaData = data;
      var curVer = (document.getElementById('stat-version').textContent || 'v0.1.0').replace(/^v/, '');
      if (data.version && data.version !== curVer) {
        txt.innerHTML = '<span style="color:#10b981;">&#10004; Update Available: <b>v' + data.version + '</b></span> (Installed: v' + curVer + ')';
        if (data.url || data.firmware_url) {
          btnInstall.style.display = 'block';
          btnInstall.textContent = '⚡ Install v' + data.version + ' from GitHub';
        }
      } else {
        txt.innerHTML = '<span style="color:#10b981;">&#10004; CYD-HA-Dashboard is up to date (v' + curVer + ')</span>';
      }
    })
    .catch(function(err) {
      txt.innerHTML = '<span style="color:#f59e0b;">' + (err.message || 'Could not reach GitHub release server.') + '</span>';
    });
}

function installGitHubRelease() {
  if (!latestOtaData) return;
  // GitHub Release asset downloads redirect to AWS/Azure blobs without CORS headers, which triggers "Failed to fetch" in browser.
  // raw.githubusercontent.com explicitly provides 'Access-Control-Allow-Origin: *' allowing browser to stream binary directly!
  var rawUrl = 'https://raw.githubusercontent.com/demonkazuya/CYD-HA-Dashboard/main/dist/cyd-ha-dashboard.bin';
  var downloadUrl = latestOtaData.cors_url || rawUrl;
  if (!confirm('Download and install firmware v' + latestOtaData.version + ' from GitHub?')) return;

  var txt = document.getElementById('ota-status-text');
  var wrap = document.getElementById('ota-progress-wrap');
  var bar = document.getElementById('ota-progress-bar');
  wrap.style.display = 'block';
  bar.style.width = '10%';
  txt.innerHTML = 'Downloading firmware binary from GitHub release...';

  fetch(downloadUrl)
    .then(function(res) {
      if (!res.ok) throw new Error('Failed to download binary from GitHub (HTTP ' + res.status + ')');
      return res.blob();
    })
    .then(function(blob) {
      txt.innerHTML = 'Streaming firmware to CYD device (do not power off)...';
      performOtaUpload(blob, 'cyd-ha-dashboard.bin');
    })
    .catch(function(err) {
      wrap.style.display = 'none';
      txt.innerHTML = '<span style="color:#ef4444;">Error: ' + err.message + '</span>';
      showAlert('GitHub download failed: ' + err.message, true);
    });
}

function uploadManualOta(input) {
  if (!input.files || input.files.length === 0) return;
  var file = input.files[0];
  if (!file.name.endsWith('.bin')) {
    showAlert('Please select a valid .bin firmware file.', true);
    return;
  }
  if (!confirm('Flash "' + file.name + '" to CYD device over WiFi?')) return;
  var box = document.getElementById('ota-status-box');
  var txt = document.getElementById('ota-status-text');
  var wrap = document.getElementById('ota-progress-wrap');
  box.style.display = 'block';
  wrap.style.display = 'block';
  txt.innerHTML = 'Uploading ' + file.name + ' to CYD...';
  performOtaUpload(file, file.name);
}

function performOtaUpload(blobOrFile, filename) {
  var txt = document.getElementById('ota-status-text');
  var bar = document.getElementById('ota-progress-bar');
  var formData = new FormData();
  formData.append('update', blobOrFile, filename);

  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);

  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      var pct = Math.round((e.loaded / e.total) * 100);
      bar.style.width = pct + '%';
      txt.innerHTML = 'Flashing firmware: <b>' + pct + '%</b> (' + Math.round(e.loaded/1024) + ' KB)';
    }
  };

  xhr.onload = function() {
    if (xhr.status === 200) {
      bar.style.width = '100%';
      txt.innerHTML = '<b style="color:#10b981;">&#10004; Flash complete! Rebooting device into new firmware...</b>';
      showAlert('Firmware flashed successfully! Device is rebooting...', false);
      setTimeout(function() {
        window.location.reload();
      }, 6000);
    } else {
      txt.innerHTML = '<b style="color:#ef4444;">Flash failed (HTTP ' + xhr.status + ').</b>';
      showAlert('OTA flash failed.', true);
    }
  };

  xhr.onerror = function() {
    txt.innerHTML = '<b style="color:#ef4444;">Upload network error.</b>';
    showAlert('OTA upload network error.', true);
  };

  xhr.send(formData);
}

function onSsidChange() {
  var sel = document.getElementById('ssid_select');
  var wrap = document.getElementById('custom_ssid_wrap');
  wrap.style.display = (sel.value === '__custom__') ? 'block' : 'none';
  if (sel.value === '__custom__') document.getElementById('custom_ssid').focus();
}

function prepareSubmit() {
  var sel = document.getElementById('ssid_select');
  var out = document.getElementById('wifi_ssid');
  out.value = (sel.value === '__custom__' || sel.value === '') ? document.getElementById('custom_ssid').value.trim() : sel.value;
}

function scanWifi() {
  var sel = document.getElementById('ssid_select');
  sel.innerHTML = '<option value="">Scanning nearby Wi-Fi...</option>';
  fetch('/scan').then(function(r){ return r.json(); }).then(function(list){
    sel.innerHTML = '';
    var seen = {};
    for (var i = 0; i < list.length; i++) {
      var net = list[i];
      if (!net.ssid || seen[net.ssid]) continue;
      seen[net.ssid] = true;
      var opt = document.createElement('option');
      opt.value = net.ssid;
      opt.textContent = net.ssid + ' (' + net.rssi + ' dBm)';
      sel.appendChild(opt);
    }
    var customOpt = document.createElement('option');
    customOpt.value = '__custom__';
    customOpt.textContent = 'Enter SSID Manually...';
    sel.appendChild(customOpt);
    onSsidChange();
  }).catch(function(){
    sel.innerHTML = '<option value="__custom__">Enter SSID Manually...</option>';
    onSsidChange();
  });
}

// Close search dropdowns when clicking outside
document.addEventListener('click', function(e){
  if (!e.target.closest('.search-wrap')) {
    document.querySelectorAll('.search-results').forEach(function(el){ el.classList.add('hidden'); });
  }
});

window.onload = function() {
  loadDashboardConfig();
  loadStatus();
  scanWifi();
  var hasCache = loadCachedEntities();
  if (!hasCache) {
    syncEntities(false);
  }
};
</script>
</body>
</html>
)rawliteral";
