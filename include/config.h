#pragma once
#include <Arduino.h>

#define APP_NAME "CYD-HA-Dashboard"
#define APP_VERSION "0.1.1"
#define HARDWARE_MODEL "ESP32-2432S028R (2.8\" Resistive Touch)"

namespace config {
  constexpr uint16_t SCREEN_WIDTH = 320;
  constexpr uint16_t SCREEN_HEIGHT = 240;
  constexpr uint8_t DEFAULT_ROTATION = 1; // Landscape
  constexpr uint8_t DRAW_BUF_LINES = 20;
  constexpr uint8_t DEFAULT_BRIGHTNESS = 200; // 0-255
}
