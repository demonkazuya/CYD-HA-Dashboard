#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "config.h"
#include "network/wifi_manager.h"
#include "network/ota_manager.h"
#include "homeassistant/ha_client.h"
#include "dashboard/dashboard_config.h"
#include "touch/touch_cal.h"
#include "ui/mdi_icons.h"
#include "ui/dashboard_theme.h"
#include <esp_task_wdt.h>

// Hardware configuration matching BambuDeck-HA
#define LCD_BACK_LIGHT_PIN 21
#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_12_BIT 12
#define LEDC_BASE_FREQ 5000

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33
#define TFT_CS_PIN   15

static TFT_eSPI display = TFT_eSPI(config::SCREEN_HEIGHT, config::SCREEN_WIDTH);
static SPIClass touchSpi = SPIClass(HSPI);
static XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[config::SCREEN_WIDTH * config::DRAW_BUF_LINES];
static lv_color_t buf2[config::SCREEN_WIDTH * config::DRAW_BUF_LINES];

static WifiManager wifiManager;
static HaClient haClient;
static DashboardConfig dashboardConfig;
static TouchCalibrator touchCal;

static bool otaStarted = false;
static bool haStarted = false;

// Header & Screen Widgets
static lv_obj_t *ui_screen = nullptr;
static lv_obj_t *ui_header = nullptr;
static lv_obj_t *label_page_title = nullptr;
static lv_obj_t *label_header_ha = nullptr;
static lv_obj_t *btn_header_ha = nullptr;
static lv_obj_t *modal_overlay = nullptr;

// Dynamic Grid Slots for Active Page (Up to 3x2 = 6 slots)
struct SlotWidget {
  lv_obj_t *container = nullptr;
  lv_obj_t *label_name = nullptr;
  lv_obj_t *label_icon = nullptr;
  lv_obj_t *label_state = nullptr;
  lv_obj_t *label_type = nullptr;
  int slotIndex = 0;
};
static const int MAX_SLOTS = 6;
static SlotWidget slots[MAX_SLOTS];
static unsigned long actionTriggerUntil[MAX_SLOTS] = { 0 };

// Thread-safe update flags
static volatile bool uiDirty = true;
static String queuedHaStatus = "HA: Init...";
static bool queuedHaOk = false;

static inline void setBrightness(uint8_t brightness) {
  uint32_t duty = (4095 / 255) * min((int)brightness, 255);
  ledcWrite(LEDC_CHANNEL_0, duty);
}

static inline void setBackLedOff() {
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  digitalWrite(4, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);
}

static void flushDisplay(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  display.startWrite();
  display.setAddrWindow(area->x1, area->y1, w, h);
  display.pushColors((uint16_t *)&color_p->full, w * h, true);
  display.endWrite();

  lv_disp_flush_ready(disp);
}

enum class DisplayPowerState {
  ACTIVE,
  DIMMED,
  SLEEP
};
static DisplayPowerState displayPowerState = DisplayPowerState::ACTIVE;
static unsigned long lastTouchActivity = 0;

static void readTouch(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (touch.touched()) {
    TS_Point point = touch.getPoint();
    int16_t x = 0, y = 0;
    touchCal.mapCoords(point.x, point.y, x, y);

    // If screen was dimmed or asleep, wake on tap & drop tap to prevent accidental actuation
    if (displayPowerState != DisplayPowerState::ACTIVE) {
      displayPowerState = DisplayPowerState::ACTIVE;
      lastTouchActivity = millis();
      setBrightness(dashboardConfig.activeBrightness);
      data->state = LV_INDEV_STATE_REL;
      Serial.println("[Power] Display woke from idle (tap consumed safely)");
      return;
    }

    lastTouchActivity = millis();
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// Find widget on active page assigned to a specific slot
static const Widget* getWidgetForSlot(int slotIdx) {
  const DashboardPage& page = dashboardConfig.getActivePage();
  for (const auto& w : page.widgets) {
    if (w.slot == slotIdx) return &w;
  }
  return nullptr;
}

static void setContainerColor(lv_obj_t* obj, lv_color_t color) {
  if (!obj) return;
  if (lv_obj_get_style_bg_color(obj, LV_PART_MAIN).full != color.full) {
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_FOCUSED);
  }
}

// Safe label update helper (avoids resetting running marquee / redraws if text didn't change)
static inline bool safe_set_label_text(lv_obj_t* label, const char* new_text) {
  if (!label || !new_text) return false;
  const char* cur = lv_label_get_text(label);
  if (!cur || strcmp(cur, new_text) != 0) {
    lv_label_set_text(label, new_text);
    return true;
  }
  return false;
}

// Configures smooth ping-pong scrolling with 3.5s pauses at start and end
static void set_label_marquee(lv_obj_t* label, bool enable, bool forceReset = false) {
  if (!label) return;
  if (enable) {
    bool modeChanged = (lv_label_get_long_mode(label) != LV_LABEL_LONG_SCROLL);
    if (modeChanged) {
      lv_obj_set_style_anim_speed(label, 25, LV_PART_MAIN); // 25 px/s (smooth 40-50 fps glide, no micro-stutter)
      lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    }
    // Only configure/reset anim delays when entering scroll mode or when text string actually changed
    if (modeChanged || forceReset) {
      lv_anim_t* a = lv_anim_get(label, NULL);
      if (a) {
        a->playback_delay = 3500; // Pause 3.5s when reaching the right end
        a->repeat_delay = 3500;   // Pause 3.5s when back at the start
        a->act_time = -3500;      // 3.5s pause before initial scroll
      }
    }
  } else {
    if (lv_label_get_long_mode(label) != LV_LABEL_LONG_DOT) {
      lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    }
  }
}
static void apply_theme() {
  const DashboardTheme& th = getThemeById(dashboardConfig.theme);
  if (ui_screen) {
    lv_obj_set_style_bg_color(ui_screen, lv_color_hex(th.screenBg), LV_PART_MAIN);
  }
  if (ui_header) {
    lv_obj_set_style_bg_color(ui_header, lv_color_hex(th.headerBg), LV_PART_MAIN);
  }
  if (label_page_title) {
    lv_obj_set_style_text_color(label_page_title, lv_color_hex(th.headerText), LV_PART_MAIN);
  }
  for (int i = 0; i < MAX_SLOTS; i++) {
    if (slots[i].container) {
      lv_obj_set_style_border_color(slots[i].container, lv_color_hex(th.tileBorder), LV_PART_MAIN);
    }
  }
}

// Update visual display of a single slot
static void update_slot_ui(int slotIdx) {
  if (slotIdx < 0 || slotIdx >= MAX_SLOTS) return;
  SlotWidget& sw = slots[slotIdx];
  const Widget* w = getWidgetForSlot(slotIdx);

  if (w == nullptr) {
    lv_obj_add_flag(sw.container, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  const DashboardTheme& th = getThemeById(dashboardConfig.theme);
  const DashboardPage& page = dashboardConfig.getActivePage();
  bool is3x2 = (page.grid == "3x2");
  int tileW = is3x2 ? 98 : 148;

  lv_obj_clear_flag(sw.container, LV_OBJ_FLAG_HIDDEN);

  // Dynamic font sizing & marquee for Friendly Name
  int labelLen = w->label.length();
  int labelMaxNormal = is3x2 ? 10 : 16;
  int labelMaxSmall  = is3x2 ? 14 : 22;

  const lv_font_t* nameFont = (labelLen > labelMaxNormal) ? &lv_font_montserrat_12 : &lv_font_montserrat_14;
  if (lv_obj_get_style_text_font(sw.label_name, LV_PART_MAIN) != nameFont) {
    lv_obj_set_style_text_font(sw.label_name, nameFont, LV_PART_MAIN);
  }
  bool nameChanged = safe_set_label_text(sw.label_name, w->label.c_str());
  set_label_marquee(sw.label_name, labelLen > labelMaxSmall, nameChanged);

  if (lv_obj_get_width(sw.label_name) != (tileW - 8)) {
    lv_obj_set_width(sw.label_name, tileW - 8);
    lv_obj_align(sw.label_name, LV_ALIGN_BOTTOM_MID, 0, -6);
  }

  if (sw.label_icon) {
    const char* sym = getRealMdiSymbol(w->icon, w->entityId, w->type);
    safe_set_label_text(sw.label_icon, sym);
  }

  HaEntity ent = haClient.getEntity(w->entityId);

  switch (w->type) {
    case WidgetType::TOGGLE: {
      bool isAction = (w->entityId.startsWith("scene.") || w->entityId.startsWith("script."));
      bool isCover = (w->entityId.startsWith("cover.") || w->entityId == "cover.cyd_demo_blinds");
      bool isClimate = (w->entityId.startsWith("climate.") || w->entityId == "climate.cyd_demo_thermostat");
      bool isTriggered = isAction && (millis() < actionTriggerUntil[slotIdx]);
      bool isOn = (ent.state == "on" || ent.state == "scening" || ent.state == "open" || ent.state == "opening" || ent.state == "heat" || ent.state == "cool" || ent.state == "auto");
      lv_obj_set_style_text_font(sw.label_icon, &ui_font_mdi_44, LV_PART_MAIN);
      lv_obj_align(sw.label_icon, LV_ALIGN_CENTER, 0, -5);
      safe_set_label_text(sw.label_state, "");
      set_label_marquee(sw.label_state, false);

      if (isClimate) {
        char cBuf[32];
        String st = ent.state;
        st.toUpperCase();
        if (ent.targetTemp > 0) {
          snprintf(cBuf, sizeof(cBuf), "%s %.1f°", st.c_str(), ent.targetTemp);
        } else {
          snprintf(cBuf, sizeof(cBuf), "%s", st.c_str());
        }
        safe_set_label_text(sw.label_type, cBuf);
        lv_color_t typeCol = (ent.state == "heat") ? lv_color_hex(0xF87171) :
                             ((ent.state == "cool") ? lv_color_hex(0x38BDF8) :
                             ((ent.state == "auto") ? lv_color_hex(0x34D399) : lv_color_hex(th.tileIconIdle)));
        lv_obj_set_style_text_color(sw.label_type, typeCol, LV_PART_MAIN);
      } else {
        safe_set_label_text(sw.label_type, isTriggered ? "ACTIVE" : (isAction ? (w->entityId.startsWith("scene.") ? "SCENE" : "SCRIPT") : (isCover ? "COVER" : "")));
        lv_obj_set_style_text_color(sw.label_type, isTriggered ? lv_color_hex(0x67E8F9) : (isCover ? lv_color_hex(0x38BDF8) : lv_color_hex(0xA5B4FC)), LV_PART_MAIN);
      }

      if (ent.available) {
        if (isAction) {
          if (isTriggered) {
            setContainerColor(sw.container, lv_color_hex(0x4338CA)); // Indigo pulse
            lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_set_style_text_color(sw.label_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
          } else {
            setContainerColor(sw.container, lv_color_hex(th.tileIdleBg));
            lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0x818CF8), LV_PART_MAIN); // Indigo idle
            lv_obj_set_style_text_color(sw.label_name, lv_color_hex(th.tileText), LV_PART_MAIN);
          }
        } else if (isClimate) {
          if (ent.state == "heat") {
            setContainerColor(sw.container, lv_color_hex(0x991B1B)); // Red-800 heat
          } else if (ent.state == "cool") {
            setContainerColor(sw.container, lv_color_hex(0x0369A1)); // Sky-700 cool
          } else if (ent.state == "auto") {
            setContainerColor(sw.container, lv_color_hex(0x065F46)); // Emerald-800 auto
          } else {
            setContainerColor(sw.container, lv_color_hex(th.tileIdleBg)); // Inactive/off
          }
          lv_obj_set_style_text_color(sw.label_icon, lv_color_hex((ent.state == "off") ? th.tileIconIdle : 0xFFFFFF), LV_PART_MAIN);
          lv_obj_set_style_text_color(sw.label_name, lv_color_hex((ent.state == "off") ? th.tileText : 0xFFFFFF), LV_PART_MAIN);
        } else if (isOn) {
          bool isLight = (w->entityId.startsWith("light.") || w->icon.indexOf("light") >= 0 || w->icon.indexOf("bulb") >= 0);
          lv_color_t bg = isCover ? lv_color_hex(0x0284C7) : (isLight ? lv_color_hex(0xD97706) : lv_color_hex(th.switchOnBg));
          if (isLight && ent.supportsRgb) {
            bg = lv_color_make(ent.rgb[0], ent.rgb[1], ent.rgb[2]);
          }
          setContainerColor(sw.container, bg);
          lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
          lv_obj_set_style_text_color(sw.label_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        } else {
          setContainerColor(sw.container, lv_color_hex(th.tileIdleBg));
          lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(th.tileIconIdle), LV_PART_MAIN);
          lv_obj_set_style_text_color(sw.label_name, lv_color_hex(th.tileText), LV_PART_MAIN);
        }
      } else {
        setContainerColor(sw.container, lv_color_hex(th.tileIdleBg));
        lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0xEF4444), LV_PART_MAIN);
        lv_obj_set_style_text_color(sw.label_name, lv_color_hex(th.tileText), LV_PART_MAIN);
      }
      break;
    }

    case WidgetType::SENSOR: {
      setContainerColor(sw.container, lv_color_hex(th.tileIdleBg));
      lv_obj_set_style_text_font(sw.label_icon, &ui_font_mdi_24, LV_PART_MAIN);
      lv_obj_align(sw.label_icon, LV_ALIGN_TOP_MID, 0, 6);
      lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0x38BDF8), LV_PART_MAIN);

      char valBuf[48];
      if (ent.unitOfMeasurement.length() > 0) {
        snprintf(valBuf, sizeof(valBuf), "%s %s", ent.state.c_str(), ent.unitOfMeasurement.c_str());
      } else {
        snprintf(valBuf, sizeof(valBuf), "%s", ent.state.c_str());
      }

      // Dynamic font sizing for sensor value based on string length & grid density
      int valLen = strlen(valBuf);
      const lv_font_t* valFont = &lv_font_montserrat_26;
      if (is3x2) {
        if (valLen > 11) {
          valFont = &lv_font_montserrat_12;
        } else if (valLen > 8) {
          valFont = &lv_font_montserrat_14;
        } else if (valLen > 5) {
          valFont = &lv_font_montserrat_20;
        } else {
          valFont = &lv_font_montserrat_26;
        }
      } else {
        // 2x2 grid (wider tile 148px)
        if (valLen > 14) {
          valFont = &lv_font_montserrat_14;
        } else if (valLen > 9) {
          valFont = &lv_font_montserrat_20;
        } else {
          valFont = &lv_font_montserrat_26;
        }
      }

      if (lv_obj_get_style_text_font(sw.label_state, LV_PART_MAIN) != valFont) {
        lv_obj_set_style_text_font(sw.label_state, valFont, LV_PART_MAIN);
      }
      if (lv_obj_get_width(sw.label_state) != (tileW - 8)) {
        lv_obj_set_width(sw.label_state, tileW - 8);
        lv_obj_align(sw.label_state, LV_ALIGN_CENTER, 0, 8);
      }
      lv_obj_set_style_text_align(sw.label_state, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

      bool valChanged = safe_set_label_text(sw.label_state, valBuf);
      set_label_marquee(sw.label_state, valLen > (is3x2 ? 14 : 18), valChanged);

      lv_obj_set_style_text_color(sw.label_state, lv_color_hex(th.tileText), LV_PART_MAIN);
      safe_set_label_text(sw.label_type, "");
      lv_obj_set_style_text_color(sw.label_name, lv_color_hex(th.tileText), LV_PART_MAIN);
      break;
    }

    case WidgetType::BINARY_SENSOR: {
      bool active = (ent.state == "on" || ent.state == "open" || ent.state == "detected" || ent.state == "problem");
      lv_obj_set_style_text_font(sw.label_icon, &ui_font_mdi_44, LV_PART_MAIN);
      lv_obj_align(sw.label_icon, LV_ALIGN_CENTER, 0, -5);
      safe_set_label_text(sw.label_state, "");
      set_label_marquee(sw.label_state, false);
      safe_set_label_text(sw.label_type, "");

      if (active) {
        setContainerColor(sw.container, lv_color_hex(0xDC2626)); // Alert red
        lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_color(sw.label_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
      } else {
        setContainerColor(sw.container, lv_color_hex(th.tileIdleBg));
        lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0x10B981), LV_PART_MAIN); // Clear green
        lv_obj_set_style_text_color(sw.label_name, lv_color_hex(th.tileText), LV_PART_MAIN);
      }
      break;
    }
  }
}

// Refresh full active page
static void refresh_active_page_ui() {
  const DashboardPage& page = dashboardConfig.getActivePage();
  if (label_page_title) {
    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "%s (%u/%u)",
             page.name.c_str(),
             (unsigned int)(dashboardConfig.getActivePageIndex() + 1),
             (unsigned int)dashboardConfig.getPageCount());
    lv_label_set_text(label_page_title, titleBuf);
  }

  bool is3x2 = (page.grid == "3x2");
  int activeCount = is3x2 ? 6 : 4;
  int tileW = is3x2 ? 98 : 148;
  int tileH = 95;

  int posX_2[2] = { 8, 164 };
  int posX_3[3] = { 6, 110, 214 };
  int posY[2] = { 38, 141 };

  for (int i = 0; i < MAX_SLOTS; i++) {
    SlotWidget& sw = slots[i];
    if (i < activeCount) {
      int col = is3x2 ? (i % 3) : (i % 2);
      int row = is3x2 ? (i / 3) : (i / 2);
      int px = is3x2 ? posX_3[col] : posX_2[col];
      int py = posY[row];

      lv_obj_set_pos(sw.container, px, py);
      lv_obj_set_size(sw.container, tileW, tileH);
      lv_obj_set_width(sw.label_name, tileW - 8);
      update_slot_ui(i);
    } else {
      lv_obj_add_flag(sw.container, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // Ensure HA monitors all entities on this dashboard
  haClient.setMonitoredEntities(dashboardConfig.getAllEntityIds());
}

static unsigned long lastGestureTime = 0;

// Screen Swipe Gesture Callback
static void screen_gesture_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_LEFT) {
      lastGestureTime = millis();
      Serial.println("[UI] Swipe Left -> Next Page");
      dashboardConfig.nextPage();
      refresh_active_page_ui();
    } else if (dir == LV_DIR_RIGHT) {
      lastGestureTime = millis();
      Serial.println("[UI] Swipe Right -> Prev Page");
      dashboardConfig.prevPage();
      refresh_active_page_ui();
    }
  }
}

// Advanced Controls Modal State
static lv_obj_t *control_overlay = nullptr;
static String activeControlEntityId = "";
static const Widget* activeControlWidget = nullptr;
static lv_obj_t *lbl_ctrl_brightness = nullptr;
static lv_obj_t *slider_ctrl_brightness = nullptr;
static lv_obj_t *lbl_ctrl_speed = nullptr;
static lv_obj_t *slider_ctrl_speed = nullptr;
static lv_obj_t *lbl_ctrl_cover = nullptr;
static lv_obj_t *slider_ctrl_cover = nullptr;
static lv_obj_t *lbl_ctrl_climate_target = nullptr;
static lv_obj_t *lbl_ctrl_climate_current = nullptr;
static lv_obj_t *btn_climate_modes[4] = { nullptr };

static void close_control_modal_cb(lv_event_t *e) {
  if (control_overlay) {
    lv_obj_del(control_overlay);
    control_overlay = nullptr;
    slider_ctrl_brightness = nullptr;
    lbl_ctrl_brightness = nullptr;
    slider_ctrl_speed = nullptr;
    lbl_ctrl_speed = nullptr;
    slider_ctrl_cover = nullptr;
    lbl_ctrl_cover = nullptr;
    lbl_ctrl_climate_target = nullptr;
    lbl_ctrl_climate_current = nullptr;
    for (int i = 0; i < 4; i++) btn_climate_modes[i] = nullptr;
    activeControlEntityId = "";
    activeControlWidget = nullptr;
  }
}

static void light_slider_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *slider = lv_event_get_target(e);
  int val = lv_slider_get_value(slider);
  if (lbl_ctrl_brightness) {
    char bBuf[32];
    snprintf(bBuf, sizeof(bBuf), "Brightness: %d%%", val);
    lv_label_set_text(lbl_ctrl_brightness, bBuf);
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_VALUE_CHANGED) {
    static unsigned long lastSend = 0;
    if (code == LV_EVENT_RELEASED || millis() - lastSend > 250) {
      lastSend = millis();
      haClient.setLightBrightness(activeControlEntityId, val);
      refresh_active_page_ui();
    }
  }
}

static void light_preset_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int pct = (int)(intptr_t)lv_event_get_user_data(e);
    if (slider_ctrl_brightness) lv_slider_set_value(slider_ctrl_brightness, pct, LV_ANIM_ON);
    if (lbl_ctrl_brightness) {
      char bBuf[32];
      snprintf(bBuf, sizeof(bBuf), "Brightness: %d%%", pct);
      lv_label_set_text(lbl_ctrl_brightness, bBuf);
    }
    haClient.setLightBrightness(activeControlEntityId, pct);
    refresh_active_page_ui();
  }
}

static void light_color_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    std::vector<ColorRgb> presets = dashboardConfig.getColorPresetsForWidget(activeControlWidget);
    if (idx >= 0 && idx < (int)presets.size()) {
      const ColorRgb& c = presets[idx];
      haClient.setLightRgb(activeControlEntityId, c.r, c.g, c.b);
      refresh_active_page_ui();
    }
  }
}

static void light_modal_power_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    HaEntity ent = haClient.getEntity(activeControlEntityId);
    String next = (ent.state == "on") ? "off" : "on";
    haClient.toggleWithTarget(next, activeControlEntityId);
    refresh_active_page_ui();
    close_control_modal_cb(nullptr);
  }
}

static void cover_action_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int act = (int)(intptr_t)lv_event_get_user_data(e); // 1 = open, 2 = stop, 3 = close
    if (act == 1) {
      haClient.openCover(activeControlEntityId);
      if (lbl_ctrl_cover) lv_label_set_text(lbl_ctrl_cover, "Position: 100% (Open)");
      if (slider_ctrl_cover) lv_slider_set_value(slider_ctrl_cover, 100, LV_ANIM_ON);
    } else if (act == 2) {
      haClient.stopCover(activeControlEntityId);
      if (lbl_ctrl_cover) lv_label_set_text(lbl_ctrl_cover, "Position: Stopped");
    } else if (act == 3) {
      haClient.closeCover(activeControlEntityId);
      if (lbl_ctrl_cover) lv_label_set_text(lbl_ctrl_cover, "Position: 0% (Closed)");
      if (slider_ctrl_cover) lv_slider_set_value(slider_ctrl_cover, 0, LV_ANIM_ON);
    }
    refresh_active_page_ui();
  }
}

static void cover_preset_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int pct = (int)(intptr_t)lv_event_get_user_data(e);
    haClient.setCoverPosition(activeControlEntityId, pct);
    if (slider_ctrl_cover) lv_slider_set_value(slider_ctrl_cover, pct, LV_ANIM_ON);
    if (lbl_ctrl_cover) {
      char cBuf[32];
      snprintf(cBuf, sizeof(cBuf), "Position: %d%%", pct);
      lv_label_set_text(lbl_ctrl_cover, cBuf);
    }
    refresh_active_page_ui();
  }
}

static void cover_slider_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *slider = lv_event_get_target(e);
  int val = lv_slider_get_value(slider);
  if (lbl_ctrl_cover) {
    char cBuf[32];
    snprintf(cBuf, sizeof(cBuf), "Position: %d%%", val);
    lv_label_set_text(lbl_ctrl_cover, cBuf);
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_VALUE_CHANGED) {
    static unsigned long lastCoverSend = 0;
    if (code == LV_EVENT_RELEASED || millis() - lastCoverSend > 250) {
      lastCoverSend = millis();
      haClient.setCoverPosition(activeControlEntityId, val);
      refresh_active_page_ui();
    }
  }
}

static void fan_preset_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int pct = (int)(intptr_t)lv_event_get_user_data(e);
    haClient.setFanSpeed(activeControlEntityId, pct);
    if (slider_ctrl_speed) lv_slider_set_value(slider_ctrl_speed, pct, LV_ANIM_ON);
    if (lbl_ctrl_speed) {
      char sBuf[32];
      const char* mode = (pct == 0) ? "Off" : ((pct <= 33) ? "Low" : ((pct <= 66) ? "Med" : "High"));
      snprintf(sBuf, sizeof(sBuf), "Speed: %d%% (%s)", pct, mode);
      lv_label_set_text(lbl_ctrl_speed, sBuf);
    }
    refresh_active_page_ui();
  }
}

static void fan_slider_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *slider = lv_event_get_target(e);
  int val = lv_slider_get_value(slider);
  if (lbl_ctrl_speed) {
    char sBuf[32];
    const char* mode = (val == 0) ? "Off" : ((val <= 33) ? "Low" : ((val <= 66) ? "Med" : "High"));
    snprintf(sBuf, sizeof(sBuf), "Speed: %d%% (%s)", val, mode);
    lv_label_set_text(lbl_ctrl_speed, sBuf);
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_VALUE_CHANGED) {
    static unsigned long lastFanSend = 0;
    if (code == LV_EVENT_RELEASED || millis() - lastFanSend > 250) {
      lastFanSend = millis();
      haClient.setFanSpeed(activeControlEntityId, val);
      refresh_active_page_ui();
    }
  }
}

static void climate_mode_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int modeIdx = (int)(intptr_t)lv_event_get_user_data(e); // 0=heat, 1=cool, 2=auto, 3=off
    const char* modes[4] = { "heat", "cool", "auto", "off" };
    haClient.setHvacMode(activeControlEntityId, modes[modeIdx]);

    const uint32_t activeCols[4] = { 0xDC2626, 0x0284C7, 0x10B981, 0x475569 };
    for (int i = 0; i < 4; i++) {
      if (btn_climate_modes[i]) {
        bool isSel = (i == modeIdx);
        lv_obj_set_style_bg_color(btn_climate_modes[i], lv_color_hex(isSel ? activeCols[i] : 0x0F172A), LV_PART_MAIN);
        lv_obj_set_style_border_color(btn_climate_modes[i], lv_color_hex(isSel ? 0x94A3B8 : 0x334155), LV_PART_MAIN);
      }
    }
    refresh_active_page_ui();
  }
}

static void climate_step_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int dir = (int)(intptr_t)lv_event_get_user_data(e);
    HaEntity ent = haClient.getEntity(activeControlEntityId);
    float step = (ent.tempStep > 0.05f) ? ent.tempStep : 0.5f;
    float newTemp = ent.targetTemp + (dir * step);
    float minT = (ent.minTemp > 5.0f) ? ent.minTemp : 16.0f;
    float maxT = (ent.maxTemp > 10.0f) ? ent.maxTemp : 30.0f;
    if (newTemp < minT) newTemp = minT;
    if (newTemp > maxT) newTemp = maxT;
    haClient.setClimateTargetTemp(activeControlEntityId, newTemp);
    if (lbl_ctrl_climate_target) {
      char tBuf[32];
      snprintf(tBuf, sizeof(tBuf), "%.1f °C", newTemp);
      lv_label_set_text(lbl_ctrl_climate_target, tBuf);
    }
    refresh_active_page_ui();
  }
}

static void climate_preset_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int preset = (int)(intptr_t)lv_event_get_user_data(e);
    haClient.setClimateTargetTemp(activeControlEntityId, (float)preset);
    if (lbl_ctrl_climate_target) {
      char tBuf[32];
      snprintf(tBuf, sizeof(tBuf), "%.1f °C", (float)preset);
      lv_label_set_text(lbl_ctrl_climate_target, tBuf);
    }
    refresh_active_page_ui();
  }
}

static void open_light_modal(const Widget* w, const String& entityId, const String& title, const HaEntity& ent) {
  close_control_modal_cb(nullptr);
  activeControlEntityId = entityId;
  activeControlWidget = w;

  control_overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(control_overlay, 320, 240);
  lv_obj_set_pos(control_overlay, 0, 0);
  lv_obj_set_style_bg_color(control_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(control_overlay, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_border_width(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(control_overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(control_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(control_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(control_overlay, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(control_overlay);
  lv_obj_set_size(card, 284, 208);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), LV_PART_MAIN);
  lv_obj_set_style_border_color(card, lv_color_hex(0x475569), LV_PART_MAIN);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_all(card, 8, LV_PART_MAIN);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *lbl_title = lv_label_create(card);
  lv_label_set_text(lbl_title, title.c_str());
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 2, 2);

  lv_obj_t *btn_close = lv_btn_create(card);
  lv_obj_set_size(btn_close, 30, 26);
  lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_close, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_close, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_close, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_x = lv_label_create(btn_close);
  lv_label_set_text(lbl_x, "X");
  lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
  lv_obj_center(lbl_x);

  lbl_ctrl_brightness = lv_label_create(card);
  char bBuf[32];
  snprintf(bBuf, sizeof(bBuf), "Brightness: %d%%", ent.brightnessPct);
  lv_label_set_text(lbl_ctrl_brightness, bBuf);
  lv_obj_set_style_text_color(lbl_ctrl_brightness, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
  lv_obj_set_pos(lbl_ctrl_brightness, 2, 28);

  slider_ctrl_brightness = lv_slider_create(card);
  lv_obj_set_size(slider_ctrl_brightness, 260, 16);
  lv_obj_set_pos(slider_ctrl_brightness, 2, 48);
  lv_slider_set_range(slider_ctrl_brightness, 1, 100);
  lv_slider_set_value(slider_ctrl_brightness, ent.brightnessPct, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider_ctrl_brightness, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider_ctrl_brightness, lv_color_hex(0xF59E0B), LV_PART_INDICATOR);
  lv_obj_add_event_cb(slider_ctrl_brightness, light_slider_cb, LV_EVENT_ALL, NULL);

  const int presets[4] = { 25, 50, 75, 100 };
  const char* pLabels[4] = { "25%", "50%", "75%", "100%" };
  for (int p = 0; p < 4; p++) {
    lv_obj_t *btn_p = lv_btn_create(card);
    lv_obj_set_size(btn_p, 58, 26);
    lv_obj_set_pos(btn_p, p * 68, 72);
    lv_obj_set_style_bg_color(btn_p, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_p, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_p, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_p, light_preset_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)presets[p]);

    lv_obj_t *lbl_p = lv_label_create(btn_p);
    lv_label_set_text(lbl_p, pLabels[p]);
    lv_obj_set_style_text_color(lbl_p, lv_color_hex(0xF1F5F9), LV_PART_MAIN);
    lv_obj_center(lbl_p);
  }

  if (ent.supportsRgb || entityId == "light.cyd_demo_rgb") {
    lv_obj_t *lbl_color = lv_label_create(card);
    lv_label_set_text(lbl_color, "Colors:");
    lv_obj_set_style_text_color(lbl_color, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_pos(lbl_color, 2, 108);

    std::vector<ColorRgb> presets = dashboardConfig.getColorPresetsForWidget(w);
    for (size_t c = 0; c < 6 && c < presets.size(); c++) {
      lv_obj_t *btn_c = lv_btn_create(card);
      lv_obj_set_size(btn_c, 34, 26);
      lv_obj_set_pos(btn_c, 52 + c * 35, 104);
      lv_obj_set_style_bg_color(btn_c, lv_color_hex(presets[c].hex), LV_PART_MAIN);
      lv_obj_set_style_radius(btn_c, 6, LV_PART_MAIN);
      lv_obj_set_style_border_width(btn_c, 1, LV_PART_MAIN);
      lv_obj_set_style_border_color(btn_c, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
      lv_obj_set_style_pad_all(btn_c, 0, LV_PART_MAIN);
      lv_obj_add_event_cb(btn_c, light_color_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)c);
    }
  }

  lv_obj_t *btn_pwr = lv_btn_create(card);
  lv_obj_set_size(btn_pwr, 120, 32);
  lv_obj_set_pos(btn_pwr, 10, 146);
  bool isOn = (ent.state == "on");
  lv_obj_set_style_bg_color(btn_pwr, isOn ? lv_color_hex(0xDC2626) : lv_color_hex(0x10B981), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_pwr, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_pwr, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_pwr, light_modal_power_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_pwr = lv_label_create(btn_pwr);
  lv_label_set_text(lbl_pwr, isOn ? "Turn Off" : "Turn On");
  lv_obj_center(lbl_pwr);

  lv_obj_t *btn_done = lv_btn_create(card);
  lv_obj_set_size(btn_done, 110, 32);
  lv_obj_set_pos(btn_done, 144, 146);
  lv_obj_set_style_bg_color(btn_done, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_done, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_done, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_done, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_done = lv_label_create(btn_done);
  lv_label_set_text(lbl_done, "Done");
  lv_obj_center(lbl_done);
}

static void open_climate_modal(const String& entityId, const String& title, const HaEntity& ent) {
  close_control_modal_cb(nullptr);
  activeControlEntityId = entityId;

  control_overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(control_overlay, 320, 240);
  lv_obj_set_pos(control_overlay, 0, 0);
  lv_obj_set_style_bg_color(control_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(control_overlay, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_border_width(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(control_overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(control_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(control_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(control_overlay, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(control_overlay);
  lv_obj_set_size(card, 284, 210);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), LV_PART_MAIN);
  lv_obj_set_style_border_color(card, lv_color_hex(0x475569), LV_PART_MAIN);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_all(card, 8, LV_PART_MAIN);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *lbl_title = lv_label_create(card);
  lv_label_set_text(lbl_title, title.c_str());
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 2, 2);

  lv_obj_t *btn_close = lv_btn_create(card);
  lv_obj_set_size(btn_close, 30, 26);
  lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_close, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_close, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_close, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_x = lv_label_create(btn_close);
  lv_label_set_text(lbl_x, "X");
  lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
  lv_obj_center(lbl_x);

  // 4 Mode Buttons: HEAT, COOL, AUTO, OFF (Row at Y=30)
  const char* modeLabels[4] = { "HEAT", "COOL", "AUTO", "OFF" };
  const char* modeNames[4] = { "heat", "cool", "auto", "off" };
  const uint32_t modeActiveColors[4] = { 0xDC2626, 0x0284C7, 0x10B981, 0x475569 };
  for (int m = 0; m < 4; m++) {
    btn_climate_modes[m] = lv_btn_create(card);
    lv_obj_set_size(btn_climate_modes[m], 62, 30);
    lv_obj_set_pos(btn_climate_modes[m], m * 68, 30);
    bool isActive = (ent.state == modeNames[m]);
    lv_obj_set_style_bg_color(btn_climate_modes[m], lv_color_hex(isActive ? modeActiveColors[m] : 0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_climate_modes[m], 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_climate_modes[m], lv_color_hex(isActive ? 0x94A3B8 : 0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_climate_modes[m], 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_climate_modes[m], 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_climate_modes[m], climate_mode_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)m);

    lv_obj_t *lbl_m = lv_label_create(btn_climate_modes[m]);
    lv_label_set_text(lbl_m, modeLabels[m]);
    lv_obj_set_style_text_color(lbl_m, lv_color_hex(isActive ? 0xFFFFFF : 0x94A3B8), LV_PART_MAIN);
    lv_obj_center(lbl_m);
  }

  // Current Temperature info (Y=66)
  lbl_ctrl_climate_current = lv_label_create(card);
  char curBuf[48];
  snprintf(curBuf, sizeof(curBuf), "Current: %.1f °C   (%s)", ent.currentTemp, ent.hvacAction.c_str());
  lv_label_set_text(lbl_ctrl_climate_current, curBuf);
  lv_obj_set_style_text_color(lbl_ctrl_climate_current, lv_color_hex(0x94A3B8), LV_PART_MAIN);
  lv_obj_set_pos(lbl_ctrl_climate_current, 4, 66);

  // Temperature Stepper Controls (Y=88)
  // [-] Button
  lv_obj_t *btn_minus = lv_btn_create(card);
  lv_obj_set_size(btn_minus, 56, 38);
  lv_obj_set_pos(btn_minus, 4, 88);
  lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_minus, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_minus, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_minus, climate_step_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

  lv_obj_t *lbl_minus = lv_label_create(btn_minus);
  lv_label_set_text(lbl_minus, "-");
  lv_obj_set_style_text_font(lbl_minus, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_set_style_text_color(lbl_minus, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_center(lbl_minus);

  // Target Temperature Display
  lbl_ctrl_climate_target = lv_label_create(card);
  char tBuf[32];
  snprintf(tBuf, sizeof(tBuf), "%.1f °C", ent.targetTemp);
  lv_label_set_text(lbl_ctrl_climate_target, tBuf);
  lv_obj_set_style_text_font(lbl_ctrl_climate_target, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_set_style_text_color(lbl_ctrl_climate_target, lv_color_hex(0xF59E0B), LV_PART_MAIN);
  lv_obj_set_size(lbl_ctrl_climate_target, 140, 38);
  lv_obj_set_style_text_align(lbl_ctrl_climate_target, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_pos(lbl_ctrl_climate_target, 64, 92);

  // [+] Button
  lv_obj_t *btn_plus = lv_btn_create(card);
  lv_obj_set_size(btn_plus, 56, 38);
  lv_obj_set_pos(btn_plus, 208, 88);
  lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_plus, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_plus, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_plus, climate_step_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

  lv_obj_t *lbl_plus = lv_label_create(btn_plus);
  lv_label_set_text(lbl_plus, "+");
  lv_obj_set_style_text_font(lbl_plus, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_set_style_text_color(lbl_plus, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_center(lbl_plus);

  // Quick Presets: 18°, 20°, 22°, 24° (Y=134)
  const int tPresets[4] = { 18, 20, 22, 24 };
  const char* tLabels[4] = { "18°C", "20°C", "22°C", "24°C" };
  for (int p = 0; p < 4; p++) {
    lv_obj_t *btn_p = lv_btn_create(card);
    lv_obj_set_size(btn_p, 62, 26);
    lv_obj_set_pos(btn_p, p * 68, 134);
    lv_obj_set_style_bg_color(btn_p, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_p, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_p, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_p, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_p, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_p, climate_preset_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)tPresets[p]);

    lv_obj_t *lbl_p = lv_label_create(btn_p);
    lv_label_set_text(lbl_p, tLabels[p]);
    lv_obj_set_style_text_color(lbl_p, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_center(lbl_p);
  }

  // Done Button (Y=168)
  lv_obj_t *btn_done = lv_btn_create(card);
  lv_obj_set_size(btn_done, 120, 26);
  lv_obj_set_pos(btn_done, 74, 168);
  lv_obj_set_style_bg_color(btn_done, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_done, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_done, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_done, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_done = lv_label_create(btn_done);
  lv_label_set_text(lbl_done, "Done");
  lv_obj_center(lbl_done);
}

static void open_cover_modal(const String& entityId, const String& title, const HaEntity& ent) {
  close_control_modal_cb(nullptr);
  activeControlEntityId = entityId;

  control_overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(control_overlay, 320, 240);
  lv_obj_set_pos(control_overlay, 0, 0);
  lv_obj_set_style_bg_color(control_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(control_overlay, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_border_width(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(control_overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(control_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(control_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(control_overlay, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(control_overlay);
  lv_obj_set_size(card, 284, 210);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), LV_PART_MAIN);
  lv_obj_set_style_border_color(card, lv_color_hex(0x475569), LV_PART_MAIN);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_all(card, 8, LV_PART_MAIN);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *lbl_title = lv_label_create(card);
  lv_label_set_text(lbl_title, title.c_str());
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 2, 2);

  lv_obj_t *btn_close = lv_btn_create(card);
  lv_obj_set_size(btn_close, 30, 26);
  lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_close, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_close, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_close, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_x = lv_label_create(btn_close);
  lv_label_set_text(lbl_x, "X");
  lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
  lv_obj_center(lbl_x);

  // Big Action Buttons: Open, Stop, Close
  const char* actLabels[3] = { "OPEN", "STOP", "CLOSE" };
  const uint32_t actColors[3] = { 0x10B981, 0x64748B, 0xDC2626 };
  for (int a = 0; a < 3; a++) {
    lv_obj_t *btn_a = lv_btn_create(card);
    lv_obj_set_size(btn_a, 84, 34);
    lv_obj_set_pos(btn_a, a * 90, 32);
    lv_obj_set_style_bg_color(btn_a, lv_color_hex(actColors[a]), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_a, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_a, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_a, cover_action_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)(a + 1));

    lv_obj_t *lbl_a = lv_label_create(btn_a);
    lv_label_set_text(lbl_a, actLabels[a]);
    lv_obj_set_style_text_color(lbl_a, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(lbl_a);
  }

  lbl_ctrl_cover = lv_label_create(card);
  char cBuf[32];
  snprintf(cBuf, sizeof(cBuf), "Position: %d%% (%s)", ent.currentPosition, (ent.currentPosition > 0) ? "Open" : "Closed");
  lv_label_set_text(lbl_ctrl_cover, cBuf);
  lv_obj_set_style_text_color(lbl_ctrl_cover, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
  lv_obj_set_pos(lbl_ctrl_cover, 2, 74);

  slider_ctrl_cover = lv_slider_create(card);
  lv_obj_set_size(slider_ctrl_cover, 260, 16);
  lv_obj_set_pos(slider_ctrl_cover, 2, 96);
  lv_slider_set_range(slider_ctrl_cover, 0, 100);
  lv_slider_set_value(slider_ctrl_cover, ent.currentPosition, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider_ctrl_cover, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider_ctrl_cover, lv_color_hex(0x0284C7), LV_PART_INDICATOR);
  lv_obj_add_event_cb(slider_ctrl_cover, cover_slider_cb, LV_EVENT_ALL, NULL);

  const int posPresets[4] = { 25, 50, 75, 100 };
  const char* posLabels[4] = { "25%", "50%", "75%", "100%" };
  for (int p = 0; p < 4; p++) {
    lv_obj_t *btn_p = lv_btn_create(card);
    lv_obj_set_size(btn_p, 58, 26);
    lv_obj_set_pos(btn_p, p * 68, 122);
    lv_obj_set_style_bg_color(btn_p, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_p, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_p, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_p, cover_preset_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)posPresets[p]);

    lv_obj_t *lbl_p = lv_label_create(btn_p);
    lv_label_set_text(lbl_p, posLabels[p]);
    lv_obj_set_style_text_color(lbl_p, lv_color_hex(0xF1F5F9), LV_PART_MAIN);
    lv_obj_center(lbl_p);
  }

  lv_obj_t *btn_done = lv_btn_create(card);
  lv_obj_set_size(btn_done, 120, 28);
  lv_obj_set_pos(btn_done, 72, 156);
  lv_obj_set_style_bg_color(btn_done, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_done, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_done, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_done, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_done_btn = lv_label_create(btn_done);
  lv_label_set_text(lbl_done_btn, "Done");
  lv_obj_center(lbl_done_btn);
}

static void open_fan_modal(const String& entityId, const String& title, const HaEntity& ent) {
  close_control_modal_cb(nullptr);
  activeControlEntityId = entityId;

  control_overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(control_overlay, 320, 240);
  lv_obj_set_pos(control_overlay, 0, 0);
  lv_obj_set_style_bg_color(control_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(control_overlay, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_border_width(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(control_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(control_overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(control_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(control_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(control_overlay, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(control_overlay);
  lv_obj_set_size(card, 284, 190);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), LV_PART_MAIN);
  lv_obj_set_style_border_color(card, lv_color_hex(0x475569), LV_PART_MAIN);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_all(card, 8, LV_PART_MAIN);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *lbl_title = lv_label_create(card);
  lv_label_set_text(lbl_title, title.c_str());
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 2, 2);

  lv_obj_t *btn_close = lv_btn_create(card);
  lv_obj_set_size(btn_close, 30, 26);
  lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_close, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_close, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_close, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_x = lv_label_create(btn_close);
  lv_label_set_text(lbl_x, "X");
  lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
  lv_obj_center(lbl_x);

  lbl_ctrl_speed = lv_label_create(card);
  char sBuf[32];
  const char* mode = (ent.speedPct == 0) ? "Off" : ((ent.speedPct <= 33) ? "Low" : ((ent.speedPct <= 66) ? "Med" : "High"));
  snprintf(sBuf, sizeof(sBuf), "Speed: %d%% (%s)", ent.speedPct, mode);
  lv_label_set_text(lbl_ctrl_speed, sBuf);
  lv_obj_set_style_text_color(lbl_ctrl_speed, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
  lv_obj_set_pos(lbl_ctrl_speed, 2, 28);

  const int speeds[4] = { 0, 33, 66, 100 };
  const char* sLabels[4] = { "Off", "Low", "Med", "High" };
  const uint32_t sColors[4] = { 0x475569, 0x0284C7, 0x2563EB, 0x7C3AED };
  for (int s = 0; s < 4; s++) {
    lv_obj_t *btn_s = lv_btn_create(card);
    lv_obj_set_size(btn_s, 58, 34);
    lv_obj_set_pos(btn_s, s * 68, 52);
    lv_obj_set_style_bg_color(btn_s, lv_color_hex(sColors[s]), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_s, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_s, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_s, fan_preset_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)speeds[s]);

    lv_obj_t *lbl_s = lv_label_create(btn_s);
    lv_label_set_text(lbl_s, sLabels[s]);
    lv_obj_set_style_text_color(lbl_s, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(lbl_s);
  }

  slider_ctrl_speed = lv_slider_create(card);
  lv_obj_set_size(slider_ctrl_speed, 260, 16);
  lv_obj_set_pos(slider_ctrl_speed, 2, 96);
  lv_slider_set_range(slider_ctrl_speed, 0, 100);
  lv_slider_set_value(slider_ctrl_speed, ent.speedPct, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider_ctrl_speed, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider_ctrl_speed, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
  lv_obj_add_event_cb(slider_ctrl_speed, fan_slider_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *btn_done = lv_btn_create(card);
  lv_obj_set_size(btn_done, 120, 32);
  lv_obj_set_pos(btn_done, 72, 128);
  lv_obj_set_style_bg_color(btn_done, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_done, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_done, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_done, close_control_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_done = lv_label_create(btn_done);
  lv_label_set_text(lbl_done, "Done");
  lv_obj_center(lbl_done);
}

// Tile Touch / Click / Long-Press Callback
static void slot_click_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_SHORT_CLICKED) {
    if (millis() - lastGestureTime < 500) {
      Serial.println("[UI] Ignored tile click following swipe gesture");
      return;
    }

    static unsigned long lastClick = 0;
    if (millis() - lastClick < 350) return;
    lastClick = millis();

    int slotIdx = (int)(intptr_t)lv_event_get_user_data(e);
    const Widget* w = getWidgetForSlot(slotIdx);
    if (!w) return;

    if (w->type == WidgetType::TOGGLE) {
      HaEntity ent = haClient.getEntity(w->entityId);
      bool isAction = (w->entityId.startsWith("scene.") || w->entityId.startsWith("script."));
      String nextState = (ent.state == "on") ? "off" : "on";
      const DashboardTheme& th = getThemeById(dashboardConfig.theme);

      // 0ms Optimistic UI flip / Action trigger flash
      SlotWidget& sw = slots[slotIdx];
      if (isAction) {
        // Hold active Indigo pulse and ACTIVE feedback for 1.5 seconds
        actionTriggerUntil[slotIdx] = millis() + 1500;
        setContainerColor(sw.container, lv_color_hex(0x4338CA));
        lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_color(sw.label_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_label_set_text(sw.label_type, "ACTIVE");
        lv_obj_set_style_text_color(sw.label_type, lv_color_hex(0x67E8F9), LV_PART_MAIN);
        Serial.printf("[UI] Slot %d (%s) -> Action Activated\n", slotIdx, w->entityId.c_str());
        haClient.toggleWithTarget("turn_on", w->entityId);
      } else if (w->entityId.startsWith("cover.") || w->entityId == "cover.cyd_demo_blinds") {
        bool willBeOpen = !(ent.state == "open" || ent.state == "opening");
        setContainerColor(sw.container, willBeOpen ? lv_color_hex(0x0284C7) : lv_color_hex(th.tileIdleBg));
        lv_obj_set_style_text_color(sw.label_icon, willBeOpen ? lv_color_hex(0xFFFFFF) : lv_color_hex(th.tileIconIdle), LV_PART_MAIN);
        lv_obj_set_style_text_color(sw.label_name, willBeOpen ? lv_color_hex(0xFFFFFF) : lv_color_hex(th.tileText), LV_PART_MAIN);
        Serial.printf("[UI] Slot %d (%s) -> Toggle Cover (%s)\n", slotIdx, w->entityId.c_str(), willBeOpen ? "open" : "close");
        haClient.toggleWithTarget(willBeOpen ? "open" : "close", w->entityId);
      } else if (w->entityId.startsWith("climate.") || w->entityId == "climate.cyd_demo_thermostat") {
        bool willBeHeat = (ent.state == "off");
        String nextMode = willBeHeat ? "heat" : "off";
        setContainerColor(sw.container, willBeHeat ? lv_color_hex(0x991B1B) : lv_color_hex(th.tileIdleBg));
        lv_obj_set_style_text_color(sw.label_icon, willBeHeat ? lv_color_hex(0xFFFFFF) : lv_color_hex(th.tileIconIdle), LV_PART_MAIN);
        lv_obj_set_style_text_color(sw.label_name, willBeHeat ? lv_color_hex(0xFFFFFF) : lv_color_hex(th.tileText), LV_PART_MAIN);
        Serial.printf("[UI] Slot %d (%s) -> Toggle Climate (%s)\n", slotIdx, w->entityId.c_str(), nextMode.c_str());
        haClient.setHvacMode(w->entityId, nextMode);
      } else {
        bool willBeOn = (nextState == "on");
        bool isLight = (w->entityId.startsWith("light.") || w->icon.indexOf("light") >= 0 || w->icon.indexOf("bulb") >= 0);
        lv_color_t bg = willBeOn ? (isLight ? lv_color_hex(0xD97706) : lv_color_hex(th.switchOnBg)) : lv_color_hex(th.tileIdleBg);
        if (willBeOn && isLight && ent.supportsRgb) {
          bg = lv_color_make(ent.rgb[0], ent.rgb[1], ent.rgb[2]);
        }
        setContainerColor(sw.container, bg);
        lv_obj_set_style_text_color(sw.label_icon, willBeOn ? lv_color_hex(0xFFFFFF) : lv_color_hex(th.tileIconIdle), LV_PART_MAIN);
        lv_obj_set_style_text_color(sw.label_name, willBeOn ? lv_color_hex(0xFFFFFF) : lv_color_hex(th.tileText), LV_PART_MAIN);

        Serial.printf("[UI] Slot %d (%s) -> %s\n", slotIdx, w->entityId.c_str(), nextState.c_str());
        haClient.toggleWithTarget(nextState, w->entityId);
      }
    } else {
      SlotWidget& sw = slots[slotIdx];
      lv_label_set_text(sw.label_type, "Refreshing...");
      haClient.triggerPoll();
    }
  } else if (code == LV_EVENT_LONG_PRESSED) {
    if (millis() - lastGestureTime < 500) return;

    int slotIdx = (int)(intptr_t)lv_event_get_user_data(e);
    const Widget* w = getWidgetForSlot(slotIdx);
    if (!w) return;

    HaEntity ent = haClient.getEntity(w->entityId);
    bool isLight = (w->entityId.startsWith("light.") || w->entityId == "light.cyd_demo_rgb");
    bool isFan = (w->entityId.startsWith("fan.") || w->entityId == "fan.cyd_demo_fan");
    bool isCover = (w->entityId.startsWith("cover.") || w->entityId == "cover.cyd_demo_blinds");
    bool isClimate = (w->entityId.startsWith("climate.") || w->entityId == "climate.cyd_demo_thermostat");

    if (isLight) {
      if (!ent.supportsBrightness && !ent.supportsRgb && w->entityId != "light.cyd_demo_rgb") {
        Serial.println("[UI] Light is standard switch (no dimmer/RGB) - long press disabled");
        return;
      }
      open_light_modal(w, w->entityId, ent.friendlyName.length() ? ent.friendlyName : w->label, ent);
    } else if (isFan) {
      if (!ent.supportsSpeed && w->entityId != "fan.cyd_demo_fan") {
        Serial.println("[UI] Fan does not support speed control - long press disabled");
        return;
      }
      open_fan_modal(w->entityId, ent.friendlyName.length() ? ent.friendlyName : w->label, ent);
    } else if (isCover) {
      open_cover_modal(w->entityId, ent.friendlyName.length() ? ent.friendlyName : w->label, ent);
    } else if (isClimate) {
      open_climate_modal(w->entityId, ent.friendlyName.length() ? ent.friendlyName : w->label, ent);
    }
  }
}

static void apply_ui_updates() {
  // Check if any action trigger animation has expired
  static bool hadActiveAction = false;
  bool hasActiveAction = false;
  unsigned long now = millis();
  for (int i = 0; i < MAX_SLOTS; i++) {
    if (actionTriggerUntil[i] > 0) {
      if (now >= actionTriggerUntil[i]) {
        actionTriggerUntil[i] = 0;
        update_slot_ui(i); // Restore normal appearance
      } else {
        hasActiveAction = true;
      }
    }
  }
  hadActiveAction = hasActiveAction;

  if (!uiDirty) return;
  uiDirty = false;

  if (label_header_ha) {
    safe_set_label_text(label_header_ha, MDI_HOME_ASSISTANT);
    lv_color_t targetColor = queuedHaOk ? lv_color_hex(0x41BDF5) : lv_color_hex(0xEF4444);
    if (lv_obj_get_style_text_color(label_header_ha, LV_PART_MAIN).full != targetColor.full) {
      lv_obj_set_style_text_color(label_header_ha, targetColor, LV_PART_MAIN);
    }
  }

  for (int i = 0; i < MAX_SLOTS; i++) {
    update_slot_ui(i);
  }
}

static void check_display_power_management() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 250) return;
  lastCheck = millis();

  unsigned long idleSec = (millis() - lastTouchActivity) / 1000;
  int sleepTimeout = dashboardConfig.sleepTimeoutSec;
  int dimTimeout = dashboardConfig.dimTimeoutSec;

  if (sleepTimeout > 0 && idleSec >= (unsigned long)sleepTimeout) {
    if (displayPowerState != DisplayPowerState::SLEEP) {
      displayPowerState = DisplayPowerState::SLEEP;
      setBrightness(0);
      Serial.println("[Power] Idle timeout reached -> SLEEP mode (0% backlight)");
    }
  } else if (dimTimeout > 0 && idleSec >= (unsigned long)dimTimeout) {
    if (displayPowerState == DisplayPowerState::ACTIVE) {
      displayPowerState = DisplayPowerState::DIMMED;
      setBrightness(dashboardConfig.dimBrightness);
      Serial.printf("[Power] Idle timeout reached -> DIMMED mode (%d backlight)\n", dashboardConfig.dimBrightness);
    }
  }
}

static void update_network_ui(bool connected, bool isAp, const String& info) {
  if (connected) {
    if (!otaStarted) {
      OtaManager::begin("cyd-ha-dashboard");
      otaStarted = true;
    }

    if (!haStarted) {
      haClient.begin(wifiManager.getHaUrl(), wifiManager.getHaToken(), wifiManager.getHaEntity());
      haClient.setMonitoredEntities(dashboardConfig.getAllEntityIds());
      haStarted = true;
    }
  }
}

static void close_modal_cb(lv_event_t *e) {
  if (modal_overlay) {
    lv_obj_del(modal_overlay);
    modal_overlay = nullptr;
  }
}

static void btn_sync_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    Serial.println("[SYSTEM] HA Force poll requested from UI modal");
    haClient.triggerPoll();
    close_modal_cb(nullptr);
  }
}

static void btn_cal_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    Preferences p;
    if (p.begin("cyd_cal_v1", false)) {
      p.clear();
      p.end();
    }
    LittleFS.remove("/touch_v1.json");
    Serial.println("[SYSTEM] Touch calibration cleared. Rebooting into calibration mode...");
    delay(300);
    ESP.restart();
  }
}

static void btn_reboot_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    Serial.println("[SYSTEM] Reboot requested from UI modal");
    delay(300);
    ESP.restart();
  }
}

static void ha_icon_click_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (millis() - lastGestureTime < 500) return;

  if (modal_overlay) {
    close_modal_cb(nullptr);
    return;
  }

  // Dark backdrop covering entire screen (320x240)
  modal_overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(modal_overlay, 320, 240);
  lv_obj_set_pos(modal_overlay, 0, 0);
  lv_obj_set_style_bg_color(modal_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(modal_overlay, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_border_width(modal_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(modal_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(modal_overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(modal_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(modal_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(modal_overlay, close_modal_cb, LV_EVENT_CLICKED, NULL);

  // Modal Card (280x200)
  lv_obj_t *card = lv_obj_create(modal_overlay);
  lv_obj_set_size(card, 280, 200);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E293B), LV_PART_MAIN);
  lv_obj_set_style_border_color(card, lv_color_hex(0x475569), LV_PART_MAIN);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

  // Card Header: Title
  lv_obj_t *lbl_title = lv_label_create(card);
  lv_label_set_text(lbl_title, "System Information");
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 2, 4);

  // Card Header: Close Button [X]
  lv_obj_t *btn_close = lv_btn_create(card);
  lv_obj_set_size(btn_close, 30, 26);
  lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_close, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_close, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btn_close, close_modal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_x = lv_label_create(btn_close);
  lv_label_set_text(lbl_x, "X");
  lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
  lv_obj_center(lbl_x);

  // Info Container Box
  lv_obj_t *info_box = lv_obj_create(card);
  lv_obj_set_size(info_box, 258, 92);
  lv_obj_align(info_box, LV_ALIGN_TOP_MID, 0, 32);
  lv_obj_set_style_bg_color(info_box, lv_color_hex(0x0F172A), LV_PART_MAIN);
  lv_obj_set_style_border_color(info_box, lv_color_hex(0x334155), LV_PART_MAIN);
  lv_obj_set_style_border_width(info_box, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(info_box, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(info_box, 6, LV_PART_MAIN);
  lv_obj_clear_flag(info_box, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl_info = lv_label_create(info_box);
  char buf[256];
  snprintf(buf, sizeof(buf),
    "HA: %s\n"
    "RAM: %u KB (Min: %u KB)\n"
    "IP: %s\n"
    "WiFi: %d dBm",
    queuedHaOk ? "#41BDF5 Connected#" : "#EF4444 Disconnected#",
    (unsigned int)(ESP.getFreeHeap() / 1024),
    (unsigned int)(ESP.getMinFreeHeap() / 1024),
    WiFi.localIP().toString().c_str(),
    WiFi.RSSI()
  );
  lv_label_set_recolor(lbl_info, true);
  lv_label_set_text(lbl_info, buf);
  lv_obj_align(lbl_info, LV_ALIGN_TOP_LEFT, 0, 0);

  // Quick Action Buttons Row (Sync, Calibrate, Reboot)
  lv_obj_t *b_sync = lv_btn_create(card);
  lv_obj_set_size(b_sync, 78, 34);
  lv_obj_set_pos(b_sync, 0, 134);
  lv_obj_set_style_bg_color(b_sync, lv_color_hex(0x2563EB), LV_PART_MAIN);
  lv_obj_set_style_radius(b_sync, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(b_sync, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(b_sync, btn_sync_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *l_sync = lv_label_create(b_sync);
  lv_label_set_text(l_sync, "Sync");
  lv_obj_center(l_sync);

  lv_obj_t *b_cal = lv_btn_create(card);
  lv_obj_set_size(b_cal, 88, 34);
  lv_obj_set_pos(b_cal, 84, 134);
  lv_obj_set_style_bg_color(b_cal, lv_color_hex(0xD97706), LV_PART_MAIN);
  lv_obj_set_style_radius(b_cal, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(b_cal, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(b_cal, btn_cal_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *l_cal = lv_label_create(b_cal);
  lv_label_set_text(l_cal, "Calibrate");
  lv_obj_center(l_cal);

  lv_obj_t *b_reboot = lv_btn_create(card);
  lv_obj_set_size(b_reboot, 78, 34);
  lv_obj_set_pos(b_reboot, 178, 134);
  lv_obj_set_style_bg_color(b_reboot, lv_color_hex(0xDC2626), LV_PART_MAIN);
  lv_obj_set_style_radius(b_reboot, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_all(b_reboot, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(b_reboot, btn_reboot_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *l_reboot = lv_label_create(b_reboot);
  lv_label_set_text(l_reboot, "Reboot");
  lv_obj_center(l_reboot);
}

static void create_ui() {
  ui_screen = lv_scr_act();
  lv_obj_clear_flag(ui_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui_screen, screen_gesture_cb, LV_EVENT_GESTURE, NULL);

  // 1. Header Bar (0..34px)
  ui_header = lv_obj_create(ui_screen);
  lv_obj_set_size(ui_header, 320, 34);
  lv_obj_set_pos(ui_header, 0, 0);
  lv_obj_set_style_border_width(ui_header, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(ui_header, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui_header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui_header, LV_OBJ_FLAG_GESTURE_BUBBLE);

  // Page Title Label (e.g. "Living Room (1/3)")
  label_page_title = lv_label_create(ui_header);
  lv_label_set_text(label_page_title, "Page 1 (1/3)");
  lv_obj_set_style_text_color(label_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(label_page_title, LV_ALIGN_LEFT_MID, 12, 0);

  // HA Status Stealth Button (invisible clickable container holding the icon)
  btn_header_ha = lv_btn_create(ui_header);
  lv_obj_set_size(btn_header_ha, 36, 32);
  lv_obj_align(btn_header_ha, LV_ALIGN_RIGHT_MID, -4, 0);
  lv_obj_set_style_bg_opa(btn_header_ha, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn_header_ha, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn_header_ha, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(btn_header_ha, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn_header_ha, 0, LV_PART_MAIN);
  lv_obj_clear_flag(btn_header_ha, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(btn_header_ha, ha_icon_click_cb, LV_EVENT_CLICKED, NULL);

  label_header_ha = lv_label_create(btn_header_ha);
  lv_obj_set_style_text_font(label_header_ha, &ui_font_mdi_24, LV_PART_MAIN);
  lv_label_set_text(label_header_ha, MDI_HOME_ASSISTANT);
  lv_obj_set_style_text_color(label_header_ha, lv_color_hex(0x41BDF5), LV_PART_MAIN);
  lv_obj_center(label_header_ha);

  // 2. Dynamic Grid Tile Layout (Up to 6 Slots, Expanded to fill screen)
  const int TILE_W = 148;
  const int TILE_H = 95;

  for (int i = 0; i < MAX_SLOTS; i++) {
    SlotWidget& sw = slots[i];
    sw.slotIndex = i;

    // Clickable tile container
    sw.container = lv_obj_create(ui_screen);
    lv_obj_set_size(sw.container, TILE_W, TILE_H);
    lv_obj_set_pos(sw.container, 8, 38);
    lv_obj_set_style_radius(sw.container, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(sw.container, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(sw.container, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw.container, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_pad_all(sw.container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(sw.container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(sw.container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(sw.container, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(sw.container, slot_click_cb, LV_EVENT_ALL, (void*)(intptr_t)i);

    // MDI Icon (Centered to dominate the button)
    sw.label_icon = lv_label_create(sw.container);
    lv_label_set_text(sw.label_icon, "");
    lv_obj_set_style_text_font(sw.label_icon, &ui_font_mdi_44, LV_PART_MAIN);
    lv_obj_set_style_text_color(sw.label_icon, lv_color_hex(0x64748B), LV_PART_MAIN);
    lv_obj_align(sw.label_icon, LV_ALIGN_CENTER, 0, -8);

    // Title / Friendly Name (Centered at bottom with 6px margin)
    sw.label_name = lv_label_create(sw.container);
    lv_label_set_text(sw.label_name, "Widget");
    lv_obj_set_style_text_font(sw.label_name, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(sw.label_name, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_align(sw.label_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(sw.label_name, TILE_W - 8);
    lv_label_set_long_mode(sw.label_name, LV_LABEL_LONG_DOT);
    lv_obj_set_style_anim_speed(sw.label_name, 15, LV_PART_MAIN);
    lv_obj_align(sw.label_name, LV_ALIGN_BOTTOM_MID, 0, -6);

    // Value / State (Sensor text or state overlay)
    sw.label_state = lv_label_create(sw.container);
    lv_label_set_text(sw.label_state, "");
    lv_obj_set_style_text_font(sw.label_state, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_set_style_text_color(sw.label_state, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_anim_speed(sw.label_state, 15, LV_PART_MAIN);
    lv_obj_align(sw.label_state, LV_ALIGN_CENTER, 0, 8);

    // Type / Badge (Centered at top of tile with 1px top margin)
    sw.label_type = lv_label_create(sw.container);
    lv_label_set_text(sw.label_type, "");
    lv_obj_set_style_text_align(sw.label_type, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(sw.label_type, lv_color_hex(0xA5B4FC), LV_PART_MAIN);
    lv_obj_align(sw.label_type, LV_ALIGN_TOP_MID, 0, 1);
  }

  apply_theme();
  refresh_active_page_ui();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n==================================");
  Serial.printf(" %s v%s\n", APP_NAME, APP_VERSION);
  Serial.printf(" Hardware: %s\n", HARDWARE_MODEL);
  Serial.println("==================================");

  // Initialize Task Watchdog (30 second timeout, panic/reboot on lockup)
  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL); // Add loop task (Core 1)

  // Prevent SPI bus conflicts
  pinMode(XPT2046_CS, OUTPUT);
  pinMode(TFT_CS_PIN, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  digitalWrite(TFT_CS_PIN, HIGH);

  setBackLedOff();

  lv_init();
  display.init();

  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(LCD_BACK_LIGHT_PIN, LEDC_CHANNEL_0);

  touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touch.begin(touchSpi);

  display.setRotation(1);
  touch.setRotation(1);
  display.invertDisplay(true);
  setBrightness(255);

  // 2-point touch calibration matching BambuDeck (LittleFS + NVS persistence)
  touchCal.begin(display, touch, false);

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, config::SCREEN_WIDTH * config::DRAW_BUF_LINES);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = config::SCREEN_WIDTH;
  disp_drv.ver_res = config::SCREEN_HEIGHT;
  disp_drv.flush_cb = flushDisplay;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = readTouch;
  indev_drv.gesture_limit = 45;       // Require distinct swipe across screen (prevents jitter taps triggering page turn)
  indev_drv.gesture_min_velocity = 3; // Require intentional swipe velocity
  lv_indev_drv_register(&indev_drv);

  // Initialize LittleFS Dashboard Configuration
  dashboardConfig.begin();
  lastTouchActivity = millis();
  displayPowerState = DisplayPowerState::ACTIVE;
  setBrightness(dashboardConfig.activeBrightness);

  create_ui();
  lv_timer_handler();

  // HA Client callbacks
  haClient.setStatusCallback([](bool connected, const String& info) {
    queuedHaOk = connected;
    queuedHaStatus = connected ? "HA: OK" : "HA: " + info;
    uiDirty = true;
  });

  haClient.setStateCallback([](const HaEntity& entity) {
    uiDirty = true;
  });

  // Wi-Fi setup
  wifiManager.setStatusCallback([](bool connected, bool isAp, const String& info) {
    update_network_ui(connected, isAp, info);
    lv_timer_handler();
  });

  wifiManager.setHaConnectedCallback([]() {
    return haClient.isConnected();
  });

  wifiManager.setOnConfigReloadCallback([]() {
    Serial.println("[UI] Config reloaded via Web Portal -> refreshing dashboard...");
    dashboardConfig.reload();
    apply_theme();
    setBrightness(dashboardConfig.activeBrightness);
    displayPowerState = DisplayPowerState::ACTIVE;
    lastTouchActivity = millis();
    haClient.setMonitoredEntities(dashboardConfig.getAllEntityIds());
    haClient.triggerPoll();
    refresh_active_page_ui();
    uiDirty = true;
  });

  wifiManager.begin();
}

void loop() {
  esp_task_wdt_reset(); // Feed Watchdog (Core 1)
  lv_timer_handler();
  wifiManager.loop();

  if (wifiManager.isConnected()) {
    OtaManager::loop();
  }

  apply_ui_updates();
  check_display_power_management();
  delay(2);
}
