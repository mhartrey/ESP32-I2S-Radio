// SPIFFS (in-memory SPI File System) replacement
#include <arduino.h>

void mountLITTLEFS()
{
  // Initialise LITTLEFS system
  if (!LITTLEFS.begin(false))
  {
    Serial.println("LITTLEFS Mount Failed.");
    while (1)
      delay(1);
  }
  else
  {
    Serial.println("LITTLEFS Mount Successful.");
  }
}

// LITTLEFS config reader
// Format of data is:
//          #comment line
//          <KEY><DATA>
// For now configfile name requires '/' prefix, e.g. /WiFiSecrets.txt
std::string readLITTLEFSInfo(const char *configfile, const char *itemRequired)
{
  char startMarker = '<';
  char endMarker = '>';
  char *receivedChars = new char[32];
  int charCnt = 0;
  char data;
  bool foundKey = false;

  // Mount LITTLEFS system
  mountLITTLEFS();

  Serial.printf("Looking for key '%s' in '%s'\n", itemRequired, configfile);

  // Get a handle to the file
  File configFile = LITTLEFS.open(configfile, FILE_READ);
  if (!configFile)
  {
    // TODO: Display error on screen
    Serial.printf("Unable to open file '%s'\n", configfile);
    while (1)
      ;
  }

  // Look for the required key
  while (configFile.available())
  {
    charCnt = 0;

    // Read until start marker found
    while (configFile.available() && configFile.peek() != startMarker)
    {
      // Do nothing, ignore spurious chars
      data = configFile.read();
      //Serial.print("Throwing away preMarker:");
      //Serial.println(data);
    }

    // If EOF this is an error
    if (!configFile.available())
    {
      // Abort - no further data
      continue;
    }

    // Throw away startMarker char
    configFile.read();

    // Read all following characters as the data (key or value)
    while (configFile.available() && configFile.peek() != endMarker)
    {
      data = configFile.read();
      receivedChars[charCnt] = data;
      charCnt++;
    }

    // Terminate string
    receivedChars[charCnt] = '\0';

    // If we previously found the matching key then return the value
    if (foundKey)
      break;

    //Serial.printf("Found: '%s'.\n", receivedChars);
    if (strcmp(receivedChars, itemRequired) == 0)
    {
      //Serial.println("Found matching key - next string will be returned");
      foundKey = true;
    }
  }

  // Terminate file
  configFile.close();

  // Did we find anything
  Serial.printf("LITTLEFS parameter '%s'\n", itemRequired);
  if (charCnt == 0)
  {
    Serial.println("' not found.");
    return "";
  }
  else
  {
    //Serial.printf("': '%s'\n", receivedChars);
  }

  return receivedChars;
}