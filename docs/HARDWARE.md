# Hardware Reference: ESP32-2432S028R ("Cheap Yellow Display" / CYD)

The **CYD-HA-Dashboard** is built specifically for the popular and budget-friendly **ESP32-2432S028R** development board (commonly known as the 2.8" Cheap Yellow Display or CYD).

---

## 1. Board Overview

- **MCU:** Espressif ESP32-D0WD-V3 (Xtensa Dual-Core 32-bit LX6 @ 240 MHz, 520 KB SRAM, 4 MB SPI Flash)
- **Display:** 2.8" TFT LCD, 320x240 resolution, 65K colors, ILI9341 controller
- **Touch Screen:** Resistive 4-wire touch panel driven by an XPT2046 touch controller
- **Storage:** Internal 4 MB Flash with LittleFS file system partition for layout & configuration
- **Wireless:** 802.11 b/g/n Wi-Fi + Bluetooth v4.2 BR/EDR & BLE
- **Power Input:** 5V via Micro-USB, USB Type-C, or 2-pin JST-XH connector

---

## 2. Pinout & Bus Mapping

### TFT Display (Driven by TFT_eSPI on VSPI)

| Signal | ESP32 GPIO | Description |
|---|---|---|
| **TFT_MISO** | GPIO 12 | SPI Master In / Slave Out |
| **TFT_MOSI** | GPIO 13 | SPI Master Out / Slave In |
| **TFT_SCLK** | GPIO 14 | SPI Serial Clock |
| **TFT_CS** | GPIO 15 | Chip Select (Active Low) |
| **TFT_DC** | GPIO 2 | Data / Command Select |
| **TFT_RST** | -1 | Tied to ESP32 Hardware EN Reset |
| **TFT_BL** | GPIO 21 | Backlight LED (LEDC PWM controlled, 5 kHz) |

### XPT2046 Resistive Touch Controller (Dedicated HSPI Bus)

To eliminate SPI bus collisions between high-bandwidth display transfers and continuous touch polling, touch communication runs on a secondary isolated SPI bus (HSPI):

| Signal | ESP32 GPIO | Description |
|---|---|---|
| **TOUCH_CLK** | GPIO 25 | Dedicated HSPI Clock |
| **TOUCH_MOSI** | GPIO 32 | Dedicated HSPI MOSI |
| **TOUCH_MISO** | GPIO 39 | Dedicated HSPI MISO (Input only) |
| **TOUCH_CS** | GPIO 33 | Touch Controller Chip Select |
| **TOUCH_IRQ** | GPIO 36 | Touch Interrupt / PenIRQ (Input only) |

### On-Board Peripherals

| Peripheral | ESP32 GPIO | Notes |
|---|---|---|
| **RGB LED (Red)** | GPIO 4 | Active LOW (Inverted logic) |
| **RGB LED (Green)** | GPIO 16 | Active LOW (Inverted logic) |
| **RGB LED (Blue)** | GPIO 17 | Active LOW (Inverted logic) |
| **LDR Light Sensor** | GPIO 34 | Analog input (ADC1_CH6) |
| **Speaker / Buzzer** | GPIO 26 | Audio output / DAC2 |
| **TF Card (MicroSD)** | GPIO 5 (CS), 18 (SCK), 19 (MISO), 23 (MOSI) | Unused; all config is stored in flash |

### Extended Expansion Headers (CN1 & P3)

The CYD provides external expansion headers for extra hardware:
- **GPIO 22:** I2C SCL or standard GPIO
- **GPIO 27:** I2C SDA or standard GPIO
- **GPIO 35:** Input only (ADC1_CH7)
- **Power:** 3.3V, GND, 5V

---

## 3. Power Consumption & Thermal Notes

- **Full Brightness (100% Backlight):** ~180 mA @ 5V (~0.9 W)
- **Dimmed State (15% Backlight):** ~95 mA @ 5V (~0.47 W)
- **Screen Sleep (0% Backlight):** ~55 mA @ 5V (~0.27 W)
- The board runs cool during 24/7 continuous operation thanks to FreeRTOS tickless idle and automatic display power management.
