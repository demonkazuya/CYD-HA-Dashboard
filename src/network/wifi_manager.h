#pragma once
#include <Arduino.h>
#include <functional>

class WifiManager {
public:
  using StatusCallback = std::function<void(bool connected, bool isAp, const String& info)>;
  using ConfigReloadCallback = std::function<void()>;
  using HaConnectedCallback = std::function<bool()>;

  WifiManager();
  void begin();
  void loop();

  bool isConnected() const;
  bool isApMode() const;
  String getIpAddress() const;
  String getSsid() const;
  String getApSsid() const;
  String getHaUrl() const;
  String getHaToken() const;
  String getHaEntity() const;

  void setStatusCallback(StatusCallback cb);
  void setOnConfigReloadCallback(ConfigReloadCallback cb);
  void setHaConnectedCallback(HaConnectedCallback cb);
  void resetCredentials();

private:
  void startAp();
  bool connectSta(const String& ssid, const String& pass, uint32_t timeout_ms = 15000);
  void setupCaptivePortal();
  void notify(bool connected, bool isAp, const String& info);

  bool _isAp = false;
  bool _connected = false;
  String _ssid;
  String _apSsid;
  String _haUrl;
  String _haToken;
  String _haEntity;
  unsigned long _lastReconnectAttempt = 0;
  StatusCallback _callback = nullptr;
  ConfigReloadCallback _configReloadCb = nullptr;
  HaConnectedCallback _haConnectedCb = nullptr;
};
