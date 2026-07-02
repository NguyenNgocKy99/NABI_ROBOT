#ifndef FUNCTIONS_H
#define FUNCTIONS_H

// WiFi functions
void connectToWiFi();
void checkWiFiStatus();

// HTTP functions
void sendSensorData();
void getCommandFromServer();

// Motor functions
void moveForward(int speed);
void moveBackward(int speed);
void turnLeft(int speed);
void turnRight(int speed);
void stopMotor();

// Command execution
void executeCommand(String action, int duration, int speed);

#endif // FUNCTIONS_H