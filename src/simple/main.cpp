#include <Arduino.h>

#define LED_PIN 8
#define TOUCH_PIN 3 // any free GPIO

void setup()
{
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  int state = digitalRead(TOUCH_PIN);

  if (state == HIGH)
  {
    Serial.println("Touched!");
  }
  digitalWrite(LED_PIN, LOW); // ON  (active low)
  delay(500);
  digitalWrite(LED_PIN, HIGH); // OFF
  delay(500);
}