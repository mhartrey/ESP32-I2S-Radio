#include <Arduino.h>

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
void checkForStationChange();
void checkForVolumeChange();
void checkForBrightnessChange();
void toggleMute();


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
  checkForStationChange();
  checkForVolumeChange();
  checkForBrightnessChange();
  displayBuffer((audio.inBufferFilled() * 100) / 8000); // Crude buffer percentage
}


// Audio Callbacks
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
    //Serial.print("bufferFilled"); Serial.println(audio.inBufferFilled());
    //Serial.print("bufferFree  "); Serial.println(audio.inBufferFree());
    //displayBuffer((audio.inBufferFilled() * 100) / 8000); // Crude buffer percentage
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


void checkForStationChange()
{ 
  // X/Y coordinates of any screen press
  uint16_t x, y;

  // See if there's any touch data for us
  if (tft.getTouch(&x, &y))
  {
    // Draw a spot to show where touch was calculated to be
    //tft.fillCircle(x, y, 2, TFT_WHITE);
    //Serial.printf("z: %i \n", tft.getTouchRawZ());

    // Has the Prev Button been pressed?
    if (getPrevButtonPress(x, y))
    {
      Serial.println("Prev Button Pressed");
      changeStation(-1);
      
      // Clear existing track information after changing channel
      displayTrackArtist("");
      
      tft.setFreeFont(&FreeSansBold12pt7b);
      prevBtn.drawButton(false);
    }
    else
    {
      // Has the Next button been pressed?
      if (getNextButtonPress(x, y))
      {
        Serial.println("Next Button Pressed");
        changeStation(+1);
        
        // Clear existing track information after changing channel
        displayTrackArtist("");

        tft.setFreeFont(&FreeSansBold12pt7b);
        nextBtn.drawButton(false);
      }   
    }
  }    
}


void checkForVolumeChange()
{   
  // X/Y coordinates of any screen press
  uint16_t x, y;
  
  // See if there's any touch data for us
  if (tft.getTouch(&x, &y))
  {
    // Draw a spot to show where touch was calculated to be
    //tft.fillCircle(x, y, 2, TFT_WHITE);

    // Has the Volume Down button been pressed?
    if (getVolDownButtonPress(x, y))
    {
      if (currentVolume > 0)
        currentVolume--;
      Serial.printf("Volume Down pressed, volume = %d\n", currentVolume);
      audio.setVolume(currentVolume);
      tft.setFreeFont(&FreeSansBold12pt7b);
      volDownBtn.drawButton(false);
      volumeLastChanged = millis();
    }
    else
    {
      // Has the Volume Up button been pressed?
      if (getVolUpButtonPress(x, y))
      {
        if (currentVolume < maxVolume)
          currentVolume++;
        Serial.printf("Volume Up pressed, volume = %d\n", currentVolume);          
        audio.setVolume(currentVolume);
        tft.setFreeFont(&FreeSansBold12pt7b);
        volUpBtn.drawButton(false);
        volumeLastChanged = millis();
      }
      else
      {
        // Has the Mute button been pressed?
        if (getMuteButtonPress(x, y))
        {
          Serial.println("Mute pressed");
          toggleMute();
        }
      }
    }
  }             
}

void checkForBrightnessChange()
{   
  // X/Y coordinates of any screen press
  uint16_t x, y;
  
  // See if there's any touch data for us
  if (tft.getTouch(&x, &y))
  {
    // Draw a spot to show where touch was calculated to be
    //tft.fillCircle(x, y, 2, TFT_WHITE);

    // Has the Brightness Down button been pressed?
    if (getBrightnessDownButtonPress(x, y))
    {
      Serial.printf("Brightness Down pressed\n");
      tft.setFreeFont(&FreeSansBold12pt7b);
      brightnessDownBtn.drawButton(false);
      decrementScreenBrightness();
      //volumeLastChanged = millis();
    }
    else
    {
      // Has the Volume Up button been pressed?
      if (getBrightnessUpButtonPress(x, y))
      {
        Serial.printf("Brightness Up pressed\n");          
        tft.setFreeFont(&FreeSansBold12pt7b);
        brightnessUpBtn.drawButton(false);
        incrementScreenBrightness();
        //volumeLastChanged = millis();
      }
    }
  }             
}

void toggleMute()
{
  if (!muted)
  {
    muted = true;
    audio.setVolume(0);
    tft.setFreeFont(&FreeSansBold12pt7b);
    muteBtn.drawButton(true);
  }
  else
  {
    muted = false;
    audio.setVolume(currentVolume);
    tft.setFreeFont(&FreeSansBold12pt7b);
    muteBtn.drawButton(false);
  }
  volumeLastChanged = millis();
}
