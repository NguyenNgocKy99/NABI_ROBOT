# 🌐 API Documentation - Nabi Robot Backend v2.0 (WebSocket)

## Architecture
ESP32 / Web Dashboard
↓ (WebSocket)
Node.js Gateway / Python Backend
↓
Database

## Base URL

**WebSocket:** `wss://nabirob-production-916d.up.railway.app`

**HTTP Fallback:** `https://nabirob-production-916d.up.railway.app`

---

## 🔌 WebSocket Events

### Client-to-Server Events

#### 1. `identify`
**Identify client type**

```javascript
socket.emit('identify', { 
  type: 'dashboard' // or 'robot'
});
```

---

#### 2. `send_command`
**Send command to robot**

```javascript
socket.emit('send_command', {
  action: 'move_forward',    // move_forward, move_backward, turn_left, turn_right, stop, led_on, led_off
  duration: 2000,             // milliseconds
  speed: 200                  // 0-255
});
```

---

#### 3. `sensor_data`
**Robot sends sensor data (ESP32 only)**

```javascript
socket.emit('sensor_data', {
  temperature: 28.5,
  humidity: 60,
  motion: 1,
  battery: 85
});
```

---

#### 4. `request_status`
**Request current status**

```javascript
socket.emit('request_status');
```

---

#### 5. `heartbeat`
**Send heartbeat/ping**

```javascript
socket.emit('heartbeat');
```

---

### Server-to-Client Events

#### 1. `welcome`
**Received when client connects**

```javascript
socket.on('welcome', function(data) {
  console.log(data.message);  // "Connected to Nabi Backend"
  console.log(data.client_id); // unique client ID
});
```

---

#### 2. `robot_command`
**Receive command (Robot only)**

```javascript
socket.on('robot_command', function(data) {
  console.log(data.action);   // 'move_forward'
  console.log(data.duration); // 2000
  console.log(data.speed);    // 200
});
```

---

#### 3. `sensor_update`
**Receive sensor data (Dashboard)**

```javascript
socket.on('sensor_update', function(data) {
  console.log(data.data.temperature);
  console.log(data.data.humidity);
  console.log(data.data.motion);
  console.log(data.data.battery);
});
```

---

#### 4. `status_response`
**Status response**

```javascript
socket.on('status_response', function(data) {
  console.log(data.robot_status);      // 'online'
  console.log(data.connected_clients); // number
  console.log(data.sensor_count);      // number
});
```

---

#### 5. `ack`
**Acknowledgement**

```javascript
socket.on('ack', function(data) {
  console.log(data.message); // 'Sensor data received' or 'Command sent'
});
```

---

#### 6. `client_count`
**Updated client count (Broadcast)**

```javascript
socket.on('client_count', function(data) {
  console.log(data.count); // number of connected clients
});
```

---

#### 7. `heartbeat_ack`
**Heartbeat acknowledgement**

```javascript
socket.on('heartbeat_ack', function(data) {
  console.log(data.timestamp);
});
```

---

## 📡 HTTP Endpoints (Fallback)

### GET /health
Response: { "status": "ok", "websocket_connected": 1 }

### POST /api/sensor
```json
Request: {
  "temperature": 28.5,
  "humidity": 60,
  "motion": 1,
  "battery": 85
}

Response: { "status": "success" }
```

### GET /api/sensor/latest
Response: { "timestamp": "...", "data": {...} }

### GET /api/status
Response: {
"robot_status": "online",
"websocket_clients": 2,
"sensor_count": 45
}

---

## 🔧 Example: Web Dashboard

```javascript
// Connect
const socket = io('https://nabirob-production-916d.up.railway.app');

// Identify
socket.on('connect', function() {
  socket.emit('identify', { type: 'dashboard' });
});

// Send command
function moveForward() {
  socket.emit('send_command', {
    action: 'move_forward',
    duration: 2000,
    speed: 200
  });
}

// Receive sensor
socket.on('sensor_update', function(data) {
  console.log('Temperature:', data.data.temperature);
});
```

---

## 🤖 Example: ESP32 Robot

```cpp
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

WebSocketsClient webSocket;

void setup() {
  webSocket.begin("nabirob-production-916d.up.railway.app", 443, "/socket.io/");
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  
  // Send sensor data
  DynamicJsonDocument doc(200);
  doc["event"] = "sensor_data";
  doc["temperature"] = 28.5;
  doc["humidity"] = 60;
  doc["motion"] = 1;
  doc["battery"] = 85;
  
  String json;
  serializeJson(doc, json);
  webSocket.sendTXT(json);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, payload);
    
    String action = doc["action"];
    // Execute command...
  }
}
```

---

## ✅ Advantages of WebSocket

✅ **Real-time Communication** - Instant message delivery
✅ **Bidirectional** - Both client and server can send anytime
✅ **Low Latency** - No polling overhead
✅ **Efficient** - Single connection, multiplexed messages
✅ **Scalable** - Handles many concurrent connections

---

## 🔄 Connection Lifecycle

Client connects → 'connect' event
Server sends 'welcome' message
Client identifies with 'identify' event
Bidirectional communication
Client/Server disconnects → 'disconnect' event
Auto-reconnect every 1-5 seconds


---

## 📊 Debug Events

### GET /api/debug
Returns entire database and connected clients

### POST /api/debug/reset
Resets all sensor history and commands

---

## 🎯 Best Practices

1. **Always identify** when connecting
2. **Handle reconnection** - WebSocket auto-reconnects
3. **Use acknowledgements** - Check for 'ack' events
4. **Monitor client count** - Track connected clients
5. **Implement heartbeat** - Send periodic pings

---

**WebSocket provides real-time, efficient communication for IoT and web applications!**