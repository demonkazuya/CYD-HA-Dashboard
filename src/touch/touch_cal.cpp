#include "touch_cal.h"
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>

const char* TouchCalibrator::TOUCH_PATH = "/touch_v1.json";
static const char PREF_TOUCH_NS[] = "cyd_cal_v1";

TouchCalibrator::TouchCalibrator() {}

bool TouchCalibrator::begin(TFT_eSPI& tft, XPT2046_Touchscreen& touch, bool forceCalibration) {
  // Check if user is holding the screen at power-on to force recalibration
  bool forceRecal = forceCalibration;
  if (touch.touched()) {
    delay(100);
    if (touch.touched()) {
      Serial.println("[Touch] Screen held at startup -> Forcing recalibration!");
      forceRecal = true;
    }
  }

  if (!forceRecal && load()) {
    Serial.printf("[Touch] Calibration loaded: xM=%.4f xC=%.2f yM=%.4f yC=%.2f\n",
                  _config.xCalM, _config.xCalC, _config.yCalM, _config.yCalC);
    return true;
  }

  runInteractiveCalibration(tft, touch);
  return true;
}

void TouchCalibrator::mapCoords(int16_t rawX, int16_t rawY, int16_t& outX, int16_t& outY) const {
  outX = round((rawX * _config.xCalM) + _config.xCalC);
  outY = round((rawY * _config.yCalM) + _config.yCalC);
  outX = constrain(outX, 0, 319);
  outY = constrain(outY, 0, 239);
}

void TouchCalibrator::runInteractiveCalibration(TFT_eSPI& tft, XPT2046_Touchscreen& touch) {
  Serial.println("[Touch] Starting 2-Point Calibration Routine...");

  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  TS_Point p;
  int16_t x1 = 0, y1 = 0, x2 = 0, y2 = 0;

  while (true) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Screen Touch Calibration", 160, 80, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Tap the RED (+) crosshair with finger or stylus", 160, 115, 2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("[ Step 1 / 2: Top-Left Corner ]", 160, 145, 2);

    // Drain touch buffer
    while (touch.touched()) {
      delay(10);
    }

    // Target 1: Top-Left (25, 25)
    tft.drawFastHLine(10, 25, 31, TFT_RED);
    tft.drawFastVLine(25, 10, 31, TFT_RED);
    tft.drawCircle(25, 25, 12, TFT_WHITE);
    tft.drawCircle(25, 25, 6, TFT_CYAN);

    // Wait for touch
    while (!touch.touched()) {
      delay(10);
    }

    delay(60);
    p = touch.getPoint();
    x1 = p.x;
    y1 = p.y;
    Serial.printf("[Touch] Point 1 captured: rawX=%d rawY=%d\n", x1, y1);

    // Erase Target 1
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Screen Touch Calibration", 160, 80, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Tap the RED (+) crosshair with finger or stylus", 160, 115, 2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("[ Step 2 / 2: Bottom-Right Corner ]", 160, 145, 2);

    // Drain release
    while (touch.touched()) {
      delay(10);
    }
    delay(200);

    // Target 2: Bottom-Right (295, 215)
    tft.drawFastHLine(280, 215, 31, TFT_RED);
    tft.drawFastVLine(295, 200, 31, TFT_RED);
    tft.drawCircle(295, 215, 12, TFT_WHITE);
    tft.drawCircle(295, 215, 6, TFT_CYAN);

    // Wait for touch
    while (!touch.touched()) {
      delay(10);
    }

    delay(60);
    p = touch.getPoint();
    x2 = p.x;
    y2 = p.y;
    Serial.printf("[Touch] Point 2 captured: rawX=%d rawY=%d\n", x2, y2);

    tft.fillScreen(TFT_BLACK);

    // Sanity check: must have enough delta
    if (abs(x2 - x1) > 400 && abs(y2 - y1) > 400) {
      break;
    }
    Serial.println("[Touch] Calibration delta too small, retrying...");
    delay(400);
  }

  // Coordinates of targets: P1(25, 25), P2(295, 215)
  int16_t xDist = 295 - 25; // 270
  int16_t yDist = 215 - 25; // 190

  _config.xCalM = (float)xDist / (float)(x2 - x1);
  _config.xCalC = 25.0f - ((float)x1 * _config.xCalM);

  _config.yCalM = (float)yDist / (float)(y2 - y1);
  _config.yCalC = 25.0f - ((float)y1 * _config.yCalM);
  _config.valid = true;

  save();
  Serial.printf("[Touch] Calibrated: xM=%.4f xC=%.2f yM=%.4f yC=%.2f\n",
                _config.xCalM, _config.xCalC, _config.yCalM, _config.yCalC);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Calibration Complete!", 160, 100, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Launching Dashboard...", 160, 140, 2);

  while (touch.touched()) {
    delay(10);
  }
  delay(500);
}

bool TouchCalibrator::load() {
  bool loaded = false;

  // 1. Try LittleFS
  if (LittleFS.begin(true) && LittleFS.exists(TOUCH_PATH)) {
    fs::File f = LittleFS.open(TOUCH_PATH, "r");
    if (f) {
      JsonDocument doc;
      if (!deserializeJson(doc, f)) {
        if (!doc["xCalM"].isNull() && !doc["yCalM"].isNull()) {
          _config.xCalM = doc["xCalM"].as<float>();
          _config.yCalM = doc["yCalM"].as<float>();
          _config.xCalC = doc["xCalC"].as<float>();
          _config.yCalC = doc["yCalC"].as<float>();
          _config.valid = true;
          loaded = true;
        }
      }
      f.close();
    }
  }

  // 2. Try NVS fallback
  if (!loaded) {
    Preferences prefs;
    if (prefs.begin(PREF_TOUCH_NS, true)) {
      if (prefs.getBool("valid", false)) {
        _config.xCalM = prefs.getFloat("xCalM", 0.0f);
        _config.yCalM = prefs.getFloat("yCalM", 0.0f);
        _config.xCalC = prefs.getFloat("xCalC", 0.0f);
        _config.yCalC = prefs.getFloat("yCalC", 0.0f);
        _config.valid = true;
        loaded = true;
      }
      prefs.end();
    }
  }

  return loaded && _config.valid;
}

void TouchCalibrator::save() {
  // 1. Save to LittleFS
  if (LittleFS.begin(true)) {
    fs::File f = LittleFS.open(TOUCH_PATH, "w");
    if (f) {
      JsonDocument doc;
      doc["xCalM"] = _config.xCalM;
      doc["yCalM"] = _config.yCalM;
      doc["xCalC"] = _config.xCalC;
      doc["yCalC"] = _config.yCalC;
      serializeJson(doc, f);
      f.close();
    }
  }

  // 2. Save to NVS
  Preferences prefs;
  if (prefs.begin(PREF_TOUCH_NS, false)) {
    prefs.putFloat("xCalM", _config.xCalM);
    prefs.putFloat("yCalM", _config.yCalM);
    prefs.putFloat("xCalC", _config.xCalC);
    prefs.putFloat("yCalC", _config.yCalC);
    prefs.putBool("valid", true);
    prefs.end();
  }
}

void TouchCalibrator::resetCalibration() {
  if (LittleFS.begin(true)) {
    LittleFS.remove(TOUCH_PATH);
  }
  Preferences prefs;
  if (prefs.begin(PREF_TOUCH_NS, false)) {
    prefs.clear();
    prefs.end();
  }
  _config.valid = false;
  Serial.println("[Touch] Calibration reset.");
}
