#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

// -- Change this to true ONCE to set time, then flash again with false --
#define SET_TIME false

const char *daysOfWeek[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void printDateTime(const DateTime &dt)
{
  Serial.printf(
      "%s, %04d-%02d-%02d %02d:%02d:%02d",
      daysOfWeek[dt.dayOfTheWeek()],
      dt.year(), dt.month(), dt.day(),
      dt.hour(), dt.minute(), dt.second());
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== DS3231 RTC Test ===");

  Wire.begin(8, 9); // SDA=8, SCL=9 (ESP32-C3 Super Mini default)

  if (!rtc.begin())
  {
    Serial.println("[ERROR] DS3231 not found! Check wiring.");
    while (true)
      delay(1000);
  }

  Serial.println("[OK] DS3231 found.");

  // -- Diagnostics --
  if (rtc.lostPower())
  {
    Serial.println("[WARN] RTC lost power / battery dead. Time is invalid!");
    // Auto-set to compile time if battery was lost
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("[INFO] Time auto-set to compile time.");
  }

#if SET_TIME
  // Manually set time — edit values as needed
  // Format: DateTime(YYYY, MM, DD, HH, MM, SS)
  rtc.adjust(DateTime(2025, 1, 15, 10, 30, 0));
  Serial.println("[INFO] Time manually set.");
#endif

  // Print current temperature (DS3231 has built-in temp sensor)
  float temp = rtc.getTemperature();
  Serial.printf("[INFO] DS3231 Temp: %.2f °C\n", temp);

  Serial.println("---");
}

void loop()
{
  DateTime now = rtc.now();

  printDateTime(now);

  // Unix timestamp (useful for JS Date interop)
  Serial.printf("  | Unix: %lu", (unsigned long)now.unixtime());

  // Temperature every 10 seconds
  static uint32_t lastTempMs = 0;
  if (millis() - lastTempMs >= 10000)
  {
    Serial.printf("  | Temp: %.2f°C", rtc.getTemperature());
    lastTempMs = millis();
  }

  Serial.println();
  delay(1000);
}