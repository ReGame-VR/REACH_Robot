Imported from [Gilbert Yap's GitHub](https://github.com/gilbertyap/REACH_Robot)

# REACH_Robot
[Northeastern University ReGame-VR Lab](http://www.northeastern.edu/regamevrlab/) [REACH Robot](https://www.northeastern.edu/rise/presentations/motion-capture-based-robotic-interfaces-enhance-engagement-adherence-pediatric-rehabilitation/) software

Contact Information:
- Gilbert Yap (Alum): gilbert.yap16@gmail.com
- Dr. Danielle Levac: d.levac@neu.edu


## Software requirements
- Leap Motion Controller SDK (currently using Orion SDK (version 3 ONLY) (Version 3 only, since Version 4 and up no longer supports Python))
- Python 2.7 (version 3 is incompatible with current code)
- PyCharm Community Edition for running Leap code in a terminal window
- Arduino IDE
- [Feather M0 RFM95 drivers](https://learn.adafruit.com/adafruit-feather-m0-radio-with-RFM95-packet-radio/setup)

## Folder breakdown

###### Hardware Requirements
- Leap Motion controller
- Adafruit Feather M0 RFM95 x 3
- [Computer that meets the Leap Motion Controller minimum requirements](https://support.leapmotion.com/hc/en-us/articles/223783668-What-are-the-system-requirements-)
- At least two (2) dedicated USB ports (splitters will not work) for the Leap Motion controller and one (1) Feather M0 RFM95

###### ReachLeap and ReachLeap_Rotate
These folders contain the Python code for recognizing movement with the Leap Motion controller. The ```_rotate``` suffix refers to using rotational movement system, whereas no suffix refers to XYZ coordinate movement. These folder rely on a text file with the limitations of movement, currently titled ```test_limits.txt```. Hand positions relative to these limits are calculated and converted to motor positions for the robot arm, then sent over serial to the Feather M0 RFM95 connected to the computer via USB.

###### LeapCalibrate and LeapCalibrate_Rotate
These folders are used for calibrating the Leap Motion controller REACH software (ReachLeap and ReachLeap_Rotate) to the range of movement of the user.

###### LeapAssests
This folder contains necessary files for using Leap with Python

**_NOTE FOR THE ABOVE FOLDERS: Depending on the computer system, it may require changing the Leap.dll and LeapPython.pyd. Check the LeapAssests folder for the 32-bit (x86) and 64-bit (x64) files_**

###### RobotArm_Feather/ServoArm
This folder contains the code for the Feather M0 RFM95 that is used to control the robot arm and the wheeled-base. The code receives the motor position data that was wireless transmitted from the computer and sets the motors on the arm accordingly. This code is also responsible for controlling the wheels on the base, which is wireless transmitted from a joystick using another Feather M0 RFM95.

###### RobotArm_Photon/ServoArm
Outdated code for use with the Particle Photon. With the switch to RF-based Feather M0 RFM95, this code is obsolete.

###### TX_Computer_Transceiver/computer_tx_leap
This folder contains the code for the Feather M0 RFM95 that will be connected to the computer using the Leap Motion controller. The Feather M0 RFM95 can be programmed with [this guide](https://learn.adafruit.com/adafruit-feather-m0-radio-with-RFM95-packet-radio/using-with-arduino-ide). The serial data containing the motor positions from the ReachLeap/ReachLeap_Rotate software is then wireless transmitted to the arm/base with a matching microcontroller.

###### Base_Joystick
This folder will contain the code for the Feather M0 RFM95 that reads joystick data and sends it wirelessly.
