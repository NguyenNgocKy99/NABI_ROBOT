#ifndef WEBSOCKET_SIMPLE_H
#define WEBSOCKET_SIMPLE_H

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define GATEWAY_URL "nabirob-gateway.up.railway.app"
#define GATEWAY_PORT 80

WebSocketsClient webSocket;
bool wsConnected = false;

void connectWebSocket() {
    Serial.println("🔌 Connecting to WebSocket...");
    webSocket.begin(GATEWAY_URL, GATEWAY_PORT, "/ws");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_CONNECTED:
            wsConnected = true;
            Serial.println("✅ WebSocket connected!");
            break;
            
        case WStype_TEXT: {
            char* data = (char*)payload;
            DynamicJsonDocument doc(256);
            deserializeJson(doc, data);
            
            String event = doc["event"];
            if (event == "ai_response") {
                String command = doc["command"];
                Serial.print("🤖 Command: ");
                Serial.println(command);
                executeCommand(command, 2000, 200);
            }
            break;
        }
        
        case WStype_DISCONNECTED:
            wsConnected = false;
            Serial.println("❌ WebSocket disconnected");
            break;
    }
}

void sendSensorViaWebSocket(float temp, int humidity, int motion, int battery) {
    if (!wsConnected) return;
    
    DynamicJsonDocument doc(200);
    doc["event"] = "sensor_telemetry";
    doc["data"]["temperature"] = temp;
    doc["data"]["humidity"] = humidity;
    doc["data"]["motion"] = motion;
    doc["data"]["battery"] = battery;
    
    String json;
    serializeJson(doc, json);
    webSocket.sendTXT(json);
}

#endif