#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL3ejLxZIaG"
#define BLYNK_TEMPLATE_NAME "LED"
#define BLYNK_AUTH_TOKEN "kSkSVmwPgsCpRZ7WcOHz5jyTHrJ94xQ7"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

char ssid[] = "Evolve_4G";
char pass[] = "Evolve@7272";

#define GAS_SENSOR 34

// MOTOR PINS (GAS FAN)
#define IN1 26
#define IN2 27

// 🔵 PUMP PINS (NEW)
#define PUMP_IN1 32
#define PUMP_IN2 33

// SERVO SETUP
Servo gasServo;
#define SERVO_PIN 14

// ULTRASONIC PINS
#define TRIG_PIN 5
#define ECHO_PIN 18

BlynkTimer timer;

int gasThreshold = 0;
int baseline = 0;
bool notificationSent = false;

// 🔵 PUMP CONTROL VARIABLES
bool pumpEnabled = false;
float currentWaterPercent = 0;

// ULTRASONIC CALIBRATION
#define FULL_DISTANCE 10
#define EMPTY_DISTANCE 80

// ================== PUMP CONTROL ==================
void pumpOn()
{
  digitalWrite(PUMP_IN1, HIGH);
  digitalWrite(PUMP_IN2, LOW);
}

void pumpOff()
{
  digitalWrite(PUMP_IN1, LOW);
  digitalWrite(PUMP_IN2, LOW);
}

// 🔵 BLYNK SWITCH V6
BLYNK_WRITE(V6)
{
  pumpEnabled = param.asInt();

  if (!pumpEnabled)
  {
    pumpOff();
  }
}

// ================== GAS CALIBRATION ==================
void calibrateSensor()
{
  Serial.println("Calibrating sensor... Keep in clean air");

  long sum = 0;

  for (int i = 0; i < 100; i++)
  {
    sum += analogRead(GAS_SENSOR);
    delay(50);
  }

  baseline = sum / 100;
  gasThreshold = baseline + 150;

  Serial.print("Baseline: ");
  Serial.println(baseline);

  Serial.print("Threshold: ");
  Serial.println(gasThreshold);
}

// ================== MOTOR CONTROL ==================
void motorOn()
{
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void motorOff()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

// ================== ULTRASONIC READ ==================
long readUltrasonicDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
  {
    return -1;
  }

  long distance = duration * 0.034 / 2;
  return distance;
}

// ================== GAS CHECK ==================
void checkGas()
{
  int gasValue = analogRead(GAS_SENSOR);

  int gasPercent = map(gasValue, baseline, 4095, 0, 100);
  gasPercent = constrain(gasPercent, 0, 100);

  Serial.print("Gas Value: ");
  Serial.print(gasValue);
  Serial.print("  Gas %: ");
  Serial.println(gasPercent);

  Blynk.virtualWrite(V4, gasPercent);

  if (gasValue > gasThreshold)
  {
    Serial.println("⚠️ GAS LEAKAGE DETECTED!");
    Blynk.virtualWrite(V1, 1);

    motorOn();
    gasServo.write(180);
  }
  else
  {
    Blynk.virtualWrite(V1, 0);

    motorOff();
    gasServo.write(0);
  }

  if (gasPercent > 50 && notificationSent == false)
  {
    Blynk.logEvent("_gas_leak", "⚠️ Gas level above 50%!");
    notificationSent = true;
  }

  if (gasPercent <= 50)
  {
    notificationSent = false;
  }
}

// ================== WATER LEVEL CHECK ==================
void checkWaterLevel()
{
  long distance = readUltrasonicDistance();

  if (distance == -1)
  {
    Serial.println("Ultrasonic error: No reading");
    return;
  }

  float waterPercent = map(distance, FULL_DISTANCE, EMPTY_DISTANCE, 100, 0);
  waterPercent = constrain(waterPercent, 0, 100);

  currentWaterPercent = waterPercent;

  Serial.print("Water Distance: ");
  Serial.print(distance);
  Serial.print(" cm  Water Level: ");
  Serial.print(waterPercent);
  Serial.println(" %");

  Blynk.virtualWrite(V5, waterPercent);

  // 🔵 AUTO PUMP LOGIC
  if (pumpEnabled)
  {
    if (waterPercent >= 100)
    {
      pumpOff();  // tank full
    }
    else if (waterPercent < 100)
    {
      pumpOn();   // refill automatically
    }
  }
}

// ================== SETUP ==================
void setup()
{
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(GAS_SENSOR, ADC_11db);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  motorOff();

  // 🔵 PUMP PIN SETUP
  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pumpOff();

  gasServo.attach(SERVO_PIN);
  gasServo.write(0);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  delay(20000);

  calibrateSensor();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(1000L, checkGas);
  timer.setInterval(1500L, checkWaterLevel);
}

void loop()
{
  Blynk.run();
  timer.run();
}
