#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "setup.h"

#define SDA_PIN 21
#define SCL_PIN 22

// Initialize display object
TFT_eSPI tft = TFT_eSPI(135, 240);

void setup()
{
  delay(1000);
  Serial.begin(115200);

  // 1. Initialize the Display
  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);

  // Draw Header
  tft.fillRect(0, 0, 240, 20, TFT_DARKGREY);
  tft.setTextColor(COLOR_TEXT, TFT_DARKGREY);
  tft.drawCentreString("I2C Scanner", 120, 2, 2);

  // 2. Start I2C Scanning process
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft.setCursor(10, 40, 2);
  tft.print("Scanning I2C devices:");

  tft.setCursor(10, 60, 2);
  tft.setTextColor(COLOR_HIGHLIGHT, COLOR_BACKGROUND);
  Wire.begin(SDA_PIN, SCL_PIN);

  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.printf("  Found device at 0x%02X\n", addr);
      tft.print("Found device at 0x");
      tft.print(addr, HEX);
      found++;
    }
    else if (error == 4)
    {
      Serial.printf("  Unknown error at 0x%02X\n", addr);
      tft.print("Unknown error at 0x");
      tft.print(addr, HEX);
    }
    // Small delay to let the bus settle
    delay(1);
  }

  if (found == 0)
  {
    Serial.println("No I2C devices found!");
    tft.print("No I2C devices found!");
  }
  else
  {
    Serial.printf("Scan complete. %d device(s) found.\n", found);
    tft.print("Scan complete. ");
    tft.print(found);
    tft.print(" device(s) found.");
  }
}

void loop()
{
  delay(5000);

  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft.setCursor(10, 40, 2);
  tft.print("Scanning I2C devices:");

  tft.setCursor(10, 60, 2);
  tft.setTextColor(COLOR_HIGHLIGHT, COLOR_BACKGROUND);
  Wire.begin(SDA_PIN, SCL_PIN);

  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.printf("  Found device at 0x%02X\n", addr);
      tft.print("Found device at 0x");
      tft.print(addr, HEX);
      found++;
    }
    else if (error == 4)
    {
      Serial.printf("  Unknown error at 0x%02X\n", addr);
      tft.print("Unknown error at 0x");
      tft.print(addr, HEX);
    }
    // Small delay to let the bus settle
    delay(1);
  }

  if (found == 0)
  {
    Serial.println("No I2C devices found!");
    tft.print("No I2C devices found!");
  }
  else
  {
    Serial.printf("Scan complete. %d device(s) found.\n", found);
    tft.print("Scan complete. \n");
    tft.print(found);
    tft.print(" device(s) found.\n");
  }
  delay(5000);
}