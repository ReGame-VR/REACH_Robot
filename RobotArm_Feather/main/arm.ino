// Reads for different serial data and "moveTo" values then executes
// TODO is this header value checking redundant?
void setMoves(uint8_t armBuf[]) {

  if (armBuf[0] == 240) { // Start indicator
    if (armBuf[1] == 241) { // Indicator for X value
      moveToB = armBuf[2]; // read again since value that follows is X
    }

    if (armBuf[3] == 242) { // Indicator for Y Value
      moveToH = armBuf[4]; // read for H
    }

    if (armBuf[5] == 243) { // Indicator for Z value "reach"
      moveToR = armBuf[6]; // read for R
    }

    if (armBuf[7] == 244) { // Indicator for Claw`
      moveToC = armBuf[8]; // read for C
    }
  }

  if (armBuf[9] == 245) { // end of all coordinates
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

void multiStepTo(uint8_t bAngle, uint8_t rAngle, uint8_t hAngle, uint8_t cAngle) {
  // Find out which angle is the largest
  float maxDiff = findMax(bAngle, rAngle, hAngle, cAngle);

  // Make sure to divide every other motors' step by the number of the largest
  float bStepAngle = difference(bAngle, trackedPosB) / maxDiff;
  float rStepAngle = difference(rAngle, trackedPosR) / maxDiff;
  float hStepAngle = difference(hAngle, trackedPosH) / maxDiff;
  float cStepAngle = difference(cAngle, trackedPosC) / maxDiff;

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

  //Serial.println(bAngle);
  //Serial.println(rAngle);
  //Serial.println(hAngle);
  //Serial.println(cAngle);

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

int findMax(uint8_t bAngle, uint8_t rAngle, uint8_t hAngle, uint8_t cAngle) {
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

float difference(uint8_t angle1, uint8_t angle2) {
  float diff = max(angle1, angle2) - min(angle1, angle2);
  return diff;
}

