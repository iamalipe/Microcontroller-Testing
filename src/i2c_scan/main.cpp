#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 8
#define SCL_PIN 9

void setup()
{
  // Wait for USB CDC serial to be ready (ESP32-C3 with USB)
  delay(2000);
  Serial.begin(115200);
  delay(100);

  Serial.println("\nHello, I2C Scanner!");
  Serial.println("Scanning for I2C devices...");

  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  // Optional: set clock speed (default 100kHz)
  // Wire.setClock(100000);

  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.printf("  Found device at 0x%02X\n", addr);
      found++;
    }
    else if (error == 4)
    {
      Serial.printf("  Unknown error at 0x%02X\n", addr);
    }
    // Small delay to let the bus settle
    delay(1);
  }

  if (found == 0)
  {
    Serial.println("No I2C devices found!");
  }
  else
  {
    Serial.printf("Scan complete. %d device(s) found.\n", found);
  }
}

void loop()
{
  // Wait for USB CDC serial to be ready (ESP32-C3 with USB)
  delay(2000);
  Serial.begin(115200);
  delay(100);

  Serial.println("\nHello, I2C Scanner!");
  Serial.println("Scanning for I2C devices...");

  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  // Optional: set clock speed (default 100kHz)
  // Wire.setClock(100000);

  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.printf("  Found device at 0x%02X\n", addr);
      found++;
    }
    else if (error == 4)
    {
      Serial.printf("  Unknown error at 0x%02X\n", addr);
    }
    // Small delay to let the bus settle
    delay(1);
  }

  if (found == 0)
  {
    Serial.println("No I2C devices found!");
  }
  else
  {
    Serial.printf("Scan complete. %d device(s) found.\n", found);
  }
}