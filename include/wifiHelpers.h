// All things WiFi go here
#include "WiFi.h"

std::string ssid;
std::string wifiPassword;
bool wiFiDisconnected = true;

// Forward declarations
void connectToWifi();
std::string getWiFiPassword();
std::string getSSID();
const char *wl_status_to_string(wl_status_t status);

// Get the WiFi SSID
std::string getSSID()
{
  std::string currSSID = readLITTLEFSInfo("/WiFiSecrets.txt", "SSID");
  if (currSSID == "")
  {
    return "SSID";
  }
  else
  {
    return currSSID;
  }
}

// Get the WiFi Password
std::string getWiFiPassword()
{
  std::string currWiFiPassword = readLITTLEFSInfo("/WiFiSecrets.txt", "WiFiPassword");
  if (currWiFiPassword == "")
  {
    return "WiFiPassword";
  }
  else
  {
    return currWiFiPassword;
  }
}


// Connect to WiFi
void connectToWifi()
{
  ssid = getSSID();
  wifiPassword = getWiFiPassword();
  
  Serial.printf("Connecting to SSID: %s\n", ssid.c_str());

  // Ensure we disconnect WiFi first to stop connection problems
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Disconnecting from WiFi");
    WiFi.disconnect(false, true);
  }

  // Set WiFi to be a client not a server
  Serial.println("Setting ESP32 to STA mode");
  WiFi.mode(WIFI_STA);

  // Don't store the SSID and password
  Serial.println("Setting ESP32 to NOT store credentials in NVR");
  WiFi.persistent(false);

  // Don't allow WiFi sleep
  Serial.println("Setting ESP32 to NOT allow sleep");
  WiFi.setSleep(false);

  //Serial.println("Setting ESP32 to NOT auto re-connect");
  //WiFi.setAutoReconnect(false);
  //WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);

  //Connect to the required WiFi
  Serial.println("Initiating connection with WiFi.");
  WiFi.begin(ssid.c_str(), wifiPassword.c_str());

  // Give things a chance to settle, avoid problems
  delay(2000);

  Serial.println("Waiting for WiFi connection...");
  uint8_t wifiStatus = WiFi.waitForConnectResult();

  // Successful connection?
  if ((wl_status_t)wifiStatus != WL_CONNECTED)
  {
    Serial.printf("WiFi Status: %s, exiting\n", wl_status_to_string((wl_status_t)wifiStatus));
    return;
  }

  Serial.printf("WiFi connected with (local) IP address of: %s\n", WiFi.localIP().toString().c_str());
  wiFiDisconnected = false;
}



// Convert the WiFi (error) response to a string we can understand
const char *wl_status_to_string(wl_status_t status)
{
  switch (status)
  {
  case WL_NO_SHIELD:
    return "WL_NO_SHIELD";
  case WL_IDLE_STATUS:
    return "WL_IDLE_STATUS";
  case WL_NO_SSID_AVAIL:
    return "WL_NO_SSID_AVAIL";
  case WL_SCAN_COMPLETED:
    return "WL_SCAN_COMPLETED";
  case WL_CONNECTED:
    return "WL_CONNECTED";
  case WL_CONNECT_FAILED:
    return "WL_CONNECT_FAILED";
  case WL_CONNECTION_LOST:
    return "WL_CONNECTION_LOST";
  case WL_DISCONNECTED:
    return "WL_DISCONNECTED";
  default:
    return "UNKNOWN";
  }
}