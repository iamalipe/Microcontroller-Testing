// ================================================================
//  ESP32-C3 Super Mini — Full Peripheral Test
//  Tests: I2C, OLED, 9 Buttons, Voltage ADC, Buzzer, WiFi, BLE
// ================================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <NimBLEDevice.h>

// ─── I2C / OLED ──────────────────────────────────────────────────────────────
#define PIN_SDA 8
#define PIN_SCL 9
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ─── Buttons (active LOW, INPUT_PULLUP) ──────────────────────────────────────
#define BTN_COUNT 9
const uint8_t BTN_PINS[BTN_COUNT] = {20, 21, 10, 5, 6, 7, 4, 1, 0};
const char *BTN_NAMES[BTN_COUNT] = {"BTN01(TX)", "BTN02(RX)", "BTN03", "BTN04", "BTN05", "BTN06", "BTN07", "BTN08", "BTN09"};

// ─── Voltage divider on GPIO 3 ───────────────────────────────────────────────
// Adjust DIVIDER_RATIO to match your resistor network
// 100kΩ + 100kΩ → 2.0 | 100kΩ + 47kΩ → 3.13 | Direct 0-3.3V → 1.0
#define PIN_VOLTAGE 3
#define ADC_MAX 4095.0f
#define VREF 3.3f
#define DIVIDER_RATIO 2.0f

// ─── Buzzer (passive) on GPIO 2 ──────────────────────────────────────────────
#define PIN_BUZZER 2
#define LEDC_CHANNEL 0
#define LEDC_RES_BITS 8

// ─── BLE ─────────────────────────────────────────────────────────────────────
#define BLE_DEVICE_NAME "ESP32-C3-TEST"
#define BLE_SERVICE_UUID "DEAD"
#define BLE_CHAR_UUID "BEEF"

NimBLEServer *pServer = nullptr;
NimBLECharacteristic *pChar = nullptr;
bool bleReady = false;

// ─── State ───────────────────────────────────────────────────────────────────
int8_t lastPressed = -1;
uint32_t lastBleNotify = 0;
uint8_t bleCounter = 0;
int wifiNetworks = 0;
bool oledOk = false;

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

void beep(uint32_t freq, uint32_t ms)
{
  ledcSetup(LEDC_CHANNEL, freq, LEDC_RES_BITS);
  ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, 128); // 50% duty
  delay(ms);
  ledcWrite(LEDC_CHANNEL, 0);
  ledcDetachPin(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
}

// 2-line OLED status used during boot sequence
void oledStatus(const char *label, const char *msg)
{
  if (!oledOk)
    return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(label);
  display.setCursor(0, 12);
  display.print(msg);
  display.display();
}

// Boot summary on OLED (shown for 2s after setup)
void oledBootSummary()
{
  if (!oledOk)
    return;
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.printf("OLED:%s WiFi:%s BLE:%s",
                 oledOk ? "Y" : "N",
                 wifiNetworks >= 0 ? "Y" : "N",
                 bleReady ? "Y" : "N");
  display.setCursor(0, 11);
  display.printf("WiFi nets found: %d", wifiNetworks);
  display.setCursor(0, 22);
  display.print(bleReady ? "BLE: " BLE_DEVICE_NAME : "BLE: FAIL");
  display.display();
}

void i2cScan()
{
  Serial.println("── I2C Scan ──");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.printf("  Found: 0x%02X\n", addr);
      found++;
    }
  }
  if (!found)
    Serial.println("  No devices found!");
  Serial.println("──────────────");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n╔══════════════════════════════╗");
  Serial.println("║  ESP32-C3 Super Mini Test    ║");
  Serial.println("╚══════════════════════════════╝\n");

  // ── I2C ──────────────────────────────────────────────────────────────────
  Wire.begin(PIN_SDA, PIN_SCL);
  i2cScan();

  // ── OLED ─────────────────────────────────────────────────────────────────
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (!oledOk)
  {
    Serial.println("[FAIL] OLED not found at 0x3C");
  }
  else
  {
    Serial.println("[OK]   OLED SSD1306 ready");
    oledStatus("I2C+OLED", "OK");
    delay(400);
  }

  // ── Buttons ──────────────────────────────────────────────────────────────
  Serial.println("\n── Buttons ──");
  for (uint8_t i = 0; i < BTN_COUNT; i++)
  {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    Serial.printf("  GPIO %2d  → %s\n", BTN_PINS[i], BTN_NAMES[i]);
  }
  Serial.println("[OK]   All buttons configured (INPUT_PULLUP)");

  // ── Buzzer ───────────────────────────────────────────────────────────────
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  Serial.printf("\n[OK]   GPIO %d  → Buzzer (passive)\n", PIN_BUZZER);

  // ── ADC / Voltage ────────────────────────────────────────────────────────
  Serial.printf("[OK]   GPIO %d  → Voltage divider (ratio: %.1f)\n",
                PIN_VOLTAGE, DIVIDER_RATIO);

  // ── WiFi Scan ─────────────────────────────────────────────────────────────
  Serial.println("\n── WiFi Scan ──");
  oledStatus("WiFi", "Scanning...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  wifiNetworks = WiFi.scanNetworks();

  if (wifiNetworks == 0)
  {
    Serial.println("  No networks found");
  }
  else
  {
    Serial.printf("  Found %d network(s):\n", wifiNetworks);
    for (int i = 0; i < wifiNetworks; i++)
    {
      Serial.printf("  [%2d] %-28s  RSSI:%4d dBm  Ch:%2d  %s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    WiFi.channel(i),
                    WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "OPEN" : "ENC");
    }
  }
  WiFi.scanDelete();
  Serial.printf("[%s]   WiFi scan — %d network(s) found\n",
                wifiNetworks >= 0 ? "OK  " : "FAIL", wifiNetworks);

  // ── BLE ──────────────────────────────────────────────────────────────────
  Serial.println("\n── BLE Init ──");
  oledStatus("BLE", "Starting...");

  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  pServer = NimBLEDevice::createServer();

  NimBLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  pChar = pService->createCharacteristic(
      BLE_CHAR_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pChar->setValue("C3-READY");
  pService->start();

  NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
  pAdv->addServiceUUID(BLE_SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->start();

  bleReady = true;
  Serial.printf("[OK]   BLE advertising as \"%s\"\n", BLE_DEVICE_NAME);
  Serial.printf("       Service UUID : 0x%s\n", BLE_SERVICE_UUID);
  Serial.printf("       Char    UUID : 0x%s  (READ + NOTIFY)\n", BLE_CHAR_UUID);

  // ── Boot summary ─────────────────────────────────────────────────────────
  Serial.println("\n╔══════════════════════════════╗");
  Serial.printf("║  OLED : %-20s║\n", oledOk ? "OK" : "FAIL");
  Serial.printf("║  WiFi : %-20s║\n", wifiNetworks >= 0 ? "OK" : "FAIL");
  Serial.printf("║  BLE  : %-20s║\n", bleReady ? "OK" : "FAIL");
  Serial.println("╚══════════════════════════════╝\n");

  // Startup beep sequence
  beep(1000, 80);
  delay(60);
  beep(1500, 80);
  delay(60);
  beep(2000, 80);

  oledBootSummary();
  delay(2000);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Loop
// ─────────────────────────────────────────────────────────────────────────────
void loop()
{
  // ── Read buttons (first pressed wins) ────────────────────────────────────
  int8_t pressed = -1;
  for (uint8_t i = 0; i < BTN_COUNT; i++)
  {
    if (digitalRead(BTN_PINS[i]) == LOW)
    {
      pressed = i;
      break;
    }
  }

  // ── On new press: beep + Serial log + BLE notify ─────────────────────────
  if (pressed != lastPressed)
  {
    if (pressed != -1)
    {
      beep(2000, 40);
      Serial.printf("PRESS  → %s  (GPIO %d)\n",
                    BTN_NAMES[pressed], BTN_PINS[pressed]);

      if (bleReady && pChar)
      {
        char buf[24];
        snprintf(buf, sizeof(buf), "BTN:%s", BTN_NAMES[pressed]);
        pChar->setValue(buf);
        pChar->notify();
      }
    }
    else
    {
      Serial.println("RELEASE");
    }
    lastPressed = pressed;
  }

  // ── Voltage ADC ──────────────────────────────────────────────────────────
  int raw = analogRead(PIN_VOLTAGE);
  float voltage = (raw / ADC_MAX) * VREF * DIVIDER_RATIO;

  // ── BLE heartbeat notify every 5s ────────────────────────────────────────
  if (bleReady && pChar && (millis() - lastBleNotify > 5000))
  {
    char buf[28];
    snprintf(buf, sizeof(buf), "PING:%u V:%.2f", bleCounter++, voltage);
    pChar->setValue(buf);
    pChar->notify();
    lastBleNotify = millis();
    Serial.printf("BLE    → notify: %s\n", buf);
  }

  // ── OLED ─────────────────────────────────────────────────────────────────
  // Row 0  (y= 0): Button name or idle message
  // Row 1  (y=11): BLE clients connected
  // Row 2  (y=22): Raw ADC + calculated voltage
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  if (pressed != -1)
  {
    display.print(BTN_NAMES[pressed]);
    display.setCursor(0, 11);
    display.printf("GPIO: %d", BTN_PINS[pressed]);
  }
  else
  {
    display.print("No button pressed");
    display.setCursor(0, 11);
    display.printf("BLE clients: %d",
                   pServer ? pServer->getConnectedCount() : 0);
  }

  display.setCursor(0, 22);
  display.printf("ADC:%4d  V:%.2fV", raw, voltage);

  display.display();

  delay(40); // ~25 fps
}