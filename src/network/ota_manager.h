#pragma once
#include <Arduino.h>

class OtaManager {
public:
  static void begin(const char* hostname = "cyd-ha-dashboard");
  static void loop();
  static bool isUpdating();

private:
  static bool _updating;
};
