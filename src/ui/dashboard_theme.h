#pragma once
#include <Arduino.h>
#include "lvgl.h"

struct DashboardTheme {
  const char* id;
  const char* name;
  bool isDark;
  uint32_t screenBg;
  uint32_t headerBg;
  uint32_t headerText;
  uint32_t tileIdleBg;
  uint32_t tileBorder;
  uint32_t tileText;
  uint32_t tileIconIdle;
  uint32_t switchOnBg;
  uint32_t cardBg;
  uint32_t cardBorder;
  uint32_t cardText;
};

inline const DashboardTheme& getThemeById(const String& themeId) {
  // 1. Home Assistant Dark (Default)
  static const DashboardTheme TH_HA_DARK = {
    "ha_dark", "Home Assistant (Dark)", true,
    0x111111, 0x1C1C1E, 0xFFFFFF,
    0x1F1F24, 0x38383A, 0xF1F5F9, 0x8E8E93,
    0x0284C7, 0x1C1C1E, 0x38383A, 0xFFFFFF
  };

  // 2. Home Assistant Light
  static const DashboardTheme TH_HA_LIGHT = {
    "ha_light", "Home Assistant (Light)", false,
    0xF8FAFC, 0xFFFFFF, 0x0F172A,
    0xFFFFFF, 0xE2E8F0, 0x0F172A, 0x64748B,
    0x0284C7, 0xFFFFFF, 0xCBD5E1, 0x0F172A
  };

  // 3. Slate Dark
  static const DashboardTheme TH_SLATE_DARK = {
    "slate_dark", "Slate (Dark)", true,
    0x0F172A, 0x1E293B, 0xFFFFFF,
    0x1E293B, 0x334155, 0xF8FAFC, 0x64748B,
    0x10B981, 0x1E293B, 0x475569, 0xFFFFFF
  };

  // 4. Slate Light
  static const DashboardTheme TH_SLATE_LIGHT = {
    "slate_light", "Slate (Light)", false,
    0xF1F5F9, 0xFFFFFF, 0x0F172A,
    0xFFFFFF, 0xCBD5E1, 0x0F172A, 0x64748B,
    0x059669, 0xFFFFFF, 0x94A3B8, 0x0F172A
  };

  // 5. Midnight OLED Dark
  static const DashboardTheme TH_OLED_DARK = {
    "oled_dark", "Midnight OLED (Dark)", true,
    0x000000, 0x0A0A0A, 0xFFFFFF,
    0x121212, 0x27272A, 0xFFFFFF, 0x71717A,
    0x6366F1, 0x121212, 0x27272A, 0xFFFFFF
  };

  // 6. Midnight OLED Light
  static const DashboardTheme TH_OLED_LIGHT = {
    "oled_light", "Midnight OLED (Light)", false,
    0xF8F9FA, 0xFFFFFF, 0x000000,
    0xFFFFFF, 0xE4E4E7, 0x09090B, 0x71717A,
    0x4F46E5, 0xFFFFFF, 0xD4D4D8, 0x09090B
  };

  // 7. Forest Dark
  static const DashboardTheme TH_FOREST_DARK = {
    "forest_dark", "Forest Pine (Dark)", true,
    0x041F16, 0x063525, 0xECFDF5,
    0x0A3F2E, 0x115E43, 0xF0FDF4, 0x6EE7B7,
    0x059669, 0x063525, 0x115E43, 0xF0FDF4
  };

  // 8. Forest Light
  static const DashboardTheme TH_FOREST_LIGHT = {
    "forest_light", "Forest Sage (Light)", false,
    0xF0FDF4, 0xFFFFFF, 0x064E3B,
    0xFFFFFF, 0xA7F3D0, 0x064E3B, 0x059669,
    0x059669, 0xFFFFFF, 0x6EE7B7, 0x064E3B
  };

  // 9. Amber Cyberpunk Dark
  static const DashboardTheme TH_AMBER_DARK = {
    "amber_dark", "Amber Cyberpunk (Dark)", true,
    0x120C06, 0x1F160C, 0xFEF3C7,
    0x261B0E, 0x543B1A, 0xFFFBEB, 0xFBBF24,
    0xD97706, 0x1F160C, 0x543B1A, 0xFFFBEB
  };

  // 10. Amber Sand Light
  static const DashboardTheme TH_AMBER_LIGHT = {
    "amber_light", "Amber Sand (Light)", false,
    0xFFFBEB, 0xFFFFFF, 0x78350F,
    0xFFFFFF, 0xFDE68A, 0x78350F, 0xB45309,
    0xD97706, 0xFFFFFF, 0xFCD34D, 0x78350F
  };

  if (themeId == "ha_light") return TH_HA_LIGHT;
  if (themeId == "slate_dark") return TH_SLATE_DARK;
  if (themeId == "slate_light") return TH_SLATE_LIGHT;
  if (themeId == "oled_dark") return TH_OLED_DARK;
  if (themeId == "oled_light") return TH_OLED_LIGHT;
  if (themeId == "forest_dark") return TH_FOREST_DARK;
  if (themeId == "forest_light") return TH_FOREST_LIGHT;
  if (themeId == "amber_dark") return TH_AMBER_DARK;
  if (themeId == "amber_light") return TH_AMBER_LIGHT;
  return TH_HA_DARK; // Default fallback
}
