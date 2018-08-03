// TO DO:
// Implement a motion profile (sinusoidal? triangular?)
// Implement base motor control (separate/parallel threads?)
// Best frequency calibration (check RSSI on various channels?)

#include "math.h"
#include <Servo.h>
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

/************ Radio Setup ***************/
#define RF69_FREQ      915.0
#define MY_ADDRESS     1
#define JOYSTICK_ADDRESS  3  
#define COMPUTER_ADDRESS  2

#if defined(ARDUINO_SAMD_FEATHER_M0) // Feather M0 w/Radio
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#define LED           12
#endif
/************ END Radio Setup ***************/

// #define STOP_BTN      11
LEFT_MOTOR_FORWARD    9
LEFT_MOTOR_BACKWARD   8
RIGHT_MOTOR_FORWARD   11
RIGHT_MOTOR_BACKWARD  12
WHEEL_EN              13

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

int16_t packetnum = 0;  // packet counter, we increment per xmission

// Set up IO
Servo baseServo; // Bottom, base rotation
Servo reachServo; // Forward and backwards of arm
Servo heightServo; // Z-axis moving of arm
Servo clawServo; // Claw servo

/**********
  --Maximum Angles of Servos--
  Base Servo - 0 = right most, 180 = left most
  Reach Servo - 60 = vertical postion, 145 = forward most
  Height Servo - 0 = lowest height, 120 = highest height (about 90 deg motion)
  Claw Servo - 0 = closed, ~<90 = past widest open
***********/

int trackedPosB = 0;
int trackedPosR = 0;
int trackedPosH = 0;
int trackedPosC = 0;

int moveToB = 0;
int moveToR = 0;
int moveToH = 0;
int moveToC = 0;

// Dont put this on the stack:
uint8_t data[] = "Node 2 Acknowledges";
// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void setup() {
  Serial.begin(115200);

  // Reset Radio
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather Addressed RFM69 RX Test!");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69_manager.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  
  Serial.println("RFM69 radio init OK!");
  
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
                  };
  rf69.setEncryptionKey(key);

  // Assign servos
  baseServo.attach(5); // Attach servos to PWM IO
  heightServo.attach(6);
  clawServo.attach(9);
  reachServo.attach(10);

  // Assign base motor control IO
  pinMode(LEFT_MOTOR_FORWARD,OUTPUT);
  pinMode(LEFT_MOTOR_BACKWARD,OUTPUT);
  pinMode(RIGHT_MOTOR_FORWARD,OUTPUT);
  pinMode(RIGHT_MOTOR_BACKWARD,OUTPUT);
  pinMode(WHEEL_EN, OUTPUT);

  // Status LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  calibrate();
}

void loop() {
  // Check for message first
  if (rf69_manager.available()) {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;

    // If a message is received
    if (rf69_manager.recvfromAck(buf, &len, &from)) {
      buf[len] = 0; // zero out remaining string

      /*
      Serial.print("Got packet from #"); Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf);
      */
      
      Blink(LED, 1, 3); //blink LED 3 times, 1ms between blinks

      // Send a reply back to the originator client
      if (!rf69_manager.sendtoWait(data, sizeof(data), from)) {
        //Serial.println("Sending failed (no ack)");
      }
    }

    if(from == COMPUTER_ADDRESS) {
      
    }

    if (buf[0] == 240 && buf[9] == 245) {
      digitalWrite(LED, HIGH);
      setMoves(buf);
      digitalWrite(LED, LOW);
    }

  } else {
    delay(0.01);
  }
}


// Reads for different serial data and "moveTo" values then executes
void setMoves(uint8_t buf[]) {

  if (buf[0] == 240) { // Start indicator
    if (buf[1] == 241) { // Indicator for X value
      moveToB = buf[2]; // read again since value that follows is X
    }

    if (buf[3] == 242) { // Indicator for Y Value
      moveToH = buf[4]; // read for H
    }

    if (buf[5] == 243) { // Indicator for Z value "reach"
      moveToR = buf[6]; // read for R
    }

    if (buf[7] == 244) { // Indicator for Claw`
      moveToC = buf[8]; // read for C
    }
  }

  if (buf[9] == 245) { // end of all coordinates
    // Move motors
    multiStepTo(moveToB, moveToR, moveToH, moveToC);
  }
}

// Set initial arm position
void calibrate() {
  stepTo('b', 0, 90, 1);
  delay(500);
  stepTo('h', 45, 100, 1); // set height first because it keeps hitting itself
  delay(500);
  stepTo('r', 45, 90, 1);
  delay(500);
  stepTo('c', 0, 85, 1);
  delay(2000);
}

// Used for calibrate to set initial trackedPos positions
void stepTo(char servo, int start, int angle, int speed) {
  if (start >= angle) {
    for (int i = start; i >= angle; i -= speed) {
      switch (servo) {
        case 'b':
          baseServo.write(i);
          trackedPosB = angle;
          break;
        case 'r':
          reachServo.write(i);
          trackedPosR = angle;
          break;
        case 'h': // vertical height
          heightServo.write(i);
          trackedPosH = angle;
          break;
        case 'c': // hand
          clawServo.write(i);
          trackedPosC = angle;
          break;
      }
      delay(10);
    }
  } else {
    for (int i = start; i <= angle; i += speed) {
      switch (servo) {
        case 'b':
          baseServo.write(i);
          trackedPosB = angle;
          break;
        case 'r':
          reachServo.write(i);
          trackedPosR = angle;
          break;
        case 'h': // vertical height
          heightServo.write(i);
          trackedPosH = angle;
          break;
        case 'c': // hand
          clawServo.write(i);
          trackedPosC = angle;
          break;
      }
      delay(10);
    }
  }
}

void multiStepTo(int bAngle, int rAngle, int hAngle, int cAngle) {
  // Find out which angle is the largest
  float maxDiff = findMax(bAngle, rAngle, hAngle, cAngle);

  // Make sure to divide every other motors' step by the number of the largest
  float bStepAngle = float(difference(bAngle, trackedPosB)) / maxDiff;
  float rStepAngle = float(difference(rAngle, trackedPosR)) / maxDiff;
  float hStepAngle = float(difference(hAngle, trackedPosH)) / maxDiff;
  float cStepAngle = float(difference(cAngle, trackedPosC)) / maxDiff;

  // Find out if step angle needs to be positive or negative
  if (bAngle < trackedPosB) {
    bStepAngle *= -1;
  }
  if (rAngle < trackedPosR) {
    rStepAngle *= -1;
  }
  if (hAngle < trackedPosH) {
    hStepAngle *= -1;
  }
  if (cAngle < trackedPosC) {
    cStepAngle *= -1;
  }

  //Serial.println(bStepAngle);
  //Serial.println(rStepAngle);
  //Serial.println(hStepAngle);
  //Serial.println(cStepAngle);

  // The servo with the most travel will move in increments of one
  // and the others will move proportionally to it

  // int + float will auto round to an int, round just in case?
  for (int step = 0; step < maxDiff; step++) {
    baseServo.write(trackedPosB + round(bStepAngle * step));
    reachServo.write(trackedPosR + round(rStepAngle * step));
    heightServo.write(trackedPosH + round(hStepAngle * step));
    clawServo.write(trackedPosC + round(cStepAngle * step));
    delay(3.0);
  }

  // Set the last known position as their current position after
  // the whole movement
  // Maybe want to make this at every movement
  trackedPosB = baseServo.read();
  trackedPosR = reachServo.read();
  trackedPosH = heightServo.read();
  trackedPosC = clawServo.read();
}

int findMax(int bAngle, int rAngle, int hAngle, int cAngle) {
  // List contains the delta of the angles
  int list[] = {difference(bAngle, trackedPosB),
                difference(rAngle, trackedPosR),
                difference(hAngle, trackedPosH),
                difference(cAngle, trackedPosC)
               };
  int hold;

  // Repeat this until all are sorted
  // sizeof() for ints needs to be divided by 4 to get length
  for (int i = 0; i <  (sizeof(list) / 4) - 1; i++) { // slot comparing to others
    for (int j = i + 1; j < (sizeof(list) / 4); j++) { // Compare-to Value in list
      if (list[i] > list[j]) {
        hold = list[j]; // Keep a copy of second item
        list[j] = list[i]; // Make next item first item
        list[i] = hold; // Replace first item with held value
      }
    }
  }
  return list[3]; // Return the right most value, most
}

int difference(int angle1, int angle2) {
  int diff = max(angle1, angle2) - min(angle1, angle2);
  return diff;
}

void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i = 0; i < loops; i++)  {
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN, LOW);
    delay(DELAY_MS);
  }
}
