#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

// ── Config ────────────────────────────────────────────────
const char *SSID = "MyHomeIOT";
const char *PASSWORD = "Abcd1234";
const char *WIFI_HOSTNAME = "nas";     // http://nas.local
const char *OTA_PASS = "Abcd1234";     // OTA password
const char *HOSTNAME = "esp32-c3-dev"; // shows in PlatformIO port list

void setupOTA()
{
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASS);

  ArduinoOTA.onStart([]()
                     {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
    Serial.println("OTA Start: " + type); });

  ArduinoOTA.onEnd([]()
                   { Serial.println("\nOTA Done. Rebooting..."); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("OTA Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)     Serial.println("End Failed"); });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);
  delay(3000);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    if (millis() - start > 15000)
    { // 15s timeout
      Serial.println("\nFailed! Reason: " + String(WiFi.status()));
      break;
    }
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  setupOTA();
}

void loop()
{
  ArduinoOTA.handle(); // MUST be here, every loop tick

  // your actual code below
}