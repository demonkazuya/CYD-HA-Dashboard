#pragma once
#include <Arduino.h>
#include "lvgl.h"
#include "../dashboard/dashboard_config.h"

// Custom MDI Fonts
LV_FONT_DECLARE(ui_font_mdi_44);
LV_FONT_DECLARE(ui_font_mdi_36);
LV_FONT_DECLARE(ui_font_mdi_24);

#define MDI_ACCOUNT                    "\xF3\xB0\x80\x84" // U+F0004 account
#define MDI_AIR_CONDITIONER            "\xF3\xB0\x80\x9B" // U+F001B air-conditioner
#define MDI_ALARM_LIGHT                "\xF3\xB0\x9E\x8F" // U+F078F alarm-light
#define MDI_ALERT_CIRCLE               "\xF3\xB0\x80\xA8" // U+F0028 alert-circle
#define MDI_BATHTUB                    "\xF3\xB1\xA0\x98" // U+F1818 bathtub
#define MDI_BATTERY                    "\xF3\xB0\x81\xB9" // U+F0079 battery
#define MDI_BATTERY_CHARGING           "\xF3\xB0\x82\x84" // U+F0084 battery-charging
#define MDI_BED                        "\xF3\xB0\x8B\xA3" // U+F02E3 bed
#define MDI_BELL                       "\xF3\xB0\x82\x9A" // U+F009A bell
#define MDI_BLINDS                     "\xF3\xB0\x82\xAC" // U+F00AC blinds
#define MDI_BLINDS_OPEN                "\xF3\xB1\x80\x91" // U+F1011 blinds-open
#define MDI_CAMERA                     "\xF3\xB0\x84\x80" // U+F0100 camera
#define MDI_CAR                        "\xF3\xB0\x84\x8B" // U+F010B car
#define MDI_CCTV                       "\xF3\xB0\x9E\xAE" // U+F07AE cctv
#define MDI_CEILING_LIGHT              "\xF3\xB0\x9D\xA9" // U+F0769 ceiling-light
#define MDI_CHECK_CIRCLE               "\xF3\xB0\x97\xA0" // U+F05E0 check-circle
#define MDI_COG                        "\xF3\xB0\x92\x93" // U+F0493 cog
#define MDI_CURTAINS                   "\xF3\xB1\xA1\x86" // U+F1846 curtains
#define MDI_CURTAINS_CLOSED            "\xF3\xB1\xA1\x87" // U+F1847 curtains-closed
#define MDI_DESK                       "\xF3\xB1\x88\xB9" // U+F1239 desk
#define MDI_DOOR                       "\xF3\xB0\xA0\x9A" // U+F081A door
#define MDI_DOOR_CLOSED                "\xF3\xB0\xA0\x9B" // U+F081B door-closed
#define MDI_DOOR_OPEN                  "\xF3\xB0\xA0\x9C" // U+F081C door-open
#define MDI_DOTS_HORIZONTAL            "\xF3\xB0\x87\x98" // U+F01D8 dots-horizontal
#define MDI_EYE                        "\xF3\xB0\x88\x88" // U+F0208 eye
#define MDI_EYE_OFF                    "\xF3\xB0\x88\x89" // U+F0209 eye-off
#define MDI_FAN                        "\xF3\xB0\x88\x90" // U+F0210 fan
#define MDI_FAN_AUTO                   "\xF3\xB1\x9C\x9D" // U+F171D fan-auto
#define MDI_FLASH                      "\xF3\xB0\x89\x81" // U+F0241 flash
#define MDI_GARAGE                     "\xF3\xB0\x9B\x99" // U+F06D9 garage
#define MDI_GARAGE_OPEN                "\xF3\xB0\x9B\x9A" // U+F06DA garage-open
#define MDI_HOME                       "\xF3\xB0\x8B\x9C" // U+F02DC home
#define MDI_HOME_ASSISTANT             "\xF3\xB0\x9F\x90" // U+F07D0 home-assistant
#define MDI_INFORMATION                "\xF3\xB0\x8B\xBC" // U+F02FC information
#define MDI_LAMP                       "\xF3\xB0\x9A\xB5" // U+F06B5 lamp
#define MDI_LIGHTBULB                  "\xF3\xB0\x8C\xB5" // U+F0335 lightbulb
#define MDI_LIGHTBULB_OUTLINE          "\xF3\xB0\x8C\xB6" // U+F0336 lightbulb-outline
#define MDI_LOCK                       "\xF3\xB0\x8C\xBE" // U+F033E lock
#define MDI_LOCK_OPEN                  "\xF3\xB0\x8C\xBF" // U+F033F lock-open
#define MDI_MOTION_SENSOR              "\xF3\xB0\xB6\x91" // U+F0D91 motion-sensor
#define MDI_MOTION_SENSOR_OFF          "\xF3\xB1\x90\xB5" // U+F1435 motion-sensor-off
#define MDI_PAUSE                      "\xF3\xB0\x8F\xA4" // U+F03E4 pause
#define MDI_PLAY                       "\xF3\xB0\x90\x8A" // U+F040A play
#define MDI_POWER                      "\xF3\xB0\x90\xA5" // U+F0425 power
#define MDI_POWER_SOCKET               "\xF3\xB0\x90\xA7" // U+F0427 power-socket
#define MDI_POWER_SOCKET_EU            "\xF3\xB0\x9F\xA7" // U+F07E7 power-socket-eu
#define MDI_POWER_SOCKET_US            "\xF3\xB0\x9F\xA9" // U+F07E9 power-socket-us
#define MDI_PRINTER                    "\xF3\xB0\x90\xAA" // U+F042A printer
#define MDI_RADIATOR                   "\xF3\xB0\x90\xB8" // U+F0438 radiator
#define MDI_RUN                        "\xF3\xB0\x9C\x8E" // U+F070E run
#define MDI_SHIELD_HOME                "\xF3\xB0\x9A\x8A" // U+F068A shield-home
#define MDI_SHIELD_LOCK                "\xF3\xB0\xA6\x9D" // U+F099D shield-lock
#define MDI_SILVERWARE_FORK_KNIFE      "\xF3\xB0\xA9\xB0" // U+F0A70 silverware-fork-knife
#define MDI_SOFA                       "\xF3\xB0\x92\xB9" // U+F04B9 sofa
#define MDI_SOLAR_POWER                "\xF3\xB0\xA9\xB2" // U+F0A72 solar-power
#define MDI_SPEAKER                    "\xF3\xB0\x93\x83" // U+F04C3 speaker
#define MDI_STOP                       "\xF3\xB0\x93\x9B" // U+F04DB stop
#define MDI_TELEVISION                 "\xF3\xB0\x94\x82" // U+F0502 television
#define MDI_THERMOMETER                "\xF3\xB0\x94\x8F" // U+F050F thermometer
#define MDI_TOGGLE_SWITCH              "\xF3\xB0\x94\xA1" // U+F0521 toggle-switch
#define MDI_TOGGLE_SWITCH_OFF          "\xF3\xB0\x94\xA2" // U+F0522 toggle-switch-off
#define MDI_VOLUME_HIGH                "\xF3\xB0\x95\xBE" // U+F057E volume-high
#define MDI_VOLUME_OFF                 "\xF3\xB0\x96\x81" // U+F0581 volume-off
#define MDI_WALL_SCONCE_FLAT           "\xF3\xB0\xA4\x9D" // U+F091D wall-sconce-flat
#define MDI_WATER_PERCENT              "\xF3\xB0\x96\x8E" // U+F058E water-percent
#define MDI_WEATHER_NIGHT              "\xF3\xB0\x96\x94" // U+F0594 weather-night
#define MDI_WEATHER_PARTLY_CLOUDY      "\xF3\xB0\x96\x95" // U+F0595 weather-partly-cloudy
#define MDI_WEATHER_RAINY              "\xF3\xB0\x96\x97" // U+F0597 weather-rainy
#define MDI_WEATHER_SUNNY              "\xF3\xB0\x96\x99" // U+F0599 weather-sunny
#define MDI_WIFI                       "\xF3\xB0\x96\xA9" // U+F05A9 wifi
#define MDI_WIFI_OFF                   "\xF3\xB0\x96\xAA" // U+F05AA wifi-off
#define MDI_WINDOW_CLOSED              "\xF3\xB0\x96\xAE" // U+F05AE window-closed
#define MDI_WINDOW_OPEN                "\xF3\xB0\x96\xB1" // U+F05B1 window-open

// Resolver function for genuine Material Design Icons
inline const char* getRealMdiSymbol(const String& icon, const String& entityId, WidgetType type) {
  String ic = icon;
  ic.toLowerCase();
  if (ic.startsWith("mdi:")) ic = ic.substring(4);

  // 1. Direct MDI name checks
  if (ic == "lightbulb" || ic == "lightbulb-outline") return MDI_LIGHTBULB;
  if (ic == "ceiling-light") return MDI_CEILING_LIGHT;
  if (ic == "lamp" || ic == "desk-lamp") return MDI_LAMP;
  if (ic == "fan") return MDI_FAN;
  if (ic == "fan-auto") return MDI_FAN_AUTO;
  if (ic == "air-conditioner") return MDI_AIR_CONDITIONER;
  if (ic == "radiator") return MDI_RADIATOR;
  if (ic == "motion-sensor") return MDI_MOTION_SENSOR;
  if (ic == "motion-sensor-off") return MDI_MOTION_SENSOR_OFF;
  if (ic == "account" || ic == "presence" || ic == "account-outline") return MDI_ACCOUNT;
  if (ic == "run" || ic == "walk") return MDI_RUN;
  if (ic == "eye") return MDI_EYE;
  if (ic == "eye-off") return MDI_EYE_OFF;
  if (ic == "power") return MDI_POWER;
  if (ic == "power-socket" || ic == "power-plug") return MDI_POWER_SOCKET;
  if (ic == "toggle-switch") return MDI_TOGGLE_SWITCH;
  if (ic == "toggle-switch-off") return MDI_TOGGLE_SWITCH_OFF;
  if (ic == "door" || ic == "door-closed") return MDI_DOOR_CLOSED;
  if (ic == "door-open") return MDI_DOOR_OPEN;
  if (ic == "window-closed") return MDI_WINDOW_CLOSED;
  if (ic == "window-open") return MDI_WINDOW_OPEN;
  if (ic == "garage") return MDI_GARAGE;
  if (ic == "garage-open") return MDI_GARAGE_OPEN;
  if (ic == "lock") return MDI_LOCK;
  if (ic == "lock-open") return MDI_LOCK_OPEN;
  if (ic == "shield-home") return MDI_SHIELD_HOME;
  if (ic == "shield-lock") return MDI_SHIELD_LOCK;
  if (ic == "thermometer" || ic == "thermostat") return MDI_THERMOMETER;
  if (ic == "water-percent") return MDI_WATER_PERCENT;
  if (ic == "weather-sunny") return MDI_WEATHER_SUNNY;
  if (ic == "weather-partly-cloudy") return MDI_WEATHER_PARTLY_CLOUDY;
  if (ic == "weather-rainy") return MDI_WEATHER_RAINY;
  if (ic == "weather-night") return MDI_WEATHER_NIGHT;
  if (ic == "television" || ic == "tv") return MDI_TELEVISION;
  if (ic == "speaker") return MDI_SPEAKER;
  if (ic == "volume-high") return MDI_VOLUME_HIGH;
  if (ic == "volume-off") return MDI_VOLUME_OFF;
  if (ic == "play") return MDI_PLAY;
  if (ic == "pause") return MDI_PAUSE;
  if (ic == "stop") return MDI_STOP;
  if (ic == "bell") return MDI_BELL;
  if (ic == "cctv" || ic == "camera") return MDI_CCTV;
  if (ic == "battery") return MDI_BATTERY;
  if (ic == "battery-charging") return MDI_BATTERY_CHARGING;
  if (ic == "wifi") return MDI_WIFI;
  if (ic == "printer") return MDI_PRINTER;
  if (ic == "car") return MDI_CAR;
  if (ic == "flash") return MDI_FLASH;
  if (ic == "cog") return MDI_COG;
  if (ic == "home") return MDI_HOME;
  if (ic == "home-assistant") return MDI_HOME_ASSISTANT;
  if (ic == "bed") return MDI_BED;
  if (ic == "sofa") return MDI_SOFA;
  if (ic == "silverware" || ic == "silverware-fork-knife") return MDI_SILVERWARE_FORK_KNIFE;
  if (ic == "bathtub") return MDI_BATHTUB;
  if (ic == "desk") return MDI_DESK;
  if (ic == "curtains") return MDI_CURTAINS;
  if (ic == "blinds") return MDI_BLINDS;
  if (ic == "solar-power") return MDI_SOLAR_POWER;

  // 2. Keyword substring matches
  if (ic.indexOf("light") >= 0 || ic.indexOf("bulb") >= 0 || ic.indexOf("lamp") >= 0 || ic.indexOf("ceiling") >= 0 || ic.indexOf("sconce") >= 0 || ic.indexOf("chandelier") >= 0) return MDI_LIGHTBULB;
  if (ic.indexOf("fan") >= 0 || ic.indexOf("air") >= 0 || ic.indexOf("vent") >= 0 || ic.indexOf("blower") >= 0) return MDI_FAN;
  if (ic.indexOf("ac") >= 0 || ic.indexOf("climate") >= 0 || ic.indexOf("hvac") >= 0 || ic.indexOf("cool") >= 0) return MDI_AIR_CONDITIONER;
  if (ic.indexOf("heat") >= 0 || ic.indexOf("radiator") >= 0 || ic.indexOf("boiler") >= 0) return MDI_RADIATOR;
  if (ic.indexOf("motion") >= 0 || ic.indexOf("pir") >= 0) return MDI_MOTION_SENSOR;
  if (ic.indexOf("presence") >= 0 || ic.indexOf("occupancy") >= 0 || ic.indexOf("person") >= 0 || ic.indexOf("human") >= 0) return MDI_ACCOUNT;
  if (ic.indexOf("eye") >= 0) return MDI_EYE;
  if (ic.indexOf("run") >= 0 || ic.indexOf("walk") >= 0) return MDI_RUN;
  if (ic.indexOf("power") >= 0 || ic.indexOf("plug") >= 0 || ic.indexOf("outlet") >= 0 || ic.indexOf("socket") >= 0) return MDI_POWER_SOCKET;
  if (ic.indexOf("switch") >= 0) return MDI_TOGGLE_SWITCH;
  if (ic.indexOf("door") >= 0) return MDI_DOOR_CLOSED;
  if (ic.indexOf("window") >= 0) return MDI_WINDOW_CLOSED;
  if (ic.indexOf("garage") >= 0) return MDI_GARAGE;
  if (ic.indexOf("lock") >= 0) return MDI_LOCK;
  if (ic.indexOf("alarm") >= 0 || ic.indexOf("siren") >= 0 || ic.indexOf("bell") >= 0) return MDI_BELL;
  if (ic.indexOf("cam") >= 0 || ic.indexOf("cctv") >= 0) return MDI_CCTV;
  if (ic.indexOf("temp") >= 0 || ic.indexOf("therm") >= 0) return MDI_THERMOMETER;
  if (ic.indexOf("humid") >= 0 || ic.indexOf("water") >= 0 || ic.indexOf("drop") >= 0 || ic.indexOf("rain") >= 0) return MDI_WATER_PERCENT;
  if (ic.indexOf("sun") >= 0 || ic.indexOf("weather") >= 0) return MDI_WEATHER_SUNNY;
  if (ic.indexOf("tv") >= 0 || ic.indexOf("media") >= 0 || ic.indexOf("cast") >= 0) return MDI_TELEVISION;
  if (ic.indexOf("speaker") >= 0 || ic.indexOf("audio") >= 0 || ic.indexOf("sound") >= 0) return MDI_SPEAKER;
  if (ic.indexOf("battery") >= 0) return MDI_BATTERY;
  if (ic.indexOf("wifi") >= 0 || ic.indexOf("network") >= 0 || ic.indexOf("router") >= 0) return MDI_WIFI;
  if (ic.indexOf("printer") >= 0) return MDI_PRINTER;
  if (ic.indexOf("curtain") >= 0 || ic.indexOf("blind") >= 0 || ic.indexOf("shutter") >= 0) return MDI_CURTAINS;

  // 3. Entity ID Domain fallback
  String id = entityId;
  id.toLowerCase();
  if (id.startsWith("light.")) return MDI_LIGHTBULB;
  if (id.startsWith("switch.")) return MDI_TOGGLE_SWITCH;
  if (id.startsWith("fan.")) return MDI_FAN;
  if (id.startsWith("scene.")) return MDI_PLAY;
  if (id.startsWith("script.")) return MDI_PLAY;
  if (id.startsWith("climate.")) return MDI_AIR_CONDITIONER;
  if (id.startsWith("media_player.")) return MDI_TELEVISION;
  if (id.startsWith("camera.")) return MDI_CCTV;
  if (id.startsWith("lock.")) return MDI_LOCK;
  if (id.startsWith("cover.")) return MDI_CURTAINS;
  if (id.indexOf("motion") >= 0) return MDI_MOTION_SENSOR;
  if (id.indexOf("presence") >= 0 || id.indexOf("occupancy") >= 0) return MDI_ACCOUNT;
  if (id.indexOf("door") >= 0) return MDI_DOOR_CLOSED;
  if (id.indexOf("window") >= 0) return MDI_WINDOW_CLOSED;
  if (id.indexOf("temp") >= 0) return MDI_THERMOMETER;
  if (id.indexOf("humid") >= 0) return MDI_WATER_PERCENT;
  if (id.indexOf("battery") >= 0) return MDI_BATTERY;

  // 4. Widget Type fallback
  switch (type) {
    case WidgetType::TOGGLE: return MDI_POWER;
    case WidgetType::SENSOR: return MDI_THERMOMETER;
    case WidgetType::BINARY_SENSOR: return MDI_MOTION_SENSOR;
  }
  return MDI_POWER;
}
