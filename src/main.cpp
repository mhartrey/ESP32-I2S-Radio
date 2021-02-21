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

// Button Handler routines
#include "buttonHandler.h"

// Digital I2S I/O pins used by PCM5102
#define I2S_DOUT 25 // DIN connection
#define I2S_BCLK 27 // Bit clock
#define I2S_LRC 26  // Left Right Clock
// FLT, DMP, SCL and LOW pulled low

int buffersample = 0;
int buffervalue = 0;

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
      vTaskDelay(5000 / portTICK_PERIOD_MS);
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

  // Start task to handle button presses
  createButtonHandlerTask();
}

// Main loop doesn't do anything
void loop()
{
  Serial.println("Main loop");
  delay(100);
  vTaskDelete(NULL);
}

// Audio Callbacks
void audio_info(const char *info)
{
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info)
{ //id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_mp3(const char *info)
{ //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}
void audio_showstation(const char *info)
{
  Serial.print("station     ");
  Serial.println(info);
  displayStationName(info);
}
void audio_showstreaminfo(const char *info)
{
  Serial.print("streaminfo  ");
  Serial.println(info);
}
void audio_showstreamtitle(const char *info)
{
  Serial.print("streamtitle ");
  Serial.println(info);
  displayTrackArtist(info);
}
void audio_bitrate(const char *info)
{
  Serial.print("bitrate     ");
  Serial.println(info);
  displayBitRate(info);
}
void audio_commercial(const char *info)
{ //duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info)
{ //homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}
void audio_lasthost(const char *info)
{ //stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info)
{
  Serial.print("eof_speech  ");
  Serial.println(info);
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
    else if (buffervalue >= 50)
      displayBufferAmber();
    else
      displayBufferRed();
    buffersample = 0;
    buffervalue = 0;
  }
}