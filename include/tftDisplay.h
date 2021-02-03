
// All of the ILI9341 TFT Touch Screen routines are included here

// Used to create critical sessions to ensure some screen updates are not preempted (leaving screen partially updated)
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Calibration data is stored in SPIFFS so we need to include it
#include "FS.h"

#include <SPI.h>

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

TFT_eSPI_Button prevBtn, nextBtn, volDownBtn, volUpBtn, muteBtn;
TFT_eSPI_Button brightnessDownBtn, brightnessUpBtn;

// This is the file name used to store the touch coordinate
// calibration data. Cahnge the name to start a new calibration.
#define CALIBRATION_FILE "/TouchCalData1.txt"

// Set REPEAT_CALIBRATION to true instead of false to run calibration 
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CALIBRATION false

// Define PWM pin for LED screen brigthness control
#define TFT_LEDPIN 32 // GPIO 32 - could potentially use different GPIO pin

// Forward declarations
void displaySetup();
void touch_calibrate();
void layoutScreen();
void displayStationName(char *stationName);
void displayTrackArtist(std::string);
void createButtons();
void displayStatusInfo(char *information);
void setScreenBrightness(int);

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

  // Border + fill
  tft.fillRect(0, 40, 320, 2, TFT_WHITE);

  // Application Title
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextSize(1);
  tft.setCursor(12, 30);
  tft.setTextColor(TFT_ORANGE);
  tft.println("APA RADIO"); 

  tft.fillRect(0, 190, 320, 2, TFT_WHITE);
  
  createButtons();
}
/*
void layoutScreen()
{
  // Write title of app on screen, using font 2 (x, y, font #)
  tft.fillScreen(TFT_BLACK);

  // Border + fill
  tft.fillRect(1, 1, 318, 43, TFT_RED);
  tft.drawRect(0, 0, 320, 45, TFT_YELLOW);

  // Application Title
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextSize(1);
  tft.setCursor(14, 30);

  // Set text colour and background
  tft.setTextColor(TFT_YELLOW, TFT_RED);
  tft.println("- APA INTERNET RADIO -"); 

  createButtons();
}
*/

void createButtons()
{

  prevBtn.initButtonUL( &tft, 
    0, // X
    200, // Y
    50, // Width
    40, // Height
    TFT_BLACK, // Outline colour
    TFT_YELLOW, // Fill colour
    TFT_RED, // Text colour
    (char *)"<", // Label
    1 // Text size
    );
      
  nextBtn.initButtonUL( &tft, 
    60, // X
    200, // Y
    50, // Width
    40, // Height
    TFT_BLACK, // Outline colour
    TFT_YELLOW, // Fill colour
    TFT_RED, // Text colour
    (char *)">", // Label
    1 // Text size
    );

  muteBtn.initButtonUL( &tft, 
    135, // X
    200, // Y
    50, // Width
    40, // Height
    TFT_BLACK, // Outline colour
    TFT_YELLOW, // Fill colour
    TFT_RED, // Text colour
    (char *)"M", // Label
    1 // Text size
    );
    
  volDownBtn.initButtonUL( &tft, 
    210, // X
    200, // Y
    50, // Width
    40, // Height
    TFT_BLACK, // Outline colour
    TFT_YELLOW, // Fill colour
    TFT_RED, // Text colour
    (char *)"-", // Label
    1 // Text size
    );

  volUpBtn.initButtonUL( &tft, 
    270, // X
    200, // Y
    50, // Width
    40, // Height
    TFT_BLACK, // Outline colour
    TFT_YELLOW, // Fill colour
    TFT_RED, // Text colour
    (char *)"+", // Label
    1 // Text size
    );
  
  brightnessDownBtn.initButtonUL( &tft, 
    210, // X
    140, // Y
    50, // Width
    40, // Height
    TFT_BLACK, // Outline colour
    TFT_YELLOW, // Fill colour
    TFT_RED, // Text colour
    (char *)"v", // Label
    1 // Text size
    );

  brightnessUpBtn.initButtonUL( &tft, 
    270, // X
    140, // Y
    50, // Width
    40, // Height
    TFT_BLACK, // Outline colour
    TFT_YELLOW, // Fill colour
    TFT_RED, // Text colour
    (char *)"^", // Label
    1 // Text size
    );

  prevBtn.drawButton();
  nextBtn.drawButton();
  muteBtn.drawButton();
  volDownBtn.drawButton();
  volUpBtn.drawButton();

  brightnessDownBtn.drawButton();
  brightnessUpBtn.drawButton();
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
    tft.println(getFriendlyName());
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
}

void displayBitRate(const char *bitrate)
{
  portENTER_CRITICAL(&mux);
  // Set text colour and background
  tft.setTextColor(TFT_SILVER, TFT_BLACK);

  // Clear the remainder of the line from before
  //tft.fillRect(0, 170, 320, 20, TFT_BLACK);
  tft.fillRect(0, 170, 200, 20, TFT_BLACK);

  // Write buffer percentage
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 185);
  tft.print(bitrate);
  tft.print(" bps");
  portEXIT_CRITICAL(&mux); 
}

void displayStatusInfo(const char *information)
{
  // Set text colour and background
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  // Clear the remainder of the line from before (eg long title)
  tft.fillRect(0, 90, 320, 50, TFT_BLACK);

  // Write artist / track info
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 110);
  tft.print(information);
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

bool getPrevButtonPress(uint16_t t_x, uint16_t t_y)
{
  if (prevBtn.contains(t_x, t_y))
  {
    prevBtn.drawButton(true);
    return true;
  }
  return false;
}

bool getNextButtonPress(uint16_t t_x, uint16_t t_y)
{
  if (nextBtn.contains(t_x, t_y))
  {
    nextBtn.drawButton(true);
    return true;
  }
  return false;
}

bool getVolDownButtonPress(uint16_t t_x, uint16_t t_y)
{
  if (volDownBtn.contains(t_x, t_y))
  {
    volDownBtn.drawButton(true);
    return true;
  }
  return false;
}

bool getVolUpButtonPress(uint16_t t_x, uint16_t t_y)
{
  if (volUpBtn.contains(t_x, t_y))
  {
    volUpBtn.drawButton(true);
    return true;
  }
  return false;
}

bool getBrightnessDownButtonPress(uint16_t t_x, uint16_t t_y)
{
  if (brightnessDownBtn.contains(t_x, t_y))
  {
    brightnessDownBtn.drawButton(true);
    return true;
  }
  return false;
}

bool getBrightnessUpButtonPress(uint16_t t_x, uint16_t t_y)
{
  if (brightnessUpBtn.contains(t_x, t_y))
  {
    brightnessUpBtn.drawButton(true);
    return true;
  }
  return false;
}

bool getMuteButtonPress(uint16_t t_x, uint16_t t_y)
{
  if (muteBtn.contains(t_x, t_y))
  {
    muteBtn.drawButton(true);
    return true;
  }
  return false;
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
  if (currentBrightness < 0)
    currentBrightness = 0;

  setScreenBrightness(currentBrightness);
}