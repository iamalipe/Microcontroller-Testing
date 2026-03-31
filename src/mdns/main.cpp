#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

#define LED_PIN 8

const char *ssid = "MyHomeIOT";
const char *password = "Abcd1234";
const char *hostname = "esp32"; // access via http://esp32.local

WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32-C3</title>
  <style>
    body { font-family: sans-serif; text-align: center; padding: 40px; background: #111; color: #eee; }
    h1   { color: #4fc3f7; }
    .btn { padding: 12px 32px; margin: 8px; border: none; border-radius: 8px;
           font-size: 1rem; cursor: pointer; }
    .on  { background: #4fc3f7; color: #111; }
    .off { background: #444;    color: #eee; }
    #status { margin-top: 20px; font-size: 0.9rem; color: #aaa; }
  </style>
</head>
<body>
  <h1>ESP32-C3 Control</h1>
  <button class="btn on"  onclick="send('/led/on')">LED ON</button>
  <button class="btn off" onclick="send('/led/off')">LED OFF</button>
  <p id="status">Ready</p>
  <script>
    async function send(path) {
      const res = await fetch(path);
      const txt = await res.text();
      document.getElementById('status').innerText = txt;
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }
void handleLedOn()
{
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "LED ON");
}
void handleLedOff()
{
  digitalWrite(LED_PIN, HIGH);
  server.send(200, "text/plain", "LED OFF");
}
void handleNotFound() { server.send(404, "text/plain", "Not found"); }

void setup()
{
  Serial.begin(115200);
  delay(2000);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // off initially

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    if (millis() - start > 15000)
    { // 15s timeout
      Serial.println("\nFailed! Reason: " + String(WiFi.status()));
      break;
    }
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // mDNS
  if (MDNS.begin(hostname))
  {
    Serial.println("mDNS: http://" + String(hostname) + ".local");
  }

  // Routes
  server.on("/", handleRoot);
  server.on("/led/on", handleLedOn);
  server.on("/led/off", handleLedOff);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}