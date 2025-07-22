# Problem1
Name- Naman Bhatia
Roll No- B24258
Power Ratings
Logic Voltage- 5V
Input voltage range-6.5 to 28V
Max Output current- 30A (Total)

Features of the motor driver-
Screw terminals for motor power input and output
Pin headers for microcontroller input ( EN, IN1, IN2, PWM )
Seperate PWM input for enhanced efficiency
Flyback diodes to protect against inductive kickback (research their placement and specification)
An onboard 5V voltage regulator
LEDs to indicate motor direction
5V output pin
Reverse polarity and ESD protection


Trace width calculations
I used 3 different types of trace widths
1.)	For main power and motor input net – 1mm
2.)	For 5V internal logic- 0.8mm
3.)	For signal traces- 0.25 mm
4.) For motor output I used 3 traces of 0.5mm in parallel to reduce the load on the pins of the motor driver
The following formula was used
![image](https://github.com/user-attachments/assets/e41d00eb-ed5e-4bd8-a35b-070c5f3c9b5c)

 
Then due to design constraints and to properly route the components, the values given above were used.
Component Selection
VNH5019 was used because of it’s higher efficiency, accuracy and ability to control powerful motors
MP2338 motor driver was used because of it’s compact size and efficiency.
A mosfet, schottky diode and a zener diode were used together at the power input to protect the pcb against ESD and reverse polarity
Other components were selected according to the recommendations given in the main IC’s datasheet.


