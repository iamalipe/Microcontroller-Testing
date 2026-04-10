// AHT21B Temperature & Humidity Sensor

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_AHTX0 aht;

void setup()
{
  Serial.begin(115200);
  delay(2000); // Wait for USB CDC serial
  Serial.println("\nAHT21B Test on ESP32-C3");

  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize the sensor
  if (!aht.begin())
  {
    Serial.println("Failed to find AHT21B sensor. Check wiring!");
    while (1)
      delay(1000);
  }
  Serial.println("AHT21B found and initialized.");

  // Optional: set measurement mode (normal = 0, low power = 1, etc.)
  // aht.setMode(AHTX0_MODE_NORMAL);  // default is normal
}

void loop()
{
  sensors_event_t humidity, temp;
  // Get new readings
  if (!aht.getEvent(&humidity, &temp))
  {
    Serial.println("Failed to read from AHT21B!");
    delay(2000);
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println(" %");

  Serial.println("---");
  delay(2000); // read every 2 seconds
}