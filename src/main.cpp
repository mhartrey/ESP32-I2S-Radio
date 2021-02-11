#include <Arduino.h>
#include "main.h"

// Audio Tasks
#include "AudioTask.h"

// Config stored in LITTLEFS
#include "littleFSHelpers.h"

// Include Wifi routines
#include "wifiHelpers.h"

// Programmed Radio stations
#include "stations.h"

// TFT Touch Screen routines
#include "tftDisplay.h"

// Clock routines
#include "clock.h"

// Digital I2S I/O pins used by PCM5102
#define I2S_DOUT      25  // DIN connection
#define I2S_BCLK      27  // Bit clock
#define I2S_LRC       26  // Left Right Clock
// FLT, DMP, SCL and LOW pulled low

// Forward Declarations
void checkForBrightnessChange();
void toggleMute();
void clearPressedButtons();
void checkForScreenPress();
bool checkForMutePressed(uint16_t x, uint16_t y);
bool checkForVolumeDownPressed(uint16_t x, uint16_t y);
bool checkForVolumeUpPressed(uint16_t x, uint16_t y);
bool checkForChannelDownPressed(uint16_t x, uint16_t y);
bool checkForChannelUpPressed(uint16_t x, uint16_t y);
bool checkForBrightnessDownPressed(uint16_t x, uint16_t y);
bool checkForBrightnessUpPressed(uint16_t x, uint16_t y);
bool checkForSettingsPressed(uint16_t x, uint16_t y);
void resetDisplayBuffer();
void calculateDisplayBuffer();

unsigned long buttonLastPressed = millis();
bool buttonPressed = false;
int buffersample = 0;
int buffervalue = 0;

int pressedButton = 0;
#define BUTTON_CHANNEL_DOWN_PRESSED 1
#define BUTTON_CHANNEL_UP_PRESSED 2
#define BUTTON_VOLUME_DOWN_PRESSED 4
#define BUTTON_VOLUME_UP_PRESSED 8
#define BUTTON_BRIGHTNESS_DOWN_PRESSED 16
#define BUTTON_BRIGHTNESS_UP_PRESSED 32
#define BUTTON_SETTINGS_PRESSED 64

void setup()
{
  Serial.begin(115200);

  displaySetup();

  // Connect to WiFi - no point continuing until connected
  do
  {
    displayStatusInfo("Connecting to WiFi...");
    connectToWifi();
    if (WiFi.status() != WL_CONNECTED)
    {
      displayStatusInfo("Failed to connect to WiFi");
      vTaskDelay (5000 / portTICK_PERIOD_MS);
    }
  } while (WiFi.status() != WL_CONNECTED);
  displayStatusInfo("");
  
  // Start task to retrieve NTP time
  createDisplayClockTask();
  
  // Start independent task to play audio
  createAudioMusicTask();
    
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(currentVolume);
 
  // Load list of Radio Stations
  loadRadioStations();

  // Load the last played station
  loadStation();

  // Allow audio task to run
  allowPlayAudio = true;
}


void loop()
{
  delay(200);
  checkForScreenPress();
  if (buttonPressed && (millis() > buttonLastPressed + 300))
  {
    clearPressedButtons();
    buttonPressed = false;
  }

  calculateDisplayBuffer();
}


// Audio Callbacks
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
    displayStationName(info);
}
void audio_showstreaminfo(const char *info){
    Serial.print("streaminfo  ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
    displayTrackArtist(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
    displayBitRate(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
void audio_eof_speech(const char *info){
    Serial.print("eof_speech  ");Serial.println(info);
}

void checkForScreenPress()
{
  // X/Y coordinates of any screen press
  uint16_t x, y;

  // See if there's any touch data for us
  if (tft.getTouch(&x, &y))
  {
    // Screen pressed
 
    // Check for Mute pressed
    if (checkForMutePressed(x, y))
    {
      return;
    }
    else if (checkForVolumeDownPressed(x, y))
    {
      pressedButton |= BUTTON_VOLUME_DOWN_PRESSED;
      return;
    }
    else if (checkForVolumeUpPressed(x, y))
    {
      pressedButton |= BUTTON_VOLUME_UP_PRESSED;
      return;
    }
    else if (checkForChannelDownPressed(x, y))
    {
      pressedButton |= BUTTON_CHANNEL_DOWN_PRESSED;
      return;
    }    
    else if (checkForChannelUpPressed(x, y))
    {
      pressedButton |= BUTTON_CHANNEL_UP_PRESSED;
      return;
    }
    else if (checkForBrightnessDownPressed(x, y))
    {
      pressedButton |= BUTTON_BRIGHTNESS_DOWN_PRESSED;
      return;
    }
    else if (checkForBrightnessUpPressed(x, y))
    {
      pressedButton |= BUTTON_BRIGHTNESS_UP_PRESSED;
      return;
    }
    else if (checkForSettingsPressed(x, y))
    {
      pressedButton |= BUTTON_SETTINGS_PRESSED;
      return;
    }    
    else
    {
      Serial.println("No matching buttons pressed");
      pressedButton = 0;
      return;
    }
  }
  return;
}

boolean checkForMutePressed(uint16_t x, uint16_t y)
{
  // Has the Mute button been pressed?
  if (isMuteButtonPressed(x, y))
  {
    Serial.println("Mute pressed");
    toggleMute();
    return true;
  }
  return false;
}

boolean checkForVolumeDownPressed(uint16_t x, uint16_t y)
{
  // Has the Volume Down button been pressed?
  if (isVolDownButtonPressed(x, y))
  {
    if (currentVolume > 0)
      currentVolume--;
    Serial.printf("Volume Down pressed, volume = %d\n", currentVolume);
    audio.setVolume(currentVolume);
    displayVolumeDownPressed();
    displayMuteOff();
    buttonPressed = true;
    buttonLastPressed = millis();
    return true;
  }
  return false;
}

boolean checkForVolumeUpPressed(uint16_t x, uint16_t y)
{
  // Has the Volume Up button been pressed?
  if (isVolUpButtonPressed(x, y))
  {
    if (currentVolume < maxVolume)
      currentVolume++;
    Serial.printf("Volume Up pressed, volume = %d\n", currentVolume);
    audio.setVolume(currentVolume);
    displayVolumeUpPressed();
    displayMuteOff();
    buttonPressed = true;
    buttonLastPressed = millis();
    return true;
  }
  return false;
}

boolean checkForBrightnessDownPressed(uint16_t x, uint16_t y)
{
  // Has the Brightness Down button been pressed?
  if (isBrightnessDownButtonPressed(x, y))
  {
    Serial.printf("Brightness Down pressed\n");
    displayBrightnessDownPressed();
    decrementScreenBrightness();

    buttonPressed = true;
    buttonLastPressed = millis();
    return true;
  }
  return false;
}

boolean checkForBrightnessUpPressed(uint16_t x, uint16_t y)
{
  // Has the Brightness Down button been pressed?
  if (isBrightnessUpButtonPressed(x, y))
  {
    Serial.printf("Brightness Up pressed\n"); 
    displayBrightnessUpPressed();
    incrementScreenBrightness();

    buttonPressed = true;
    buttonLastPressed = millis();
    return true;
  }
  return false;
}

boolean checkForChannelDownPressed(uint16_t x, uint16_t y)
{
  // Has the Channel Down (prev) Button been pressed?
  if (isPrevButtonPressed(x, y))
  {
    Serial.println("Channel Down (prev) Button Pressed");
    changeStation(-1);
    resetDisplayBuffer();
    clearBitRate();

    // Clear existing track information after changing channel
    displayTrackArtist("");

    displayChannelDownPressed();
    buttonPressed = true;
    buttonLastPressed = millis();
    return true;
  }
  return false;
}

boolean checkForChannelUpPressed(uint16_t x, uint16_t y)
{
  // Has the Channel Up (next) Button been pressed?
  if (isNextButtonPressed(x, y))
  {
    Serial.println("Channel Up (next) Button Pressed");
    changeStation(+1);
    resetDisplayBuffer();
    clearBitRate();

    // Clear existing track information after changing channel
    displayTrackArtist("");

    displayChannelUpPressed();
    buttonPressed = true;
    buttonLastPressed = millis();
    return true;
  }
  return false;
}

boolean checkForSettingsPressed(uint16_t x, uint16_t y)
{
  // Has Settings button been pressed?
  if (isSettingsButtonPressed(x, y))
  {
    Serial.printf("Settings button pressed\n"); 
    displaySettingsPressed();

    buttonPressed = true;
    buttonLastPressed = millis();
    return true;
  }
  return false;
}

// Redraw any buttons pressed (allows for more than 1 button pressed)
void clearPressedButtons()
{
  Serial.printf("Clearing pressed button(s) : %d\n", pressedButton);

  if (pressedButton & BUTTON_CHANNEL_DOWN_PRESSED)
  {
    displayChannelDown();
    pressedButton -= BUTTON_CHANNEL_DOWN_PRESSED;
  }
  if (pressedButton & BUTTON_CHANNEL_UP_PRESSED)
  {
    displayChannelUp();
    pressedButton -= BUTTON_CHANNEL_UP_PRESSED;
  }
  if (pressedButton & BUTTON_VOLUME_DOWN_PRESSED)
  {
    displayVolumeDown();
    pressedButton -= BUTTON_VOLUME_DOWN_PRESSED;
  }
  if (pressedButton & BUTTON_VOLUME_UP_PRESSED)
  {
    displayVolumeUp();
    pressedButton -= BUTTON_VOLUME_UP_PRESSED;
  }
  if (pressedButton & BUTTON_BRIGHTNESS_DOWN_PRESSED)
  {
    displayBrightnessDown();
    pressedButton -= BUTTON_BRIGHTNESS_DOWN_PRESSED;
  }
  if (pressedButton & BUTTON_BRIGHTNESS_UP_PRESSED)
  {
    displayBrightnessUp();
    pressedButton -= BUTTON_BRIGHTNESS_UP_PRESSED;
  }
  if (pressedButton & BUTTON_SETTINGS_PRESSED)
  {
    displaySettings();
    pressedButton -= BUTTON_SETTINGS_PRESSED;
  }  
}

void toggleMute()
{
  if (!muted)
  {
    muted = true;
    audio.setVolume(0);
    displayMuteOn();
  }
  else
  {
    muted = false;
    audio.setVolume(currentVolume);
    displayMuteOff();
  }
  volumeLastChanged = millis();
}

void resetDisplayBuffer()
{
  buffersample = 0;
  buffervalue = 0;
  displayBufferInactive();
}

void calculateDisplayBuffer()
{
  if (buffersample < 10)
  {
    buffervalue += audio.inBufferFilled();
    //Serial.printf("buffersample[%d] = %d\n", buffersample, buffervalue);
    buffersample++;
  }
  else
  {
    buffervalue = (buffervalue * 100) / 80000; // each buffer is 8000 bytes * 10 samples
    //Serial.printf("Average buffer value = %d\n", buffervalue);
    if (buffervalue >= 70)
      displayBufferGreen();
    else if (buffervalue >=50)
      displayBufferAmber();
    else
      displayBufferRed();
    buffersample = 0;
    buffervalue = 0;
  }
}