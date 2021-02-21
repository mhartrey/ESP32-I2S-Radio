#include "Audio.h"

static Audio audio;
uint8_t currentVolume = 15; // 0..21
const uint8_t maxVolume = 21;
boolean muted = false;
unsigned long volumeLastChanged = millis();

// Semaphore used to protect audio operations
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

// Create the task handle (a reference to the task being created later)
TaskHandle_t playAudioTaskHandle;

volatile bool allowPlayAudio = false;

// This is the task that we will start running (on Core 1, don't use Core 0)
void playAudioTask(void *parameter)
{
  static unsigned long prevMillis = 0;
  Serial.println("Started playAudioTask");

  // Loop forever
  while (1)
  {
    if (allowPlayAudio)
    {
      // Play audio stream - semaphore protects against channel change
      xSemaphoreTake(xMutex, portMAX_DELAY);
      audio.loop();
      xSemaphoreGive(xMutex);

      // Ensure lower priority tasks can run
      // - yield() will only give way to higher priority tasks, delay() allows all tasks to run
      delay(1);
    }
    else
    {
      // Not allowed to play yet
      delay(10);
    }

    // We should check that the stack size allocated was correct. This shows the FREE
    // stack space every second. Assuming we have run all paths it should remain constant.
    // Comment out this code once satisfied that we have allocated the correct stack space.
    if (millis() - prevMillis > 15000)
    {
      unsigned long remainingStack = uxTaskGetStackHighWaterMark(NULL);
      Serial.printf("Audio Free stack:%lu\n", remainingStack);
      prevMillis = millis();
    }
  }
}

// Called from the main setup() routine
// - the task starts running it as soon as it declared
void createAudioMusicTask()
{
  // Independent Task to play music
  xTaskCreatePinnedToCore(
      playAudioTask,        /* Function to implement the task */
      "PlayAudio",          /* Name of the task */
      3000,                 /* Stack size in words */
      NULL,                 /* Task input parameter */
      2,                    /* Priority of the task - must be higher than 0 (idle)*/
      &playAudioTaskHandle, /* Task handle. */
      1);                   /* Core where the task should run */
}