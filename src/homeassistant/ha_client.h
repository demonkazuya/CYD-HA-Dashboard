#pragma once
#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct HaEntity {
  String id;
  String friendlyName;
  String state = "off";
  String unitOfMeasurement;
  bool available = false;
  bool supportsBrightness = false;
  bool supportsRgb = false;
  bool supportsSpeed = false;
  bool supportsPosition = false;
  bool supportsClimate = false;
  int brightness = 255;
  int brightnessPct = 100;
  uint8_t rgb[3] = {255, 200, 100};
  int speedPct = 0;
  int currentPosition = 0; // 0 = closed, 100 = open
  float targetTemp = 21.0f;
  float currentTemp = 21.0f;
  float minTemp = 16.0f;
  float maxTemp = 30.0f;
  float tempStep = 0.5f;
  String hvacAction = "idle";
};

class HaClient {
public:
  using StateCallback = std::function<void(const HaEntity& entity)>;
  using StatusCallback = std::function<void(bool connected, const String& info)>;

  HaClient();
  void begin(const String& haUrl, const String& haToken, const String& defaultEntity = "");
  void setMonitoredEntities(const std::vector<String>& entityIds);
  void setStateCallback(StateCallback cb);
  void setStatusCallback(StatusCallback cb);

  bool toggle(const String& entityId);
  bool toggleWithTarget(const String& targetState, const String& entityId);
  bool setLightBrightness(const String& entityId, int brightnessPct);
  bool setLightRgb(const String& entityId, uint8_t r, uint8_t g, uint8_t b);
  bool setFanSpeed(const String& entityId, int speedPct);
  bool openCover(const String& entityId);
  bool closeCover(const String& entityId);
  bool stopCover(const String& entityId);
  bool setCoverPosition(const String& entityId, int positionPct);
  bool setHvacMode(const String& entityId, const String& mode);
  bool setClimateTargetTemp(const String& entityId, float temp);
  void triggerPoll();

  bool isConnected() const;
  HaEntity getEntity(const String& entityId);
  void notifyAction();

private:
  static void backgroundTask(void* param);
  bool checkApi();
  bool fetchState(const String& entityId);
  bool callService(const String& domain, const String& service, const String& entityId);
  bool callServiceWithJson(const String& domain, const String& service, const String& jsonPayload);

  String _url;
  String _token;
  bool _connected = false;

  std::vector<String> _monitoredEntities;
  std::map<String, HaEntity> _entities;

  struct Holdoff {
    unsigned long deadline = 0;
    String targetState;
  };
  std::map<String, Holdoff> _holdoffs;

  volatile bool _toggleRequested = false;
  String _toggleEntityId;
  String _toggleTargetState;

  StateCallback _stateCallback = nullptr;
  StatusCallback _statusCallback = nullptr;
  TaskHandle_t _taskHandle = nullptr;
  SemaphoreHandle_t _mutex = nullptr;
};
