/*
🤖 NABI ROBOT - ESP32 Firmware v1.0
Smart Voice Interactive Robot
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// ============ VARIABLES ============
unsigned long lastSensorTime = 0;
unsigned long lastCommandTime = 0;
int sensorDataCount = 0;

// ============ SETUP ============
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  // GPIO Setup
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTION_PIN, INPUT);
  
  printBanner();
  connectToWiFi();
  
  Serial.println("\n✅ Nabi Robot initialized!");
  Serial.println("📡 Ready to communicate\n");
}

// ============ MAIN LOOP ============
void loop() {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi disconnected, reconnecting...");
    connectToWiFi();
    return;
  }
  
  // Send sensor data
  if (millis() - lastSensorTime >= SENSOR_INTERVAL) {
    sendSensorData();
    lastSensorTime = millis();
  }
  
  // Get command
  if (millis() - lastCommandTime >= COMMAND_INTERVAL) {
    getCommandFromServer();
    lastCommandTime = millis();
  }
  
  delay(100);
}

// ============ WiFi FUNCTIONS ============

void connectToWiFi() {
  Serial.print("🔌 Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  unsigned long startTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && 
         millis() - startTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("📍 IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("📶 Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("\n❌ WiFi connection failed!");
    digitalWrite(LED_PIN, LOW);
  }
}

// ============ SENSOR FUNCTIONS ============

float readTemperature() {
  float temp = 25.0 + (random(-10, 10) / 10.0);
  return temp + TEMP_OFFSET;
}

int readHumidity() {
  int humidity = 50 + random(-20, 20);
  return constrain(humidity + HUMIDITY_OFFSET, 0, 100);
}

int readMotion() {
  return digitalRead(MOTION_PIN);
}

int readBattery() {
  int adcValue = analogRead(BATTERY_PIN);
  float voltage = adcValue * (3.3 / 4095.0);
  int batteryPercent = map(voltage * 100, 
                           BATTERY_MIN * 100, 
                           BATTERY_MAX * 100, 
                           0, 100);
  return constrain(batteryPercent, 0, 100);
}

void sendSensorData() {
  Serial.println("📊 Sending sensor data...");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return;
  }
  
  HTTPClient http;
  String url = String(SERVER_URL) + "/api/sensor";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  float temperature = readTemperature();
  int humidity = readHumidity();
  int motion = readMotion();
  int battery = readBattery();
  
  DynamicJsonDocument doc(200);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["motion"] = motion;
  doc["battery"] = battery;
  
  String json;
  serializeJson(doc, json);
  
  if (DEBUG_API) {
    Serial.print("   Payload: ");
    Serial.println(json);
  }
  
  int httpCode = http.POST(json);
  
  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.print("✅ Success. Count: ");
      sensorDataCount++;
      Serial.println(sensorDataCount);
    } else {
      Serial.print("⚠️  HTTP ");
      Serial.println(httpCode);
    }
  } else {
    Serial.print("❌ Error: ");
    Serial.println(http.errorToString(httpCode));
  }
  
  http.end();
}

// ============ COMMAND FUNCTIONS ============

void getCommandFromServer() {
  if (DEBUG_API) {
    Serial.println("🎛️  Polling command...");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return;
  }
  
  HTTPClient http;
  String url = String(SERVER_URL) + "/api/command";
  
  http.begin(url);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    
    DynamicJsonDocument doc(200);
    deserializeJson(doc, response);
    
    String action = doc["action"].as<String>();
    int duration = doc["duration"];
    int speed = doc["speed"];
    
    if (DEBUG_API) {
      Serial.print("   Action: ");
      Serial.print(action);
      Serial.print(" (");
      Serial.print(duration);
      Serial.print("ms, speed:");
      Serial.print(speed);
      Serial.println(")");
    }
    
    executeCommand(action, duration, speed);
  }
  
  http.end();
}

void executeCommand(String action, int duration, int speed) {
  Serial.print("⚡ Execute: ");
  Serial.println(action);
  
  if (action == "idle") {
    stopMotor();
  }
  else if (action == "move_forward") {
    moveForward(speed);
    delay(duration);
    stopMotor();
  }
  else if (action == "move_backward") {
    moveBackward(speed);
    delay(duration);
    stopMotor();
  }
  else if (action == "turn_left") {
    turnLeft(speed);
    delay(duration);
    stopMotor();
  }
  else if (action == "turn_right") {
    turnRight(speed);
    delay(duration);
    stopMotor();
  }
  else if (action == "led_on") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("   LED ON");
  }
  else if (action == "led_off") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("   LED OFF");
  }
  else if (action == "stop") {
    stopMotor();
  }
}

// ============ MOTOR CONTROL ============

void moveForward(int speed) {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  analogWrite(MOTOR_ENA, speed);
  Serial.println("🚀 Moving forward");
}

void moveBackward(int speed) {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, HIGH);
  analogWrite(MOTOR_ENA, speed);
  Serial.println("🔙 Moving backward");
}

void turnLeft(int speed) {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  analogWrite(MOTOR_ENA, speed / 2);
  Serial.println("⬅️  Turning left");
}

void turnRight(int speed) {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  analogWrite(MOTOR_ENA, speed);
  Serial.println("➡️  Turning right");
}

void stopMotor() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  analogWrite(MOTOR_ENA, 0);
  Serial.println("⏹️  Motor stopped");
}

// ============ UTILITY FUNCTIONS ============

void printBanner() {
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║   🤖 NABI ROBOT - ESP32 v1.0       ║");
  Serial.println("║   Smart Voice Interactive Robot    ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  Serial.println("Configuration:");
  Serial.print("  WiFi SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("  Server: ");
  Serial.println(SERVER_URL);
  Serial.print("  Sensor Interval: ");
  Serial.print(SENSOR_INTERVAL);
  Serial.println("ms");
  Serial.print("  Command Interval: ");
  Serial.print(COMMAND_INTERVAL);
  Serial.println("ms");
  Serial.println();
}