/*
 * File: main.cpp
 * Created: 2021-02-23 09:59:27
 * Modified: 2021-02-23 21:51:00
 * Author: mcxiaoke (github@mcxiaoke.com)
 * License: Apache License 2.0
 */

#include <Arduino.h>
#include <ArduinoBlue.h>
#include <PCF8574.h>
#include <SoftwareSerial.h>
#include <Wire.h>

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
// BLE RX, TX
SoftwareSerial ble(BLE_RX, BLE_TX);
ArduinoBlue remote(ble);
String bleStr("");
String bleCmd("");
int bleIndex;

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
#define CMD_IDLE 0

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
    if (newCmd != CMD_OK && newCmd != CMD_IDLE) {
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
  ble.begin(9600);
  Serial.println("setup()");
  setupPCF8574();
  setupSR();
  delay(500);
  // ble.println("AT+NAME");
}

void executeCmd(int cmd) {
  Serial.print("executeCmd:");
  Serial.println(cmd);
  switch (cmd) {
    case CMD_UP:
      Serial.println("UP");
      moveForward();
      break;
    case CMD_DOWN:
      Serial.println("DOWN");
      moveBackward();
      break;
    case CMD_LEFT:
      Serial.println("LEFT");
      moveLeft();
      break;
    case CMD_RIGHT:
      Serial.println("RIGHT");
      moveRight();
      break;
    case CMD_OK:
    case CMD_IDLE:
    default:
      Serial.println("IDLE");
      motorIdle();
      break;
  }
  // lastCmd = CMD_OK;
}

void handleBLE() {
  int btnId = remote.getButton();
  if (btnId != -1) {
    newCmd = btnId;
    Serial.print("BLE newCmd: ");
    Serial.print(newCmd);
    Serial.print(",lastCmd: ");
    Serial.println(lastCmd);
  }
}

void loop() {
  handleBLE();
  if (millis() - sensorLastCheck > 30) {
    sensorLastCheck = millis();
    checkDistance();
  }

  if (forceCmd || newCmd != lastCmd) {
    forceCmd = false;
    executeCmd(newCmd);
    lastCmd = newCmd;
  }
}
