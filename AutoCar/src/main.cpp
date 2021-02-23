/*
 * File: main.cpp
 * Created: 2021-02-23 09:59:27
 * Modified: 2021-02-23 21:51:00
 * Author: mcxiaoke (github@mcxiaoke.com)
 * License: Apache License 2.0
 */

#include <Arduino.h>
#include <IRremote.h>
#include <PCF8574.h>
#include <Wire.h>

#define MOTOR_DELAY 100

PCF8574 pcf(0x20);

void motorIdle() {
  // Serial.println("idle");
  pcf.write8(0x00);
}

void moveForward() {
  // 右前方 0,1
  // 左前方 3,2
  // 右后方 4,5
  // 左后方 7,6
  // pcf.write(0, HIGH);
  // pcf.write(1, LOW);
  // pcf.write(2, LOW);
  // pcf.write(3, HIGH);
  // pcf.write(4, HIGH);
  // pcf.write(5, LOW);
  // pcf.write(6, LOW);
  // pcf.write(7, HIGH);
  // Serial.println("forward");
  pcf.write8(0b10011001);
  delay(MOTOR_DELAY);
}

void moveBackward() {
  // pcf.write(0, LOW);
  // pcf.write(1, HIGH);
  // pcf.write(2, HIGH);
  // pcf.write(3, LOW);
  // pcf.write(4, LOW);
  // pcf.write(5, HIGH);
  // pcf.write(6, HIGH);
  // pcf.write(7, LOW);
  // Serial.println("backward");
  pcf.write8(0b01100110);
  delay(MOTOR_DELAY);
}

void moveLeft() {
  // pcf.write(0, HIGH);
  // pcf.write(1, LOW);
  // pcf.write(2, LOW);
  // pcf.write(3, LOW);
  // pcf.write(4, HIGH);
  // pcf.write(5, LOW);
  // pcf.write(6, LOW);
  // pcf.write(7, LOW);
  // Serial.println("left");
  pcf.write8(0b00010001);
  delay(MOTOR_DELAY);
}

void moveRight() {
  // pcf.write(0, LOW);
  // pcf.write(1, LOW);
  // pcf.write(2, LOW);
  // pcf.write(3, HIGH);
  // pcf.write(4, LOW);
  // pcf.write(5, LOW);
  // pcf.write(6, LOW);
  // pcf.write(7, HIGH);
  // Serial.println("right");
  pcf.write8(0b10001000);
  delay(MOTOR_DELAY);
}

// IR Commands (Black A)
// Protocol=NEC Address=0x0
// OK = 0x1c, UP = 0x18, DOWN = 0x52, LEFT = 0x8, RIGHT = 0x5a
// 1 = 0x45, 2 = 0x46, 3 = 0x47, 4 = 0x44, 5 = 0x40
// 6 = 0x43, 7 = 0x7, 8 = 0x15, 9 = 0x9, 0 = 0x19
// * = 0x16, # = 0xd
#define CMD_OK 0x1c
#define CMD_UP 0x18
#define CMD_DOWN 0x52
#define CMD_LEFT 0x8
#define CMD_RIGHT 0x5a

#define CMD_N1 0x45
#define CMD_N2 0x46
#define CMD_N3 0x47
#define CMD_N4 0x44
#define CMD_N5 0x40
#define CMD_N6 0x43
#define CMD_N7 0x7
#define CMD_N8 0x15
#define CMD_N9 0x9

#define CMD_N0 0x19
#define CMD_N0 0x19
#define CMD_STAR 0x16
#define CMD_BOX 0xd

uint16_t lastCmd = CMD_OK;
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

// ENA PWM控制速度
// 如果不需要速度直接跳线帽连接5V
// ENB ENC END 以此类推
// 电机A IN1 IN2, 电机B IN3 IN4
// 电机C IN5 IN6, 电机D IN7 IN8
// 驱动版单独供电 7V-12V, GND接Arduino GND

void setupIR() {
  pinMode(LED_BUILTIN, OUTPUT);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK,
                   USE_DEFAULT_FEEDBACK_LED_PIN);
}

void loopIR() {
  if (IrReceiver.decode()) {
    // Print a short summary of received data
    IrReceiver.printIRResultShort(&Serial);
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      // We have an unknown protocol here, print more info
      IrReceiver.printIRResultRawFormatted(&Serial, true);
    }
    Serial.println();

    /*
     * !!!Important!!! Enable receiving of the next value,
     * since receiving has stopped after the end of the current received data
     * packet.
     */
    IrReceiver.resume();  // Enable receiving of the next value

    /*
     * Finally, check the received data and perform actions according to the
     * received command
     */
    lastCmd = IrReceiver.decodedIRData.command;
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

void printPCF8574() {
  Serial.print("Read ");
  Serial.println(pcf.read8(), BIN);
}

void testPCF8574() {
  // 右前方 0,1
  // 左前方 3,2
  // 右后方 4,5
  // 左后方 7,6
  pcf.write8(0x00);
  pcf.write(0, HIGH);
  pcf.write(1, LOW);
  printPCF8574();
  pcf.write8(0x00);
  pcf.write(3, HIGH);
  pcf.write(2, LOW);
  printPCF8574();
  pcf.write8(0x00);
  pcf.write(4, HIGH);
  pcf.write(5, LOW);
  printPCF8574();
  pcf.write8(0x00);
  pcf.write(7, HIGH);
  pcf.write(6, LOW);
  printPCF8574();
  // // pcf.write8(0x55);
  // // printPCF8574();

  // pcf.write8(0xaa);
  // printPCF8574();

  // pcf.write8(0x00);
  // printPCF8574();
}

void wirePrint() {
  if (Wire.requestFrom((uint8_t)0x20, (uint8_t)1) == 1) {
    uint8_t d = Wire.read();
    Serial.print("Read ");
    Serial.println(d, BIN);
  }
}

void testWire() {
  Wire.beginTransmission(0x20);
  Wire.write(0xAA);
  Wire.endTransmission();
  wirePrint();
  delay(2000);
  Wire.beginTransmission(0x20);
  Wire.write(0x55);
  Wire.endTransmission();
  wirePrint();
  delay(2000);
  Wire.beginTransmission(0x20);
  Wire.write(0x00);
  Wire.endTransmission();
  wirePrint();
  delay(2000);
}

void setupWire() {
  Wire.begin();
  Wire.beginTransmission(0x20);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(1000);
  wirePrint();
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup()");
  setupPCF8574();
  setupIR();
}

void checkDirection() {
  switch (lastCmd) {
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
    case CMD_N0:
    case CMD_STAR:
    case CMD_BOX:
      motorIdle();
      break;
    default:
      break;
  }
  lastCmd = CMD_OK;
}

void loop() {
  loopIR();
  checkDirection();
}
