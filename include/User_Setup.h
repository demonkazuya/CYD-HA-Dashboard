/**
 * @file User_Setup.h
 * @brief BambuDeck CYD Display & Hardware SPI Configuration
 * 
 * Hardware Target: ESP32-2432S028R (CYD 2.8" TFT with resistive touch)
 * Display Controller: ILI9341
 * Touch Controller: XPT2046
 */

#ifndef _BAMBUDECK_USER_SETUP_H_
#define _BAMBUDECK_USER_SETUP_H_

#define USER_SETUP_INFO "BambuDeck CYD ESP32-2432S028R"

/* Display Controller */
#define ILI9341_2_DRIVER

/* Dimensions (Portrait native, rotated via software to landscape) */
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

/* Display SPI Hardware Pin Mapping */
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  12

/* Backlight Control */
#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH

/* Touch Controller (XPT2046) Chip Select */
#define TOUCH_CS 33

/* Bus Clock Speeds */
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

/* Fonts */
#define LOAD_GLCD   // Font 1. Standard 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel font
#define LOAD_FONT4  // Font 4. Medium 26 pixel font
#define LOAD_FONT6  // Font 6. Large 48 pixel numeric
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font
#define LOAD_FONT8  // Font 8. Large 75 pixel font
#define LOAD_GFXFF  // FreeFonts
#define SMOOTH_FONT

#endif /* _BAMBUDECK_USER_SETUP_H_ */
