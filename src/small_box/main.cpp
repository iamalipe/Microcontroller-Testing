#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── PINS ────────────────────────────────────────────────────────────────────
#define BTN1_PIN 10
#define BTN2_PIN 1
#define BTN3_PIN 2
#define BTN4_PIN 3
#define BTN5_PIN 5
#define ADC_PIN 4
#define OLED_SDA 8
#define OLED_SCL 9

// ─── CONFIG ──────────────────────────────────────────────────────────────────
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C
#define DEBOUNCE_MS 30
#define DIVIDER_RATIO 2.0f

// ─── GLOBALS ─────────────────────────────────────────────────────────────────
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

const uint8_t BTN_PINS[5] = {BTN1_PIN, BTN2_PIN, BTN3_PIN, BTN4_PIN, BTN5_PIN};
bool lastState[5] = {HIGH, HIGH, HIGH, HIGH, HIGH};
bool currState[5] = {HIGH, HIGH, HIGH, HIGH, HIGH};
uint32_t debounce[5] = {0, 0, 0, 0, 0};
uint32_t pressCount[5] = {0, 0, 0, 0, 0};
uint32_t lastPressed = 0; // which button last pressed (1–5, 0=none)

// ─── BATTERY ─────────────────────────────────────────────────────────────────
float readVoltage()
{
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++)
  {
    sum += analogRead(ADC_PIN);
    delay(1);
  }
  return (sum / 16.0f / 4095.0f) * 3.3f * DIVIDER_RATIO;
}

int voltageToPercent(float v)
{
  return (int)constrain((v - 3.0f) / 1.2f * 100.0f, 0.0f, 100.0f);
}

// ─── OLED ────────────────────────────────────────────────────────────────────
void updateDisplay(float voltage)
{
  display.clearDisplay();

  // ── Header ───────────────────────────────────────────────────────────────
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.print(F("HARDWARE TEST v1"));
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // ── Last pressed ─────────────────────────────────────────────────────────
  display.setCursor(0, 13);
  display.print(F("LAST BTN: "));
  if (lastPressed > 0)
  {
    display.setTextSize(2);
    display.print(lastPressed);
    display.setTextSize(1);
  }
  else
  {
    display.print(F("--"));
  }

  // ── Press counts ─────────────────────────────────────────────────────────
  display.setCursor(0, 30);
  display.print(F("CNT:"));
  for (int i = 0; i < 5; i++)
  {
    display.print(pressCount[i]);
    if (i < 4)
      display.print(F(" "));
  }

  // ── Live button state (filled = pressed) ─────────────────────────────────
  for (int i = 0; i < 5; i++)
  {
    int x = 2 + i * 25;
    if (currState[i] == LOW)
    {
      display.fillRoundRect(x, 41, 20, 12, 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }
    else
    {
      display.drawRoundRect(x, 41, 20, 12, 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(x + 6, 44);
    display.print(i + 1);
    display.setTextColor(SSD1306_WHITE);
  }

  // ── Battery ──────────────────────────────────────────────────────────────
  int pct = voltageToPercent(voltage);

  // battery outline (right side)
  display.drawRect(95, 41, 28, 12, SSD1306_WHITE);
  display.fillRect(123, 44, 3, 6, SSD1306_WHITE); // tip
  int barW = (int)(pct / 100.0f * 26);
  if (barW > 0)
    display.fillRect(96, 42, barW, 10, SSD1306_WHITE);

  display.setCursor(95, 55);
  display.print(voltage, 1);
  display.print(F("V"));

  // ── Voltage line ─────────────────────────────────────────────────────────
  display.setCursor(0, 55);
  display.print(F("BAT:"));
  display.print(voltage, 2);
  display.print(F("V "));
  display.print(pct);
  display.print(F("%"));

  display.display();
}

// ─── SETUP ───────────────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n=== HW TEST BOOT ==="));

  for (int i = 0; i < 5; i++)
  {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    Serial.println(F("OLED NOT FOUND — check SDA/SCL"));
    while (true)
      delay(1000);
  }

  // Splash
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(28, 20);
  display.println(F("HARDWARE TEST"));
  display.setCursor(35, 35);
  display.println(F("Starting..."));
  display.display();
  delay(1000);

  Serial.println(F("Ready — press any button"));
}

// ─── LOOP ────────────────────────────────────────────────────────────────────
void loop()
{
  static uint32_t lastDisplay = 0;

  // ── Buttons ──────────────────────────────────────────────────────────────
  for (int i = 0; i < 5; i++)
  {
    bool reading = digitalRead(BTN_PINS[i]);
    if (reading != lastState[i])
      debounce[i] = millis();
    if ((millis() - debounce[i]) > DEBOUNCE_MS)
    {
      if (reading != currState[i])
      {
        currState[i] = reading;
        if (currState[i] == LOW)
        {
          pressCount[i]++;
          lastPressed = i + 1;
          Serial.printf("BTN%d pressed  (total: %lu)\n", i + 1, pressCount[i]);
        }
      }
    }
    lastState[i] = reading;
  }

  // ── Display @ 100ms ──────────────────────────────────────────────────────
  if (millis() - lastDisplay >= 100)
  {
    lastDisplay = millis();
    updateDisplay(readVoltage());
  }
}