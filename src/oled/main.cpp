#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    Serial.println("SSD1306 init failed!");
    while (true)
      ;
  }

  // --- Test 1: Splash ---
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("ESP32-C3");
  display.setTextSize(1);
  display.setCursor(25, 35);
  display.println("OLED Test OK");
  display.setCursor(20, 50);
  display.println("128x64 SSD1306");
  display.display();
  delay(2000);

  // --- Test 2: Pixel corners ---
  display.clearDisplay();
  display.drawPixel(0, 0, SSD1306_WHITE);
  display.drawPixel(127, 0, SSD1306_WHITE);
  display.drawPixel(0, 63, SSD1306_WHITE);
  display.drawPixel(127, 63, SSD1306_WHITE);
  display.setCursor(30, 28);
  display.setTextSize(1);
  display.println("Corner Test");
  display.display();
  delay(2000);

  // --- Test 3: Border rect ---
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.setCursor(20, 28);
  display.println("Border Test");
  display.display();
  delay(2000);

  // --- Test 4: Fill screen ---
  display.fillScreen(SSD1306_WHITE);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  delay(500);

  // --- Test 5: Circle ---
  display.clearDisplay();
  display.drawCircle(64, 32, 30, SSD1306_WHITE);
  display.setCursor(46, 28);
  display.println("Circle");
  display.display();
  delay(2000);
}

void loop()
{
  // Scroll a counter
  static uint32_t count = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("-- Loop Counter --");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(20, 22);
  display.println(count++);
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("Uptime: ");
  display.print(millis() / 1000);
  display.print("s");
  display.display();
  delay(500);
}

// #include <Arduino.h>
// #include <Wire.h>

// #define SDA_PIN 8
// #define SCL_PIN 9

// void setup()
// {
//   Serial.begin(115200);
//   Serial.println("Hello, I2C!");
//   delay(2000);
//   Serial.println("Start Scanning I2C...");
//   Wire.begin(SDA_PIN, SCL_PIN);

//   Serial.println("Scanning I2C...");
//   int found = 0;
//   for (uint8_t addr = 1; addr < 127; addr++)
//   {
//     Wire.beginTransmission(addr);
//     if (Wire.endTransmission() == 0)
//     {
//       Serial.printf("Found device at 0x%02X\n", addr);
//       found++;
//     }
//   }
//   if (found == 0)
//     Serial.println("No I2C devices found!");
// }

// void loop() {}