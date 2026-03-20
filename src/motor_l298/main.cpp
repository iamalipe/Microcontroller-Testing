#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ── WiFi ───────────────────────────────────────────
const char *STA_SSID = "My_Home";
const char *STA_PASS = "w4@Rwifi??";

// ── Pins ───────────────────────────────────────────
#define ENA_PIN 5
#define IN1_PIN 6
#define IN2_PIN 7

// ── PWM (LEDC) ─────────────────────────────────────
#define PWM_CH 0
#define PWM_FREQ 5000
#define PWM_BITS 8

// ── Server & WS ────────────────────────────────────
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ── Motor ──────────────────────────────────────────
void motorRun(char dir, int pwm)
{
  pwm = constrain(pwm, 0, 255);
  switch (dir)
  {
  case 'F':
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(PWM_CH, pwm);
    break;
  case 'B':
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
    ledcWrite(PWM_CH, pwm);
    break;
  default: // 'S'
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(PWM_CH, 0);
  }
  Serial.printf("[MOTOR] dir=%c  pwm=%d\n", dir, pwm);
}

// ── WebSocket ──────────────────────────────────────
// Message format: "F:200" | "B:150" | "S:0"
void onWsEvent(AsyncWebSocket *s, AsyncWebSocketClient *c,
               AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("[WS] Client #%u connected from %s\n",
                  c->id(), c->remoteIP().toString().c_str());
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("[WS] Client #%u disconnected — stopping motor\n", c->id());
    motorRun('S', 0);
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 &&
        info->len == len && info->opcode == WS_TEXT)
    {
      char msg[16] = {0};
      memcpy(msg, data, min(len, (size_t)15));
      char dir = msg[0];
      int pwm = atoi(msg + 2);
      motorRun(dir, pwm);
    }
  }
}

// ── Setup ──────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[BOOT] Motor Control starting...");

  // Motor pins
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);

  // PWM
  ledcSetup(PWM_CH, PWM_FREQ, PWM_BITS);
  ledcAttachPin(ENA_PIN, PWM_CH);
  ledcWrite(PWM_CH, 0);
  Serial.println("[PWM]  LEDC configured on GPIO5");

  // LittleFS
  if (!LittleFS.begin())
  {
    Serial.println("[FS]   LittleFS mount FAILED — did you run 'pio run -t uploadfs'?");
  }
  else
  {
    Serial.println("[FS]   LittleFS mounted OK");
  }

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PASS);
  Serial.printf("[WiFi] Connecting to '%s'", STA_SSID);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30)
  {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  }
  else
  {
    Serial.println("\n[WiFi] FAILED to connect — check SSID/password");
  }

  // mDNS
  if (MDNS.begin("motor"))
  {
    Serial.println("[mDNS] http://motor.local");
  }

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
            { req->send(LittleFS, "/index.html", "text/html"); });

  server.onNotFound([](AsyncWebServerRequest *req)
                    { req->send(404, "text/plain", "Not found"); });

  server.begin();
  Serial.println("[HTTP] Server started");
  Serial.println("──────────────────────────────");
  Serial.printf("  Open: http://%s\n", WiFi.localIP().toString().c_str());
  Serial.println("  Also: http://motor.local");
  Serial.println("──────────────────────────────");
}

void loop()
{
  ws.cleanupClients();
}