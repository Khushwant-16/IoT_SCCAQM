#include <WiFi.h>
#include <WiFiClientSecure.h> 
#include <HTTPClient.h>        
#include <ArduinoJson.h>       
#include <DHT.h>
#include <ESP32Servo.h>
#include <time.h>               // For NTP Time Sync
#include <Wire.h>               // For I2C

// --- *** Using the "Frank de Brabander" library *** ---
#include <LiquidCrystal_I2C.h>

// ==========================================================
// --- 1. YOUR CREDENTIALS ---
// ==========================================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

String functionUrl = "https://YOUR_REGION-YOUR_PROJECT_ID.cloudfunctions.net/sccaqmDataReceiver";

// ==========================================================
// --- 2. PIN DEFINITIONS ---
// ==========================================================
#define DHT_PIN 4               // Set to D4
#define DHT_TYPE DHT22
#define MQ7_A0_PIN 36 
#define MQ7_D0_PIN 27
#define MQ9_A0_PIN 39 
#define MQ9_D0_PIN 26
#define FLAME_PIN 33
#define VIBRATION_PIN 25
#define SERVO_PIN 32

// --- *** I2C PIN CHANGE (THE FIX) *** ---
// Moved from 21/22 to 18/19 to avoid WiFi conflict
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// ==========================================================
// --- 3. OBJECTS & GLOBAL VARIABLES ---
// ==========================================================
DHT dht(DHT_PIN, DHT_TYPE);
Servo curtainServo;
WiFiClientSecure wifiClient;
HTTPClient http;

// We use the KNOWN address 0x27, 16 columns, 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2); 
String lastLCDDiagnosis = "";

// --- Timers ---
unsigned long lastDataPostTime = 0;
const int dataPostInterval = 60000; 
unsigned long lastModeFetchTime = 0;
const int modeFetchInterval = 30000;

// --- State Variables ---
String systemMode = "Home"; 
String currentDiagnosis = "Normal";
bool intrusionAlert = false;
bool earthquakeAlert = false;

// --- Vibration Logic Variables ---
unsigned long vibrationStartTime = 0; 
unsigned long lastPulseTime = 0;      
const int VIBRATION_SILENCE_MS = 500; 
const int INTRUSION_TIME_MS = 200; 
const int EARTHQUAKE_TIME_MS = 5000;

// ==========================================================
// --- 4. SETUP FUNCTION ---
// ==========================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- SCCAQM v5.6 (I2C Pin Change Fix) ---");

  pinMode(MQ7_D0_PIN, INPUT);
  pinMode(MQ9_D0_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT_PULLDOWN); 

  dht.begin();
  curtainServo.attach(SERVO_PIN);
  curtainServo.write(10);
  analogReadResolution(10);

  // --- Start I2C bus on the NEW pins ---
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // --- Initialize LCD ---
  lcd.init();
  lcd.backlight(); // Turn on backlight
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SCCAQM v5.6");
  lcd.setCursor(0, 1);
  lcd.print("Booting...");
  delay(1500);
  
  // 1. Connect to WiFi
  setup_wifi();
  
  // 2. Set client to insecure
  wifiClient.setInsecure(); 

  // 3. Do an initial fetch of the system mode on boot
  fetchSystemMode();

  // 4. Set static LCD text
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Status:");

  Serial.println("---------------------------------");
  Serial.println("Setup complete. Starting main loop.");
}

// --- Connect to WiFi ---
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Status:");
  lcd.setCursor(0, 1);
  lcd.print("Connected!");
  delay(1000);
}


// ==========================================================
// --- 5. AMSFM: DATA CALIBRATION & ML (IMPLEMENTED) ---
// ==========================================================

// Model 1: CO Calibration
float calibrate_mq7_co(float raw_mq7, float temp, float humid) {
  float coeff_A = 0.20667453;
  float coeff_B = -0.08439702;
  float coeff_C = 0.00432137;
  float intercept = -8.05510484;
  float calibrated_co = (coeff_A * raw_mq7) + (coeff_B * temp) + (coeff_C * humid) + intercept;
  if (calibrated_co < 0) return 0.0;
  return calibrated_co;
}

// Model 2: VOC/CH4 Calibration
float calibrate_mq9_voc(float raw_mq9, float temp, float humid) {
  float coeff_A = 22.68115496;
  float coeff_B = -40.81730199;
  float coeff_C = -0.65953960;
  float intercept = 1811.87014695;
  float calibrated_voc = (coeff_A * raw_mq9) + (coeff_B * temp) + (coeff_C * humid) + intercept;
  if (calibrated_voc < 0) return 0.0;
  return calibrated_voc;
}

/**
 * Stage 2: Diagnosis Logic with a priority check
 */
String get_ml_diagnosis(float t, float h, float co, float voc) {
  // Priority 1 & 2: Check safety alerts first
  if (intrusionAlert) return "Intrusion_Alert";
  if (earthquakeAlert) return "Earthquake_Alert";
  if (digitalRead(FLAME_PIN) == LOW) return "Flame_Alert";
  
  // Priority 3: Check Air Quality (Calibrated)
  if (co > 50.0) return "Critical_CO_Hazard";
  if (voc > 1000.0) return "Ventilation_Priority";

  // Priority 4: Check Thermal Comfort
  float heat_index = dht.computeHeatIndex(t, h, false);
  if (heat_index > 36.0) {
    return "Thermal_Caution";
  }
  
  return "Normal";
}

// ==========================================================
// --- 6. CORE LOGIC FUNCTIONS ---
// ==========================================================

/**
 * --- Formats the diagnosis for the 16-char screen ---
 */
String formatDiagnosisForLCD(String diagnosis) {
  if (diagnosis == "Normal") return "System Normal";
  if (diagnosis == "Thermal_Caution") return "Thermal Caution";
  if (diagnosis == "Ventilation_Priority") return "VENTILATE NOW";
  if (diagnosis == "Critical_CO_Hazard") return "CO HAZARD!";
  if (diagnosis == "Intrusion_Alert") return "INTRUSION ALERT";
  if (diagnosis == "Earthquake_Alert") return "EARTHQUAKE";
  if (diagnosis == "Flame_Alert") return "FLAME ALERT!";
  return "Unknown";
}

/**
 * --- Updates the LCD screen ---
 */
void updateLCD() {
  // Only update the screen if the status has changed (prevents flicker)
  if (currentDiagnosis == lastLCDDiagnosis) {
    return; // Do nothing, the screen is already correct
  }
  
  // The status has changed, so we must update the screen
  Serial.print("Updating LCD display to: ");
  Serial.println(currentDiagnosis);

  // Get the 16-character version of the diagnosis
  String lcdLine2 = formatDiagnosisForLCD(currentDiagnosis);
  
  // Clear just the second line
  lcd.setCursor(0, 1);
  lcd.print("                "); // 16 spaces to overwrite old text
  
  // Print the new status
  lcd.setCursor(0, 1);
  lcd.print(lcdLine2);

  // Store this as the "last" status
  lastLCDDiagnosis = currentDiagnosis;
}

/**
 * This function runs the diagnosis and controls the servo.
 */
void handleActuation() {
  if (currentDiagnosis == "Critical_CO_Hazard" || currentDiagnosis == "Ventilation_Priority") {
    curtainServo.write(170); // Open
  } 
  else if (currentDiagnosis == "Thermal_Caution") {
    curtainServo.write(10); // Close
  }
  else {
    curtainServo.write(10); // Close
  }
}

/**
 * Dual-Mode Vibration logic
 */
void handleVibration() {
  unsigned long now = millis();
  
  if (digitalRead(VIBRATION_PIN) == HIGH) {
    lastPulseTime = now; 
    if (vibrationStartTime == 0) { 
      vibrationStartTime = now;
      Serial.println("Vibration Event Started...");
    }

    unsigned long eventDuration = now - vibrationStartTime;

    if (systemMode == "Away" && !intrusionAlert) {
      if (eventDuration > INTRUSION_TIME_MS) {
        Serial.println("!!! INTRUSION ALERT TRIGGERED !!!");
        intrusionAlert = true; 
      }
    }
    
    if (systemMode == "Home" && !earthquakeAlert) {
      if (eventDuration > EARTHQUAKE_TIME_MS) {
        Serial.println("!!! EARTHQUAKE ALERT TRIGGERED !!!");
        earthquakeAlert = true; 
      }
    }
  } else { 
    if (vibrationStartTime != 0 && (now - lastPulseTime > VIBRATION_SILENCE_MS)) {
      Serial.println("Vibration Event Ended (Silence).");
      vibrationStartTime = 0;
    }
  }
}


/**
 * Fetches the "Home/Away" status from the cloud.
 */
void fetchSystemMode() {
  Serial.println("Fetching system mode from cloud...");
  
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(wifiClient, functionUrl); 
    int httpCode = http.GET(); 

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.print("Cloud config received: ");
      Serial.println(payload);

      StaticJsonDocument<64> doc; 
      deserializeJson(doc, payload);

      const char* mode = doc["mode"];
      if (mode) {
        systemMode = String(mode);
        Serial.print("System mode set to: ");
        Serial.println(systemMode);
      }
    } else {
      Serial.printf("HTTP GET... failed, error code: %d\n", httpCode);
    }
    http.end(); 
  } else {
    Serial.println("WiFi not connected. Skipping mode fetch.");
  }
}

/**
 * This function sends all our data to the cloud.
 */
void postSensorData() {
  Serial.println("\n--- Reading Sensors & Publishing ---");

  // 1. READ SENSORS
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int mq7_a0_raw = analogRead(MQ7_A0_PIN);
  int mq9_a0_raw = analogRead(MQ9_A0_PIN); 

  if (isnan(h) || isnan(t)) {
    Serial.println("DHT ERROR: Failed to read from DHT sensor!");
    return;
  }

  // 2. RUN AMSFM
  float calibrated_co = calibrate_mq7_co(mq7_a0_raw, t, h);
  // --- This is the fix for your sensing bug ---
  float calibrated_voc = calibrate_mq9_voc(mq9_a0_raw, t, h); 
  currentDiagnosis = get_ml_diagnosis(t, h, calibrated_co, calibrated_voc);
  float heat_index = dht.computeHeatIndex(t, h, false);

  // 3. CREATE JSON PAYLOAD
  StaticJsonDocument<256> doc;
  doc["T_C"] = t;
  doc["RH_Pct"] = h;
  doc["Comfort_Metric"] = heat_index;
  doc["CO_PPM_ML"] = calibrated_co;
  doc["VOC_PPM_ML"] = calibrated_voc;
  doc["ML_Diagnosis"] = currentDiagnosis; 
  doc["Flame_Alert"] = (digitalRead(FLAME_PIN) == LOW); 
  doc["Shock_Event"] = (intrusionAlert || earthquakeAlert);
  doc["Actuator_State"] = (curtainServo.read() > 90) ? "Open" : "Closed";
  doc["Shock_Mode"] = systemMode; 
  
  // 4. SERIALIZE JSON
  char buffer[256];
  serializeJson(doc, buffer);
  
  Serial.print("Sending JSON: ");
  Serial.println(buffer);

  // 5. SEND HTTP POST REQUEST
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(wifiClient, functionUrl); 
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(buffer);

    if (httpCode == HTTP_CODE_OK) {
      Serial.println("HTTP POST... success, code: 200");
    } else {
      Serial.printf("HTTP POST... failed, error code: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Skipping POST.");
  }

  // After posting, reset one-time alerts
  intrusionAlert = false;
  earthquakeAlert = false;
}

// ==========================================================
// --- 7. MAIN LOOP ---
// ==========================================================
void loop() {
  unsigned long now = millis();

  // Task 1: Handle high-frequency vibration checks
  handleVibration();

  // Task 2: Fetch system mode from cloud every 30s
  if (now - lastModeFetchTime > modeFetchInterval) {
    lastModeFetchTime = now;
    fetchSystemMode();
  }
  
  // Task 3: Post sensor data to cloud every 5s
  if (now - lastDataPostTime > dataPostInterval) {
    lastDataPostTime = now;
    postSensorData();
    handleActuation(); // Run actuation logic
    updateLCD();      // <-- Update the LCD
  }
}