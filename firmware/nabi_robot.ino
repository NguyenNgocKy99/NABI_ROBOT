/*
🤖 NABI ROBOT - ESP32 WebSocket Firmware v2.0
Real-time communication with Node.js Gateway
*/

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "config.h"

// ============ WEBSOCKET CLIENT ============
WebSocketsClient webSocket;
bool wsConnected = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000; // 30 seconds

// ============ VARIABLES ============
unsigned long lastSensorTime = 0;
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
  
  // WiFi Setup
  connectToWiFi();
  
  // WebSocket Setup
  setupWebSocket();
  
  Serial.println("\n✅ Nabi Robot initialized!");
  Serial.println("🔌 Connecting to WebSocket...\n");
}

// ============ MAIN LOOP ============
void loop() {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi disconnected, reconnecting...");
    connectToWiFi();
    delay(5000);
    return;
  }
  
  // WebSocket loop (IMPORTANT!)
  webSocket.loop();
  
  // Send sensor data every SENSOR_INTERVAL
  if (millis() - lastSensorTime >= SENSOR_INTERVAL) {
    sendSensorData();
    lastSensorTime = millis();
  }
  
  // Send heartbeat every 30 seconds
  if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = millis();
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

// ============ WEBSOCKET FUNCTIONS ============

void setupWebSocket() {
  // Extract host from SERVER_URL
  // SERVER_URL format: "https://nabirob-gateway.up.railway.app"
  String host = String(SERVER_URL);
  host.replace("https://", "");
  host.replace("http://", "");
  
  Serial.print("🔌 WebSocket connecting to: ");
  Serial.println(host);
  
  webSocket.begin(host.c_str(), 443, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.setAuthorization("user", "pass");
  
  // Enable SSL
  webSocket.setUseSSL(true);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      wsConnected = true;
      Serial.println("✅ WebSocket connected to Gateway!");
      digitalWrite(LED_PIN, HIGH);
      sendStatusUpdate("online");
      break;
      
    case WStype_TEXT:
      handleWebSocketMessage((char*)payload, length);
      break;
      
    case WStype_DISCONNECTED:
      wsConnected = false;
      Serial.println("❌ WebSocket disconnected");
      digitalWrite(LED_PIN, LOW);
      break;
      
    case WStype_ERROR:
      Serial.print("⚠️ WebSocket error: ");
      Serial.println((char*)payload);
      break;
  }
}

void handleWebSocketMessage(char* payload, size_t length) {
  // Parse JSON message
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("❌ JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  String event = doc["event"];
  
  if (DEBUG_MODE) {
    Serial.print("📥 Received event: ");
    Serial.println(event);
  }
  
  // Handle different events
  if (event == "welcome") {
    Serial.println("🎉 Welcome message from Gateway!");
  }
  
  else if (event == "ai_response") {
    handleAIResponse(doc);
  }
  
  else if (event == "command") {
    handleCommand(doc);
  }
  
  else if (event == "pong") {
    Serial.println("💓 Pong received");
  }
  
  else if (event == "ack") {
    Serial.println("✓ Message acknowledged");
  }
}

void handleAIResponse(JsonDocument& doc) {
  String reply = doc["reply"] | "No response";
  String command = doc["command"] | "idle";
  
  Serial.print("🤖 AI Reply: ");
  Serial.println(reply);
  
  Serial.print("⚡ Executing command: ");
  Serial.println(command);
  
  // Execute command
  executeCommand(command, 2000, 200);
}

void handleCommand(JsonDocument& doc) {
  String action = doc["action"] | "idle";
  int duration = doc["duration"] | 0;
  int speed = doc["speed"] | 0;
  
  Serial.print("📤 Command received: ");
  Serial.print(action);
  Serial.print(" (");
  Serial.print(duration);
  Serial.print("ms, speed: ");
  Serial.print(speed);
  Serial.println(")");
  
  executeCommand(action, duration, speed);
}

void sendHeartbeat() {
  if (!wsConnected) return;
  
  DynamicJsonDocument doc(100);
  doc["event"] = "ping";
  
  String json;
  serializeJson(doc, json);
  webSocket.sendTXT(json);
  
  if (DEBUG_MODE) {
    Serial.println("💓 Heartbeat sent");
  }
}

// ============ SENSOR FUNCTIONS ============

float readTemperature() {
  // Simulate sensor (replace with actual sensor reading)
  float temp = 25.0 + (random(-10, 10) / 10.0);
  return temp + TEMP_OFFSET;
}

int readHumidity() {
  // Simulate sensor
  int humidity = 50 + random(-20, 20);
  return constrain(humidity + HUMIDITY_OFFSET, 0, 100);
}

int readMotion() {
  return digitalRead(MOTION_PIN);
}

int readBattery() {
  // Read battery voltage from ADC
  int adcValue = analogRead(BATTERY_PIN);
  float voltage = adcValue * (3.3 / 4095.0);
  int batteryPercent = map(voltage * 100, 
                           BATTERY_MIN * 100, 
                           BATTERY_MAX * 100, 
                           0, 100);
  return constrain(batteryPercent, 0, 100);
}

void sendSensorData() {
  if (!wsConnected) {
    if (DEBUG_MODE) {
      Serial.println("⚠️ WebSocket not connected, skipping sensor send");
    }
    return;
  }
  
  Serial.println("📊 Sending sensor data via WebSocket...");
  
  float temperature = readTemperature();
  int humidity = readHumidity();
  int motion = readMotion();
  int battery = readBattery();
  
  DynamicJsonDocument doc(200);
  doc["event"] = "sensor_telemetry";
  doc["data"]["temperature"] = temperature;
  doc["data"]["humidity"] = humidity;
  doc["data"]["motion"] = motion;
  doc["data"]["battery"] = battery;
  
  String json;
  serializeJson(doc, json);
  
  if (DEBUG_MODE) {
    Serial.print("   Payload: ");
    Serial.println(json);
  }
  
  webSocket.sendTXT(json);
  
  Serial.print("✅ Sent. Count: ");
  sensorDataCount++;
  Serial.println(sensorDataCount);
}

void sendStatusUpdate(String status) {
  if (!wsConnected) return;
  
  DynamicJsonDocument doc(100);
  doc["event"] = "status_update";
  doc["status"] = status;
  
  String json;
  serializeJson(doc, json);
  webSocket.sendTXT(json);
  
  Serial.print("📍 Status sent: ");
  Serial.println(status);
}

// ============ COMMAND EXECUTION ============

void executeCommand(String action, int duration, int speed) {
  Serial.print("⚡ Executing: ");
  Serial.println(action);
  
  if (action == "idle" || action == "stop") {
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
    Serial.println("   💡 LED ON");
  }
  else if (action == "led_off") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("   ⚫ LED OFF");
  }
  else {
    Serial.print("   ⚠️ Unknown action: ");
    Serial.println(action);
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
  Serial.println("║   🤖 NABI ROBOT - ESP32 v2.0       ║");
  Serial.println("║   WebSocket Real-time Robot        ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  Serial.println("Configuration:");
  Serial.print("  WiFi SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("  Gateway: ");
  Serial.println(SERVER_URL);
  Serial.print("  Sensor Interval: ");
  Serial.print(SENSOR_INTERVAL);
  Serial.println("ms");
  Serial.print("  WebSocket: ws://");
  Serial.println(SERVER_URL);
  Serial.println();
}

void printStatus() {
  Serial.println("\n╔════ STATUS ════╗");
  Serial.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✅ ");
    Serial.println(WiFi.SSID());
  } else {
    Serial.println("❌ Disconnected");
  }
  
  Serial.print("WebSocket: ");
  Serial.println(wsConnected ? "✅ Connected" : "❌ Disconnected");
  
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  Serial.print("Signal: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  Serial.print("Sensor Data Sent: ");
  Serial.println(sensorDataCount);
  Serial.println("╚════════════════╝\n");
}