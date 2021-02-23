#include <Arduino.h>
#include <SoftwareSerial.h>

// ENA PWM控制速度
// 如果不需要速度直接跳线帽连接5V
// ENB ENC END 以此类推
// 电机A IN1 IN2
// 电机B IN3 IN4
// 电机C IN5 IN6
// 电机D IN7 IN8
// 驱动版单独供电 7V-12V
// 驱动版GND接Arduino GND

// ble definations
// button 2, go forward
#define BLE_UP "2"
// button 4, go left
#define BLE_LEFT "4"
// button 6, go right
#define BLE_RIGHT "6"
// button 8, go backward
#define BLE_DOWN "8"

// button 1, turn small left
#define BLE_SLEFT "1"
// button 3, turn small right
#define BLE_SRIGHT "3"
// button 7, next turn left
#define BLE_NEXT_LEFT "7"
// button 9, nextturn right
#define BLE_NEXT_RIGHT "9"

// button +, speed up 20
#define BLE_SPEED_INC "_"
// button -, speed down 20
#define BLE_SPEED_DEC "_"
// button eq, speed toggle
#define BLE_SPEED_TOGGLE "_"

// button A, auto mode
#define BLE_AUTO_MODE "10"
// button M, manual mode
#define BLE_MANUAL_MODE "11"

// button 5, stop
#define BLE_STOP "5"
#define BLE_STOP2 "55"
// button pause, toggle distance
#define BLE_DISTANCE "_"
// button 0, reset and stop
#define BLE_RESET "0"
#define BLE_RESET2 "00"

typedef enum {
  GO_UP,
  GO_LEFT,
  GO_RIGHT,
  GO_DOWN,
  GO_SLEFT,
  GO_SRIGHT,
  GO_LBACK,
  GO_RBACK,
  STOP
} Command;

const byte FRONT_RIGHT = 0;
const byte FRONT_LEFT = 1;
const byte BACK_RIGHT = 2;
const byte BACK_LEFT = 3;
// wheel a, right
const byte FRONT_RIGHT_1 = 2;  // 1
const byte FRONT_RIGHT_2 = 3;  // 2
// wheel b, left
const byte FRONT_LEFT_1 = 4;  // 3
const byte FRONT_LEFT_2 = 7;  // 4
// wheel c, right
const byte BACK_RIGHT_1 = A0;  // 5
const byte BACK_RIGHT_2 = A1;  // 6
// wheel d, left
const byte BACK_LEFT_1 = A2;  // 7
const byte BACK_LEFT_2 = A3;  // 8

// pmw control
const byte ENA = 5;
const byte ENB = 6;
const byte ENC = 9;
const byte END = 10;

// IRemote PIN
const byte RECV_PIN = 12;

// Ultrasonic Sensor
const byte TRIG_PIN = A4;  // trigger pin
const byte ECHO_PIN = A5;  // echo pin

// Bluetooth const
const byte BLE_TX = 8;   // connect cc2541-rx
const byte BLE_RX = 11;  // connect cc2541-tx

// Car const
const int TURN_DURATION = 1500;
const int MOVE_DURATION = 200;
const int TURN_SPEED = 180;
const int SPEED_HIGH = 240;
const int SPEED_LOW = 200;
const int INIT_SPEED = SPEED_HIGH;
const Command INIT_COMMAND = STOP;

float maxDistance = 20.0;
bool nextLeft = true;
bool autoMode = false;
int currentSpeed = INIT_SPEED;
Command nextCommand = INIT_COMMAND;

String bleStr("");
String bleCmd("");
int bleIndex;

SoftwareSerial ble(BLE_RX, BLE_TX);  // RX, TX

void setSpeed(int);
void left(int);
void right(int);
void up(int);
void down(int);

void setup() {
  // pinMode(TRIG_PIN, OUTPUT);
  // pinMode(ECHO_PIN, INPUT);
  pinMode(FRONT_RIGHT_1, OUTPUT);
  pinMode(FRONT_RIGHT_2, OUTPUT);
  pinMode(FRONT_LEFT_1, OUTPUT);
  pinMode(FRONT_LEFT_2, OUTPUT);
  pinMode(BACK_RIGHT_1, OUTPUT);
  pinMode(BACK_RIGHT_2, OUTPUT);
  pinMode(BACK_LEFT_1, OUTPUT);
  pinMode(BACK_LEFT_2, OUTPUT);
  // pinMode(ENA, OUTPUT);
  // pinMode(ENB, OUTPUT);
  // pinMode(ENC, OUTPUT);
  // pinMode(END, OUTPUT);
  // setSpeed(currentSpeed);
  Serial.begin(9600);
  ble.begin(9600);
  ble.println("AT+NAME");
}

void setSpeed(int speed) {
  // analogWrite(ENA, speed);
  // analogWrite(ENB, speed);
  // analogWrite(ENC, speed);
  // analogWrite(END, speed);
}

void wait() {
  digitalWrite(FRONT_RIGHT_1, HIGH);
  digitalWrite(FRONT_RIGHT_2, HIGH);
  digitalWrite(FRONT_LEFT_1, HIGH);
  digitalWrite(FRONT_LEFT_2, HIGH);
  digitalWrite(BACK_RIGHT_1, HIGH);
  digitalWrite(BACK_RIGHT_2, HIGH);
  digitalWrite(BACK_LEFT_1, HIGH);
  digitalWrite(BACK_LEFT_2, HIGH);
}

void left_wait() {
  digitalWrite(FRONT_LEFT_1, HIGH);
  digitalWrite(FRONT_LEFT_2, HIGH);
  digitalWrite(BACK_LEFT_1, HIGH);
  digitalWrite(BACK_LEFT_2, HIGH);
}

void right_wait() {
  digitalWrite(FRONT_RIGHT_1, HIGH);
  digitalWrite(FRONT_RIGHT_2, HIGH);
  digitalWrite(BACK_RIGHT_1, HIGH);
  digitalWrite(BACK_RIGHT_2, HIGH);
}

void test() {
  digitalWrite(FRONT_RIGHT_1, HIGH);
  digitalWrite(FRONT_RIGHT_2, LOW);
  digitalWrite(FRONT_LEFT_1, HIGH);
  digitalWrite(FRONT_LEFT_2, LOW);
  digitalWrite(BACK_RIGHT_1, HIGH);
  digitalWrite(BACK_RIGHT_2, LOW);
  digitalWrite(BACK_LEFT_1, HIGH);
  digitalWrite(BACK_LEFT_2, LOW);
  delay(2000);
  digitalWrite(FRONT_RIGHT_2, HIGH);
  digitalWrite(FRONT_RIGHT_1, LOW);
  digitalWrite(FRONT_LEFT_2, HIGH);
  digitalWrite(FRONT_LEFT_1, LOW);
  digitalWrite(BACK_RIGHT_2, HIGH);
  digitalWrite(BACK_RIGHT_1, LOW);
  digitalWrite(BACK_LEFT_2, HIGH);
  digitalWrite(BACK_LEFT_1, LOW);
  delay(2000);
  digitalWrite(FRONT_RIGHT_2, LOW);
  digitalWrite(FRONT_RIGHT_1, LOW);
  digitalWrite(FRONT_LEFT_2, LOW);
  digitalWrite(FRONT_LEFT_1, LOW);
  digitalWrite(BACK_RIGHT_2, LOW);
  digitalWrite(BACK_RIGHT_1, LOW);
  digitalWrite(BACK_LEFT_2, LOW);
  digitalWrite(BACK_LEFT_1, LOW);
  delay(2000);
}

void demo() {
  up(1500);
  left(1500);
  up(1500);
  right(1500);
  wait();
}

void toggleState() {
  if (nextCommand == STOP) {
    nextCommand = GO_UP;
  } else {
    nextCommand = STOP;
  }
}

void reset() {
  bleStr = "";
  maxDistance = 20.0;
  autoMode = false;
  nextLeft = true;
  currentSpeed = INIT_SPEED;
  nextCommand = STOP;
}

void move(int wheel, bool forward) {
  int w1 = -1;
  int w2 = -2;
  switch (wheel) {
    case FRONT_RIGHT:
      w1 = FRONT_RIGHT_1;
      w2 = FRONT_RIGHT_2;
      break;
    case FRONT_LEFT:
      w1 = FRONT_LEFT_1;
      w2 = FRONT_LEFT_2;
      break;
    case BACK_RIGHT:
      w1 = BACK_RIGHT_1;
      w2 = BACK_RIGHT_2;
      break;
    case BACK_LEFT:
      w1 = BACK_LEFT_1;
      w2 = BACK_LEFT_2;
      break;
  }
  if (w1 < 0 || w2 < 0) {
    return;
  }
  digitalWrite(w1, forward ? HIGH : LOW);
  digitalWrite(w2, forward ? LOW : HIGH);
}

void moveUp(int wheel) { move(wheel, true); }

void moveDown(int wheel) { move(wheel, false); }

void up(int duration) {
  moveUp(FRONT_RIGHT);
  moveUp(FRONT_LEFT);
  moveUp(BACK_RIGHT);
  moveUp(BACK_LEFT);
  delay(duration);
}

void down(int duration) {
  moveDown(FRONT_RIGHT);
  moveDown(FRONT_LEFT);
  moveDown(BACK_RIGHT);
  moveDown(BACK_LEFT);
  delay(duration);
}

void left(int duration) {
  wait();
  delay(MOVE_DURATION);
  // setSpeed(TURN_SPEED);
  moveUp(FRONT_RIGHT);
  moveUp(BACK_RIGHT);
  moveDown(FRONT_LEFT);
  moveDown(BACK_LEFT);
  delay(duration);
  wait();
  delay(MOVE_DURATION);
  // setSpeed(currentSpeed);
}

void leftBack() { left(TURN_DURATION * 2); }

void right(int duration) {
  wait();
  delay(MOVE_DURATION);
  // setSpeed(TURN_SPEED);
  moveUp(FRONT_LEFT);
  moveUp(BACK_LEFT);
  moveDown(FRONT_RIGHT);
  moveDown(BACK_RIGHT);
  delay(duration);
  wait();
  delay(MOVE_DURATION);
  // setSpeed(currentSpeed);
}

void rightBack() { right(TURN_DURATION * 2); }

void leftM(int duration) {
  moveUp(FRONT_RIGHT);
  moveUp(BACK_RIGHT);
  moveDown(FRONT_LEFT);
  moveDown(BACK_LEFT);
  delay(duration);
}

void rightM(int duration) {
  moveUp(FRONT_LEFT);
  moveUp(BACK_LEFT);
  moveDown(FRONT_RIGHT);
  moveDown(BACK_RIGHT);
  delay(duration);
}

// void parseCommand() {
//  if (irrecv.decode(&results)) {
//    Serial.println(results.value, HEX);
//    if (results.value == IR_UP) {
//      nextCommand = GO_UP;
//    } else if (results.value == IR_LEFT) {
//      nextCommand = GO_LEFT;
//    } else if (results.value == IR_RIGHT) {
//      nextCommand = GO_RIGHT;
//    } else if (results.value == IR_DOWN) {
//      nextCommand = GO_DOWN;
//    } else if (results.value == IR_SLEFT) {
//      nextCommand = GO_SLEFT;
//    } else if (results.value == IR_SRIGHT) {
//      nextCommand = GO_SRIGHT;
//    } else if (results.value == IR_NEXT_LEFT) {
//      nextLeft = true;
//    } else if (results.value == IR_NEXT_RIGHT) {
//      nextLeft = false;
//    } else if (results.value == IR_SPEED_INC) {
//      currentSpeed = min(240, currentSpeed + 20);
//    } else if (results.value == IR_SPEED_DEC) {
//      currentSpeed = max(140, currentSpeed - 20);
//    } else if (results.value == IR_SPEED_TOGGLE) {
//      if (currentSpeed >= SPEED_HIGH) {
//        currentSpeed = SPEED_LOW;
//      } else {
//        currentSpeed = SPEED_HIGH;
//      }
//    } else if (results.value == IR_SWITCH_A) {
//      toggleState();
//    } else if (results.value == IR_SWITCH_B) {
//      toggleState();
//    }  else if (results.value == IR_DISTANCE) {
//      if (maxDistance < 15.0) {
//        maxDistance = 20.0;
//      } else {
//        maxDistance = 10.0;
//      }
//    } else if (results.value == IR_RESET) {
//      reset();
//      nextCommand = STOP;
//    }
//    results.value = 0;
//    irrecv.resume(); // Receive the next value
//  }
//}

void readBleData() {
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
}

void parseBleCommand() {
  readBleData();
  if (bleCmd == "") {
    return;
  }
  Serial.print("String: ");
  Serial.println(bleStr);
  Serial.print("Command: ");
  Serial.println(bleCmd);
  if (bleCmd == BLE_UP) {
    nextCommand = GO_UP;
  } else if (bleCmd == BLE_LEFT) {
    nextCommand = GO_LEFT;
  } else if (bleCmd == BLE_RIGHT) {
    nextCommand = GO_RIGHT;
  } else if (bleCmd == BLE_DOWN) {
    nextCommand = GO_DOWN;
  } else if (bleCmd == BLE_SLEFT) {
    nextCommand = GO_SLEFT;
  } else if (bleCmd == BLE_SRIGHT) {
    nextCommand = GO_SRIGHT;
  } else if (bleCmd == BLE_NEXT_LEFT) {
    nextLeft = true;
  } else if (bleCmd == BLE_NEXT_RIGHT) {
    nextLeft = false;
  } else if (bleCmd == BLE_AUTO_MODE) {
    autoMode = true;
  } else if (bleCmd == BLE_MANUAL_MODE) {
    autoMode = false;
  } else if (bleCmd == BLE_SPEED_INC) {
    currentSpeed = min(240, currentSpeed + 20);
  } else if (bleCmd == BLE_SPEED_DEC) {
    currentSpeed = max(140, currentSpeed - 20);
  } else if (bleCmd == BLE_SPEED_TOGGLE) {
    if (currentSpeed >= SPEED_HIGH) {
      currentSpeed = SPEED_LOW;
    } else {
      currentSpeed = SPEED_HIGH;
    }
  } else if (bleCmd == BLE_STOP || bleCmd == BLE_STOP2) {
    nextCommand = STOP;
  } else if (bleCmd == BLE_DISTANCE) {
    if (maxDistance < 15.0) {
      maxDistance = 20.0;
    } else {
      maxDistance = 10.0;
    }
  } else if (bleCmd == BLE_RESET || bleCmd == BLE_RESET2) {
    reset();
    nextCommand = STOP;
  }
  bleCmd = "";
  bleStr = "";
}

void manualExecute() {
  switch (nextCommand) {
    case GO_UP:
      up(MOVE_DURATION);
      break;
    case GO_LEFT:
      leftM(MOVE_DURATION / 2);
      break;
    case GO_RIGHT:
      rightM(MOVE_DURATION / 2);
      break;
    case GO_DOWN:
      down(MOVE_DURATION);
      break;
    case GO_SLEFT:
      leftM(MOVE_DURATION / 2);
      break;
    case GO_SRIGHT:
      rightM(MOVE_DURATION / 2);
      break;
    case STOP:
      wait();
      break;
    default:
      break;
  }
  nextCommand = STOP;
}

void autoExecute() {
  switch (nextCommand) {
    case GO_UP:
      up(MOVE_DURATION);
      nextCommand = GO_UP;
      break;
    case GO_LEFT:
      left(TURN_DURATION);
      nextCommand = GO_UP;
      break;
    case GO_RIGHT:
      right(TURN_DURATION);
      nextCommand = GO_UP;
      break;
    case GO_DOWN:
      down(TURN_DURATION);
      nextCommand = STOP;
      break;
    case GO_SLEFT:
      left(TURN_DURATION / 2);
      nextCommand = GO_UP;
      break;
    case GO_SRIGHT:
      right(TURN_DURATION / 2);
      nextCommand = GO_UP;
      break;
    case GO_LBACK:
      leftBack();
      nextCommand = GO_UP;
      break;
    case GO_RBACK:
      rightBack();
      nextCommand = GO_UP;
      break;
    case STOP:
      wait();
      break;
    default:
      break;
  }
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  float delta = pulseIn(ECHO_PIN, HIGH);
  float distance = delta / 58.8;  // in cm
  //  Serial.print("Echo = ");
  //  Serial.print(delta);
  //  Serial.print(" || Distance = ");
  //  Serial.print(distance);
  //  Serial.println("cm");
  return distance;
}

// US distance check
void checkBarrier() {
  if (!autoMode) {
    return;
  }
  if (nextCommand == STOP) {
    return;
  }
  float distance = getDistance();
  if (distance <= maxDistance) {
    wait();
    delay(MOVE_DURATION);
    down(MOVE_DURATION);
    if (nextLeft) {
      left(TURN_DURATION);
    } else {
      right(TURN_DURATION);
    }
  }
}

void loop() {
  parseBleCommand();
  test();
  // if (autoMode) {
  //   autoExecute();
  //   checkBarrier();
  // } else {
  //   manualExecute();
  // }
}