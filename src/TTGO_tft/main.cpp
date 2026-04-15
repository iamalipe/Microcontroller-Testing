// PCF8574 Button Test

#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "setup.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define PCF8574_ADDR 0x27A

TFT_eSPI tft = TFT_eSPI(135, 240);

bool pcfFound = false;
uint8_t lastState = 0xFF; // All HIGH = no button pressed

void drawHeader()
{
  tft.fillRect(0, 0, 240, 20, TFT_DARKGREY);
  tft.setTextColor(COLOR_TEXT, TFT_DARKGREY);
  tft.drawCentreString("PCF8574 Buttons", 120, 2, 2);
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
}

uint8_t readPCF()
{
  Wire.requestFrom(PCF8574_ADDR, (uint8_t)1);
  if (Wire.available())
    return Wire.read();
  return 0xFF;
}

void writePCF(uint8_t val)
{
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(val);
  Wire.endTransmission();
}

// Draw all 8 button states
void drawButtons(uint8_t state)
{
  // 2 rows x 4 cols layout
  // Row 1: P0-P3  (y=35)
  // Row 2: P4-P7  (y=85)

  for (int i = 0; i < 8; i++)
  {
    int col = i % 4;
    int row = i / 4;

    int x = 10 + col * 57;
    int y = 35 + row * 50;

    bool pressed = !(state & (1 << i)); // LOW = pressed

    uint16_t bgColor = pressed ? COLOR_SUCCESS : TFT_DARKGREY;
    uint16_t textColor = pressed ? TFT_BLACK : COLOR_TEXT;

    tft.fillRoundRect(x, y, 50, 36, 6, bgColor);

    char label[6];
    snprintf(label, sizeof(label), "P%d", i);
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(label, x + 25, y + 5, 2);

    const char *status = pressed ? "ON" : "--";
    tft.drawCentreString(status, x + 25, y + 20, 2);
  }

  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
}

void showLastPressed(uint8_t state)
{
  // Find which button was just pressed
  for (int i = 7; i >= 0; i--)
  {
    if (!(state & (1 << i)))
    {
      char buf[24];
      snprintf(buf, sizeof(buf), "Last: P%d pressed  ", i);
      tft.setTextColor(COLOR_HIGHLIGHT, COLOR_BACKGROUND);
      tft.setCursor(10, 122, 2);
      tft.print(buf);
      tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
      return;
    }
  }
  // Nothing pressed
  tft.setTextColor(TFT_DARKGREY, COLOR_BACKGROUND);
  tft.setCursor(10, 122, 2);
  tft.print("Last: --              ");
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  // Check PCF8574
  Wire.beginTransmission(PCF8574_ADDR);
  pcfFound = (Wire.endTransmission() == 0);

  if (!pcfFound)
  {
    tft.setTextColor(COLOR_ERROR, COLOR_BACKGROUND);
    tft.drawCentreString("PCF8574 Not Found!", 120, 60, 2);
    tft.drawCentreString("Check wiring/addr", 120, 80, 2);
    Serial.println("PCF8574 not found.");
    return;
  }

  Serial.printf("PCF8574 found at 0x%02X\n", PCF8574_ADDR);

  writePCF(0xFF); // All pins as input (pulled HIGH)
  drawButtons(0xFF);
}

void loop()
{
  if (!pcfFound)
    return;

  uint8_t state = readPCF();

  if (state != lastState)
  {
    lastState = state;
    drawButtons(state);
    showLastPressed(state);

    Serial.printf("State: 0x%02X  |  ", state);
    for (int i = 0; i < 8; i++)
    {
      Serial.printf("P%d:%s ", i, (!(state & (1 << i))) ? "ON" : "--");
    }
    Serial.println();
  }

  delay(20); // ~50Hz polling, debounce friendly
}