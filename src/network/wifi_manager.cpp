#include "wifi_manager.h"
#include "portal_html.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <FS.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_task_wdt.h>
#include "config.h"

static WebServer server(80);
static DNSServer dnsServer;
static Preferences prefs;
static const byte DNS_PORT = 53;

static const char PREF_NAMESPACE[] = "cyd_cfg";
static const char KEY_SSID[] = "ssid";
static const char KEY_PASS[] = "pass";
static const char KEY_HA_URL[] = "ha_url";
static const char KEY_HA_TOKEN[] = "ha_token";
static const char KEY_HA_ENTITY[] = "ha_entity";

static String cachedScanJson = "";

static String scanNetworksJson() {
  int n = WiFi.scanNetworks(false, false);
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    String net = WiFi.SSID(i);
    if (net.length() > 0) {
      JsonObject obj = arr.add<JsonObject>();
      obj["ssid"] = net;
      obj["rssi"] = WiFi.RSSI(i);
    }
  }
  WiFi.scanDelete();
  String out;
  serializeJson(arr, out);
  return out;
}

WifiManager::WifiManager() : _isAp(false), _connected(false) {}

void WifiManager::setStatusCallback(StatusCallback cb) {
  _callback = cb;
}

void WifiManager::setOnConfigReloadCallback(ConfigReloadCallback cb) {
  _configReloadCb = cb;
}

void WifiManager::setHaConnectedCallback(HaConnectedCallback cb) {
  _haConnectedCb = cb;
}

void WifiManager::notify(bool connected, bool isAp, const String& info) {
  _connected = connected;
  _isAp = isAp;
  if (_callback) {
    _callback(connected, isAp, info);
  }
}

bool WifiManager::isConnected() const {
  return _connected && (WiFi.status() == WL_CONNECTED);
}

bool WifiManager::isApMode() const {
  return _isAp;
}

String WifiManager::getIpAddress() const {
  if (_isAp) {
    return WiFi.softAPIP().toString();
  }
  return WiFi.localIP().toString();
}

String WifiManager::getSsid() const {
  return _ssid;
}

String WifiManager::getApSsid() const {
  return _apSsid;
}

String WifiManager::getHaUrl() const {
  return _haUrl;
}

String WifiManager::getHaToken() const {
  return _haToken;
}

String WifiManager::getHaEntity() const {
  return _haEntity.length() > 0 ? _haEntity : "switch.living_room_light";
}

void WifiManager::resetCredentials() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  Serial.println("[WiFi] Saved credentials erased.");
}

bool WifiManager::connectSta(const String& ssid, const String& pass, uint32_t timeout_ms) {
  _ssid = ssid;
  Serial.printf("[WiFi] Connecting to %s ...\n", ssid.c_str());
  notify(false, false, "Connecting to " + ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  const int maxAttempts = 2;
  for (int attempt = 1; attempt <= maxAttempts; attempt++) {
    if (attempt == 1) {
      WiFi.begin(ssid.c_str(), pass.length() > 0 ? pass.c_str() : nullptr);
    } else {
      WiFi.disconnect();
      delay(200);
      WiFi.begin(ssid.c_str(), pass.length() > 0 ? pass.c_str() : nullptr);
    }

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < timeout_ms)) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
      notify(true, false, WiFi.localIP().toString());
      return true;
    }
  }

  Serial.println("[WiFi] Connection failed.");
  return false;
}

void WifiManager::setupCaptivePortal() {
  server.on("/", HTTP_GET, [this]() {
    String html = FPSTR(PORTAL_HTML);
    html.replace("%HA_URL%", _haUrl);
    // Never leak actual token in cleartext HTML source
    html.replace("%HA_TOKEN_PLACEHOLDER%", (_haToken.length() > 0) ? "•••••••••••••••• (Token configured)" : "Bearer token from your HA Profile");
    html.replace("%HA_ENTITY%", _haEntity);
    html.replace("%HARDWARE_MODEL%", HARDWARE_MODEL);
    server.send(200, "text/html", html);
  });

  server.on("/scan", HTTP_GET, []() {
    if (cachedScanJson.length() <= 2) {
      cachedScanJson = scanNetworksJson();
    }
    server.send(200, "application/json", cachedScanJson);
    cachedScanJson = ""; // refresh on next manual click
  });

  // REST API: Config
  server.on("/api/config", HTTP_GET, [this]() {
    if (!LittleFS.begin(true) || !LittleFS.exists("/config.json")) {
      server.send(404, "application/json", "{\"error\":\"config.json not found\"}");
      return;
    }
    fs::File f = LittleFS.open("/config.json", "r");
    if (!f) {
      server.send(500, "application/json", "{\"error\":\"Failed to open config.json\"}");
      return;
    }
    if (server.hasArg("download")) {
      server.sendHeader("Content-Disposition", "attachment; filename=\"config.json\"");
    }
    String content = f.readString();
    f.close();
    server.send(200, "application/json", content);
  });

  server.on("/api/config", HTTP_POST, [this]() {
    String payload = server.arg("plain");
    if (payload.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"Empty payload\"}");
      return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    if (!doc["pages"].is<JsonArray>()) {
      server.send(400, "application/json", "{\"error\":\"Missing pages array\"}");
      return;
    }

    if (LittleFS.begin(true)) {
      fs::File f = LittleFS.open("/config.json", "w");
      if (f) {
        serializeJsonPretty(doc, f);
        f.close();
      }
    }

    if (_configReloadCb) {
      _configReloadCb();
    }

    server.send(200, "application/json", "{\"success\":true,\"message\":\"Dashboard configuration updated!\"}");
  });

  // REST API: Status
  server.on("/api/status", HTTP_GET, [this]() {
    JsonDocument doc;
    doc["ip"] = WiFi.localIP().toString();
    doc["wifi_ssid"] = WiFi.SSID();
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["min_free_heap"] = ESP.getMinFreeHeap();
    doc["max_alloc_heap"] = ESP.getMaxAllocHeap();
    doc["chip_model"] = ESP.getChipModel();
    doc["hardware_model"] = HARDWARE_MODEL;
    doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
    doc["uptime_s"] = millis() / 1000;
    doc["version"] = APP_VERSION;
    doc["app_name"] = APP_NAME;
    doc["ha_connected"] = _haConnectedCb ? _haConnectedCb() : false;
    doc["ha_url"] = _haUrl;

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // REST API: Reboot
  server.on("/api/reboot", HTTP_POST, [this]() {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Rebooting device...\"}");
    delay(500);
    ESP.restart();
  });

  // Web OTA Firmware Update
  static bool otaUploadSuccess = false;

  server.on("/update", HTTP_POST, [this]() {
    server.sendHeader("Connection", "close");
    if (otaUploadSuccess && !Update.hasError()) {
      otaUploadSuccess = false;
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Firmware update successful! Rebooting...\"}");
      delay(500);
      ESP.restart();
    } else {
      otaUploadSuccess = false;
      server.send(500, "application/json", "{\"success\":false,\"message\":\"Firmware update failed or no file provided.\"}");
    }
  }, [this]() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      otaUploadSuccess = false;
      Serial.printf("[OTA] Web Update Start: %s\n", upload.filename.c_str());
      esp_task_wdt_reset();
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      esp_task_wdt_reset();
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      esp_task_wdt_reset();
      if (Update.end(true)) {
        otaUploadSuccess = true;
        Serial.printf("[OTA] Web Update Complete: %u bytes\n", upload.totalSize);
      } else {
        otaUploadSuccess = false;
        Update.printError(Serial);
      }
    }
  });

  // REST API: Entity Discovery via Home Assistant /api/template
  server.on("/api/entities", HTTP_GET, [this]() {
    if (WiFi.status() != WL_CONNECTED || _haUrl.length() == 0 || _haToken.length() == 0) {
      server.send(200, "application/json", "[]");
      return;
    }

    String query = server.arg("q");
    query.trim();
    query.toLowerCase();

    String domainFilter = server.arg("domain");
    domainFilter.trim();
    domainFilter.toLowerCase();

    String templateBody;
    if (query.length() > 0) {
      templateBody = "{\"template\": \"[{% set first = namespace(f=true) %}{% set count = namespace(c=0) %}{% for s in states if (('" + query + "' in s.entity_id.lower() or '" + query + "' in s.name.lower()) and count.c < 50) %}{% if not first.f %},{% endif %}{% set first.f = false %}{% set count.c = count.c + 1 %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"{{ 'sensor' if s.domain == 'sensor' else ('binary_sensor' if s.domain == 'binary_sensor' else 'toggle') }}\\\",\\\"rgb\\\":{{ (s.domain == 'light' and ('rgb' in (s.attributes.supported_color_modes | default([])) or 'hs' in (s.attributes.supported_color_modes | default([])) or 'xy' in (s.attributes.supported_color_modes | default([])))) | tojson }}}{% endfor %}]\"}";
    } else if (domainFilter == "toggle") {
      templateBody = "{\"template\": \"[{% set first = namespace(f=true) %}{% for s in states.light %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":{{ ('rgb' in (s.attributes.supported_color_modes | default([])) or 'hs' in (s.attributes.supported_color_modes | default([])) or 'xy' in (s.attributes.supported_color_modes | default([]))) | tojson }}}{% endfor %}{% for s in states.switch %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.fan %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.cover %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:blinds')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.scene %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:palette')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.script %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:script-text-play')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.climate %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:thermostat')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}]\"}";
    } else if (domainFilter == "binary_sensor") {
      templateBody = "{\"template\": \"[{% set first = namespace(f=true) %}{% for s in states.binary_sensor %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"binary_sensor\\\",\\\"rgb\\\":false}{% endfor %}]\"}";
    } else {
      templateBody = "{\"template\": \"[{% set first = namespace(f=true) %}{% for s in states.light %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":{{ ('rgb' in (s.attributes.supported_color_modes | default([])) or 'hs' in (s.attributes.supported_color_modes | default([])) or 'xy' in (s.attributes.supported_color_modes | default([]))) | tojson }}}{% endfor %}{% for s in states.switch %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.fan %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.cover %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:blinds')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.scene %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:palette')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.script %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:script-text-play')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.climate %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('mdi:thermostat')) | tojson}},\\\"type\\\":\\\"toggle\\\",\\\"rgb\\\":false}{% endfor %}{% for s in states.binary_sensor %}{% if not first.f %},{% endif %}{% set first.f = false %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"binary_sensor\\\",\\\"rgb\\\":false}{% endfor %}{% set count = namespace(c=0) %}{% for s in states.sensor if (s.attributes.device_class is defined and count.c < 150) %}{% if not first.f %},{% endif %}{% set first.f = false %}{% set count.c = count.c + 1 %}{\\\"id\\\":\\\"{{s.entity_id}}\\\",\\\"name\\\":{{s.name | tojson}},\\\"icon\\\":{{(s.attributes.icon | default('')) | tojson}},\\\"type\\\":\\\"sensor\\\",\\\"rgb\\\":false}{% endfor %}]\"}";
    }

    WiFiClient client;
    HTTPClient http;
    http.begin(client, _haUrl + "/api/template");
    http.addHeader("Authorization", "Bearer " + _haToken);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "close");
    http.setTimeout(6000);

    int httpCode = http.POST(templateBody);
    if (httpCode != 200) {
      server.send(200, "application/json", "[]");
      http.end();
      return;
    }

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "");

    WiFiClient* stream = http.getStreamPtr();
    stream->setTimeout(300);
    int totalBytes = http.getSize();
    int bytesRead = 0;
    uint8_t buffer[1024];

    while (http.connected() && (totalBytes < 0 || bytesRead < totalBytes)) {
      int toRead = sizeof(buffer);
      if (totalBytes > 0 && (totalBytes - bytesRead < toRead)) {
        toRead = totalBytes - bytesRead;
      }
      int len = stream->read(buffer, toRead);
      if (len > 0) {
        server.sendContent((const char*)buffer, len);
        bytesRead += len;
      } else if (!stream->connected() && !stream->available()) {
        break;
      } else {
        delay(1);
      }
    }
    server.sendContent("");
    http.end();
  });

  server.on("/calibrate", HTTP_POST, []() {
    Preferences p;
    if (p.begin("cyd_cal_v1", false)) {
      p.clear();
      p.end();
    }
    if (p.begin("cyd_touch", false)) {
      p.clear();
      p.end();
    }
    if (LittleFS.begin(true)) {
      LittleFS.remove("/touch_v1.json");
      LittleFS.remove("/touch.json");
    }
    server.send(200, "text/html",
      "<!DOCTYPE html><html><body style='background:#141414;color:#eee;text-align:center;padding:40px;'>"
      "<h2 style='color:#00e676;'>Calibration Reset</h2>"
      "<p>Rebooting device into calibration mode...</p></body></html>");
    delay(1000);
    ESP.restart();
  });

  server.on("/save", HTTP_POST, [this]() {
    String reqSsid = server.arg("wifi_ssid");
    reqSsid.trim();
    String reqPass = server.arg("wifi_password");
    String reqHaUrl = server.arg("ha_url");
    reqHaUrl.trim();
    String reqHaToken = server.arg("ha_token");
    reqHaToken.trim();
    String reqHaEntity = server.arg("ha_entity");
    reqHaEntity.trim();

    if (reqSsid.length() == 0) {
      server.send(400, "text/plain", "SSID cannot be empty");
      return;
    }

    prefs.begin(PREF_NAMESPACE, false);
    prefs.putString(KEY_SSID, reqSsid);
    // Only update Wi-Fi password if a new one was provided
    if (reqPass.length() > 0) {
      prefs.putString(KEY_PASS, reqPass);
    }
    if (reqHaUrl.length() > 0) prefs.putString(KEY_HA_URL, reqHaUrl);
    // Only update HA token if a new token was provided (and not the masked placeholder)
    if (reqHaToken.length() > 0 && reqHaToken.indexOf("••") < 0) {
      prefs.putString(KEY_HA_TOKEN, reqHaToken);
    }
    if (reqHaEntity.length() > 0) prefs.putString(KEY_HA_ENTITY, reqHaEntity);
    prefs.end();

    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<title>Saved</title>"
                  "<style>body{font-family:sans-serif;background:#141414;color:#00e676;text-align:center;padding:40px;}"
                  ".card{background:#1e1e1e;padding:24px;border-radius:12px;max-width:360px;margin:auto;border:1px solid #333;color:#eee;}"
                  "</style></head>"
                  "<body><div class='card'><h2 style='color:#00e676;'>Configuration Saved!</h2>"
                  "<p>Rebooting and connecting to <b>" + reqSsid + "</b>...</p>"
                  "<p style='color:#888;font-size:14px;'>Screen will update with device IP once connected.</p></div></body></html>";
    server.send(200, "text/html", html);
    delay(1000);
    ESP.restart();
  });

  auto captiveRedirect = [this]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  };

  server.on("/generate_204", captiveRedirect);
  server.on("/fwlink", captiveRedirect);
  server.on("/hotspot-detect.html", captiveRedirect);
  server.on("/ncsi.txt", captiveRedirect);
  server.onNotFound(captiveRedirect);

  server.begin();
  Serial.println("[Web] Captive portal HTTP server started on port 80.");
}

void WifiManager::startAp() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char apName[32];
  snprintf(apName, sizeof(apName), "CYD-Setup-%02X%02X", mac[4], mac[5]);
  _apSsid = String(apName);

  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);
  cachedScanJson = scanNetworksJson(); // initial fast cache

  WiFi.softAP(_apSsid.c_str());
  delay(100);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  dnsServer.start(DNS_PORT, "*", apIP);
  setupCaptivePortal();

  Serial.printf("[WiFi] SoftAP started: %s (IP: 192.168.4.1)\n", _apSsid.c_str());
  notify(false, true, "AP: " + _apSsid + " (192.168.4.1)");
}

void WifiManager::begin() {
  prefs.begin(PREF_NAMESPACE, true);
  String savedSsid = prefs.getString(KEY_SSID, "");
  String savedPass = prefs.getString(KEY_PASS, "");
  _haUrl = prefs.getString(KEY_HA_URL, "");
  _haToken = prefs.getString(KEY_HA_TOKEN, "");
  _haEntity = prefs.getString(KEY_HA_ENTITY, "switch.living_room_light");
  prefs.end();

  if (savedSsid.length() > 0) {
    bool ok = connectSta(savedSsid, savedPass);
    if (ok) {
      setupCaptivePortal(); // Start web portal on port 80 in STA mode as well
      return;
    }
  }

  startAp();
}

void WifiManager::loop() {
  server.handleClient(); // Always process HTTP web requests

  if (_isAp) {
    dnsServer.processNextRequest();
  } else {
    if (WiFi.status() != WL_CONNECTED) {
      if (_connected) {
        _connected = false;
        notify(false, false, "Disconnected");
      }
      unsigned long now = millis();
      if (now - _lastReconnectAttempt > 10000) {
        _lastReconnectAttempt = now;
        Serial.println("[WiFi] Lost connection, attempting reconnect...");
        notify(false, false, "Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
      }
    } else if (!_connected) {
      _connected = true;
      notify(true, false, WiFi.localIP().toString());
    }
  }
}
