#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft;

// ── helpers ──────────────────────────────────────────────────────────────────

void drawColorBars()
{
  // 8 vertical color bars across 128px
  const uint16_t colors[] = {
      TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREEN,
      TFT_MAGENTA, TFT_RED, TFT_BLUE, TFT_BLACK};
  int barW = tft.width() / 8; // 16px each
  for (int i = 0; i < 8; i++)
  {
    tft.fillRect(i * barW, 0, barW, tft.height(), colors[i]);
  }
}

void drawCheckerboard(uint16_t c1, uint16_t c2, int cellSize = 16)
{
  tft.fillScreen(TFT_BLACK);
  for (int y = 0; y < tft.height(); y += cellSize)
  {
    for (int x = 0; x < tft.width(); x += cellSize)
    {
      bool even = ((x / cellSize) + (y / cellSize)) % 2 == 0;
      tft.fillRect(x, y, cellSize, cellSize, even ? c1 : c2);
    }
  }
}

void drawGradientRainbow()
{
  // Horizontal hue sweep using HSV → RGB565
  for (int x = 0; x < tft.width(); x++)
  {
    float h = (float)x / tft.width() * 360.0f;
    int i = (int)(h / 60.0f);
    float f = h / 60.0f - i;
    uint8_t v = 255;
    uint8_t p = 0;
    uint8_t q = (uint8_t)(v * (1 - f));
    uint8_t t = (uint8_t)(v * f);
    uint8_t r, g, b;
    switch (i % 6)
    {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    default:
      r = v;
      g = p;
      b = q;
      break;
    }
    tft.drawFastVLine(x, 0, tft.height(), tft.color565(r, g, b));
  }
}

void drawTextTest()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setTextSize(1);
  tft.setCursor(4, 4);
  tft.println("Size 1: Hello C3!");

  tft.setTextSize(2);
  tft.setCursor(4, 20);
  tft.println("Size 2");

  tft.setTextSize(3);
  tft.setCursor(4, 46);
  tft.println("Size3");

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 80);
  tft.printf("W=%d H=%d\n", tft.width(), tft.height());
  tft.setCursor(4, 92);
  tft.printf("SPI @ 27MHz\n");
  tft.setCursor(4, 104);
  tft.printf("ST7735 driver\n");
}

void drawShapes()
{
  tft.fillScreen(TFT_BLACK);

  // Filled circle
  tft.fillCircle(32, 32, 28, TFT_RED);
  tft.drawCircle(32, 32, 28, TFT_WHITE);

  // Hollow rect
  tft.drawRect(72, 8, 48, 48, TFT_CYAN);
  tft.drawRect(74, 10, 44, 44, TFT_CYAN);

  // Triangle
  tft.fillTriangle(10, 130, 60, 80, 110, 130, TFT_YELLOW);
  tft.drawTriangle(10, 130, 60, 80, 110, 130, TFT_WHITE);

  // Rounded rect
  tft.fillRoundRect(8, 138, 112, 18, 6, TFT_MAGENTA);
  tft.setTextColor(TFT_WHITE, TFT_MAGENTA);
  tft.setTextSize(1);
  tft.setCursor(14, 143);
  tft.print("rounded rect");
}

// ── animation test ───────────────────────────────────────────────────────────

void drawBouncingBall()
{
  int x = tft.width() / 2;
  int y = tft.height() / 2;
  int vx = 3, vy = 2;
  const int r = 8;
  const int W = tft.width(), H = tft.height();
  tft.fillScreen(TFT_BLACK);

  for (int frame = 0; frame < 120; frame++)
  {
    // Erase old position by drawing black circle
    tft.fillCircle(x, y, r, TFT_BLACK);

    x += vx;
    y += vy;
    if (x - r <= 0 || x + r >= W)
    {
      vx = -vx;
      x += vx * 2;
    }
    if (y - r <= 0 || y + r >= H)
    {
      vy = -vy;
      y += vy * 2;
    }

    tft.fillCircle(x, y, r, TFT_GREEN);
    tft.drawCircle(x, y, r, TFT_WHITE);
    delay(16); // ~60 fps target
  }
}

// ── setup / loop ─────────────────────────────────────────────────────────────

struct Test
{
  const char *name;
  void (*fn)();
  uint32_t holdMs;
};

const Test tests[] = {
    {"Color bars", drawColorBars, 2000},
    {"Checkerboard", []
     { drawCheckerboard(TFT_WHITE, TFT_BLACK); }, 1500},
    {"Rainbow grad.", drawGradientRainbow, 2000},
    {"Text test", drawTextTest, 3000},
    {"Shapes", drawShapes, 3000},
    {"Bounce", drawBouncingBall, 0}, // runs for 120 frames internally
};
const int NUM_TESTS = sizeof(tests) / sizeof(tests[0]);

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[TFT] Init ST7735 128x160...");

  tft.init();
  tft.setRotation(0); // portrait; try 1/2/3 if image is sideways
  tft.fillScreen(TFT_BLACK);

  // Quick init splash
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(8, 60);
  tft.println("TFT OK!");
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(8, 85);
  tft.println("ESP32-C3 Super Mini");
  tft.setCursor(8, 97);
  tft.println("ST7735 128x160");
  delay(1500);

  Serial.println("[TFT] Starting test loop...");
}

void loop()
{
  for (int i = 0; i < NUM_TESTS; i++)
  {
    Serial.printf("[TFT] Test %d/%d: %s\n", i + 1, NUM_TESTS, tests[i].name);
    tests[i].fn();
    if (tests[i].holdMs > 0)
      delay(tests[i].holdMs);
  }
}