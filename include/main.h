// Ensure this header file is included only once
#ifndef _ESP32I2SRadio_
#define _ESP32I2SRadio_

// ===================== WiFi ============================
// Standard ESP32 WiFi (not secure https)
#include <WiFi.h>

std::string ssid;
std::string wifiPassword;
bool wiFiDisconnected = true;

// WiFi helper functions
void connectToWifi();
std::string getWiFiPassword();
std::string getSSID();
const char *wl_status_to_string(wl_status_t status);
// =======================================================

// ===================== LittleFS ========================
// SPIFFS (in-memory SPI File System) replacement
#include <LITTLEFS.h>

// LITTLEFS foward declarations
std::string readLITTLEFSInfo(const char *configfile, const char *itemRequired);
void mountLITTLEFS();
// =======================================================

// ===================== Touch Screen ====================
// Touch Screen Display
#include <TFT_eSPI.h> // Hardware-specific library

// TFT Display forward declarations
void displaySetup();
void touch_calibrate();
void layoutScreen();
void displayStationName(char *stationName);
void displayTrackArtist(std::string);
void createButtons();
void displayStatusInfo(char *information);
void setScreenBrightness(int);
void displayMuteOff();
void displayMuteOn();
void displayWiFiOff();
void displayWiFiOn();
void displayBufferInactive();
void displayBufferRed();
void displayBufferAmber();
void displayBufferGreen();
void displayVolumeUp();
void displayVolumeUpPressed();
void displayVolumeDown();
void displayVolumeDownPressed();
void displayChannelUp();
void displayChannelUpPressed();
void displayChannelDown();
void displayChannelDownPressed();
void displayBrightnessUp();
void displayBrightnessUpPressed();
void displayBrightnessDown();
void displayBrightnessDownPressed();
void displaySettings();
void displaySettingsPressed();
void clearBitRate();

// Instantiate screen (object) using hardware SPI. Defaults to 320H x 240W
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
// =======================================================

// ===================== Stations ====================
// Utility to write to (psuedo) EEPROM
#include <Preferences.h>

// Stations forward declarations
void loadStation();
void changeStation(int8_t upOrDown);
//void connectToStation();
const char *getFriendlyStationName();

// EEPROM writing routines (eg: remembers previous radio stn)
Preferences preferences;
// =======================================================

#endif
