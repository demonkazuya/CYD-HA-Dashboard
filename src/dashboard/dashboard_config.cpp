#include "dashboard_config.h"
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char* DashboardConfig::CONFIG_PATH = "/config.json";

static WidgetType inferWidgetType(const String& entityId, const String& explicitType) {
  if (explicitType == "sensor") return WidgetType::SENSOR;
  if (explicitType == "binary_sensor") return WidgetType::BINARY_SENSOR;
  if (explicitType == "toggle") return WidgetType::TOGGLE;

  if (entityId.startsWith("binary_sensor.")) return WidgetType::BINARY_SENSOR;
  if (entityId.startsWith("sensor.") || entityId.startsWith("weather.") || entityId.startsWith("number.")) return WidgetType::SENSOR;
  return WidgetType::TOGGLE;
}

ColorRgb DashboardConfig::hexToRgb(const String& hexStr) {
  ColorRgb c;
  String h = hexStr;
  h.trim();
  if (h.startsWith("#")) h = h.substring(1);
  if (h.length() == 6) {
    uint32_t val = (uint32_t)strtoul(h.c_str(), nullptr, 16);
    c.hex = val;
    c.r = (val >> 16) & 0xFF;
    c.g = (val >> 8) & 0xFF;
    c.b = val & 0xFF;
  }
  return c;
}

std::vector<ColorRgb> DashboardConfig::getColorPresets() const {
  std::vector<ColorRgb> list;
  for (const auto& hexStr : colorPresets) {
    list.push_back(hexToRgb(hexStr));
  }
  const char* defs[6] = { "#FFF1E0", "#EF4444", "#F59E0B", "#10B981", "#38BDF8", "#A855F7" };
  while (list.size() < 6) {
    list.push_back(hexToRgb(defs[list.size()]));
  }
  return list;
}

std::vector<ColorRgb> DashboardConfig::getColorPresetsForWidget(const Widget* w) const {
  std::vector<ColorRgb> list;
  if (w && !w->colorPresets.empty()) {
    for (const auto& hexStr : w->colorPresets) {
      list.push_back(hexToRgb(hexStr));
    }
  }
  if (list.size() < 6) {
    std::vector<ColorRgb> defs = getColorPresets();
    for (size_t i = list.size(); i < 6 && i < defs.size(); i++) {
      list.push_back(defs[i]);
    }
  }
  return list;
}

DashboardConfig::DashboardConfig() {
  colorPresets = { "#FFF1E0", "#EF4444", "#F59E0B", "#10B981", "#38BDF8", "#A855F7" };
}

bool DashboardConfig::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("[Config] LittleFS Mount Failed!");
    createDefaults();
    return false;
  }

  if (!LittleFS.exists(CONFIG_PATH)) {
    Serial.println("[Config] config.json does not exist. Creating defaults...");
    createDefaults();
    save();
    return true;
  }

  return load();
}

void DashboardConfig::createDefaults(const String& userEntity) {
  pages.clear();

  // Page 1: Living Room
  DashboardPage p1;
  p1.id = "living_room";
  p1.name = "Living Room";

  Widget w1;
  w1.id = "main_light";
  w1.entityId = userEntity.length() > 0 ? userEntity : "switch.living_room_light";
  w1.type = WidgetType::TOGGLE;
  w1.label = "Main Light";
  w1.slot = 0;
  p1.widgets.push_back(w1);

  Widget w2;
  w2.id = "room_temp";
  w2.entityId = "sensor.living_room_temperature";
  w2.type = WidgetType::SENSOR;
  w2.label = "Temperature";
  w2.slot = 1;
  p1.widgets.push_back(w2);

  Widget w3;
  w3.id = "tv_socket";
  w3.entityId = "switch.living_room_tv";
  w3.type = WidgetType::TOGGLE;
  w3.label = "TV Outlet";
  w3.slot = 2;
  p1.widgets.push_back(w3);

  Widget w4;
  w4.id = "motion";
  w4.entityId = "binary_sensor.living_room_presence";
  w4.type = WidgetType::BINARY_SENSOR;
  w4.label = "Presence";
  w4.slot = 3;
  p1.widgets.push_back(w4);

  pages.push_back(p1);

  // Page 2: Office & 3D Lab
  DashboardPage p2;
  p2.id = "office";
  p2.name = "Office & 3D Lab";

  Widget w5;
  w5.id = "desk_lamp";
  w5.entityId = "switch.desk_lamp";
  w5.type = WidgetType::TOGGLE;
  w5.label = "Desk Lamp";
  w5.slot = 0;
  p2.widgets.push_back(w5);

  Widget w6;
  w6.id = "printer_status";
  w6.entityId = "sensor.bambu_printer_status";
  w6.type = WidgetType::SENSOR;
  w6.label = "3D Printer";
  w6.slot = 1;
  p2.widgets.push_back(w6);

  Widget w7;
  w7.id = "soldering_iron";
  w7.entityId = "switch.workbench_power";
  w7.type = WidgetType::TOGGLE;
  w7.label = "Workbench";
  w7.slot = 2;
  p2.widgets.push_back(w7);

  Widget w8;
  w8.id = "power_usage";
  w8.entityId = "sensor.office_power_watts";
  w8.type = WidgetType::SENSOR;
  w8.label = "Power Draw";
  w8.slot = 3;
  p2.widgets.push_back(w8);

  pages.push_back(p2);

  // Page 3: Security & Home Status
  DashboardPage p3;
  p3.id = "security";
  p3.name = "Security";

  Widget w9;
  w9.id = "front_door";
  w9.entityId = "binary_sensor.front_door_contact";
  w9.type = WidgetType::BINARY_SENSOR;
  w9.label = "Front Door";
  w9.slot = 0;
  p3.widgets.push_back(w9);

  Widget w10;
  w10.id = "outdoor_light";
  w10.entityId = "switch.porch_light";
  w10.type = WidgetType::TOGGLE;
  w10.label = "Porch Light";
  w10.slot = 1;
  p3.widgets.push_back(w10);

  Widget w11;
  w11.id = "garage_door";
  w11.entityId = "binary_sensor.garage_door_contact";
  w11.type = WidgetType::BINARY_SENSOR;
  w11.label = "Garage Door";
  w11.slot = 2;
  p3.widgets.push_back(w11);

  Widget w12;
  w12.id = "smoke_detector";
  w12.entityId = "binary_sensor.smoke_alarm";
  w12.type = WidgetType::BINARY_SENSOR;
  w12.label = "Smoke Alarm";
  w12.slot = 3;
  p3.widgets.push_back(w12);

  pages.push_back(p3);

  // Page 4: Demo Lab (Dimmer & Fan Speed Controls)
  DashboardPage p4;
  p4.id = "demo_lab";
  p4.name = "Demo Controls";
  p4.grid = "2x2";

  Widget w13;
  w13.id = "demo_rgb";
  w13.entityId = "light.cyd_demo_rgb";
  w13.type = WidgetType::TOGGLE;
  w13.label = "Demo RGB Light";
  w13.icon = "mdi:lightbulb";
  w13.slot = 0;
  p4.widgets.push_back(w13);

  Widget w14;
  w14.id = "demo_fan";
  w14.entityId = "fan.cyd_demo_fan";
  w14.type = WidgetType::TOGGLE;
  w14.label = "Demo Fan Speed";
  w14.icon = "mdi:fan";
  w14.slot = 1;
  p4.widgets.push_back(w14);

  Widget w15;
  w15.id = "demo_blinds";
  w15.entityId = "cover.cyd_demo_blinds";
  w15.type = WidgetType::TOGGLE;
  w15.label = "Demo Blinds";
  w15.icon = "mdi:blinds";
  w15.slot = 2;
  p4.widgets.push_back(w15);

  Widget w16;
  w16.id = "demo_thermostat";
  w16.entityId = "climate.cyd_demo_thermostat";
  w16.type = WidgetType::TOGGLE;
  w16.label = "Demo Thermostat";
  w16.icon = "mdi:thermostat";
  w16.slot = 3;
  p4.widgets.push_back(w16);

  pages.push_back(p4);
  _activePageIndex = 0;
}

void DashboardConfig::resetToDefaults() {
  createDefaults();
  save();
  _activePageIndex = 0;
}

bool DashboardConfig::reload() {
  return load();
}

bool DashboardConfig::loadFromJson(const String& jsonStr) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonStr);
  if (err) {
    Serial.printf("[Config] JSON parse error: %s\n", err.c_str());
    return false;
  }

  pages.clear();
  if (doc["display"].is<JsonObject>()) {
    if (doc["display"]["brightness"].is<int>()) activeBrightness = doc["display"]["brightness"].as<int>();
    if (doc["display"]["dim_brightness"].is<int>()) dimBrightness = doc["display"]["dim_brightness"].as<int>();
    if (doc["display"]["dim_timeout"].is<int>()) dimTimeoutSec = doc["display"]["dim_timeout"].as<int>();
    if (doc["display"]["sleep_timeout"].is<int>()) sleepTimeoutSec = doc["display"]["sleep_timeout"].as<int>();
  }
  if (doc["theme"].is<const char*>()) {
    theme = doc["theme"].as<String>();
  }
  if (doc["color_presets"].is<JsonArray>()) {
    colorPresets.clear();
    for (JsonVariant c : doc["color_presets"].as<JsonArray>()) {
      colorPresets.push_back(c.as<String>());
    }
  }
  JsonArray arrPages = doc["pages"].as<JsonArray>();
  for (JsonObject pObj : arrPages) {
    DashboardPage page;
    page.id = pObj["id"].as<String>();
    page.name = pObj["name"].as<String>();
    page.grid = pObj["grid"].isNull() ? "2x2" : pObj["grid"].as<String>();

    JsonArray arrWidgets = pObj["widgets"].as<JsonArray>();
    for (JsonObject wObj : arrWidgets) {
      Widget w;
      w.id = wObj["id"].as<String>();
      w.entityId = wObj["entity_id"].as<String>();
      w.label = wObj["label"].as<String>();
      w.icon = wObj["icon"].isNull() ? "" : wObj["icon"].as<String>();
      w.slot = wObj["slot"] | 0;

      String typeStr = wObj["type"].as<String>();
      w.type = inferWidgetType(w.entityId, typeStr);

      w.colorPresets.clear();
      if (wObj["color_presets"].is<JsonArray>()) {
        for (JsonVariant cp : wObj["color_presets"].as<JsonArray>()) {
          w.colorPresets.push_back(cp.as<String>());
        }
      }

      page.widgets.push_back(w);
    }
    pages.push_back(page);
  }

  if (pages.empty()) {
    createDefaults();
  }

  if (_activePageIndex >= pages.size()) {
    _activePageIndex = 0;
  }

  save();
  Serial.printf("[Config] Applied %u pages from JSON and saved to LittleFS\n", pages.size());
  return true;
}

String DashboardConfig::getJsonString() const {
  JsonDocument doc;
  doc["schema_version"] = 1;

  JsonObject disp = doc["display"].to<JsonObject>();
  disp["brightness"] = activeBrightness;
  disp["dim_brightness"] = dimBrightness;
  disp["dim_timeout"] = dimTimeoutSec;
  disp["sleep_timeout"] = sleepTimeoutSec;
  doc["theme"] = theme;

  JsonArray arrColors = doc["color_presets"].to<JsonArray>();
  for (const auto& c : colorPresets) {
    arrColors.add(c);
  }

  JsonArray arrPages = doc["pages"].to<JsonArray>();
  for (const auto& p : pages) {
    JsonObject pObj = arrPages.add<JsonObject>();
    pObj["id"] = p.id;
    pObj["name"] = p.name;
    pObj["grid"] = p.grid.length() > 0 ? p.grid : "2x2";

    JsonArray arrWidgets = pObj["widgets"].to<JsonArray>();
    for (const auto& w : p.widgets) {
      JsonObject wObj = arrWidgets.add<JsonObject>();
      wObj["id"] = w.id;
      wObj["entity_id"] = w.entityId;
      wObj["label"] = w.label;
      wObj["icon"] = w.icon;
      wObj["slot"] = w.slot;

      switch (w.type) {
        case WidgetType::SENSOR:
          wObj["type"] = "sensor";
          break;
        case WidgetType::BINARY_SENSOR:
          wObj["type"] = "binary_sensor";
          break;
        case WidgetType::TOGGLE:
        default:
          wObj["type"] = "toggle";
          break;
      }

      if (!w.colorPresets.empty()) {
        JsonArray arrCp = wObj["color_presets"].to<JsonArray>();
        for (const auto& cp : w.colorPresets) {
          arrCp.add(cp);
        }
      }
    }
  }

  String out;
  serializeJsonPretty(doc, out);
  return out;
}

bool DashboardConfig::load() {
  fs::File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) {
    Serial.println("[Config] Failed to open config.json for reading");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.printf("[Config] JSON parse error: %s\n", err.c_str());
    return false;
  }

  pages.clear();
  if (doc["display"].is<JsonObject>()) {
    if (doc["display"]["brightness"].is<int>()) activeBrightness = doc["display"]["brightness"].as<int>();
    if (doc["display"]["dim_brightness"].is<int>()) dimBrightness = doc["display"]["dim_brightness"].as<int>();
    if (doc["display"]["dim_timeout"].is<int>()) dimTimeoutSec = doc["display"]["dim_timeout"].as<int>();
    if (doc["display"]["sleep_timeout"].is<int>()) sleepTimeoutSec = doc["display"]["sleep_timeout"].as<int>();
  }
  if (doc["theme"].is<const char*>()) {
    theme = doc["theme"].as<String>();
  }
  if (doc["color_presets"].is<JsonArray>()) {
    colorPresets.clear();
    for (JsonVariant c : doc["color_presets"].as<JsonArray>()) {
      colorPresets.push_back(c.as<String>());
    }
  }
  JsonArray arrPages = doc["pages"].as<JsonArray>();
  for (JsonObject pObj : arrPages) {
    DashboardPage page;
    page.id = pObj["id"].as<String>();
    page.name = pObj["name"].as<String>();
    page.grid = pObj["grid"].isNull() ? "2x2" : pObj["grid"].as<String>();

    JsonArray arrWidgets = pObj["widgets"].as<JsonArray>();
    for (JsonObject wObj : arrWidgets) {
      Widget w;
      w.id = wObj["id"].as<String>();
      w.entityId = wObj["entity_id"].as<String>();
      w.label = wObj["label"].as<String>();
      w.icon = wObj["icon"].isNull() ? "" : wObj["icon"].as<String>();
      w.slot = wObj["slot"] | 0;

      String typeStr = wObj["type"].as<String>();
      w.type = inferWidgetType(w.entityId, typeStr);

      w.colorPresets.clear();
      if (wObj["color_presets"].is<JsonArray>()) {
        for (JsonVariant cp : wObj["color_presets"].as<JsonArray>()) {
          w.colorPresets.push_back(cp.as<String>());
        }
      }

      page.widgets.push_back(w);
    }
    pages.push_back(page);
  }

  if (pages.empty()) {
    createDefaults();
  }

  if (_activePageIndex >= pages.size()) {
    _activePageIndex = 0;
  }

  Serial.printf("[Config] Loaded %u dashboard pages from LittleFS\n", pages.size());
  return true;
}

bool DashboardConfig::save() {
  JsonDocument doc;
  doc["schema_version"] = 1;

  JsonObject disp = doc["display"].to<JsonObject>();
  disp["brightness"] = activeBrightness;
  disp["dim_brightness"] = dimBrightness;
  disp["dim_timeout"] = dimTimeoutSec;
  disp["sleep_timeout"] = sleepTimeoutSec;
  doc["theme"] = theme;

  JsonArray arrColors = doc["color_presets"].to<JsonArray>();
  for (const auto& c : colorPresets) {
    arrColors.add(c);
  }

  JsonArray arrPages = doc["pages"].to<JsonArray>();
  for (const auto& p : pages) {
    JsonObject pObj = arrPages.add<JsonObject>();
    pObj["id"] = p.id;
    pObj["name"] = p.name;
    pObj["grid"] = p.grid.length() > 0 ? p.grid : "2x2";

    JsonArray arrWidgets = pObj["widgets"].to<JsonArray>();
    for (const auto& w : p.widgets) {
      JsonObject wObj = arrWidgets.add<JsonObject>();
      wObj["id"] = w.id;
      wObj["entity_id"] = w.entityId;
      wObj["label"] = w.label;
      wObj["icon"] = w.icon;
      wObj["slot"] = w.slot;

      switch (w.type) {
        case WidgetType::SENSOR:
          wObj["type"] = "sensor";
          break;
        case WidgetType::BINARY_SENSOR:
          wObj["type"] = "binary_sensor";
          break;
        case WidgetType::TOGGLE:
        default:
          wObj["type"] = "toggle";
          break;
      }

      if (!w.colorPresets.empty()) {
        JsonArray arrCp = wObj["color_presets"].to<JsonArray>();
        for (const auto& cp : w.colorPresets) {
          arrCp.add(cp);
        }
      }
    }
  }

  fs::File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) {
    Serial.println("[Config] Failed to open config.json for writing");
    return false;
  }

  serializeJsonPretty(doc, f);
  f.close();
  Serial.println("[Config] Saved config.json to LittleFS");
  return true;
}

size_t DashboardConfig::getPageCount() const {
  return pages.size();
}

size_t DashboardConfig::getActivePageIndex() const {
  return _activePageIndex;
}

const DashboardPage& DashboardConfig::getActivePage() const {
  static DashboardPage emptyPage;
  if (pages.empty()) return emptyPage;
  return pages[_activePageIndex];
}

bool DashboardConfig::setPage(size_t index) {
  if (index >= pages.size()) return false;
  _activePageIndex = index;
  return true;
}

bool DashboardConfig::nextPage() {
  if (pages.empty()) return false;
  _activePageIndex = (_activePageIndex + 1) % pages.size();
  return true;
}

bool DashboardConfig::prevPage() {
  if (pages.empty()) return false;
  _activePageIndex = (_activePageIndex + pages.size() - 1) % pages.size();
  return true;
}

std::vector<String> DashboardConfig::getAllEntityIds() const {
  std::vector<String> list;
  for (const auto& p : pages) {
    for (const auto& w : p.widgets) {
      if (w.entityId.length() > 0) {
        bool exists = false;
        for (const auto& existing : list) {
          if (existing == w.entityId) {
            exists = true;
            break;
          }
        }
        if (!exists) list.push_back(w.entityId);
      }
    }
  }
  return list;
}

std::vector<String> DashboardConfig::getActivePageEntityIds() const {
  std::vector<String> list;
  if (_activePageIndex < pages.size()) {
    for (const auto& w : pages[_activePageIndex].widgets) {
      if (w.entityId.length() > 0) {
        list.push_back(w.entityId);
      }
    }
  }
  return list;
}
