#include <Arduino.h>
#include "main.h"

int pressedButtonBitMap = 0;
unsigned long buttonLastPressed = millis();
bool buttonPressed = false;

#define BUTTON_CHANNEL_DOWN_PRESSED 1
#define BUTTON_CHANNEL_UP_PRESSED 2
#define BUTTON_VOLUME_DOWN_PRESSED 4
#define BUTTON_VOLUME_UP_PRESSED 8
#define BUTTON_BRIGHTNESS_DOWN_PRESSED 16
#define BUTTON_BRIGHTNESS_UP_PRESSED 32
#define BUTTON_SETTINGS_PRESSED 64

// Forward declarations
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
void checkMainButtons(uint16_t x, uint16_t y);
void checkSettingsButtons(uint16_t x, uint16_t y);
void toggleMute();
void calculateDisplayBuffer();
void resetDisplayBuffer();

// Create the task handle (a reference to the task being created later)
TaskHandle_t buttonHandlerTaskHandle;

// This is the task that we will start running (on Core 1, don't use Core 0)
void buttonHandlerTask(void *parameter)
{
  static unsigned long prevMillis = 0;
  Serial.println("Started buttonHandlerTask");

  // Loop forever - this is now essentially the main loop of the program
  while (1)
  {
    //Serial.println("ButtonHandler loop started");
    // Ensure lower priority tasks can run
    // - yield() will only give way to higher priority tasks, delay() allows all tasks to run

    vTaskDelay(50 / portTICK_PERIOD_MS);

    yield();
    calculateDisplayBuffer(); // Maybe this should be its own task - but would require a semapore

    yield();
    if (buttonPressed)
    {
      clearPressedButtons();
      buttonPressed = false;
    }

    yield();
    if (millis() - buttonLastPressed > 200)
    {
      checkForScreenPress();
    }

    yield();
    // Temporarily display stack size - can remove later
    if (millis() - prevMillis > 15000)
    {
      unsigned long remainingStack = uxTaskGetStackHighWaterMark(NULL);
      Serial.printf("ButtonHandler Free stack:%lu\n", remainingStack);
      prevMillis = millis();
    }
  }
}

// Called from the main setup() routine
// - the task starts running it as soon as it declared
void createButtonHandlerTask()
{
  // Independent Task to look for and handle button presses
  xTaskCreatePinnedToCore(
      buttonHandlerTask,        /* Function to implement the task */
      "ButtonHandler",          /* Name of the task */
      3000,                     /* Stack size in words */
      NULL,                     /* Task input parameter */
      1,                        /* Priority of the task - must be higher than 0 (idle)*/
      &buttonHandlerTaskHandle, /* Task handle. */
      1);                       /* Core where the task should run */
}

// Redraw any buttons pressed (allows for more than 1 button pressed)
void clearPressedButtons()
{
  if (pressedButtonBitMap > 0)
  {
    Serial.printf("Clearing pressed button(s) : %d\n", pressedButtonBitMap);

    if (pressedButtonBitMap & BUTTON_CHANNEL_DOWN_PRESSED)
    {
      displayChannelDown();
      pressedButtonBitMap -= BUTTON_CHANNEL_DOWN_PRESSED;
    }
    else if (pressedButtonBitMap & BUTTON_CHANNEL_UP_PRESSED)
    {
      displayChannelUp();
      pressedButtonBitMap -= BUTTON_CHANNEL_UP_PRESSED;
    }
    else if (pressedButtonBitMap & BUTTON_VOLUME_DOWN_PRESSED)
    {
      displayVolumeDown();
      pressedButtonBitMap -= BUTTON_VOLUME_DOWN_PRESSED;
    }
    else if (pressedButtonBitMap & BUTTON_VOLUME_UP_PRESSED)
    {
      displayVolumeUp();
      pressedButtonBitMap -= BUTTON_VOLUME_UP_PRESSED;
    }
    else if (pressedButtonBitMap & BUTTON_BRIGHTNESS_DOWN_PRESSED)
    {
      displayBrightnessDown();
      pressedButtonBitMap -= BUTTON_BRIGHTNESS_DOWN_PRESSED;
    }
    else if (pressedButtonBitMap & BUTTON_BRIGHTNESS_UP_PRESSED)
    {
      displayBrightnessUp();
      pressedButtonBitMap -= BUTTON_BRIGHTNESS_UP_PRESSED;
    }
    else if (pressedButtonBitMap & BUTTON_SETTINGS_PRESSED)
    {
      //displaySettings();
      pressedButtonBitMap -= BUTTON_SETTINGS_PRESSED;
    }
  }
}

void checkForScreenPress()
{
  // X/Y coordinates of any screen press
  uint16_t x, y;

  // See if there's any touch data for us
  if (tft.getTouch(&x, &y))
  {
    // Screen pressed
    if (settingsSelected)
    {
      checkSettingsButtons(x, y);
    }
    else
    {
      checkMainButtons(x, y);
    }
  }
  return;
}

// Checks for main buttons when settings not selected
void checkMainButtons(uint16_t x, uint16_t y)
{
  if (checkForMutePressed(x, y))
  {
    return;
  }
  else if (checkForVolumeDownPressed(x, y))
  {
    pressedButtonBitMap |= BUTTON_VOLUME_DOWN_PRESSED;
  }
  else if (checkForVolumeUpPressed(x, y))
  {
    pressedButtonBitMap |= BUTTON_VOLUME_UP_PRESSED;
  }
  else if (checkForChannelDownPressed(x, y))
  {
    pressedButtonBitMap |= BUTTON_CHANNEL_DOWN_PRESSED;
  }
  else if (checkForChannelUpPressed(x, y))
  {
    pressedButtonBitMap |= BUTTON_CHANNEL_UP_PRESSED;
  }
  else if (checkForSettingsPressed(x, y))
  {
    pressedButtonBitMap |= BUTTON_SETTINGS_PRESSED;
  }
  else
  {
    Serial.println("No matching buttons pressed");
    pressedButtonBitMap = 0;
  }
}

// Checks for settings buttons when settings selected
void checkSettingsButtons(uint16_t x, uint16_t y)
{
  if (checkForSettingsPressed(x, y))
  {
    //pressedButtonBitMap |= BUTTON_SETTINGS_PRESSED;
    pressedButtonBitMap = 0;
  }
  else if (checkForBrightnessDownPressed(x, y))
  {
    pressedButtonBitMap |= BUTTON_BRIGHTNESS_DOWN_PRESSED;
  }
  else if (checkForBrightnessUpPressed(x, y))
  {
    pressedButtonBitMap |= BUTTON_BRIGHTNESS_UP_PRESSED;
  }
  else
  {
    Serial.println("DEBUG: No matching settings buttons pressed");

    // Clear settings menu if no matching buttons pressed - including main screen
    if (settingsSelected)
    {
      displaySettingsPressed();
    }
    pressedButtonBitMap = 0;
  }
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
    displayVolumeUp(); // Clear Up
    displayMuteOff();  // Clear Down
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
    displayVolumeDown(); // Clear Down
    displayMuteOff();    // Clear Mute
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