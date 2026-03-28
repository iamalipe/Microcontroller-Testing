#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

// ── Config ──────────────────────────────────────────────
#define BUZZER_PIN D5 // GPIO14 on NodeMCU

const char *SSID = "MyHomeDev";
const char *PASSWORD = "yourpassword";
// ────────────────────────────────────────────────────────

AsyncWebServer server(80);

bool buzzerActive = false;
uint32_t currentFreq = 1000;

// ── Buzzer Helpers ───────────────────────────────────────
void buzzerOn(uint32_t freq)
{
  tone(BUZZER_PIN, freq);
  buzzerActive = true;
  currentFreq = freq;
}

void buzzerOff()
{
  noTone(BUZZER_PIN);
  buzzerActive = false;
}

void beep(uint32_t freq, uint32_t duration_ms)
{
  buzzerOn(freq);
  delay(duration_ms);
  buzzerOff();
}

void playAlarm()
{
  for (int i = 0; i < 4; i++)
  {
    buzzerOn(1200);
    delay(150);
    buzzerOn(600);
    delay(150);
  }
  buzzerOff();
}

void playMelody()
{
  uint32_t notes[] = {262, 294, 330, 349, 392, 440, 494, 523};
  for (int i = 0; i < 8; i++)
  {
    buzzerOn(notes[i]);
    delay(160);
    buzzerOff();
    delay(40);
  }
}

void playSOS()
{
  auto dot = []()
  { buzzerOn(880); delay(100); buzzerOff(); delay(100); };
  auto dash = []()
  { buzzerOn(880); delay(300); buzzerOff(); delay(100); };
  for (int i = 0; i < 3; i++)
    dot();
  delay(200);
  for (int i = 0; i < 3; i++)
    dash();
  delay(200);
  for (int i = 0; i < 3; i++)
    dot();
}
// ─────────────────────────────────────────────────────────

// ── Embedded HTML ─────────────────────────────────────────
const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Buzzer</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@400;700;900&display=swap');

  :root {
    --bg:     #080c10;
    --panel:  #0d1117;
    --border: #1c2a3a;
    --accent: #00e5ff;
    --green:  #00ff88;
    --red:    #ff3b5c;
    --amber:  #ffaa00;
    --purple: #bf5fff;
    --text:   #c8d8e8;
    --dim:    #4a6070;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Share Tech Mono', monospace;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 1.5rem 1rem 3rem;
    background-image: repeating-linear-gradient(
      0deg, transparent, transparent 2px,
      rgba(0,229,255,0.012) 2px, rgba(0,229,255,0.012) 4px
    );
  }
  header { text-align: center; margin-bottom: 1.8rem; }
  header h1 {
    font-family: 'Orbitron', sans-serif;
    font-weight: 900;
    font-size: clamp(1.4rem, 5vw, 2rem);
    letter-spacing: 0.15em;
    color: var(--accent);
    text-shadow: 0 0 20px rgba(0,229,255,0.5);
  }
  header p { font-size: 0.7rem; color: var(--dim); margin-top: 0.25rem; letter-spacing: 0.2em; }

  .status-bar {
    display: flex; align-items: center; gap: 0.6rem;
    padding: 0.45rem 1.1rem;
    border: 1px solid var(--border);
    border-radius: 4px; margin-bottom: 1.6rem;
    font-size: 0.75rem; letter-spacing: 0.1em;
    color: var(--dim); transition: all 0.3s;
    background: var(--panel);
  }
  .status-bar.active { border-color: var(--green); color: var(--green); box-shadow: 0 0 10px rgba(0,255,136,0.15); }
  .led { width: 8px; height: 8px; border-radius: 50%; background: var(--dim); transition: all 0.3s; flex-shrink: 0; }
  .status-bar.active .led { background: var(--green); box-shadow: 0 0 6px var(--green), 0 0 14px var(--green); animation: pulse 1.2s ease-in-out infinite; }
  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.4} }

  .card {
    width: 100%; max-width: 400px;
    background: var(--panel); border: 1px solid var(--border);
    border-radius: 6px; padding: 1.2rem; margin-bottom: 0.9rem;
    position: relative; overflow: hidden;
  }
  .card::before {
    content: ''; position: absolute; top: 0; left: 0; right: 0; height: 2px;
    background: linear-gradient(90deg, transparent, var(--accent), transparent); opacity: 0.4;
  }
  .card-label {
    font-size: 0.6rem; letter-spacing: 0.2em; color: var(--dim);
    text-transform: uppercase; margin-bottom: 1rem;
    display: flex; align-items: center; gap: 0.5rem;
  }
  .card-label::after { content: ''; flex: 1; height: 1px; background: var(--border); }

  .freq-display { text-align: center; margin-bottom: 0.8rem; }
  .freq-display .big {
    font-family: 'Orbitron', sans-serif; font-size: 2.2rem;
    font-weight: 700; color: var(--accent);
    text-shadow: 0 0 15px rgba(0,229,255,0.4); line-height: 1;
  }
  .freq-display .unit { font-size: 0.7rem; color: var(--dim); letter-spacing: 0.15em; margin-top: 0.15rem; }

  .slider-wrap { margin-bottom: 1rem; }
  input[type=range] {
    -webkit-appearance: none; width: 100%; height: 4px;
    background: var(--border); border-radius: 2px; outline: none; cursor: pointer;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none; width: 16px; height: 16px;
    border-radius: 50%; background: var(--accent); box-shadow: 0 0 8px var(--accent); cursor: pointer;
  }
  .range-labels { display: flex; justify-content: space-between; font-size: 0.6rem; color: var(--dim); margin-top: 0.3rem; }

  .btn {
    display: block; width: 100%; padding: 0.7rem 1rem;
    border: 1px solid transparent; border-radius: 4px;
    font-family: 'Share Tech Mono', monospace; font-size: 0.8rem;
    letter-spacing: 0.12em; cursor: pointer; transition: all 0.15s; text-transform: uppercase;
  }
  .btn:active { transform: scale(0.97); }
  .btn-on     { background: rgba(0,255,136,0.1); border-color: var(--green);  color: var(--green); }
  .btn-on:hover  { background: rgba(0,255,136,0.2); box-shadow: 0 0 12px rgba(0,255,136,0.2); }
  .btn-off    { background: rgba(255,59,92,0.1);  border-color: var(--red);    color: var(--red); }
  .btn-off:hover { background: rgba(255,59,92,0.2);  box-shadow: 0 0 12px rgba(255,59,92,0.2); }
  .btn-beep   { background: rgba(0,229,255,0.1);  border-color: var(--accent); color: var(--accent); }
  .btn-beep:hover { background: rgba(0,229,255,0.2); box-shadow: 0 0 12px rgba(0,229,255,0.2); }
  .btn-amber  { background: rgba(255,170,0,0.1);  border-color: var(--amber);  color: var(--amber); }
  .btn-amber:hover { background: rgba(255,170,0,0.2); box-shadow: 0 0 12px rgba(255,170,0,0.2); }
  .btn-purple { background: rgba(191,95,255,0.1); border-color: var(--purple); color: var(--purple); }
  .btn-purple:hover { background: rgba(191,95,255,0.2); box-shadow: 0 0 12px rgba(191,95,255,0.2); }

  .grid2 { display: grid; grid-template-columns: 1fr 1fr; gap: 0.5rem; }
  .grid3 { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 0.5rem; }

  .dual-slider { display: flex; flex-direction: column; gap: 0.8rem; margin-bottom: 1rem; }
  .dual-slider label { font-size: 0.65rem; color: var(--dim); display: flex; justify-content: space-between; }
  .dual-slider label span { color: var(--accent); }

  .wave-wrap {
    height: 36px; display: flex; align-items: center;
    justify-content: center; gap: 3px; margin-bottom: 0.8rem;
  }
  .bar { width: 3px; background: var(--accent); border-radius: 2px; opacity: 0.3; height: 4px; transition: height 0.1s; }
  .wave-wrap.playing .bar { animation: wave 0.8s ease-in-out infinite; opacity: 0.8; }
  .bar:nth-child(1){animation-delay:0.0s} .bar:nth-child(2){animation-delay:0.1s}
  .bar:nth-child(3){animation-delay:0.2s} .bar:nth-child(4){animation-delay:0.3s}
  .bar:nth-child(5){animation-delay:0.4s} .bar:nth-child(6){animation-delay:0.3s}
  .bar:nth-child(7){animation-delay:0.2s} .bar:nth-child(8){animation-delay:0.1s}
  .bar:nth-child(9){animation-delay:0.0s}
  @keyframes wave { 0%,100%{height:4px} 50%{height:28px} }
</style>
</head>
<body>

<header>
  <h1>&#9650; BUZZER CTRL</h1>
  <p>NodeMCU ESP8266</p>
</header>

<div class="status-bar" id="statusBar">
  <div class="led"></div>
  <span id="statusText">STANDBY &mdash; 0 Hz</span>
</div>

<div class="wave-wrap" id="wave">
  <div class="bar"></div><div class="bar"></div><div class="bar"></div>
  <div class="bar"></div><div class="bar"></div><div class="bar"></div>
  <div class="bar"></div><div class="bar"></div><div class="bar"></div>
</div>

<!-- Continuous Tone -->
<div class="card">
  <div class="card-label">Continuous Tone</div>
  <div class="freq-display">
    <div class="big" id="freqBig">1000</div>
    <div class="unit">HERTZ</div>
  </div>
  <div class="slider-wrap">
    <input type="range" id="freqSlider" min="100" max="5000" value="1000"
           oninput="document.getElementById('freqBig').textContent=this.value">
    <div class="range-labels"><span>100 Hz</span><span>5000 Hz</span></div>
  </div>
  <div class="grid2">
    <button class="btn btn-on"  onclick="post('/on',{freq:slider()})">&#9654; EMIT</button>
    <button class="btn btn-off" onclick="post('/off',{})">&#9632; STOP</button>
  </div>
</div>

<!-- Single Beep -->
<div class="card">
  <div class="card-label">Single Beep</div>
  <div class="dual-slider">
    <div>
      <label>FREQUENCY <span id="bfVal">1000</span> Hz</label>
      <input type="range" id="bfSlider" min="100" max="5000" value="1000"
             oninput="document.getElementById('bfVal').textContent=this.value">
    </div>
    <div>
      <label>DURATION <span id="bdVal">300</span> ms</label>
      <input type="range" id="bdSlider" min="50" max="2000" value="300"
             oninput="document.getElementById('bdVal').textContent=this.value">
    </div>
  </div>
  <button class="btn btn-beep" onclick="post('/beep',{freq:bfreq(),duration:bdur()})">&#9889; BEEP</button>
</div>

<!-- Patterns -->
<div class="card">
  <div class="card-label">Patterns</div>
  <div class="grid3">
    <button class="btn btn-amber"  onclick="post('/alarm',{})">&#128680; ALARM</button>
    <button class="btn btn-beep"   onclick="post('/melody',{})">&#127925; SCALE</button>
    <button class="btn btn-purple" onclick="post('/sos',{})">&#9889; SOS</button>
  </div>
</div>

<script>
  const slider = () => +document.getElementById('freqSlider').value;
  const bfreq  = () => +document.getElementById('bfSlider').value;
  const bdur   = () => +document.getElementById('bdSlider').value;

  async function post(url, params) {
    const body = new URLSearchParams(params);
    await fetch(url, { method: 'POST', body });
    await refreshStatus();
  }

  async function refreshStatus() {
    try {
      const res  = await fetch('/status');
      const data = await res.json();
      const bar  = document.getElementById('statusBar');
      const txt  = document.getElementById('statusText');
      const wave = document.getElementById('wave');
      if (data.active) {
        bar.className = 'status-bar active';
        txt.textContent = 'ACTIVE \u2014 ' + data.freq + ' Hz';
        wave.className = 'wave-wrap playing';
      } else {
        bar.className = 'status-bar';
        txt.textContent = 'STANDBY \u2014 0 Hz';
        wave.className = 'wave-wrap';
      }
    } catch(e) {}
  }

  setInterval(refreshStatus, 2000);
  refreshStatus();
</script>
</body>
</html>
)rawliteral";
// ─────────────────────────────────────────────────────────

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n[BOOT] Buzzer Control starting...");

  pinMode(BUZZER_PIN, OUTPUT);
  buzzerOff();

  // WiFi
  WiFi.mode(WIFI_STA);
  // WiFi.begin(SSID, PASSWORD);
  WiFi.begin(SSID);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());

  // ── Routes ────────────────────────────────────────────────
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
            { req->send_P(200, "text/html", HTML); });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req)
            {
    String json = "{\"active\":" + String(buzzerActive ? "true" : "false") +
                  ",\"freq\":"   + String(currentFreq) + "}";
    req->send(200, "application/json", json); });

  server.on("/on", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    uint32_t freq = req->hasParam("freq", true)
                    ? req->getParam("freq", true)->value().toInt() : 1000;
    buzzerOn(freq);
    req->send(200, "application/json", "{\"status\":\"on\"}"); });

  server.on("/off", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    buzzerOff();
    req->send(200, "application/json", "{\"status\":\"off\"}"); });

  server.on("/beep", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    uint32_t freq = req->hasParam("freq", true)
                    ? req->getParam("freq", true)->value().toInt() : 1000;
    uint32_t dur  = req->hasParam("duration", true)
                    ? req->getParam("duration", true)->value().toInt() : 300;
    beep(freq, dur);
    req->send(200, "application/json", "{\"status\":\"beeped\"}"); });

  server.on("/alarm", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    playAlarm();
    req->send(200, "application/json", "{\"status\":\"alarm\"}"); });

  server.on("/melody", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    playMelody();
    req->send(200, "application/json", "{\"status\":\"melody\"}"); });

  server.on("/sos", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    playSOS();
    req->send(200, "application/json", "{\"status\":\"sos\"}"); });

  server.begin();
  Serial.println("[HTTP] Server started → http://" + WiFi.localIP().toString());
}

void loop()
{
  // ESP8266 needs yield() in long blocking calls
  // All patterns use delay() which calls yield() internally — fine
}