#pragma once
#include <Arduino.h>
#include <vector>

enum class WidgetType {
  TOGGLE,
  SENSOR,
  BINARY_SENSOR
};

struct Widget {
  String id;
  String entityId;
  WidgetType type = WidgetType::TOGGLE;
  String label;
  String icon;
  int slot = 0; // 0..3 (2x2 grid)
  std::vector<String> colorPresets;
};

struct DashboardPage {
  String id;
  String name;
  String grid = "2x2"; // "2x2" (4 slots) or "3x2" (6 slots)
  std::vector<Widget> widgets;
};

struct ColorRgb {
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 255;
  uint32_t hex = 0xFFFFFF;
};

class DashboardConfig {
public:
  DashboardConfig();
  bool begin();
  bool load();
  bool save();
  bool reload();
  bool loadFromJson(const String& jsonStr);
  String getJsonString() const;
  void createDefaults(const String& userEntity = "switch.living_room_light");
  void resetToDefaults();

  size_t getPageCount() const;
  size_t getActivePageIndex() const;
  const DashboardPage& getActivePage() const;
  bool setPage(size_t index);
  bool nextPage();
  bool prevPage();

  std::vector<String> getAllEntityIds() const;
  std::vector<String> getActivePageEntityIds() const;

  std::vector<String> colorPresets;
  std::vector<ColorRgb> getColorPresets() const;
  std::vector<ColorRgb> getColorPresetsForWidget(const Widget* w) const;
  static ColorRgb hexToRgb(const String& hexStr);

  std::vector<DashboardPage> pages;

  int activeBrightness = 255;  // 10-255 (default 255 = 100%)
  int dimBrightness = 40;      // 0-255 (default 40 = ~15%)
  int dimTimeoutSec = 30;      // Idle seconds before dimming (0 = disabled)
  int sleepTimeoutSec = 60;    // Idle seconds before sleep/off (0 = disabled)
  String theme = "ha_dark";    // e.g. "ha_dark", "ha_light", "slate_dark", "slate_light", etc.

private:
  size_t _activePageIndex = 0;
  static const char* CONFIG_PATH;
};
