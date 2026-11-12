#include "ha_client.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

HaClient::HaClient() {
  _mutex = xSemaphoreCreateMutex();
}

void HaClient::begin(const String& haUrl, const String& haToken, const String& defaultEntity) {
  _url = haUrl;
  _token = haToken;

  if (_url.endsWith("/")) {
    _url = _url.substring(0, _url.length() - 1);
  }

  if (defaultEntity.length() > 0) {
    _monitoredEntities.push_back(defaultEntity);
  }

  Serial.printf("[HA] Client initialized for %s (%u entities monitored)\n",
                _url.c_str(), _monitoredEntities.size());

  if (_taskHandle == nullptr) {
    xTaskCreatePinnedToCore(
      backgroundTask,
      "ha_task",
      8192,
      this,
      2,
      &_taskHandle,
      0 // Core 0
    );
  }
}

void HaClient::setMonitoredEntities(const std::vector<String>& entityIds) {
  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    _monitoredEntities = entityIds;
    xSemaphoreGive(_mutex);
  }
  notifyAction();
}

void HaClient::setStateCallback(StateCallback cb) {
  _stateCallback = cb;
}

void HaClient::setStatusCallback(StatusCallback cb) {
  _statusCallback = cb;
}

bool HaClient::isConnected() const {
  return _connected;
}

HaEntity HaClient::getEntity(const String& entityId) {
  HaEntity result;
  result.id = entityId;
  result.friendlyName = entityId;
  result.state = "unknown";
  result.available = false;

  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    auto it = _entities.find(entityId);
    if (it != _entities.end()) {
      result = it->second;
    }
    xSemaphoreGive(_mutex);
  }
  return result;
}

void HaClient::notifyAction() {
  if (_taskHandle != nullptr) {
    xTaskNotifyGive(_taskHandle);
  }
}

void HaClient::triggerPoll() {
  notifyAction();
}

bool HaClient::toggleWithTarget(const String& targetState, const String& entityId) {
  if (entityId.length() == 0) return false;

  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    _toggleEntityId = entityId;
    _toggleTargetState = targetState;
    _toggleRequested = true;

    Holdoff hold;
    hold.deadline = millis() + 3500;
    hold.targetState = targetState;
    _holdoffs[entityId] = hold;

    _entities[entityId].state = targetState;
    _entities[entityId].available = true;
    xSemaphoreGive(_mutex);
  }

  notifyAction();
  return true;
}

bool HaClient::toggle(const String& entityId) {
  HaEntity cur = getEntity(entityId);
  String nextState = (cur.state == "on") ? "off" : "on";
  return toggleWithTarget(nextState, entityId);
}

bool HaClient::checkApi() {
  if (WiFi.status() != WL_CONNECTED || _url.length() == 0 || _token.length() == 0) {
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String endpoint = _url + "/api/";

  http.begin(client, endpoint);
  http.addHeader("Authorization", "Bearer " + _token);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2500);

  int code = http.GET();
  bool ok = (code == 200);
  http.end();

  if (_connected != ok) {
    _connected = ok;
    if (_statusCallback) {
      _statusCallback(ok, ok ? "Connected" : String("HTTP ") + String(code));
    }
  }

  return ok;
}

bool HaClient::fetchState(const String& entityId) {
  // Support Simulated In-Memory Demo Entities
  if (entityId == "light.cyd_demo_rgb" || entityId == "fan.cyd_demo_fan" || entityId == "scene.cyd_demo_movie" || entityId == "cover.cyd_demo_blinds" || entityId == "climate.cyd_demo_thermostat") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      HaEntity& ent = _entities[entityId];
      if (ent.id.length() == 0) {
        ent.id = entityId;
        ent.available = true;
        if (entityId == "light.cyd_demo_rgb") {
          ent.friendlyName = "Demo RGB Light";
          ent.state = "on";
          ent.supportsBrightness = true;
          ent.supportsRgb = true;
          ent.brightness = 204;
          ent.brightnessPct = 80;
          ent.rgb[0] = 56; ent.rgb[1] = 189; ent.rgb[2] = 248; // HA Cyan
        } else if (entityId == "fan.cyd_demo_fan") {
          ent.friendlyName = "Demo Ceiling Fan";
          ent.state = "on";
          ent.supportsSpeed = true;
          ent.speedPct = 66;
        } else if (entityId == "scene.cyd_demo_movie") {
          ent.friendlyName = "Movie Night Scene";
          ent.state = "scening";
        } else if (entityId == "cover.cyd_demo_blinds") {
          ent.friendlyName = "Demo Window Blinds";
          ent.state = "open";
          ent.supportsPosition = true;
          ent.currentPosition = 100;
        } else if (entityId == "climate.cyd_demo_thermostat") {
          ent.friendlyName = "Demo Thermostat";
          ent.state = "heat";
          ent.supportsClimate = true;
          ent.targetTemp = 22.0f;
          ent.currentTemp = 21.5f;
          ent.minTemp = 16.0f;
          ent.maxTemp = 30.0f;
          ent.tempStep = 0.5f;
          ent.hvacAction = "heating";
        }
      }
      HaEntity copy = ent;
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }

  if (WiFi.status() != WL_CONNECTED || _url.length() == 0 || entityId.length() == 0) {
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String endpoint = _url + "/api/states/" + entityId;

  http.begin(client, endpoint);
  http.addHeader("Authorization", "Bearer " + _token);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2500);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      String newState = doc["state"].as<String>();
      String friendlyName = entityId;
      String unit = "";

      if (doc["attributes"]["friendly_name"].is<String>()) {
        friendlyName = doc["attributes"]["friendly_name"].as<String>();
      }
      if (doc["attributes"]["unit_of_measurement"].is<String>()) {
        unit = doc["attributes"]["unit_of_measurement"].as<String>();
      }

      if (!_connected) {
        _connected = true;
        if (_statusCallback) _statusCallback(true, "Connected");
      }

      // Check per-entity holdoff against stale HA response
      if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        auto hIt = _holdoffs.find(entityId);
        if (hIt != _holdoffs.end() && millis() < hIt->second.deadline) {
          if (newState != hIt->second.targetState) {
            Serial.printf("[HA] Holdoff %s: ignoring stale state '%s' (waiting for '%s')\n",
                          entityId.c_str(), newState.c_str(), hIt->second.targetState.c_str());
            xSemaphoreGive(_mutex);
            return true;
          } else {
            hIt->second.deadline = 0; // Confirmed!
          }
        }

        HaEntity& ent = _entities[entityId];
        ent.id = entityId;
        ent.friendlyName = friendlyName;
        ent.state = newState;
        ent.unitOfMeasurement = unit;
        ent.available = (newState != "unavailable" && newState != "unknown");

        // Parse brightness
        if (doc["attributes"]["brightness"].is<int>()) {
          ent.brightness = doc["attributes"]["brightness"].as<int>();
          ent.brightnessPct = (ent.brightness * 100) / 255;
          ent.supportsBrightness = true;
        }

        // Parse color modes
        if (doc["attributes"]["supported_color_modes"].is<JsonArray>()) {
          JsonArray modes = doc["attributes"]["supported_color_modes"].as<JsonArray>();
          for (JsonVariant m : modes) {
            String s = m.as<String>();
            if (s == "brightness") ent.supportsBrightness = true;
            if (s == "hs" || s == "rgb" || s == "rgbw" || s == "rgbww" || s == "xy") {
              ent.supportsBrightness = true;
              ent.supportsRgb = true;
            }
          }
        }

        // Parse RGB color
        if (doc["attributes"]["rgb_color"].is<JsonArray>()) {
          JsonArray rgbArr = doc["attributes"]["rgb_color"].as<JsonArray>();
          if (rgbArr.size() >= 3) {
            ent.rgb[0] = rgbArr[0].as<uint8_t>();
            ent.rgb[1] = rgbArr[1].as<uint8_t>();
            ent.rgb[2] = rgbArr[2].as<uint8_t>();
            ent.supportsRgb = true;
          }
        }

        // Parse fan speed percentage
        if (doc["attributes"]["percentage"].is<int>()) {
          ent.speedPct = doc["attributes"]["percentage"].as<int>();
          ent.supportsSpeed = true;
        }
        if (entityId.startsWith("fan.")) {
          ent.supportsSpeed = true;
        }

        // Parse cover position
        if (doc["attributes"]["current_position"].is<int>()) {
          ent.currentPosition = doc["attributes"]["current_position"].as<int>();
          ent.supportsPosition = true;
        }
        if (entityId.startsWith("cover.")) {
          ent.supportsPosition = true;
        }

        // Parse climate attributes
        if (entityId.startsWith("climate.")) {
          ent.supportsClimate = true;
          if (doc["attributes"]["temperature"].is<float>()) {
            ent.targetTemp = doc["attributes"]["temperature"].as<float>();
          }
          if (doc["attributes"]["current_temperature"].is<float>()) {
            ent.currentTemp = doc["attributes"]["current_temperature"].as<float>();
          }
          if (doc["attributes"]["min_temp"].is<float>()) {
            ent.minTemp = doc["attributes"]["min_temp"].as<float>();
          }
          if (doc["attributes"]["max_temp"].is<float>()) {
            ent.maxTemp = doc["attributes"]["max_temp"].as<float>();
          }
          if (doc["attributes"]["target_temp_step"].is<float>()) {
            ent.tempStep = doc["attributes"]["target_temp_step"].as<float>();
          }
          if (doc["attributes"]["hvac_action"].is<const char*>()) {
            ent.hvacAction = doc["attributes"]["hvac_action"].as<String>();
          }
        }

        xSemaphoreGive(_mutex);

        if (_stateCallback) {
          _stateCallback(ent);
        }
      }
      return true;
    }
  } else {
    http.end();
    if (code == 401 || code < 0) {
      if (_connected) {
        _connected = false;
        if (_statusCallback) _statusCallback(false, String("HTTP ") + String(code));
      }
    }
  }

  return false;
}

bool HaClient::callServiceWithJson(const String& domain, const String& service, const String& jsonPayload) {
  if (WiFi.status() != WL_CONNECTED || _url.length() == 0 || _token.length() == 0) {
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String endpoint = _url + "/api/services/" + domain + "/" + service;

  http.begin(client, endpoint);
  http.addHeader("Authorization", "Bearer " + _token);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2500);

  Serial.printf("[HA] POST %s -> %s\n", endpoint.c_str(), jsonPayload.c_str());
  int code = http.POST(jsonPayload);
  http.end();

  Serial.printf("[HA] Service response: %d\n", code);
  return (code >= 200 && code < 300);
}

bool HaClient::setLightBrightness(const String& entityId, int brightnessPct) {
  if (brightnessPct < 0) brightnessPct = 0;
  if (brightnessPct > 100) brightnessPct = 100;

  if (entityId == "light.cyd_demo_rgb") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      HaEntity& ent = _entities[entityId];
      ent.brightnessPct = brightnessPct;
      ent.brightness = (brightnessPct * 255) / 100;
      ent.state = (brightnessPct > 0) ? "on" : "off";
      HaEntity copy = ent;
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }

  DynamicJsonDocument doc(256);
  doc["entity_id"] = entityId;
  doc["brightness_pct"] = brightnessPct;
  String body;
  serializeJson(doc, body);
  return callServiceWithJson("light", "turn_on", body);
}

bool HaClient::setLightRgb(const String& entityId, uint8_t r, uint8_t g, uint8_t b) {
  if (entityId == "light.cyd_demo_rgb") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      HaEntity& ent = _entities[entityId];
      ent.rgb[0] = r;
      ent.rgb[1] = g;
      ent.rgb[2] = b;
      ent.state = "on";
      HaEntity copy = ent;
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }

  DynamicJsonDocument doc(256);
  doc["entity_id"] = entityId;
  JsonArray rgbArr = doc.createNestedArray("rgb_color");
  rgbArr.add(r);
  rgbArr.add(g);
  rgbArr.add(b);
  String body;
  serializeJson(doc, body);
  return callServiceWithJson("light", "turn_on", body);
}

bool HaClient::setFanSpeed(const String& entityId, int speedPct) {
  if (speedPct < 0) speedPct = 0;
  if (speedPct > 100) speedPct = 100;

  if (entityId == "fan.cyd_demo_fan") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      HaEntity& ent = _entities[entityId];
      ent.speedPct = speedPct;
      ent.state = (speedPct > 0) ? "on" : "off";
      HaEntity copy = ent;
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }

  DynamicJsonDocument doc(256);
  doc["entity_id"] = entityId;
  doc["percentage"] = speedPct;
  String body;
  serializeJson(doc, body);
  return callServiceWithJson("fan", "set_percentage", body);
}

bool HaClient::openCover(const String& entityId) {
  if (entityId == "cover.cyd_demo_blinds") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      _entities[entityId].state = "open";
      _entities[entityId].currentPosition = 100;
      HaEntity copy = _entities[entityId];
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }
  return callService("cover", "open_cover", entityId);
}

bool HaClient::closeCover(const String& entityId) {
  if (entityId == "cover.cyd_demo_blinds") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      _entities[entityId].state = "closed";
      _entities[entityId].currentPosition = 0;
      HaEntity copy = _entities[entityId];
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }
  return callService("cover", "close_cover", entityId);
}

bool HaClient::stopCover(const String& entityId) {
  if (entityId == "cover.cyd_demo_blinds") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      _entities[entityId].state = (_entities[entityId].currentPosition > 0) ? "open" : "closed";
      HaEntity copy = _entities[entityId];
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }
  return callService("cover", "stop_cover", entityId);
}

bool HaClient::setCoverPosition(const String& entityId, int positionPct) {
  if (positionPct < 0) positionPct = 0;
  if (positionPct > 100) positionPct = 100;

  if (entityId == "cover.cyd_demo_blinds") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      _entities[entityId].currentPosition = positionPct;
      _entities[entityId].state = (positionPct > 0) ? "open" : "closed";
      HaEntity copy = _entities[entityId];
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }

  DynamicJsonDocument doc(256);
  doc["entity_id"] = entityId;
  doc["position"] = positionPct;
  String body;
  serializeJson(doc, body);
  return callServiceWithJson("cover", "set_cover_position", body);
}

bool HaClient::setHvacMode(const String& entityId, const String& mode) {
  if (entityId == "climate.cyd_demo_thermostat") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      _entities[entityId].state = mode;
      _entities[entityId].hvacAction = (mode == "off") ? "off" : (mode == "heat" ? "heating" : (mode == "cool" ? "cooling" : "idle"));
      HaEntity copy = _entities[entityId];
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }

  DynamicJsonDocument doc(256);
  doc["entity_id"] = entityId;
  doc["hvac_mode"] = mode;
  String body;
  serializeJson(doc, body);
  return callServiceWithJson("climate", "set_hvac_mode", body);
}

bool HaClient::setClimateTargetTemp(const String& entityId, float temp) {
  if (entityId == "climate.cyd_demo_thermostat") {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      _entities[entityId].targetTemp = temp;
      HaEntity copy = _entities[entityId];
      xSemaphoreGive(_mutex);
      if (_stateCallback) _stateCallback(copy);
    }
    return true;
  }

  DynamicJsonDocument doc(256);
  doc["entity_id"] = entityId;
  doc["temperature"] = temp;
  String body;
  serializeJson(doc, body);
  return callServiceWithJson("climate", "set_temperature", body);
}

bool HaClient::callService(const String& domain, const String& service, const String& entityId) {
  if (WiFi.status() != WL_CONNECTED || _url.length() == 0 || _token.length() == 0) {
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String endpoint = _url + "/api/services/" + domain + "/" + service;

  http.begin(client, endpoint);
  http.addHeader("Authorization", "Bearer " + _token);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2500);

  DynamicJsonDocument doc(256);
  doc["entity_id"] = entityId;
  String body;
  serializeJson(doc, body);

  Serial.printf("[HA] POST %s for %s\n", endpoint.c_str(), entityId.c_str());
  int code = http.POST(body);
  http.end();

  Serial.printf("[HA] Service response: %d\n", code);
  return (code >= 200 && code < 300);
}

void HaClient::backgroundTask(void* param) {
  HaClient* client = static_cast<HaClient*>(param);
  esp_task_wdt_add(NULL); // Attach Core 0 background task to TWDT

  uint32_t backoffMs = 0;
  const uint32_t MAX_BACKOFF_MS = 30000;

  for (;;) {
    esp_task_wdt_reset();

    // Base poll interval is 4s, or backoff interval if HA is down
    uint32_t waitMs = (backoffMs > 4000) ? backoffMs : 4000;
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(waitMs));
    esp_task_wdt_reset();

    if (WiFi.status() == WL_CONNECTED) {
      if (client->_toggleRequested) {
        String target;
        if (xSemaphoreTake(client->_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          target = client->_toggleEntityId;
          client->_toggleRequested = false;
          xSemaphoreGive(client->_mutex);
        }

        if (target.length() > 0) {
          if (target == "light.cyd_demo_rgb" || target == "fan.cyd_demo_fan" || target == "scene.cyd_demo_movie") {
            client->fetchState(target);
          } else if (target == "cover.cyd_demo_blinds") {
            HaEntity ent = client->_entities[target];
            if (ent.state == "open") {
              client->closeCover(target);
            } else {
              client->openCover(target);
            }
          } else if (target.startsWith("cover.")) {
            client->callService("cover", "toggle", target);
            vTaskDelay(pdMS_TO_TICKS(750));
            client->fetchState(target);
          } else if (target == "climate.cyd_demo_thermostat") {
            HaEntity ent = client->_entities[target];
            String nextMode = (ent.state == "off") ? "heat" : "off";
            client->setHvacMode(target, nextMode);
          } else if (target.startsWith("climate.")) {
            HaEntity ent = client->_entities[target];
            String nextMode = (ent.state == "off") ? "heat" : "off";
            client->setHvacMode(target, nextMode);
            vTaskDelay(pdMS_TO_TICKS(750));
            client->fetchState(target);
          } else if (target.startsWith("scene.")) {
            client->callService("scene", "turn_on", target);
            vTaskDelay(pdMS_TO_TICKS(500));
            client->fetchState(target);
          } else if (target.startsWith("script.")) {
            client->callService("script", "turn_on", target);
            vTaskDelay(pdMS_TO_TICKS(500));
            client->fetchState(target);
          } else {
            // Universal Home Assistant service toggle
            client->callService("homeassistant", "toggle", target);

            // Allow hardware relay / mesh 750ms to report to HA
            vTaskDelay(pdMS_TO_TICKS(750));
            client->fetchState(target);
          }
        }
        esp_task_wdt_reset();
      } else {
        // First check HA API if currently flagged disconnected
        if (!client->_connected) {
          bool ok = client->checkApi();
          if (!ok) {
            if (backoffMs == 0) backoffMs = 2000;
            else backoffMs = (backoffMs * 2 > MAX_BACKOFF_MS) ? MAX_BACKOFF_MS : (backoffMs * 2);
            Serial.printf("[HA] Outage backoff active: retry in %u ms\n", backoffMs);
            continue;
          } else {
            backoffMs = 0; // Recovered!
          }
        }

        // Direct periodic fetch for all monitored entities
        std::vector<String> toFetch;
        if (xSemaphoreTake(client->_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          toFetch = client->_monitoredEntities;
          xSemaphoreGive(client->_mutex);
        }

        bool anyFetchFailed = false;
        for (const auto& entId : toFetch) {
          esp_task_wdt_reset();
          if (client->_toggleRequested) {
            // Priority user interaction broke periodic poll loop!
            break;
          }
          bool ok = client->fetchState(entId);
          if (!ok && !entId.startsWith("cyd_demo_") && entId.indexOf(".cyd_demo_") < 0) {
            anyFetchFailed = true;
          }
          vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (anyFetchFailed && !client->_connected) {
          if (backoffMs == 0) backoffMs = 4000;
          else backoffMs = (backoffMs * 2 > MAX_BACKOFF_MS) ? MAX_BACKOFF_MS : (backoffMs * 2);
        } else {
          backoffMs = 0;
        }
      }
    } else {
      // Wi-Fi disconnected, wait without hammer
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}
