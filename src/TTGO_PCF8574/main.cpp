#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <Adafruit_AHTX0.h>
#include "esp_adc_cal.h" // ← calibrated ADC
#include "setup.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define PCF8574_ADDR 0x27

TFT_eSPI tft = TFT_eSPI(135, 240);
Adafruit_AHTX0 aht;

bool pcfFound = false;
bool ahtFound = false;
uint8_t lastState = 0xFF;

float lastTemp = 0.0f;
float lastHum = 0.0f;
int lastBat = -1;
unsigned long lastSensorTick = 0;

// ── ADC calibration ────────────────────────────────────────
esp_adc_cal_characteristics_t adcChars;

void initBattery()
{
  // Characterize ADC: Unit1, 11dB atten (0–3.9V range), 12-bit, default 1100mV Vref
  esp_adc_cal_value_t calType = esp_adc_cal_characterize(
      ADC_UNIT_1,
      ADC_ATTEN_DB_11,
      ADC_WIDTH_BIT_12,
      1100,
      &adcChars);

  if (calType == ESP_ADC_CAL_VAL_EFUSE_VREF)
  {
    Serial.printf("ADC cal: eFuse Vref = %dmV\n", adcChars.vref);
  }
  else
  {
    Serial.println("ADC cal: using default Vref 1100mV");
  }

  // Set attenuation for the pin (required on ESP32-C3)
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
}

int readBatteryPercent()
{
  uint32_t raw = analogRead(BATTERY_PIN);

  // esp_adc_cal gives calibrated mV at the pin
  uint32_t pinMv = esp_adc_cal_raw_to_voltage(raw, &adcChars);

  // Undo the voltage divider to get actual battery voltage
  uint32_t batMv = (uint32_t)(pinMv * BATTERY_DIVIDER);

  int pct = (int)(((float)(batMv - BATTERY_VMIN) /
                   (float)(BATTERY_VMAX - BATTERY_VMIN)) *
                  100.0f);

  Serial.printf("ADC raw: %d | pin: %dmV | bat: %dmV | %d%%\n",
                raw, pinMv, batMv, pct);

  return constrain(pct, 0, 100);
}

// ── Header ─────────────────────────────────────────────────
void drawHeader(int batPct)
{
  tft.fillRect(0, 0, 240, 20, TFT_DARKGREY);
  tft.setTextColor(COLOR_TEXT, TFT_DARKGREY);
  tft.drawCentreString("PCF8574 Buttons", 100, 3, 2);

  char buf[8];
  snprintf(buf, sizeof(buf), "%3d%%", batPct);
  uint16_t batColor = (batPct > 20) ? TFT_GREEN : TFT_RED;
  tft.setTextColor(batColor, TFT_DARKGREY);
  tft.drawString(buf, 192, 5, 1);

  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
}

// ── PCF8574 ────────────────────────────────────────────────
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

// ── Button grid ────────────────────────────────────────────
void drawButtons(uint8_t state)
{
  for (int i = 0; i < 8; i++)
  {
    int col = i % 4;
    int row = i / 4;
    int x = 10 + col * 57;
    int y = 22 + row * 40;

    bool pressed = !(state & (1 << i));
    uint16_t bgColor = pressed ? COLOR_SUCCESS : TFT_DARKGREY;
    uint16_t textColor = pressed ? TFT_BLACK : COLOR_TEXT;

    tft.fillRoundRect(x, y, 50, 32, 6, bgColor);

    char label[6];
    snprintf(label, sizeof(label), "P%d", i);
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(label, x + 25, y + 3, 2);
    tft.drawCentreString(pressed ? "ON" : "--", x + 25, y + 17, 2);
  }
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
}

// ── Bottom info bar ────────────────────────────────────────
void drawInfoBar(float temp, float hum, uint8_t state)
{
  const int y = 103;
  tft.fillRect(0, y - 1, 240, 33, COLOR_BACKGROUND);
  tft.drawFastHLine(0, y - 1, 240, TFT_DARKGREY);

  tft.setTextColor(COLOR_HIGHLIGHT, COLOR_BACKGROUND);
  tft.drawString("T:", 4, y + 3, 2);
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  if (ahtFound)
  {
    char tbuf[12];
    snprintf(tbuf, sizeof(tbuf), "%.1f\xB0"
                                 "C",
             temp);
    tft.drawString(tbuf, 22, y + 3, 2);
  }
  else
  {
    tft.drawString("N/A", 22, y + 3, 2);
  }

  tft.setTextColor(COLOR_HIGHLIGHT, COLOR_BACKGROUND);
  tft.drawString("H:", 90, y + 3, 2);
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  if (ahtFound)
  {
    char hbuf[12];
    snprintf(hbuf, sizeof(hbuf), "%.0f%%", hum);
    tft.drawString(hbuf, 108, y + 3, 2);
  }
  else
  {
    tft.drawString("N/A", 108, y + 3, 2);
  }

  tft.setTextColor(TFT_DARKGREY, COLOR_BACKGROUND);
  tft.drawString("Btn:", 165, y + 3, 2);
  for (int i = 7; i >= 0; i--)
  {
    if (!(state & (1 << i)))
    {
      char bbuf[4];
      snprintf(bbuf, sizeof(bbuf), "P%d", i);
      tft.setTextColor(COLOR_HIGHLIGHT, COLOR_BACKGROUND);
      tft.drawString(bbuf, 197, y + 3, 2);
      tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
      return;
    }
  }
  tft.setTextColor(TFT_DARKGREY, COLOR_BACKGROUND);
  tft.drawString("--", 197, y + 3, 2);
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
}

// ── Setup ──────────────────────────────────────────────────
void setup()
{
  delay(1000);
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  initBattery(); // ← calibrate ADC before first read

  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(COLOR_BACKGROUND);

  ahtFound = aht.begin(&Wire);
  Serial.println(ahtFound ? "AHT21B OK" : "AHT21B NOT found");

  lastBat = readBatteryPercent();
  drawHeader(lastBat);

  Wire.beginTransmission(PCF8574_ADDR);
  pcfFound = (Wire.endTransmission() == 0);

  if (!pcfFound)
  {
    tft.setTextColor(COLOR_ERROR, COLOR_BACKGROUND);
    tft.drawCentreString("PCF8574 Not Found!", 120, 45, 2);
    tft.drawCentreString("Check wiring/addr", 120, 65, 2);
    Serial.println("PCF8574 not found.");
  }
  else
  {
    Serial.printf("PCF8574 found at 0x%02X\n", PCF8574_ADDR);
    writePCF(0xFF);
    drawButtons(0xFF);
  }

  drawInfoBar(lastTemp, lastHum, 0xFF);
}

// ── Loop ───────────────────────────────────────────────────
void loop()
{
  if (millis() - lastSensorTick >= SENSOR_INTERVAL_MS)
  {
    lastSensorTick = millis();

    int bat = readBatteryPercent();
    if (bat != lastBat)
    {
      lastBat = bat;
      drawHeader(bat);
    }

    if (ahtFound)
    {
      sensors_event_t hEvt, tEvt;
      aht.getEvent(&hEvt, &tEvt);
      lastTemp = tEvt.temperature;
      lastHum = hEvt.relative_humidity;
    }

    drawInfoBar(lastTemp, lastHum, lastState);
    Serial.printf("Bat: %d%% | Temp: %.1f°C | Hum: %.1f%%\n",
                  lastBat, lastTemp, lastHum);
  }

  if (!pcfFound)
    return;

  uint8_t state = readPCF();
  if (state != lastState)
  {
    lastState = state;
    drawButtons(state);
    drawInfoBar(lastTemp, lastHum, state);

    Serial.printf("State: 0x%02X | ", state);
    for (int i = 0; i < 8; i++)
      Serial.printf("P%d:%s ", i, (!(state & (1 << i))) ? "ON" : "--");
    Serial.println();
  }

  delay(20);
}