// Stations stored in JSON format
#include <ArduinoJson.h>

// Utility to write to (psuedo) EEPROM
#include <Preferences.h>

// EEPROM writing routines (eg: remembers previous radio stn)
Preferences preferences;

void loadStation();
void changeStation(int8_t upOrDown);
void connectToStation();
const char *getFriendlyName();

unsigned long channelLastChanged = millis();
const unsigned long AllowChangeMS = 500;


struct station
{
  char name[40];  // friendly name
  char url[80];   // station URL
};

const int maxStations = 32;
struct station radioStation[maxStations]; // Global list of stations
int8_t numberOfStations = 0;

// Current Station Number
unsigned int currentStation;


// Loads the configuration from a file
void loadRadioStations()
{
  Serial.println(F("Loading radio stations"));
  const char *filename = "/Stations.txt";

  // Open file for reading
  File file = LITTLEFS.open(filename, FILE_READ);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<2048> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  file.close();

  // Iterate through radio stations and store in global radio list
  JsonArray stationsArray = doc["stations"];
  for (int i = 0; i < stationsArray.size(); i++)
  {
    JsonVariant s = stationsArray.getElement(i);
    strlcpy(radioStation[i].name, s["name"].as<char *>(), sizeof(radioStation[i].name));
    strlcpy(radioStation[i].url, s["url"].as<char *>(), sizeof(radioStation[i].url));

    // Record number of stations read from configuration
    numberOfStations = i;
  }

  // Set number of stations found (not the index)
  if (numberOfStations > 0)
    numberOfStations++;

  // Output list of loaded radio stations
  for (int i = 0; i < numberOfStations; i++)
  {
    Serial.printf("\t%s (%s)\n", radioStation[i].name, radioStation[i].url);
  }

  Serial.printf("Finished loading %d radio stations", numberOfStations);
}


void loadStation()
{
  // Retreive the last played station (if available)
  preferences.begin("Radio", false);
  currentStation = preferences.getUInt("currentStation", 0);

  Serial.printf("Debug: loadStation() - currentStation = %d\n", currentStation);

  connectToStation();
}


void changeStation(int8_t btnValue)
{
  int8_t nextStation;

  nextStation = currentStation + btnValue;
  
  if (nextStation > numberOfStations)
    nextStation = 0;
  else if (nextStation < 0)
    nextStation = numberOfStations - 1;

  currentStation = nextStation;

  // Connect to selected Station
  connectToStation();

  // Store (new) current station in EEPROM
  preferences.putUInt("currentStation", currentStation);

  // Record time channel last changed
  channelLastChanged = millis();
}

void connectToStation()
{
  Serial.printf("Connecting to %d - %s\n", currentStation, radioStation[currentStation].name);

  // Semaphore required to protect against audio.loop() in playAudioTask
  xSemaphoreTake(xMutex, portMAX_DELAY);
  audio.connecttohost(radioStation[currentStation].url);
  xSemaphoreGive(xMutex);
}


const char *getFriendlyName()
{
  return radioStation[currentStation].name;
}


boolean allowChannelChange()
{
  // Used to debounce channel changes
  return (millis() - channelLastChanged > AllowChangeMS) ? true : false;
}