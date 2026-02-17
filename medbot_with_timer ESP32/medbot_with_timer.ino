#include <WiFi.h>
#include <WebServer.h>

// ================= WIFI AP =================
const char* ssid = "Enter your wifi name";
const char* password = "Enter your wifi password";

// ================= MOTOR PINS =================
#define M1A 12
#define M1B 13
#define M2A 14
#define M2B 15

// ================= PUMP (L298N) =================
#define PUMP_IN1 26
#define PUMP_IN2 27

#define BUZZER_PIN 4

WebServer server(80);

// ================= ALARM VARIABLES =================
unsigned long alarmMillis = 0;
bool alarmSet = false;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;

// ================= HTML PAGE =================
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Medical Robot</title>
<style>
body{
  font-family:Arial;
  background:#f4f6f8;
  display:flex;
  flex-direction:column;
  align-items:center;
}

h2,h3{margin:10px;}

.control{
  display:grid;
  grid-template-columns:80px 80px 80px;
  grid-template-rows:60px 60px 60px;
  gap:10px;
  margin:15px;
}

button{
  font-size:14px;
  border:none;
  border-radius:8px;
  color:white;
}

/* MOTOR BUTTONS */
.forward{background:#27ae60; grid-column:2; grid-row:1;}
.left{background:#2980b9; grid-column:1; grid-row:2;}
.stop{background:#c0392b; grid-column:2; grid-row:2;}
.right{background:#2980b9; grid-column:3; grid-row:2;}
.backward{background:#e67e22; grid-column:2; grid-row:3;}

/* PUMP BUTTONS (UPDATED) */
.pump-container{
  display:flex;
  gap:20px;
  margin:15px 0;
}

.pump-on,
.pump-off{
  width:170px;
  height:55px;
  font-size:17px;
}

.pump-on{background:#8e44ad;}
.pump-off{background:#7f8c8d;}

.alarm{background:#16a085;}

input{
  width:160px;
  height:40px;
  margin:6px;
  font-size:15px;
  text-align:center;
}
</style>
</head>

<body>

<h2>Medical Delivery Robot</h2>

<!-- MOTOR CONTROL -->
<div class="control">
  <button class="forward" onclick="fetch('/forward')">Forward</button>
  <button class="left" onclick="fetch('/left')">Left</button>
  <button class="stop" onclick="fetch('/stop')">Stop</button>
  <button class="right" onclick="fetch('/right')">Right</button>
  <button class="backward" onclick="fetch('/backward')">Backward</button>
</div>

<h3>Water Pump</h3>
<div class="pump-container">
  <button class="pump-on" onclick="fetch('/pump_on')">Pump ON</button>
  <button class="pump-off" onclick="fetch('/pump_off')">Pump OFF</button>
</div>

<h3>Medicine Alarm</h3>
<input type="number" id="seconds" placeholder="Seconds from now">
<br>
<button class="alarm" onclick="setAlarm()">Set Alarm</button>

<script>
function setAlarm(){
  let sec = document.getElementById("seconds").value;
  fetch('/set_alarm?sec=' + sec);
}
</script>

</body>
</html>
)rawliteral";

// ================= MOTOR CONTROL =================
void stopMotors(){
  digitalWrite(M1A, LOW); digitalWrite(M1B, LOW);
  digitalWrite(M2A, LOW); digitalWrite(M2B, LOW);
}

void forward(){
  digitalWrite(M1A, HIGH); digitalWrite(M1B, LOW);
  digitalWrite(M2A, LOW);  digitalWrite(M2B, HIGH);
}

void backward(){
  digitalWrite(M1A, LOW);  digitalWrite(M1B, HIGH);
  digitalWrite(M2A, HIGH); digitalWrite(M2B, LOW);
}

void left(){
  digitalWrite(M1A, LOW); digitalWrite(M1B, LOW);
  digitalWrite(M2A, LOW); digitalWrite(M2B, HIGH);
}

void right(){
  digitalWrite(M2A, LOW); digitalWrite(M2B, LOW);
  digitalWrite(M1A, HIGH); digitalWrite(M1B, LOW);
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  pinMode(M1A, OUTPUT);
  pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT);
  pinMode(M2B, OUTPUT);

  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(PUMP_IN1, LOW);
  digitalWrite(PUMP_IN2, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  stopMotors();

  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  server.on("/", [](){ server.send_P(200, "text/html", MAIN_page); });

  server.on("/forward", [](){ forward(); server.send(200); });
  server.on("/backward", [](){ backward(); server.send(200); });
  server.on("/left", [](){ left(); server.send(200); });
  server.on("/right", [](){ right(); server.send(200); });
  server.on("/stop", [](){ stopMotors(); server.send(200); });

  server.on("/pump_on", [](){
    digitalWrite(PUMP_IN1, HIGH);
    digitalWrite(PUMP_IN2, LOW);
    server.send(200);
  });

  server.on("/pump_off", [](){
    digitalWrite(PUMP_IN1, LOW);
    digitalWrite(PUMP_IN2, LOW);
    server.send(200);
  });

  server.on("/set_alarm", [](){
    if(server.hasArg("sec")){
      int sec = server.arg("sec").toInt();
      alarmMillis = millis() + (sec * 1000UL);
      alarmSet = true;
      server.send(200, "text/plain", "Alarm Set");
    }
  });

  server.begin();
}

// ================= LOOP =================
void loop(){
  server.handleClient();

  if(alarmSet && millis() >= alarmMillis){
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerStartTime = millis();
    buzzerActive = true;
    alarmSet = false;
  }

  if(buzzerActive && millis() - buzzerStartTime >= 10000){
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

