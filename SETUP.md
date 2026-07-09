# 🔧 Setup Guide - Nabi Robot

## Backend Setup (Railway)

### 1. Create GitHub Repository
```bash
git clone https://github.com/NguyenNgocKy99/NABI_ROBOT.git
cd nabi_robot
```

### 2. Deploy to Railway
1. Go to https://railway.app/
2. Connect GitHub repository
3. Select `nabi_robot` repo
4. Railway auto-deploys!
5. Copy public URL

### 3. Test Backend
```bash
curl https://web-production-4f0df.up.railway.app/health
```

## ESP32 Setup

### 1. Install Arduino IDE
Download: https://www.arduino.cc/en/software

### 2. Install ESP32 Board
1. File → Preferences
2. Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Tools → Board Manager → Search "ESP32" → Install

### 3. Install Libraries
- ArduinoJson (by Benoit Blanchon)
- WiFi (built-in)
- HTTPClient (built-in)

### 4. Configuration
Edit `firmware/config.h`:
```cpp
#define WIFI_SSID "C0p2Ec2-WLAN"
#define WIFI_PASSWORD "4Emah5LdS"
#define SERVER_URL "https://web-production-4f0df.up.railway.app"
```

### 5. Upload
1. Open `firmware/nabi_robot.ino`
2. Select Board: ESP32 Dev Module
3. Select COM Port
4. Click Upload

## Testing

### Test Sensor Data
```bash
curl -X POST https://web-production-4f0df.up.railway.app/api/sensor \
  -H "Content-Type: application/json" \
  -d '{"temperature":28.5,"humidity":60,"motion":1,"battery":85}'
```

### Test Command
```bash
curl https://web-production-4f0df.up.railway.app/api/command
```

### View Logs
Check ESP32 Serial Monitor (115200 baud)
```
✓ WiFi connected!
IP: 192.168.x.x
📊 Sending sensor data...
✓ Response: {"status":"success",...}
```