#pragma once
#include <Arduino.h>

namespace board {
  // Backlight pin
  constexpr int PIN_BACKLIGHT = 21;

  // Touch pins (VSPI)
  constexpr int TOUCH_IRQ   = 36;
  constexpr int TOUCH_MOSI  = 32;
  constexpr int TOUCH_MISO  = 39;
  constexpr int TOUCH_CLK   = 25;
  constexpr int TOUCH_CS    = 33;

  // Touch calibration limits
  constexpr int TOUCH_RAW_X_MIN = 250;
  constexpr int TOUCH_RAW_X_MAX = 3850;
  constexpr int TOUCH_RAW_Y_MIN = 250;
  constexpr int TOUCH_RAW_Y_MAX = 3850;
}
