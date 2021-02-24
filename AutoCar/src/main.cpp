/*
 * File: main.cpp
 * Created: 2021-02-23 09:59:27
 * Modified: 2021-02-23 21:51:00
 * Author: mcxiaoke (github@mcxiaoke.com)
 * License: Apache License 2.0
 */

#include <Arduino.h>
#include <PCF8574.h>
#include <SoftwareSerial.h>
#include <Wire.h>

//#define USE_ARDUINO_BLUE

#ifdef USE_ARDUINO_BLUE
#include <ArduinoBlue.h>
#else
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <Dabble.h>
#endif

// ENA PWM控制速度
// 如果不需要速度直接跳线帽连接5V
// ENB ENC END 以此类推
// 电机A IN1 IN2, 电机B IN3 IN4
// 电机C IN5 IN6, 电机D IN7 IN8
// 驱动版单独供电 7V-12V, GND接Arduino GND

PCF8574 pcf(0x20);

// HC-SR04 const
const byte trigPin = 2;
const byte echoPin = 3;

// Bluetooth const
// connect cc2541-rx
const byte BLE_TX = 9;
// connect cc2541-tx
const byte BLE_RX = 8;

#ifdef USE_ARDUINO_BLUE
// BLE RX, TX
SoftwareSerial ble(BLE_RX, BLE_TX);
ArduinoBlue remote(ble);
String bleStr("");
String bleCmd("");
int bleIndex;

#endif

void motorIdle() { pcf.write8(0x00); }

void moveForward() { pcf.write8(0b10011001); }

void moveBackward() { pcf.write8(0b01100110); }

void moveLeft() { pcf.write8(0b00010001); }

void moveRight() { pcf.write8(0b10001000); }

#define CMD_OK 5
#define CMD_UP 2
#define CMD_DOWN 8
#define CMD_LEFT 4
#define CMD_RIGHT 6
#define SET_MODE 9

String getCmdName(int cmd) {
  switch (cmd) {
    case SET_MODE:
      return "SET_MODE";
      break;
    case CMD_UP:
      return "CMD_UP";
      break;
    case CMD_DOWN:
      return "CMD_DOWN";
      break;
    case CMD_LEFT:
      return "CMD_LEFT";
      break;
    case CMD_RIGHT:
      return "CMD_RIGHT";
      break;
    case CMD_OK:
      return "CMD_OK";
      break;
    default:
      return "NONE";
      break;
  }
}

bool autoMode = false;
bool forceCmd = false;
int newCmd = CMD_OK;
int lastCmd = CMD_OK;
unsigned long cmdLastCheck = 0;
unsigned long sensorLastCheck = 0;

void setupPCF8574() {
  // SDA=A4, SCL=A5
  pcf.begin(0x00);
  int x = pcf.read8();
  Serial.print("Init ");
  Serial.println(x, BIN);
}

void setupSR() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

unsigned long checkOnce() {
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  return pulseIn(echoPin, HIGH);
}

void checkDistance() {
  float duration = checkOnce();

  // convert the time into a distance
  double cm = (duration / 2) / 29.1;
  double inches = (duration / 2) / 74;

  if (cm < 20 && cm > 0) {
    Serial.print("Distance: ");
    Serial.print(inches);
    Serial.print(" in, ");
    Serial.print(cm);
    Serial.print(" cm");
    Serial.println();
    // near, stop
    if (newCmd == CMD_UP) {
      Serial.println("stop and turn right");
      moveBackward();
      delay(200);
      moveRight();
      delay(400);
      newCmd = CMD_UP;
      forceCmd = true;
    }
  }
}

void setup() {
  Serial.begin(9600);
#ifdef USE_ARDUINO_BLUE
  autoMode = true;
  ble.begin(9600);
#else
  Dabble.begin(9600, BLE_RX, BLE_TX);
#endif
  Serial.println("setup()");
  setupPCF8574();
  setupSR();
  delay(500);
  // ble.println("AT+NAME");
}

void showCmd(String prefix) {
  Serial.print(millis());
  Serial.print(",");
  Serial.print(prefix);
  Serial.print(", new:");
  Serial.print(getCmdName(newCmd));
  Serial.print(", last:");
  Serial.print(getCmdName(lastCmd));
  Serial.print(", auto:");
  Serial.println(autoMode ? 1 : 0);
}

void executeCmd() {
  // Serial.print(millis());
  // Serial.print(",executeCmd:");
  // Serial.println(getCmdName(newCmd));
  switch (newCmd) {
    case CMD_UP:
      moveForward();
      break;
    case CMD_DOWN:
      moveBackward();
      break;
    case CMD_LEFT:
      moveLeft();
      break;
    case CMD_RIGHT:
      moveRight();
      break;
    case CMD_OK:
    default:
      motorIdle();
      break;
  }
  lastCmd = newCmd;
  delay(20);
}

#ifdef USE_ARDUINO_BLUE
void handleArduinoBlue() {
  String text = remote.getText();
  if (text != "" && text.length() > 0) {
    Serial.println(text);
  }
  int bleCmd = remote.getButton();
  if (bleCmd != -1) {
    if (bleCmd == SET_MODE) {
      autoMode = !autoMode;
    } else {
      newCmd = bleCmd;
    }
    showCmd("blue");
  }
}
#else
void handleDabble() {
  Dabble.processInput();
  if (GamePad.isSelectPressed()) {
    autoMode = !autoMode;
    Serial.print(millis());
    Serial.print(", autoMode:");
    Serial.println(autoMode ? 1 : 0);
    return;
  }

  int bleCmd = -1;
  if (GamePad.isUpPressed() || GamePad.isTrianglePressed()) {
    bleCmd = CMD_UP;
  } else if (GamePad.isDownPressed() || GamePad.isCrossPressed()) {
    bleCmd = CMD_DOWN;
  } else if (GamePad.isLeftPressed() || GamePad.isSquarePressed()) {
    bleCmd = CMD_LEFT;
  } else if (GamePad.isRightPressed() || GamePad.isCirclePressed()) {
    bleCmd = CMD_RIGHT;
  } else if (GamePad.isStartPressed()) {
    bleCmd = CMD_OK;
  }

  if (bleCmd != -1) {
    newCmd = bleCmd;
    showCmd("dabble");
  }
}
#endif

void loop() {
#ifdef USE_ARDUINO_BLUE
  handleArduinoBlue();
#else
  handleDabble();
#endif
  if (autoMode) {
    if (millis() - sensorLastCheck > 50) {
      sensorLastCheck = millis();
      checkDistance();
    }
  }
  if (!autoMode) {
    executeCmd();
    newCmd = CMD_OK;
  } else if (forceCmd || newCmd != lastCmd) {
    forceCmd = false;
    showCmd("loop");
    executeCmd();
  }
  if (!autoMode) {
  }
}
