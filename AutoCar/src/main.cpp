/*
 * File: main.cpp
 * Created: 2021-02-23 09:59:27
 * Modified: 2021-02-23 21:51:00
 * Author: mcxiaoke (github@mcxiaoke.com)
 * License: Apache License 2.0
 */

#include <Arduino.h>
#include <ArduinoBlue.h>
#include <IRremote.h>
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

int lastCmd = CMD_OK;
unsigned long lastCheck = 0;
//
// ###############
//
// IR Commands (White B)
// CH- = 0x45, CH = 0x46, CH+ = 0x47
// PREV = 0x44, NEXT = 0x40, PLAY = 0x43
// - = 0x7, + = 0x15, 100 = 0x19, 200 = 0xd
// 1 = 0xc, 2 = 0x18, 3 = 0x5e, 4 = 0x8, 5 = 0x1c
// 6 = 0x5a, 7 = 0x42, 8 = 0x52, 9 = 0x4a, 0 = 0x16

int IR_RECEIVE_PIN = A0;

void setupIR() {
  pinMode(LED_BUILTIN, OUTPUT);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK,
                   USE_DEFAULT_FEEDBACK_LED_PIN);
}

void loopIR() {
  if (IrReceiver.decode()) {
    // Print a short summary of received data
    IrReceiver.printIRResultShort(&Serial);
    // if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    //   // We have an unknown protocol here, print more info
    //   IrReceiver.printIRResultRawFormatted(&Serial, true);
    // }
    Serial.println();
    IrReceiver.resume();  // Enable receiving of the next value
    if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      lastCmd = IrReceiver.decodedIRData.command;
    }
    // lastCmd = lastCmdRetain;
  }
  delay(5);
}

void setupPCF8574() {
  // SDA=A4, SCL=A5
  pcf.begin(0x00);
  int x = pcf.read8();
  Serial.print("Init ");
  Serial.println(x, BIN);
}

void readBlEData() {
  char c;
  while (ble.available() > 0) {
    c = ble.read();
    bleStr += c;
    if (bleIndex < 2) {
      bleCmd += c;
    }
    bleIndex++;
    delay(1);
  }
  bleIndex = 0;
  if (bleStr.length() > 0) {
    Serial.println(bleStr);
    bleCmd = "";
    bleStr = "";
  }
}

void setup() {
  Serial.begin(9600);
  ble.begin(9600);
  Serial.println("setup()");
  setupPCF8574();
  delay(500);
  ble.println("AT+NAME");
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
  int newCmd = remote.getButton();
  if (newCmd == -1) {
    return;
  }

  Serial.print("BLE newCmd: ");
  Serial.print(newCmd);
  Serial.print(",lastCmd: ");
  Serial.println(lastCmd);

  if (newCmd != lastCmd) {
    if (millis() - lastCheck > 50) {
      lastCheck = millis();
      executeCmd(newCmd);
    }
  }

  lastCmd = newCmd;
}

void loop() { handleBLE(); }
