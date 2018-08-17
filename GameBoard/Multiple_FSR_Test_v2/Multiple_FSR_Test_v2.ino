// include the library code:
#include <LiquidCrystal.h> //This is for the LCD specific functions

LiquidCrystal lcd(7, 8, 9, 10, 11 , 12); //These are the pins on the Arduino Uno that the LCD is connected to

//testing holding fsr values in an array
#define numSensors 6 //When more FSRs are added, change this number here 
int FSR[numSensors];
int oldFSR[numSensors];
String StringOne = ("Congrats! You got #");
String StringTwo = ("You removed #");
String PrintString;
int StartMillis[numSensors];
int ElapsedMillis[numSensors];


void setup(void) {
  Serial.begin(9600);  
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
}
 
void loop(void) {
  
  for (int i = 0; i <= numSensors - 1; i++){  //Iterate through however many sensors are connected
    FSR[i] = analogRead(i);
    if (FSR[i] - oldFSR[i] > 100) { //If the FSR experiences a substantial change in force
      StartMillis[i] = millis();
      PrintString = StringOne + (i+1);
      lcd.setCursor(0,0);
      lcd.print(PrintString);
      delay(1000);
      for (int positionCounter = 0; positionCounter < 7; positionCounter++){
        lcd.scrollDisplayLeft();
        delay(500);
      }
      lcd.clear();
      oldFSR[i] = FSR[i];
    }
    if (FSR[i] - oldFSR[i] < -100) {
      ElapsedMillis[i] = millis();
      if (ElapsedMillis[i] - StartMillis[i] > 5000){  //only detect removal if 5 seconds has passed
        PrintString = StringTwo + (i+1);
        lcd.setCursor(0,0);
        lcd.print(PrintString);
        delay(1250);
        lcd.clear();
      }
    }
    oldFSR[i] = FSR[i]; //Stored each FSR value so each loop iteration can detect the change in FSR values
  }
 lcd.setCursor(0,1);
 lcd.print("Time: ");
 lcd.setCursor(6,1);
 lcd.print(millis()/1000);
 delay(100);
}
