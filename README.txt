Changes I needed to make to Libraries:

ESP32-audioI2S-master
=====================
Audio.cpp - commented out setInsecure() line:
Audio::Audio(const uint8_t BCLK, const uint8_t LRC, const uint8_t DOUT) {
   //clientsecure.setInsecure();  // if that can't be resolved update to ESP32 Arduino version 1.0.5-rc05 or higher

LittleFS_esp32
==============
esp_littlefs.c - uncommented following line:
#define CONFIG_LITTLEFS_FOR_IDF_3_2      /* For old IDF - like in release 1.0.4 */

TFT_eSPI
========
Changes to User_Setup.h for GPIO information (comment out NodeMCU settings and make sure ESP32 setting set)

// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
//#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

#define TOUCH_CS 21     // Chip select pin (T_CS) of touch screen

Other Notes
===========
Show timestamp on serial monitor:
pio device monitor -f time
