#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

struct TouchCalibration {
  float xCalM = 0.088f;
  float xCalC = -24.0f;
  float yCalM = 0.065f;
  float yCalC = -16.0f;
  bool valid = false;
};

class TouchCalibrator {
public:
  TouchCalibrator();
  bool begin(TFT_eSPI& tft, XPT2046_Touchscreen& touch, bool forceCalibration = false);
  void mapCoords(int16_t rawX, int16_t rawY, int16_t& outX, int16_t& outY) const;
  void runInteractiveCalibration(TFT_eSPI& tft, XPT2046_Touchscreen& touch);
  void resetCalibration();

  const TouchCalibration& getConfig() const { return _config; }

private:
  bool load();
  void save();

  TouchCalibration _config;
  static const char* TOUCH_PATH;
};
