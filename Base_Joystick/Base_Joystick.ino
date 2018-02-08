#include <math.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

/************ Radio Setup ***************/
#define RF69_FREQ      915.0

#define MY_ADDRESS    3
#define DEST_ADDRESS  1

#if defined(ARDUINO_SAMD_FEATHER_M0) // Feather M0 w/Radio
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#define LED           12
#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

int16_t packetnum = 0;  // packet counter, we increment per xmission

// Dont put this on the stack:
char serial_packet[10];
char old_serial_packet[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = "  OK";
/************ END Radio Setup ***************/

//source: http://www.goodliffe.org.uk/arduino/joystick.php
//values are such that (9,6) [min] is the top left corner of the joystick
int analogInputPinX = 1;
int analogInputPinY = 0;
long analogInputValX;
long analogInputValY;
int digitalX;
int digitalY;
int packet_length = 4;
int numOfCycles = 0;

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
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  pinMode(analogInputPinX, INPUT);
  pinMode(analogInputPinY, INPUT);
}

void loop() {
  analogInputValX = analogRead(analogInputPinX) - 542;
  analogInputValY = -1 * analogRead(analogInputPinY) + 525;
  numOfCycles++;
  digitalX += round(analogInputValX);
  digitalY += round(analogInputValY);

  /*
  Serial.print("X: ");
  Serial.print(analogInputValX);
  Serial.print(", Y: ");
  Serial.print(analogInputValY);
  Serial.println(" "); // println, to end with a carriage return
  */

  // Send transmission every 10 ms
  if(millis() % 10 != 0) {
    // Low pass filter for values
    digitalX = digitalX/numOfCycles;
    digitalY = digitalY/numOfCycles;
    numOfCycles = 0;
    // Change 2D coordinates to tire control
    int motorX = changeX(digitalX);
    int motorY = changeY(digitalY);

    // Since int is 2 bytes, need to split each motor value into 2 char (1 byte)
    buf[0] = char(motorX>>8);
    buf[1] = char(motorX);
    buf[2] = char(motorY>>8);
    buf[3] = char(motorY);
    
    // Send values to base
    if (rf69_manager.sendtoWait((uint8_t *)serial_packet, strlen(serial_packet), DEST_ADDRESS)) {
      // Now wait for a reply from the server
      uint8_t len = sizeof(buf);
      uint8_t from;
      if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
        buf[len] = 0; // zero out remaining string

        Serial.print("Got reply from #"); Serial.print(from);
        Serial.print(" [RSSI :");
        Serial.print(rf69.lastRssi());
        Serial.print("] : ");
        Serial.println((char*)buf);
      } else {
        Serial.println("No reply, is anyone listening?");
      }
    } else {
      Serial.println("Sending failed (no ack)");
    }
  } else {
    delay(2);
  }
}

// Determines the amount of turning
int changeX(int axisX) {
  // Check these min/max values
  return ((1-axisX)*-255) + (axisX*255);
}

// Determines that forward/backward speed
int changeY(int axisY) {
  // Check these min/max values
  return ((1-axisY)*-255) + (axisY*255);
}

