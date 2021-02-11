// All of the ILI9341 TFT Touch Screen routines are included here
#include <arduino.h>
#include "main.h"
#include "bitmapHelper.h"

// Used to create critical sessions to ensure some screen updates are not preempted (leaving screen partially updated)
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

TFT_eSPI_Button prevBtn, nextBtn, volDownBtn, volUpBtn, muteBtn;
TFT_eSPI_Button brightnessDownBtn, brightnessUpBtn;
TFT_eSPI_Button settingsBtn;

// This is the file name used to store the touch coordinate
// calibration data. Cahnge the name to start a new calibration.
#define CALIBRATION_FILE "/TouchCalData1.txt"

// Set REPEAT_CALIBRATION to true instead of false to run calibration 
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CALIBRATION false

// Define PWM pin for LED screen brigthness control
#define TFT_LEDPIN 32 // GPIO 32 - could potentially use different GPIO pin

// WiFi icon
#define WIFI_ICON_X 0
#define WIFI_ICON_Y 7

// Buffer icon
#define BUFFER_ICON_X 28
#define BUFFER_ICON_Y 7

// Buffer icon
#define BITRATE_LOCATION_X 60
#define BITRATE_LOCATION_Y 28

// Radio Title
#define TITLE_LOCATION_X 105
#define TITLE_LOCATION_Y 30

// Mute button / icon
#define MUTE_BUTTON_X 135
#define MUTE_BUTTON_Y 200
#define MUTE_ICON_X 145
#define MUTE_ICON_Y 205

// Volume down button / icon
#define VOLUMEDOWN_BUTTON_X 210
#define VOLUMEDOWN_BUTTON_Y 200
#define VOLUMEDOWN_ICON_X 220
#define VOLUMEDOWN_ICON_Y 205

// Volume up button / icon
#define VOLUMEUP_BUTTON_X 270
#define VOLUMEUP_BUTTON_Y 200
#define VOLUMEUP_ICON_X 280
#define VOLUMEUP_ICON_Y 205

// Channel down (prev) button / icon
#define CHANNELDOWN_BUTTON_X 0
#define CHANNELDOWN_BUTTON_Y 200
#define CHANNELDOWN_ICON_X 10
#define CHANNELDOWN_ICON_Y 205

// Channel up (next) button / icon
#define CHANNELUP_BUTTON_X 60
#define CHANNELUP_BUTTON_Y 200
#define CHANNELUP_ICON_X 70
#define CHANNELUP_ICON_Y 205

// Brightness down button / icon
#define BRIGHTNESSDOWN_BUTTON_X 210
#define BRIGHTNESSDOWN_BUTTON_Y 140
#define BRIGHTNESSDOWN_ICON_X 220
#define BRIGHTNESSDOWN_ICON_Y 145

// Brightness up button / icon
#define BRIGHTNESSUP_BUTTON_X 270
#define BRIGHTNESSUP_BUTTON_Y 140
#define BRIGHTNESSUP_ICON_X 280
#define BRIGHTNESSUP_ICON_Y 145

// Settings button / icon
#define SETTINGS_BUTTON_X 150
#define SETTINGS_BUTTON_Y 140
#define SETTINGS_ICON_X 160
#define SETTINGS_ICON_Y 145

int currentBrightness;

void displaySetup()
{
  tft.init();

  // Set rotation as required before calibration, the touch calibration MUST match this
  // 0 & 2 Portrait. 1 & 3 landscape
  tft.setRotation(3);

  // call screen calibration
  touch_calibrate();

  // Setup PWM for screen brightness control
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_LEDPIN, 0);

  int prevBrightness = preferences.getUInt("Brightness", 255);
  if (prevBrightness < 20)
  {
    Serial.printf("TFT brightness level increased from %d to %d", prevBrightness, 20);
    prevBrightness = 20;
  }
  currentBrightness = prevBrightness;
  setScreenBrightness(currentBrightness);

  // Finally layout the screen
  layoutScreen();
}


void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  mountLITTLEFS();

  // check if calibration file exists and size is correct
  if (LITTLEFS.exists(CALIBRATION_FILE))
  {
    if (REPEAT_CALIBRATION)
    {
      // Delete existing calibration file as we want to re-calibrate
      LITTLEFS.remove(CALIBRATION_FILE);
      Serial.println("Deleted touch calibration file");
    }
    else
    {
      // Open existing calibration file
      File f = LITTLEFS.open(CALIBRATION_FILE, "r");
      if (f)
      {
        if (f.readBytes((char *)calData, 14) == 14)
        {
          calDataOK = 1;
          Serial.println("Successfully read touch calibration file");
        }
        f.close();
      }
      // Dump calData to serial
      Serial.println("Dumping calData:");
      for (int i = 0; i < 5; i++)
      {
        Serial.printf("%04X, ", calData[i]);
      }
      Serial.println("");
    }
  }

  if (calDataOK && !REPEAT_CALIBRATION)
  {
    // calibration touch with existing data
    tft.setTouch(calData);
    Serial.println("Applied existing touch calibration");
  }
  else
  {
    // data not valid so recalibrate
    Serial.println("Need to re-calibrate");
    
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CALIBRATION)
    {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CALIBRATION to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store calibration data
    File f = LITTLEFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      f.write((const unsigned char *)calData, 14);
      f.close();
      Serial.println("Saved touch calibration data");
    }
    // Dump calData to serial
    Serial.println("Dumping calData:");
    for (int i = 0; i < 5; i++)
    {
      Serial.printf("%04X, ", calData[i]);
    }
    Serial.println("");
  }
}

void layoutScreen()
{
  // Write title of app on screen, using font 2 (x, y, font #)
  tft.fillScreen(TFT_BLACK);

  // Top divider
  tft.fillRect(0, 40, 320, 2, TFT_WHITE);

  // Application Title
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextSize(1);
  tft.setCursor(TITLE_LOCATION_X, TITLE_LOCATION_Y);
  tft.setTextColor(TFT_ORANGE);
  tft.println("APA RADIO");

  // Bottom divider
  tft.fillRect(0, 190, 320, 2, TFT_WHITE);

  createButtons();

  displayWiFiOff();
  displayBufferInactive();
  displayMuteOff();
  displayVolumeUp();
  displayVolumeDown();
  displayChannelUp();
  displayChannelDown();
  displayBrightnessUp();
  displayBrightnessDown();
  displaySettings();
}

void createButtons()
{

  prevBtn.initButtonUL( &tft, 
    CHANNELDOWN_BUTTON_X, // X
    CHANNELDOWN_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );
      
  nextBtn.initButtonUL( &tft, 
    CHANNELUP_BUTTON_X, // X
    CHANNELUP_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );

  muteBtn.initButtonUL( &tft, 
    MUTE_BUTTON_X, // X
    MUTE_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );
    
  volDownBtn.initButtonUL( &tft, 
    VOLUMEDOWN_BUTTON_X, // X
    VOLUMEDOWN_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );

  volUpBtn.initButtonUL( &tft, 
    VOLUMEUP_BUTTON_X, // X
    VOLUMEUP_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );
  
  brightnessDownBtn.initButtonUL( &tft, 
    BRIGHTNESSDOWN_BUTTON_X, // X
    BRIGHTNESSDOWN_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );

  brightnessUpBtn.initButtonUL( &tft, 
    BRIGHTNESSUP_BUTTON_X, // X
    BRIGHTNESSUP_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );

  settingsBtn.initButtonUL( &tft, 
    SETTINGS_BUTTON_X, // X
    SETTINGS_BUTTON_Y, // Y
    50, // Width
    40, // Height
    TFT_YELLOW, // Outline colour
    TFT_BLACK, // Fill colour
    TFT_BLACK, // Text colour
    (char *)"", // Label
    1 // Text size
    );

  prevBtn.drawButton();
  nextBtn.drawButton();
  muteBtn.drawButton();
  volDownBtn.drawButton();
  volUpBtn.drawButton();

  brightnessDownBtn.drawButton();
  brightnessUpBtn.drawButton();
  settingsBtn.drawButton();
}

void displayStationName(const char *stationName)
{
  portENTER_CRITICAL(&mux);  
  // Set text colour and background
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  // Clear the remainder of the line from before (eg long title)
  tft.fillRect(0, 50, 320, 40, TFT_BLACK);

  // Write station name
  tft.setCursor(0, 75);
  tft.setFreeFont(&FreeSansOblique12pt7b);
  tft.setTextSize(1);
  if (strlen(stationName) == 0)
  {
    // Station name not broadcast therefore use configured name
    tft.println(getFriendlyStationName());
  }
  else
  {
    tft.println(stationName);
  }
  portEXIT_CRITICAL(&mux);
}

void displayTrackArtist(const char *trackArtist)
{
  portENTER_CRITICAL(&mux); 
  // Set text colour and background
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  // Clear the remainder of the line from before (eg long title)
  tft.fillRect(0, 90, 320, 50, TFT_BLACK);

  // Write artist / track info
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 110);
  tft.print(trackArtist);
  portEXIT_CRITICAL(&mux); 
}

void displayBuffer(uint16_t bufferpercentage)
{
  portENTER_CRITICAL(&mux);

  // Set text colour and background
  if (bufferpercentage > 70)
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  if (bufferpercentage <= 70 && bufferpercentage > 50)
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  if (bufferpercentage <= 50)
    tft.setTextColor(TFT_RED, TFT_BLACK);    

  //tft.fillRect(0, 140, 320, 20, TFT_BLACK);
  tft.fillRect(0, 140, 50, 20, TFT_BLACK);

  // Write buffer percentage
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 155);
  tft.print(bufferpercentage);
  tft.print("%");

  portEXIT_CRITICAL(&mux);
}

void displayStatusInfo(const char *information)
{
  portENTER_CRITICAL(&mux);

  // Set text colour and background
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  // Clear the remainder of the line from before (eg long title)
  tft.fillRect(0, 90, 320, 50, TFT_BLACK);

  // Write artist / track info
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 110);
  tft.print(information);

  portEXIT_CRITICAL(&mux);
}

void displayClock(const char *time)
{
  portENTER_CRITICAL(&mux);

  tft.fillRect(250, 5, 320, 30, TFT_BLACK); // Clear previous time
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextSize(1);
  tft.setCursor(250, 30);
  tft.setTextColor(TFT_ORANGE);
  tft.print(time);

  portEXIT_CRITICAL(&mux);
}

// Not really used yet - could be used when radio not on
void displayLargeClock(const char *time)
{
  portENTER_CRITICAL(&mux);

  tft.fillRect(80, 90, 140, 70, TFT_BLACK); // Clear previous time
  tft.setFreeFont(&FreeSerifBold24pt7b);
  tft.setTextSize(1);
  tft.setCursor(90, 130);
  tft.setTextColor(TFT_ORANGE);
  tft.print(time);

  portEXIT_CRITICAL(&mux);
}

bool isPrevButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (prevBtn.contains(t_x, t_y))
  {
    prevBtn.drawButton(true);
    return true;
  }
  return false;
}

bool isNextButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (nextBtn.contains(t_x, t_y))
  {
    nextBtn.drawButton(true);
    return true;
  }
  return false;
}

bool isVolDownButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (volDownBtn.contains(t_x, t_y))
  {
    return true;
  }
  return false;
}

bool isVolUpButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (volUpBtn.contains(t_x, t_y))
  {
    return true;
  }
  return false;
}

bool isBrightnessDownButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (brightnessDownBtn.contains(t_x, t_y))
  {
    return true;
  }
  return false;
}

bool isBrightnessUpButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (brightnessUpBtn.contains(t_x, t_y))
  {
    return true;
  }
  return false;
}

bool isMuteButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (muteBtn.contains(t_x, t_y))
  {
    return true;
  }
  return false;
}

bool isSettingsButtonPressed(uint16_t t_x, uint16_t t_y)
{
  if (settingsBtn.contains(t_x, t_y))
  {
    return true;
  }
  return false;
}

void displayMuteOn()
{
  drawBmp("/speaker-off.bmp", MUTE_ICON_X, MUTE_ICON_Y);  
}

void displayMuteOff()
{
    drawBmp("/speaker-on.bmp", MUTE_ICON_X, MUTE_ICON_Y);
}

void displayWiFiOn()
{
  drawBmp("/wifi-active.bmp", WIFI_ICON_X, WIFI_ICON_Y);  
}

void displayWiFiOff()
{
    drawBmp("/wifi-inactive.bmp", WIFI_ICON_X, WIFI_ICON_Y);
}

void displayVolumeUp()
{
  drawBmp("/volume-up.bmp", VOLUMEUP_ICON_X, VOLUMEUP_ICON_Y);  
}

void displayVolumeUpPressed()
{
  drawBmp("/volume-up-pressed.bmp", VOLUMEUP_ICON_X, VOLUMEUP_ICON_Y);  
}

void displayVolumeDown()
{
    drawBmp("/volume-down.bmp", VOLUMEDOWN_ICON_X, VOLUMEDOWN_ICON_Y);
}

void displayVolumeDownPressed()
{
  drawBmp("/volume-down-pressed.bmp", VOLUMEDOWN_ICON_X, VOLUMEDOWN_ICON_Y);  
}

void displayChannelUp()
{
  drawBmp("/channel-up.bmp", CHANNELUP_ICON_X, CHANNELUP_ICON_Y);  
}

void displayChannelUpPressed()
{
  drawBmp("/channel-up-pressed.bmp", CHANNELUP_ICON_X, CHANNELUP_ICON_Y);  
}

void displayChannelDown()
{
  drawBmp("/channel-down.bmp", CHANNELDOWN_ICON_X, CHANNELDOWN_ICON_Y);  
}

void displayChannelDownPressed()
{
  drawBmp("/channel-down-pressed.bmp", CHANNELDOWN_ICON_X, CHANNELDOWN_ICON_Y);  
}

void displayBufferInactive()
{
  drawBmp("/buffer-inactive.bmp", BUFFER_ICON_X, BUFFER_ICON_Y);  
}

void displayBrightnessUp()
{
  drawBmp("/brightness-up.bmp", BRIGHTNESSUP_ICON_X, BRIGHTNESSUP_ICON_Y);  
}

void displayBrightnessUpPressed()
{
  drawBmp("/brightness-up-pressed.bmp", BRIGHTNESSUP_ICON_X, BRIGHTNESSUP_ICON_Y);  
}

void displayBrightnessDown()
{
  drawBmp("/brightness-down.bmp", BRIGHTNESSDOWN_ICON_X, BRIGHTNESSDOWN_ICON_Y);  
}

void displayBrightnessDownPressed()
{
  drawBmp("/brightness-down-pressed.bmp", BRIGHTNESSDOWN_ICON_X, BRIGHTNESSDOWN_ICON_Y);  
}

void displaySettings()
{
  drawBmp("/settings.bmp", SETTINGS_ICON_X, SETTINGS_ICON_Y);  
}

void displaySettingsPressed()
{
  drawBmp("/settings-pressed.bmp", SETTINGS_ICON_X, SETTINGS_ICON_Y);  
}

void displayBufferRed()
{
  drawBmp("/buffer-red.bmp", BUFFER_ICON_X, BUFFER_ICON_Y);  
}

void displayBufferAmber()
{
  drawBmp("/buffer-amber.bmp", BUFFER_ICON_X, BUFFER_ICON_Y);  
}

void displayBufferGreen()
{
  drawBmp("/buffer-green.bmp", BUFFER_ICON_X, BUFFER_ICON_Y);  
}

void displayBitRate(const char *bitrate)
{
  portENTER_CRITICAL(&mux);

  // Set text colour and background
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

  // Clear the remainder of the line from before
  tft.fillRect(BITRATE_LOCATION_X, BITRATE_LOCATION_Y - 15, 40, 20, TFT_BLACK);

  // Write buffer percentage
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(BITRATE_LOCATION_X, BITRATE_LOCATION_Y);
  tft.printf("%.3sk", bitrate); // display at most 3 digits (might fail for 64k!)

  portEXIT_CRITICAL(&mux); 
}

void clearBitRate()
{
  tft.fillRect(BITRATE_LOCATION_X, BITRATE_LOCATION_Y - 15, 40, 20, TFT_BLACK);
}

// 0 (off) to 255(bright) duty cycle
void setScreenBrightness(int brightness)
{
  Serial.printf("Screen brightness set to %d\n", brightness);
  ledcWrite(0, brightness);
}

void incrementScreenBrightness()
{
  currentBrightness += 20;
  if (currentBrightness > 255)
    currentBrightness = 255;

  setScreenBrightness(currentBrightness);
}

void decrementScreenBrightness()
{
  currentBrightness -= 20;
  if (currentBrightness < 5)
    currentBrightness = 5;

  setScreenBrightness(currentBrightness);
}