#ifndef CONFIG_H
#define CONFIG_H

// ============ WiFi Configuration ============
#define WIFI_SSID "C0p2Ec2-WLAN"
#define WIFI_PASSWORD "4Emah5LdS"

// ============ Server Configuration ============
#define SERVER_URL "https://nabirob-production.up.railway.app"

// ============ Pin Configuration (ESP32) ============
#define MOTOR_PIN1 12      // Motor IN1
#define MOTOR_PIN2 13      // Motor IN2
#define MOTOR_ENA 14       // Motor Enable (PWM)
#define LED_PIN 2          // LED (Eye)
#define MOTION_PIN 33      // PIR Motion Sensor

// ============ Timing Configuration (ms) ============
#define SENSOR_INTERVAL 10000    // Send sensor every 10s
#define COMMAND_INTERVAL 5000    // Poll command every 5s
#define WIFI_TIMEOUT 20000       // WiFi timeout 20s
#define SERIAL_BAUD 115200       // Serial baud rate

// ============ Sensor Calibration ============
#define TEMP_OFFSET 0
#define HUMIDITY_OFFSET 0
#define BATTERY_MIN 3.0
#define BATTERY_MAX 4.2
#define BATTERY_PIN 34

// ============ Debug Mode ============
#define DEBUG_MODE 1
#define DEBUG_API 1

#endif // CONFIG_H