/**
 * @file lv_conf.h
 * @brief BambuDeck LVGL v8 Configuration for ESP32
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0
#define LV_COLOR_SCREEN_TRANSP  0
#define LV_COLOR_MIX_ROUND_OFS  128
#define LV_COLOR_CHROMA_KEY     lv_color_hex(0x00FF00)

/*====================
   MEMORY SETTINGS
 *====================*/
/* Use standard system heap (malloc/free/realloc) */
#define LV_MEM_CUSTOM           1
#define LV_MEM_CUSTOM_INCLUDE   <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC     malloc
#define LV_MEM_CUSTOM_FREE      free
#define LV_MEM_CUSTOM_REALLOC   realloc
#define LV_MEM_BUF_MAX_NUM      16
#define LV_MEMCPY_MEMSET_STD    0

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD 25   /* [ms] Screen refresh period */
#define LV_INDEV_DEF_READ_PERIOD 40  /* [ms] Touch input read period */
#define LV_DPI_DEF              130  /* [px/inch] */

/* System Tick: Arduino millis() */
#define LV_TICK_CUSTOM          1
#define LV_TICK_CUSTOM_INCLUDE  "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

/*====================
   FEATURE CONFIG
 *====================*/
#define LV_DRAW_COMPLEX         1
#define LV_CIRCLE_CACHE_SIZE    4
#define LV_GRADIENT_MAX_STOPS   2
#define LV_DISP_ROT_MAX_BUF     (10 * 1024)

#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER       while(1);

#define LV_USE_USER_DATA        1

/*====================
   FONTS
 *====================*/
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_26   1
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_36   1
#define LV_FONT_MONTSERRAT_40   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14
#define LV_USE_FONT_COMPRESSED  1

/*====================
   TEXT CONFIG
 *====================*/
#define LV_TXT_ENC              LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS      " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD        "#"

/*====================
   WIDGETS
 *====================*/
#define LV_USE_ARC              1
#define LV_USE_BAR              1
#define LV_USE_BTN              1
#define LV_USE_BTNMATRIX        1
#define LV_USE_CANVAS           1
#define LV_USE_CHECKBOX         1
#define LV_USE_DROPDOWN         1
#define LV_USE_IMG              1
#define LV_USE_LABEL            1
#define LV_LABEL_LONG_TXT_HINT  1
#define LV_USE_LINE             1
#define LV_USE_ROLLER           1
#define LV_ROLLER_INF_PAGES     7
#define LV_USE_SLIDER           1
#define LV_USE_SWITCH           1
#define LV_USE_TEXTAREA         1
#define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#define LV_USE_TABLE            1
#define LV_USE_KEYBOARD         1

/*====================
   EXTRA COMPONENTS
 *====================*/
#define LV_USE_MSG              1
#define LV_USE_FLEX             1
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_GROW   1
#define LV_THEME_DEFAULT_TRANSITION_TIME 80

/* Image Decoders */
#define LV_USE_PNG              1
#define LV_USE_BMP              1
#define LV_USE_SJPG             1

#endif /* LV_CONF_H */
