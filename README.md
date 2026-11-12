# CYD-HA-Dashboard 📱✨

### Touchscreen Smart Home Dashboard for Home Assistant (CYD 2.8" ESP32-2432S028R)

[![GitHub Release](https://img.shields.io/badge/Release-v0.1.0-blue.svg?style=for-the-badge)](https://github.com/demonkazuya/CYD-HA-Dashboard/releases)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Supported-orange.svg?style=for-the-badge)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--2432S028R-red.svg?style=for-the-badge)](docs/HARDWARE.md)
[![Ko-fi](https://img.shields.io/badge/Ko--fi-Tip%20a%20Coffee-F16061?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/demonkazuya)

**CYD-HA-Dashboard** is a standalone, open-source touchscreen smart home controller built for the sub-$15 **Cheap Yellow Display (ESP32-2432S028R)**. It connects directly to your local **Home Assistant** server to deliver instant touch controls, live sensor telemetries, advanced device modals, and customizable multi-page layouts.

Everything runs locally on internal flash memory (LittleFS)—no external bridge servers, SD cards, or complex cloud dependencies required.

---

## 🌟 Key Features

- **🚀 100% Standalone & Local:** Boots from internal 4MB flash (LittleFS). Zero SD cards or companion containers needed.
- **⚡ 0ms Optimistic Touch Feedback:** Immediate UI state toggle with a 3.5s anti-stale holdoff window to keep buttons ultra-responsive regardless of mesh latency.
- **🎛️ Advanced Control Modals:**
  - **Lights:** Brightness slider + 6 quick presets (10%, 25%, 50%, 75%, 100%, Off) + custom per-entity RGB color palettes.
  - **Fans:** 4-speed slider + presets (Off, Low 33%, Med 66%, High 100%).
  - **Covers & Blinds:** Open/Stop/Close controls + position slider + 4 quick position presets (0%, 25%, 50%, 100%).
  - **Climate & Thermostats:** Live current and target temperature readouts, `[-]`/`[+]` half-degree steppers, quick target presets (18°, 20°, 22°, 24°), and HVAC mode toggling (`HEAT`, `COOL`, `AUTO`, `OFF`).
  - **Scenes & Scripts:** Instant tap feedback with cyan active badge and indigo pulse hold.
  - **Sensors:** Live values with unit of measurement, large font readouts, and alert indicators.
- **🎨 10 Color Schemes & Light/Dark Modes:**
  - **Home Assistant Official:** Dark (Default) & Light
  - **Slate:** Dark & Light
  - **Midnight OLED:** True Black & Light
  - **Forest:** Pine Dark & Sage Light
  - **Amber Cyberpunk:** Cyberpunk Dark & Sand Light
- **📱 Dynamic Grid Layouts:** Multi-page dashboard supporting both spacious **2x2** (4 large tiles) and compact **3x2** (6 tiles) grid layouts per page with horizontal swipe gestures.
- **🌐 Responsive Web Configuration Portal (`http://<device-ip>/`):**
  - Live visual slot editor with material design icon (MDI) picker.
  - Interactive Home Assistant entity search and discovery.
  - One-click configuration export & import (backup/restore).
- **💡 Smart Display Power Management:**
  - Configurable active brightness (10–100%).
  - Auto-dim to 15% after 30s idle.
  - Screen sleep (backlight completely off) after 60s idle.
  - **Safe touch wake:** First tap on a sleeping or dimmed screen safely restores backlight without triggering button actions.
- **🛡️ Security & Reliability:**
  - Secrets protection: HA tokens and Wi-Fi passwords are kept in non-volatile storage (NVS) and masked in web portal exports.
  - Hardware Task Watchdog (`esp_task_wdt`): Dual-core watchdog recovers automatically from network lockups.
  - Exponential backoff (2s → 4s → 8s → 16s → 30s) during Home Assistant or Wi-Fi outages.
- **🔄 GitHub OTA Updates & Web Flasher:**
  - Check for latest GitHub releases right from the Web Portal.
  - 1-click over-the-air firmware update directly from GitHub releases or manual `.bin` drag-and-drop.
  - Ready-to-flash factory binary (`cyd-ha-dashboard.factory.bin`) at offset `0x0`.

---

## 🛠️ Hardware Requirements

| Component | Description |
|---|---|
| **Board** | **ESP32-2432S028R** (2.8" Resistive Touch CYD, 320x240 ILI9341, XPT2046 touch, 4MB Flash) |
| **Power** | 5V via Micro-USB, USB-C, or 2-pin JST-XH power header (~180 mA active, ~55 mA sleep) |
| **Case** | Any standard 2.8" CYD 3D-printed enclosure or desktop stand |

*See [docs/HARDWARE.md](docs/HARDWARE.md) for full schematic pinout tables and bus isolation details.*

---

## 🚀 Installation & Flashing

### Option 1: Web Installer (Recommended)
You can flash CYD-HA-Dashboard directly from Chrome, Edge, or Opera without installing any tools:

1. Connect your CYD board to your computer via USB.
2. Open an ESP Web Tools flasher (e.g. [web.esphome.io](https://web.esphome.io/) or [ESP Web Flasher](https://espressif.github.io/esptool-js/)).
3. Select `dist/cyd-ha-dashboard.factory.bin` and flash at offset `0x0`.

### Option 2: PlatformIO (CLI / VSCode)
Clone the repository and flash using PlatformIO:

```bash
git clone https://github.com/demonkazuya/CYD-HA-Dashboard.git
cd CYD-HA-Dashboard

# Build firmware and generate release assets
pio run

# Flash over serial USB
pio run -t upload --upload-port /dev/ttyUSB0
```

---

## 📶 First-Time Setup & Captive Portal

1. On first boot (or if saved Wi-Fi is unreachable), the CYD creates an access point:
   - **SSID:** `CYD-Dashboard-Setup`
   - **IP:** `192.168.4.1`
2. Connect with your phone or laptop. The setup portal will open automatically (or visit `http://192.168.4.1/`).
3. Select your local 2.4 GHz Wi-Fi network and enter:
   - **Wi-Fi Password**
   - **Home Assistant Base URL** (e.g., `http://192.168.1.100:8123`)
   - **Long-Lived Access Token** (generated under your HA User Profile → Security)
4. Click **Save & Reconnect**. The CYD will reboot, connect to your network, and display its new local IP address on screen.

---

## 🧭 Web Configuration Portal

Once connected, visit `http://<device-ip>/` in any web browser on your network to customize your dashboard:

- **Tab 1: Dashboard Editor:**
  - Add, delete, and reorder pages.
  - Choose between **2x2** (4 large tiles) or **3x2** (6 tiles) grid layout per page.
  - Click any tile to assign entities, friendly labels, and MDI icons with live entity autocomplete.
- **Tab 2: Wi-Fi & Home Assistant Settings:**
  - Safely update Wi-Fi or HA server settings. Stored passwords and tokens are never leaked in cleartext.
- **Tab 3: System & Tools:**
  - **Theme Selection:** Pick from 10 live-rendered palettes.
  - **Display Power:** Adjust backlight brightness, dimming timeout, and screen sleep timers.
  - **Firmware Updates:** Check GitHub for new releases and flash OTAs with 1 click.
  - **Backup & Restore:** Export or import your dashboard configuration as JSON.
  - **Touchscreen Recalibration:** Launch 2-point touch calibration with saved NVS offsets.

---

## ☕ Support the Project

If having a dedicated smart home touch deck on your desk puts a smile on your face, thank you for supporting the project!

Tips directly fund more ESP32 dev boards, test displays, and late-night firmware hacking:

[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-Tip%20a%20Coffee-F16061?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/demonkazuya)

---

## 📄 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
