# REST API & Web Endpoints

CYD-HA-Dashboard runs an integrated, responsive web server on port `80`. It serves both the Web Configuration Portal and a JSON REST API for automation, diagnostics, and OTA updates.

Base URL: `http://<device-ip>/`

---

## 1. System & Metrics API

### `GET /api/status`
Returns real-time hardware metrics, network telemetry, and Home Assistant connection state.

**Response:**
```json
{
  "ip": "192.168.1.102",
  "wifi_ssid": "Home-Network-2.4G",
  "wifi_rssi": -46,
  "free_heap": 173340,
  "min_free_heap": 163280,
  "max_alloc_heap": 110580,
  "chip_model": "ESP32-D0WD-V3",
  "cpu_freq_mhz": 240,
  "uptime_s": 842,
  "version": "0.1.0",
  "app_name": "CYD-HA-Dashboard",
  "ha_connected": true,
  "ha_url": "http://192.168.1.130:8123"
}
```

### `POST /api/reboot`
Safely restarts the ESP32.

**Response:**
```json
{
  "success": true,
  "message": "Rebooting device..."
}
```

---

## 2. Dashboard Layout & Theme Configuration

### `GET /api/config`
Retrieves the active dashboard configuration JSON from internal LittleFS storage (`/config.json`).

**Optional Query Parameter:**
- `?download=1`: Appends `Content-Disposition: attachment; filename="cyd-dashboard-config.json"` for instant browser backup downloads.

**Response Schema:**
```json
{
  "schema_version": 1,
  "pages": [
    {
      "id": "living_room",
      "name": "Living Room",
      "grid": "3x2",
      "widgets": [
        {
          "id": "w_0",
          "slot": 0,
          "label": "Light",
          "type": "toggle",
          "icon": "mdi:lightbulb",
          "entity_id": "light.living_room_light"
        }
      ]
    }
  ],
  "color_presets": [
    "#fff1e0", "#ef4444", "#f59e0b", "#10b981", "#38bdf8", "#a855f7"
  ],
  "display": {
    "brightness": 255,
    "dim_brightness": 40,
    "dim_timeout": 30,
    "sleep_timeout": 60
  },
  "theme": "ha_dark"
}
```

### `POST /api/config`
Validates and saves a new dashboard layout, display power settings, or theme palette directly to `/config.json`, and triggers a live zero-reboot UI reload on the hardware screen.

**Headers:**
- `Content-Type: application/json`

**Response:**
```json
{
  "success": true,
  "message": "Dashboard configuration updated!"
}
```

---

## 3. Entity Discovery & Diagnostics

### `GET /api/entities`
Proxies Home Assistant's `/api/template` endpoint to dynamically discover all available entities on the local HA server with their friendly names, domains, and state.

**Response:**
```json
[
  { "id": "light.living_room", "name": "Living Room Light", "domain": "light", "state": "on" },
  { "id": "switch.desk_fan", "name": "Desk Fan", "domain": "switch", "state": "off" }
]
```

### `GET /scan`
Performs a live Wi-Fi network survey and returns nearby SSIDs with RSSI signal strengths in dBm.

---

## 4. Firmware Updates & OTA

### `POST /update`
Accepts a multipart firmware binary upload (`.bin`) and flashes it to the inactive OTA partition using ESP32 `Update.h`. Once flashed and verified, the device automatically reboots into the new firmware.

**Response:**
- `200 OK` on success
- `500 Internal Server Error` on flash or CRC failure

### `POST /save`
Saves Wi-Fi credentials (`wifi_ssid`, `wifi_password`) and Home Assistant server configuration (`ha_url`, `ha_token`, `ha_entity`) into non-volatile storage (NVS) under the `cyd_cfg` namespace.
- If `wifi_password` or `ha_token` are left blank, existing stored secrets are preserved.
