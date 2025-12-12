#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"

// ====== WiFi credentials ======
const char* ssid = "Enr.05";
const char* password = "Ebenezer05";

// ====== Motor pins ======
#define IN1 12
#define IN2 13
#define IN3 14
#define IN4 15

// ====== Buzzer pin ======
#define BUZZER_PIN 2   // You can change this to another GPIO (avoid 1, 3, 0, 16)

// ====== NTP Settings ======
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;  // For IST (GMT+5:30)
const int daylightOffset_sec = 0;

WebServer server(80);

// ====== Medicine Time Variables ======
int medHour[3] = {8, 13, 20};   // Default: 8AM, 1PM, 8PM
int medMin[3]  = {0, 0, 0};
bool buzzerActive = false;
unsigned long buzzerStart = 0;

// ====== Camera config for AI Thinker ESP32-CAM ======
camera_config_t config = {
  .pin_pwdn       = 32,
  .pin_reset      = -1,
  .pin_xclk       = 0,
  .pin_sccb_sda   = 26,
  .pin_sccb_scl   = 27,
  .pin_d7         = 35,
  .pin_d6         = 34,
  .pin_d5         = 39,
  .pin_d4         = 36,
  .pin_d3         = 21,
  .pin_d2         = 19,
  .pin_d1         = 18,
  .pin_d0         = 5,
  .pin_vsync      = 25,
  .pin_href       = 23,
  .pin_pclk       = 22,
  .xclk_freq_hz   = 20000000,
  .ledc_timer     = LEDC_TIMER_0,
  .ledc_channel   = LEDC_CHANNEL_0,
  .pixel_format   = PIXFORMAT_JPEG,
  .frame_size     = FRAMESIZE_QVGA,
  .jpeg_quality   = 10,
  .fb_count       = 1
};

// ====== Motor control function ======
void moveMotor(String direction) {
  if (direction == "forward") {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else if (direction == "backward") {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  } else if (direction == "left") {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else if (direction == "right") {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  }
}

// ====== Styled HTML UI ======
void handleRoot() {
  String html = R"rawliteral(
  <html>
  <head>
  <title>ESP32-CAM Rover + Medicine Reminder</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { background:#111; color:white; font-family:Arial; text-align:center; margin:0; padding:0; }
    h1 { color:#00e0ff; margin-top:10px; }
    .video-container { margin:10px auto; width:340px; height:260px; border:3px solid #00e0ff; border-radius:10px; overflow:hidden; }
    img { width:100%; height:auto; display:block; }
    .controls { display:grid; grid-template-columns:100px 100px 100px; grid-gap:10px; justify-content:center; margin-top:20px; }
    button {
      background:#00e0ff; border:none; color:black; font-size:22px; font-weight:bold;
      padding:20px; border-radius:10px; transition:0.1s; user-select:none;
    }
    button:active { background:#00aacc; color:white; transform:scale(0.95); }
    .empty { visibility:hidden; }
    .section { margin-top:30px; background:#222; padding:15px; border-radius:10px; width:80%; margin:auto; }
    input { padding:5px; width:60px; border:none; border-radius:5px; text-align:center; font-size:18px; }
  </style>
  <script>
    function sendCmd(cmd) { fetch('/move?dir=' + cmd); }
    function stopCmd() { fetch('/move?dir=stop'); }
    function press(cmd) { sendCmd(cmd); }
    function release() { stopCmd(); }

    function saveTime() {
      let h1=document.getElementById('h1').value, m1=document.getElementById('m1').value;
      let h2=document.getElementById('h2').value, m2=document.getElementById('m2').value;
      let h3=document.getElementById('h3').value, m3=document.getElementById('m3').value;
      fetch(`/save?s1=${h1}:${m1}&s2=${h2}:${m2}&s3=${h3}:${m3}`).then(r=>alert("Times Updated!"));
    }
  </script>
  </head>
  <body>
  <h1>ESP32-CAM Rover Control</h1>
  <div class="video-container"><img src="/stream"></div>

  <div class="controls">
    <div class="empty"></div>
    <button onmousedown="press('forward')" onmouseup="release()" ontouchstart="press('forward')" ontouchend="release()">↑</button>
    <div class="empty"></div>

    <button onmousedown="press('left')" onmouseup="release()" ontouchstart="press('left')" ontouchend="release()">←</button>
    <button onmousedown="press('stop')" onmouseup="release()" ontouchstart="press('stop')" ontouchend="release()">⏹</button>
    <button onmousedown="press('right')" onmouseup="release()" ontouchstart="press('right')" ontouchend="release()">→</button>

    <div class="empty"></div>
    <button onmousedown="press('backward')" onmouseup="release()" ontouchstart="press('backward')" ontouchend="release()">↓</button>
    <div class="empty"></div>
  </div>

  <div class="section">
    <h2>💊 Medicine Reminder Settings</h2>
    <p>Set 24-hour time format (HH:MM)</p>
    <div>
      Morning: <input type="number" id="h1" min="0" max="23" value="%H1%"> : <input type="number" id="m1" min="0" max="59" value="%M1%"><br><br>
      Afternoon: <input type="number" id="h2" min="0" max="23" value="%H2%"> : <input type="number" id="m2" min="0" max="59" value="%M2%"><br><br>
      Night: <input type="number" id="h3" min="0" max="23" value="%H3%"> : <input type="number" id="m3" min="0" max="59" value="%M3%"><br><br>
      <button onclick="saveTime()">💾 Save Schedule</button>
    </div>
  </div>

  </body>
  </html>
  )rawliteral";

  html.replace("%H1%", String(medHour[0]));
  html.replace("%M1%", String(medMin[0]));
  html.replace("%H2%", String(medHour[1]));
  html.replace("%M2%", String(medMin[1]));
  html.replace("%H3%", String(medHour[2]));
  html.replace("%M3%", String(medMin[2]));

  server.send(200, "text/html", html);
}

// ====== Handle movement ======
void handleMove() {
  if (server.hasArg("dir")) {
    String direction = server.arg("dir");
    moveMotor(direction);
    Serial.println("Move: " + direction);
  }
  server.send(200, "text/plain", "OK");
}

// ====== Handle saving medicine time ======
void handleSave() {
  for (int i = 0; i < 3; i++) {
    String arg = "s" + String(i + 1);
    if (server.hasArg(arg)) {
      String t = server.arg(arg);
      int colon = t.indexOf(':');
      if (colon > 0) {
        medHour[i] = t.substring(0, colon).toInt();
        medMin[i] = t.substring(colon + 1).toInt();
      }
    }
  }
  server.send(200, "text/plain", "Times Updated");
  Serial.println("Medicine times updated");
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  moveMotor("stop");

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    return;
  }

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/save", handleSave);

  // ====== Video stream ======
  server.on("/stream", HTTP_GET, []() {
    WiFiClient client = server.client();
    String response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    client.print(response);
    while (client.connected()) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) continue;
      client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
      client.write(fb->buf, fb->len);
      client.print("\r\n");
      esp_camera_fb_return(fb);
      delay(100);
    }
  });

  server.begin();
}

// ====== Loop ======
void loop() {
  server.handleClient();

  // Get current time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  int hourNow = timeinfo.tm_hour;
  int minNow = timeinfo.tm_min;

  // Check for medicine times
  for (int i = 0; i < 3; i++) {
    if (hourNow == medHour[i] && minNow == medMin[i] && !buzzerActive) {
      buzzerActive = true;
      buzzerStart = millis();
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.printf("Medicine time %d reached! Buzzer ON\n", i + 1);
    }
  }

  // Turn off buzzer after 10 sec
  if (buzzerActive && millis() - buzzerStart > 10000) {
    buzzerActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Buzzer OFF");
  }
}
