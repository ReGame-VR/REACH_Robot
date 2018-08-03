// TO DO:
// Implement a motion profile (sinusoidal? triangular?)
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
#define LED           14
#endif
/************ END Radio Setup ***************/

#define LEFT_MOTOR_FORWARD    18
#define LEFT_MOTOR_BACKWARD   17
#define RIGHT_MOTOR_FORWARD   11
#define RIGHT_MOTOR_BACKWARD  12
#define WHEEL_EN              13

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
  uint8_t key[] = "ReGameVrLab";
  rf69.setEncryptionKey(key);

  // Assign servos
  baseServo.attach(10); // Attach servos to PWM IO
  heightServo.attach(9);
  reachServo.attach(6);
  clawServo.attach(5);

  // Assign base motor control IO
  pinMode(LEFT_MOTOR_FORWARD, OUTPUT);
  pinMode(LEFT_MOTOR_BACKWARD, OUTPUT);
  pinMode(RIGHT_MOTOR_FORWARD, OUTPUT);
  pinMode(RIGHT_MOTOR_BACKWARD, OUTPUT);
  pinMode(WHEEL_EN, OUTPUT);

  // Status LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  calibrate();
}

void loop() {
  // Buffers for the arm and base movements
  uint8_t armBuf[10];
  uint8_t baseBuf[2];

  // Check for message first
  if (rf69_manager.available()) {
    uint8_t len = sizeof(buf);
    uint8_t from;

    // A message is received
    if (rf69_manager.recvfromAck(buf, &len, &from)) {
      buf[len] = {0}; // zero out remaining string
      Blink(LED, 1, 3); //blink LED 3 times, 1ms between blinks

      /*
        Serial.print("Got packet from #"); Serial.print(from);
        Serial.print(" [RSSI :");
        Serial.print(rf69.lastRssi());
        Serial.print("] : ");
        Serial.println((char*)buf);
      */
      
      // Send a reply back to the originator client
      if (!rf69_manager.sendtoWait(data, sizeof(data), from)) {
        //Serial.println("Sending failed (no ack)");
      }

      // Update the buffers for the base and for the arm
<<<<<<< HEAD
      //if (from == COMPUTER_ADDRESS) {
=======
      if (from == COMPUTER_ADDRESS) {
        digitalWrite(LED, HIGH);
        memcpy(armBuf, buf, sizeof(buf));
        digitalWrite(LED, LOW);
      } else if (from == JOYSTICK_ADDRESS) {
>>>>>>> 74a7ba646cf7f9d7181aaed072cbb8691890d9b2
        digitalWrite(LED, HIGH);
        for(int i = 0; i < sizeof(armBuf); i++) {
          armBuf[i] = buf[i];
        }
        digitalWrite(LED, LOW);
<<<<<<< HEAD
      //} else if (from == JOYSTICK_ADDRESS) {
      //  digitalWrite(LED, HIGH);
      //  for(int i = 0; i < sizeof(baseBuf); i++) {
      //    baseBuf[i] = buf[i];
      //  }
      //  digitalWrite(LED, LOW);
      //}
=======
      }
>>>>>>> 74a7ba646cf7f9d7181aaed072cbb8691890d9b2

      // Every 10 milliseconds, update all motors (is 10 ms too long?)
      //if (millis() % 10 != 0) {
      digitalWrite(LED, HIGH);
      setMotors(armBuf, baseBuf);
      digitalWrite(LED, LOW);
      //} else {
      delay(2);
      //}
    }
  }
}

// Distribute motor controls to corresponding functions
void setMotors(uint8_t armBuf[], uint8_t baseBuf[]) {
  // Set Motors first (since their value will hold continously)
  //setBaseMotors(baseBuf);

  // Then set servo motors
  setMoves(armBuf);
}

// Blink LED
void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i = 0; i < loops; i++)  {
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN, LOW);
    delay(DELAY_MS);
  }
}
