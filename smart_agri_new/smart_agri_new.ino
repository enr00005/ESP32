#define BLYNK_TEMPLATE_ID "TMPL3Mzl4NUfj"
#define BLYNK_TEMPLATE_NAME "smart irrigation"
#define BLYNK_AUTH_TOKEN "6QphUh7au89Doq0LVBAU5_8a4nScN-vi"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// =====================================================
// Wi-Fi Credentials
// =====================================================

char ssid[] = "moto";
char pass[] = "Ebenezer";

// =====================================================
// Hardware Pins
// =====================================================

#define IN1 21
#define IN2 33
#define MOISTURE_PIN 34

// =====================================================
// Moisture Calibration
// =====================================================

const int DRY_VALUE = 3200;
const int WET_VALUE = 2800;

// =====================================================
// Automatic Moisture Thresholds
// =====================================================

// If moisture is 0% or below → Pump ON
const int AUTO_ON_THRESHOLD = 0;

// If moisture reaches 90% → Pump OFF
const int AUTO_OFF_THRESHOLD = 90;

// =====================================================
// Variables
// =====================================================

// Actual pump state
bool currentPumpState = false;

// Master switch from Blynk
// false = irrigation disabled
// true  = automatic irrigation enabled
bool irrigationEnabled = false;

BlynkTimer timer;


// =====================================================
// PUMP CONTROL
// =====================================================

void setPumpState(bool turnOn) {

  currentPumpState = turnOn;

  if (turnOn) {

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    Serial.println("================================");
    Serial.println("PUMP STATUS: ON");
    Serial.println("================================");

  } else {

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    Serial.println("================================");
    Serial.println("PUMP STATUS: OFF");
    Serial.println("================================");
  }
}


// =====================================================
// READ SOIL MOISTURE
// =====================================================

void checkSoilMoisture() {

  int rawValue = analogRead(MOISTURE_PIN);

  // Convert raw ADC value to percentage
  int moisturePercent = map(
    rawValue,
    DRY_VALUE,
    WET_VALUE,
    0,
    100
  );

  moisturePercent = constrain(
    moisturePercent,
    0,
    100
  );

  // =================================================
  // SEND MOISTURE TO BLYNK
  // =================================================

  Blynk.virtualWrite(V1, moisturePercent);

  // =================================================
  // SERIAL MONITOR
  // =================================================

  Serial.println();
  Serial.println("--------------------------------");

  Serial.print("Raw Sensor Value: ");
  Serial.println(rawValue);

  Serial.print("Soil Moisture: ");
  Serial.print(moisturePercent);
  Serial.println("%");

  Serial.print("Irrigation System: ");

  if (irrigationEnabled) {
    Serial.println("ENABLED");
  } else {
    Serial.println("DISABLED");
  }

  Serial.print("Pump: ");

  if (currentPumpState) {
    Serial.println("ON");
  } else {
    Serial.println("OFF");
  }

  Serial.println("--------------------------------");


  // =================================================
  // MASTER SWITCH CHECK
  // =================================================

  // If V2 is OFF, pump MUST remain OFF.
  // Sensor readings are ignored.
  
  if (!irrigationEnabled) {

    if (currentPumpState) {
      setPumpState(false);
    }

    return;
  }


  // =================================================
  // AUTOMATIC IRRIGATION
  // =================================================

  // At this point V2 is ON.
  // Automatic irrigation is enabled.


  // =================================================
  // SOIL VERY DRY → PUMP ON
  // =================================================

  if (
    moisturePercent <= AUTO_ON_THRESHOLD &&
    currentPumpState == false
  ) {

    setPumpState(true);

    Blynk.virtualWrite(V2, 1);

    Serial.println("SOIL IS DRY!");
    Serial.println("Automatic irrigation started.");

    Blynk.logEvent(
      "moisture_low",
      "Soil moisture reached 0%. Pump turned ON automatically."
    );
  }


  // =================================================
  // SOIL WET → PUMP OFF
  // =================================================

  if (
    moisturePercent >= AUTO_OFF_THRESHOLD &&
    currentPumpState == true
  ) {

    setPumpState(false);

    Blynk.virtualWrite(V2, 1);

    Serial.println("SOIL MOISTURE REACHED 90%!");
    Serial.println("Pump turned OFF automatically.");

    Blynk.logEvent(
      "moisture_high",
      "Soil moisture reached 90%. Pump turned OFF automatically."
    );
  }
}


// =====================================================
// BLYNK CONNECTED
// =====================================================

BLYNK_CONNECTED() {

  Serial.println("Blynk connected.");

  // Get the V2 switch state from Blynk
  Blynk.syncVirtual(V2);
}


// =====================================================
// BLYNK MASTER SWITCH
// =====================================================

BLYNK_WRITE(V2) {

  int switchState = param.asInt();

  // =================================================
  // USER TURNED V2 OFF
  // =================================================

  if (switchState == 0) {

    irrigationEnabled = false;

    Serial.println();
    Serial.println("================================");
    Serial.println("IRRIGATION DISABLED");
    Serial.println("BLYNK SWITCH: OFF");
    Serial.println("PUMP FORCED OFF");
    Serial.println("================================");

    // Immediately stop pump
    setPumpState(false);
  }


  // =================================================
  // USER TURNED V2 ON
  // =================================================

  else {

    irrigationEnabled = true;

    Serial.println();
    Serial.println("================================");
    Serial.println("IRRIGATION ENABLED");
    Serial.println("BLYNK SWITCH: ON");
    Serial.println("AUTOMATIC MODE ACTIVE");
    Serial.println("================================");

    // Do NOT immediately turn pump ON.
    //
    // The moisture sensor will decide.
    //
    // checkSoilMoisture() will:
    //
    // 0%  → Pump ON
    // 90% → Pump OFF
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("================================");
  Serial.println("SMART IRRIGATION SYSTEM");
  Serial.println("================================");
  Serial.println("MASTER SWITCH + AUTOMATIC MODE");
  Serial.println("================================");


  // =================================================
  // PIN CONFIGURATION
  // =================================================

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(MOISTURE_PIN, INPUT);

  // ESP32 ADC resolution
  analogReadResolution(12);


  // =================================================
  // INITIAL PUMP STATE
  // =================================================

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  currentPumpState = false;
  irrigationEnabled = false;


  // =================================================
  // CONNECT TO BLYNK
  // =================================================

  Serial.println("Connecting to Blynk...");

  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );


  // =================================================
  // CHECK SENSOR EVERY 2 SECONDS
  // =================================================

  timer.setInterval(
    2000L,
    checkSoilMoisture
  );


  Serial.println();
  Serial.println("System started.");
  Serial.println("Irrigation is DISABLED by default.");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  Blynk.run();

  timer.run();
}
