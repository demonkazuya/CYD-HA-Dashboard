# Troubleshooting Guide

Solutions to common issues with CYD-HA-Dashboard.

---

## 1. Hardware & Display Issues

### White Screen or Blank Screen
- **Cause:** Power brownout or incomplete flash.
- **Fix:**
  - Ensure the CYD is powered by a reliable 5V power supply (at least 1A / 5W). Avoid unpowered USB hubs or keyboard passthrough ports.
  - Reflash `dist/cyd-ha-dashboard.factory.bin` starting at offset `0x0` using ESP Web Tools or PlatformIO.

### Touch Coordinates Off / Inverted Touch
- **Cause:** Resistive touch calibration required for your specific panel.
- **Fix:**
  - In the Web Portal, navigate to **Tab 3 (System & Tools) → Maintenance & Calibration**.
  - Click **Recalibrate Screen Touch**.
  - The device reboots into a 2-point calibration screen. Use a stylus or non-sharp object to tap the crosshair targets at the top-left and bottom-right corners.
  - Calibration offsets are automatically saved to internal NVS flash.

---

## 2. Connectivity & Home Assistant

### Screen Shows `HA: Disconnected` or `HTTP 401`
- **HTTP 401 Unauthorized:** Your Long-Lived Access Token is invalid or expired. Generate a new token in Home Assistant (Profile → Security) and re-enter it in Tab 2 of the Web Portal.
- **HTTP -1 or Timeout:**
  - Verify your Home Assistant Base URL includes `http://` or `https://` and the port `:8123` (e.g., `http://192.168.1.100:8123`).
  - Do not use `http://homeassistant.local:8123` if your router does not support mDNS resolution; prefer the static local IP of your Home Assistant server.
  - Ensure the CYD and Home Assistant are on the same subnet/VLAN or that firewall rules allow port 8123 traffic between them.

### Cannot Connect to Wi-Fi
- The ESP32 hardware radio only supports **2.4 GHz Wi-Fi** (802.11 b/g/n). It cannot connect to 5 GHz-only SSIDs.
- If your home network uses a combined 2.4/5 GHz SSID (band steering), temporarily separate them or assign the CYD to an IoT 2.4 GHz SSID.

---

## 3. Power Management & Sleep

### Screen Turns Off Too Quickly or Stays Awake
- Adjust the timers in the Web Portal under **Tab 3 (System & Tools) → Display & Backlight Power**:
  - `Dim Timeout`: Sets when backlight drops to 15% (default 30 seconds).
  - `Screen Sleep`: Sets when backlight turns off completely (default 60 seconds).
  - Set both to `Disabled` if you prefer an always-on display.

### Tapping Screen While Asleep
- The dashboard features **Safe Touch Wake**: the first tap on a sleeping or dimmed screen only wakes the display. It drops the touch event so buttons are never accidentally pressed in the dark. A second tap toggles widgets normally.

---

## 4. Captive Portal Recovery

If you change your Wi-Fi router or move the device to another network:
1. Power on the CYD.
2. If it cannot connect to the saved Wi-Fi network within 15 seconds, it automatically launches the `CYD-Dashboard-Setup` recovery hotspot (`192.168.4.1`).
3. Connect and configure the new Wi-Fi credentials.
