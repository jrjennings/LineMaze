# LineMaze

Description:  
Maze Solving Robot   
This bot is designed to solve a Line Maze as definded in the Robothon Rules by the Seattle Robotics Society.    https://robothon.org/rules-line-maze/  

Components:  
ATMEGA 328P-PU - Mouser #556-ATMEGA328P-PU  
30:1 Micro Metal Gearmotor MP - Pololu item #2364  
Micro Metal Gearmotor Bracket Extended Pair - Pololu item #1089  
Wheel 32×7mm Pair - White - Pololu item #1088  
STMicroelectronics L293D motor driver - Mouser #511-L293D  
9V Step-Up Voltage Regulator U3V12F9 - Pololu item #2116   
5V, 500mA Step-Down Voltage Regulator D24V5F5 - Pololu #2843  
2 CNY70 (IR) reflective sensors connected to MC analog pins for following line - Mouser #782-CNY70  
4 CNY70 (IR) reflective sensors connected to MC digital pins for maze logic - Mouser #782-CNY70  
Ball Caster with 3/8" Metal Ball - Pololu #951  


Programing:  
This design steers the bot by reducing the motor rotation relative to sensor reading.  
The more black the sensor sees the more reduction to motor rotation on that side.  
The bot will navigate favoring the left turn at intersections on the initial run.  

Arrangement of the CNY70 (IR) reflective sensors attached to their designated pins.

#########################################  
|  |  | | |  |  |
|-|-|-|-|-|-|
|-|-| irPin0 |-|-| Digital |
|-| irPin1 |-| irPin2 |-| Analog |
| irPin3 |-| irPin4 |-| irPin5| Digital |
  

L293D Truth Table   
#########################################
| Enable | 1A | 2A | function |
|---|---|---|---|
| H | H | L | Turn Right |  
| H | L | H | Turn Left |      
| H | H | H | Brake | 
| L | X | X | Brake | 

Basic MAZE Logic  
#########################################  
Formula for creating direction value dVal:  
dVal = irV[0] * 100 + irV[3] * 10 + irV[5] * 1  

| dVal |Action | Type intersection | 
|---|---|---|
| 100 | S | * Right Branch |
| 010 | L | * Tee |
| 000 | L | * 4Way |  
| 110 | R | Right Turn (no straight path) |  
| 001 | S | * Left Branch |
| 101 | L | Left turn (no straight path) |     
| 111 | B | * End of line |
| 011 | S | Tracking Line |
* = indicates an official intersection to be recorded 
in the path and used when optimizing the path.  

L = left turn  
R = right turn  
S = going straight past a turn  
B = turning around  

Substitutions to optimize path  
LBR = B  
LBS = R  
RBL = B  
SBL = R  
SBS = B  
LBL = S  


![alt text](https://github.com/jrjennings/LineMaze/blob/master/1.jpeg?raw=true)
![alt text](https://github.com/jrjennings/LineMaze/blob/master/2.jpeg?raw=true)
