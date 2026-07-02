#ifndef CONFIG_H
#define CONFIG_H

// ============ WiFi Configuration ============
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ============ Server Configuration ============
#define SERVER_URL "https://YOUR_RAILWAY_URL"

// ============ Pin Configuration ============
#define MOTOR_PIN1 12
#define MOTOR_PIN2 13
#define MOTOR_ENA 14
#define LED_PIN 2
#define SENSOR_PIN 35

// ============ Timing Configuration ============
#define SENSOR_INTERVAL 10000   // 10 seconds
#define COMMAND_INTERVAL 5000   // 5 seconds
#define WIFI_TIMEOUT 20000      // 20 seconds

#endif // CONFIG_H