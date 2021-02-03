// Routines related to NTP time go here

#include "time.h"

static char latestTime[10];

const char* ntpServer = "pool.ntp.org";
//const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

// Forward declarations
void getLocalTime();
char *retrieveTime();

// Create the task handle (a reference to the task being created later)
TaskHandle_t displayClockTaskHandle;


// This is the task that we will start running (on Core 1, don't use Core 0)
void displayClockTask(void *parameter)
{
  Serial.println("Started displayClockTask");
  
  // Loop forever
  while (1)
  {
    getLocalTime();
    //Serial.printf("*** TIME =%s\n", latestTime);
    displayClock(retrieveTime());
    //displayLargeClock(retrieveTime()); // Could be used when radio if off

    vTaskDelay (15000 / portTICK_PERIOD_MS);
    
    // Temporarily display stack size - can remove later
    unsigned long remainingStack = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("Clock Free stack:%lu\n", remainingStack);
  }
}


// Called from the main setup() routine
// - the task starts running it as soon as it declared
void createDisplayClockTask()
{
  // Independent Task to play music
  xTaskCreatePinnedToCore(
    displayClockTask,  /* Function to implement the task */
    "DisplayClock",    /* Name of the task */
    2000,           /* Stack size in words */
    NULL,           /* Task input parameter */
    1,              /* Priority of the task - must be higher than 0 (idle)*/
    &displayClockTaskHandle, /* Task handle. */
    1);             /* Core where the task should run */
}


void getLocalTime()
{
  struct tm timeinfo;
  strcpy(latestTime, "");
      
  if (WiFi.status() == WL_CONNECTED)
  {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    if(!getLocalTime(&timeinfo))
    {
      Serial.println("*** printLocalTime() - Failed to obtain time");
    }
    else
    {
      strftime(latestTime, sizeof(latestTime), "%H:%M", &timeinfo);
      //strftime(latestTime, sizeof(latestTime), "%H:%M:%S", &timeinfo);
    }
  }
  else
  {
    Serial.println("*** printLocalTime() - WiFi NOT connected");
  }
}

char *retrieveTime()
{
  return latestTime;
}