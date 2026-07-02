# 🌐 API Documentation - Nabi Robot Backend

## Base URL
web-production-4f0df.up.railway.app

---

## 📋 API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Server health check |
| `/api/sensor` | POST | Send sensor data |
| `/api/sensor/latest` | GET | Get latest sensor data |
| `/api/sensor/history` | GET | Get sensor history |
| `/api/command` | GET | Get command for robot |
| `/api/command/send` | POST | Send command to robot |
| `/api/status` | GET | Get robot status |

---

## 📡 API Detailed

### 1. Health Check
GET /health
**Response:**
```json
{
  "status": "ok",
  "message": "Server is running",
  "timestamp": "2024-01-15T10:30:00.123456"
}
```

---

### 2. Send Sensor Data
**ESP32 gửi dữ liệu cảm biến**
POST /api/sensor
Content-Type: application/json

**Request:**
```json
{
  "temperature": 28.5,
  "humidity": 60,
  "motion": 1,
  "battery": 85
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Sensor data received",
  "data_id": 1,
  "server_time": "2024-01-15T10:30:00.123456"
}
```

---

### 3. Get Latest Sensor Data
GET /api/sensor/latest

**Response:**
```json
{
  "timestamp": "2024-01-15T10:30:00.123456",
  "data": {
    "temperature": 28.5,
    "humidity": 60,
    "motion": 1,
    "battery": 85
  }
}
```

---

### 4. Get Sensor History
GET /api/sensor/history?limit=10

**Response:**
```json
{
  "count": 10,
  "data": [
    {
      "timestamp": "2024-01-15T10:30:00.123456",
      "data": {
        "temperature": 28.5,
        "humidity": 60,
        "motion": 1,
        "battery": 85
      }
    }
  ]
}
```

---

### 5. Get Command (ESP32 lấy lệnh)
GET /api/command

**Response:**
```json
{
  "action": "move_forward",
  "duration": 2000,
  "speed": 255,
  "timestamp": "2024-01-15T10:30:00.123456"
}
```

**Action Types:**
- `idle` - Không làm gì
- `move_forward` - Đi tới
- `move_backward` - Đi lùi
- `turn_left` - Quay trái
- `turn_right` - Quay phải
- `led_on` - Bật đèn
- `led_off` - Tắt đèn

---

### 6. Send Command (Web gửi lệnh)
POST /api/command/send
Content-Type: application/json

**Request:**
```json
{
  "action": "move_forward",
  "duration": 2000,
  "speed": 200
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Command queued",
  "command": {
    "action": "move_forward",
    "duration": 2000,
    "speed": 200,
    "timestamp": "2024-01-15T10:30:00.123456"
  }
}
```

---

### 7. Get Status
GET /api/status

**Response:**
```json
{
  "robot_status": "online",
  "last_update": "2024-01-15T10:30:00.123456",
  "sensor_count": 45,
  "pending_commands": 1,
  "server_time": "2024-01-15T10:30:00.123456"
}
```

---

## 🔧 Example: Test with PowerShell

```bash
# Health check
curl https://web-production-4f0df.up.railway.app/health

# Send sensor data
curl -X POST https://web-production-4f0df.up.railway.app/api/sensor `
  -H "Content-Type: application/json" `
  -d '{"temperature":28.5,"humidity":60,"motion":1,"battery":85}'

# Get command
curl https://web-production-4f0df.up.railway.app/api/command

# Send command
curl -X POST https://web-production-4f0df.up.railway.app/api/command/send `
  -H "Content-Type: application/json" `
  -d '{"action":"move_forward","duration":2000,"speed":200}'
```

---

## 📱 ESP32 Code Example

```cpp
// Send sensor data
void sendSensorData() {
  HTTPClient http;
  http.begin("https://web-production-4f0df.up.railway.app/api/sensor");
  http.addHeader("Content-Type", "application/json");
  
  String json = "{\"temperature\":28.5,\"humidity\":60,\"motion\":1,\"battery\":85}";
  http.POST(json);
  http.end();
}

// Get command
void getCommand() {
  HTTPClient http;
  http.begin("https://web-production-4f0df.up.railway.app/api/command");
  
  if (http.GET() == 200) {
    String response = http.getString();
    // Parse JSON and execute
  }
  http.end();
}
```