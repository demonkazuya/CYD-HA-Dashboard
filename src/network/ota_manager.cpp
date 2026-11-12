#include "ota_manager.h"
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

bool OtaManager::_updating = false;

void OtaManager::begin(const char* hostname) {
  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.onStart([]() {
    _updating = true;
    esp_task_wdt_reset();
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.printf("[OTA] Start updating %s\n", type.c_str());
  });

  ArduinoOTA.onEnd([]() {
    _updating = false;
    esp_task_wdt_reset();
    Serial.println("\n[OTA] Update complete. Rebooting...");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    esp_task_wdt_reset();
    static unsigned int last_pct = 0;
    unsigned int pct = (progress / (total / 100));
    if (pct != last_pct && pct % 10 == 0) {
      last_pct = pct;
      Serial.printf("[OTA] Progress: %u%%\n", pct);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    _updating = false;
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.printf("[OTA] ArduinoOTA listening on hostname: %s\n", hostname);
}

void OtaManager::loop() {
  ArduinoOTA.handle();
}

bool OtaManager::isUpdating() {
  return _updating;
}
