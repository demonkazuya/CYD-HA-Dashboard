# Setup & Configuration Guide

This guide walks you through setting up **CYD-HA-Dashboard** from first power-on to creating your custom multi-page dashboard.

---

## 1. Prerequisites

Before starting, ensure you have:
1. An **ESP32-2432S028R** (2.8" Cheap Yellow Display).
2. A 2.4 GHz Wi-Fi network (ESP32 does not support 5 GHz Wi-Fi).
3. A running **Home Assistant** instance reachable over your local network.

---

## 2. Generating a Home Assistant Long-Lived Access Token

CYD-HA-Dashboard communicates directly with Home Assistant using its secure REST API:

1. Open your Home Assistant web interface.
2. Click your **User Profile** icon (bottom-left corner).
3. Switch to the **Security** tab.
4. Scroll to the bottom to **Long-Lived Access Tokens**.
5. Click **Create Token**, name it `CYD-Dashboard`, and click **OK**.
6. Copy the generated token string immediately (Home Assistant only displays it once).

---

## 3. First-Time Wi-Fi & Credential Setup

1. Power the CYD board using a USB cable (5V / 1A minimum).
2. On initial startup, the device launches a Captive Portal:
   - **Access Point SSID:** `CYD-Dashboard-Setup`
   - **Default IP:** `192.168.4.1`
3. Connect your smartphone or computer to the `CYD-Dashboard-Setup` Wi-Fi network.
4. The configuration page opens automatically (if not, open your browser to `http://192.168.4.1`).
5. Fill in the network and Home Assistant fields:
   - **Wi-Fi SSID:** Select from the scanned dropdown or enter manually.
   - **Wi-Fi Password:** Your Wi-Fi network password.
   - **Home Assistant Base URL:** e.g., `http://192.168.1.100:8123` (include `http://` or `https://` and port `:8123`).
   - **Long-Lived Access Token:** Paste the token generated in Step 2.
   - **Default Entity ID:** Optional fallback entity (e.g. `switch.living_room_light`).
6. Click **Save & Reconnect**.
7. The CYD will reboot, connect to your Wi-Fi, and display its assigned local IP address on the screen.

---

## 4. Designing Your Dashboard in the Web Portal

Once connected to your home network, open any browser to:
```
http://<cyd-ip>/
```

### Tab 1: Dashboard Editor
- **Add Pages:** Click `+ Add Page` to create rooms or categories (e.g., *Living Room*, *Climate*, *Lighting*).
- **Grid Layout:** Choose between **2x2** (4 large tiles, great for switches and sensors) or **3x2** (6 tiles, optimal for dense button layouts).
- **Assign Slots:** Click any slot on the grid:
  - **Label:** Friendly name displayed on the tile.
  - **Widget Type:**
    - `toggle`: Switches, lights, fans, covers, scenes, scripts, thermostats.
    - `sensor`: Temperature, humidity, power, battery levels (shows unit and large value readout).
    - `binary_sensor`: Door contacts, motion sensors, smoke detectors (green idle, alert red when active).
  - **Entity ID:** Search live Home Assistant entities or type manually.
  - **Icon:** Material Design Icon (e.g. `mdi:lightbulb`, `mdi:fan`, `mdi:blinds`, `mdi:thermostat`).
- Click **Save Dashboard Configuration** to immediately push changes to the screen without restarting.

### Tab 2: Wi-Fi & Home Assistant Settings
- Safely update credentials. Stored tokens and passwords remain masked and are never leaked in cleartext.

### Tab 3: System & Tools
- **Themes & Modes:** Switch between 10 live palettes (Home Assistant Dark/Light, Slate, OLED, Forest, Amber).
- **Display Power:** Customize active brightness (10–100%), auto-dimming timeout (default 30s), and screen sleep timeout (default 60s).
- **Firmware Updates:** Check GitHub releases for 1-click updates or flash local `.bin` files.
- **Config Backup:** Click `Export Config` to save a JSON backup on your computer, or `Import Config` to restore it.
