/*
Line Maze Solving Robot
by John Jennings
johnrjennings@me.com
##########################################################################################
##########################################################################################
Major Components:
ATMEGA 328P-PU
150:1 Micro Metal Gearmotor HP - Pololu item #997
Micro Metal Gearmotor Bracket Extended Pair Pololu item #1089
Wheel 32×7mm Pair - White Pololu item #1088
STMicroelectronics L293D motor driver - Mouser #511-L293D
Adjustable Boost Regulator 2.5-9.5 (set to 6v) Pololu item #791
5V, 500mA Step-Down Voltage Regulator D24V5F5 Pololu #2843
2 CNY70 (IR) reflective sensors connected to MC analog pins for following line
4 CNY70 (IR) reflective sensors connected to MC digital pins for maze logic
Ball Caster with 3/8" Metal Ball Pololu #951

This design steers the bot by reducing the motor rotation relative to sensor reading
The more black the sensor sees the more reduction to motor rotation on that side
The bot will favor the left wall in the initial run

PINS attached to CNY70 (IR) reflective sensors
##########################################################################################
##########################################################################################
                    irPin0                 Digital
              irPin1      irPin2           Analog 
        irPin3      irPin4       irPin5    Digital 

L293D Truth Table 
##########################################################################################
##########################################################################################
Enable   1A  2A   function
H        H   L    Turn Right
H        L   H    Turn Left      
H        H   H    Brake
L        X   X    Brake

MAZE Logic
##########################################################################################
##########################################################################################
Actions taken are based on values of 6 sensors

Action are taken when irV[3] or irV[5] are LOW (see the black line)
OR when irV[4] is HIGH (see white)

Formula for direction value:
dVal = irV[0] * 100 + irV[3] * 10 + irV[5]*1

irV[0],[3],[5]  dVal        Action    Type intersection  
 
1,0,0           100           S        *Right Branch
0,1,0            10           L        *T
0,0,0             0           L        *4Way
1,1,0           110           R        Right Turn (no straight path) 
0,0,1             1           S        *Left Branch
1,0,1           101           L        Left turn (no straight path) 
1,1,1           111           B        *End of line
0,1,1            11           S        Tracking Line

(*) indicates an official intersection to be recorded in the path array
    and used when optimizing the path.

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
*/

int irPin[] = {5,A4,A5,11,6,4};                                                         
const int buttonPin = 13;              // PIN  User Button
int led1Pin = A3;                      // PIN 1 REd Led  
int led2Pin = A1;                      // PIN 2 Green Led 
int led3Pin = 12;                      // PIN 3 Red Led by Rt wheel 
int buttonState = 0;
int spMax = 100;
int dVal = 0;
long irV[] ={0,0,0,0,0,0};      
long irVMax[] = {0,1000,10,0,0,0};
long irVMin[] = {1,100,100,1,1,1};
long initialMillis = 0;
int ledState = LOW;                    // ledState used to set the LED
char path[50] = {};
//char path[3] = {'S','R','R'};
int pathLength;
int readLength;
int turn;

/*
SETUP
##########################################################################################
##########################################################################################
*/
void setup() { 
  Serial.begin(9600);
  
  //Setup Right motor
  pinMode(7, OUTPUT);                  
  pinMode(8, OUTPUT); 
  //Setup Left motor
  pinMode(2, OUTPUT);                  
  pinMode(3, OUTPUT); 
  //Setup PWM
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);

  pinMode(buttonPin, INPUT);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
 
 waitForButton();
 delay(2000);
 rotateRight();
 calibrateSensors();
 halt();
 waitForButton();
 delay(3000);
} 

/*
MAIN
##########################################################################################
##########################################################################################
*/
void loop() { 
  int i;
  for (i = 0; i < 6; i ++) {
    if ((i > 0) && (i < 3)) {       // read analog sensor 1 or 2
      irV[i] = map(analogRead(irPin[i]),irVMin[i],irVMax[i],-40,spMax); 
      irV[i] = constrain(irV[i], 0, spMax);
      }
    else {                          //read digital sensor 0, 3, 4, 5.
      irV[i] = digitalRead(irPin[i]);   
      }
       if ((irV[4]== HIGH || irV[3]== LOW || irV[5]== LOW) && turn == 0) {
        getDirection();
      }
      else if ((irV[4]== HIGH || irV[3]== LOW || irV[5]== LOW) && turn > 0) {    
        giveDirection(); 
      }
    else {
      forward();
      analogWrite(9, irV[1]);    //Speed of the motor on Left Motor
      analogWrite(10, irV[2]);       //Speed of the motor on Right Motor   
      }   
   }
}

/*
HALT
##########################################################################################
##########################################################################################
*/
void halt() {
  digitalWrite(2, HIGH);  
  digitalWrite(3, HIGH);   
  analogWrite(9, 0);   
  digitalWrite(8, HIGH);  
  digitalWrite(7, HIGH);   
  analogWrite(10, 0);
  delay(10);
}

/*
TURN LEFT
##########################################################################################
##########################################################################################
*/
void turnLeft() {
  digitalWrite(2, LOW);                //Establishes slower reverse direction 
  digitalWrite(3, HIGH);               //of Left Motor
  analogWrite(9, 35);  
  digitalWrite(8, HIGH);               //Establishes Forward direction  
  digitalWrite(7, LOW);                //of right Motor
  analogWrite(10, 35); 
  delay(250);                          //delay to move irPin[0] over white 
  while (digitalRead(irPin[0]) == HIGH) {                //if at a cross or branch
  digitalWrite(2, LOW);                //Establishes slower reverse direction 
  digitalWrite(3, HIGH);               //of Left Motor
  analogWrite(9, 35);  
  digitalWrite(8, HIGH);               //Establishes Forward direction of right Motor 
  digitalWrite(7, LOW);    
  analogWrite(10, 35);
  }
  digitalWrite(2, HIGH);               // Brake Left Motor
  digitalWrite(3, LOW);  
  analogWrite(9, 255);                  
  digitalWrite(8, LOW);                // Brake Right Motor 
  digitalWrite(7, HIGH);   
  analogWrite(10, 255);
  delay(1); 
}

/*
TURN RIGHT
##########################################################################################
##########################################################################################
*/
void turnRight() {
  digitalWrite(2, HIGH);               //Establishes forward direction of Left Motor
  digitalWrite(3, LOW);  
  analogWrite(9, 35);        //analogWrite(9, 44); 
  digitalWrite(8, LOW);                //Establishes slower reverse direction  
  digitalWrite(7, HIGH);               //of right Motor
  analogWrite(10, 35);       //analogWrite(10, 30);
  delay(200);
  while (digitalRead(irPin[0]) == HIGH) {        //if at a cross or branch
  digitalWrite(2, HIGH);               //Establishes forward direction of Left Motor
  digitalWrite(3, LOW);  
  analogWrite(9, 35);       //analogWrite(9, 44); 
  digitalWrite(8, LOW);                //Establishes slower reverse direction  
  digitalWrite(7, HIGH);               //of right Motor
  analogWrite(10, 35);      //analogWrite(10, 30);
  }
   digitalWrite(2, LOW);               //Brake reverse direction of Left Motor
  digitalWrite(3, HIGH);  
  analogWrite(9, 255);                  
  digitalWrite(8, HIGH);              //Brake forward direction of Right Motor 
  digitalWrite(7, LOW);   
  analogWrite(10, 255);
  delay(1); 
}

/*
STRAIGHT
##########################################################################################
##########################################################################################
*/
void straight() {
  digitalWrite(2, HIGH);               //Establishes forward direction of Left Motor
  digitalWrite(3, LOW);
  analogWrite(9, 30);                  //set left motor to 70% to match rt motor speed
  digitalWrite(8, HIGH);               //Establishes forward direction of Right Motor
  digitalWrite(7, LOW);
  analogWrite(10, 30);
  delay(30);                          //time to get through intersection
}

/*
ROTATE RIGHT
##########################################################################################
##########################################################################################
*/
void rotateRight() {
  digitalWrite(2, HIGH);               //Establishes forward direction of Left Motor
  digitalWrite(3, LOW);  
  analogWrite(9, 40);           
  digitalWrite(8, LOW);                //Establishes reverse direction of Right Motor 
  digitalWrite(7, HIGH);   
  analogWrite(10, 40);
}

/*
ROTATE RIGHT 180
##########################################################################################
##########################################################################################
*/
void rotateRight180() {
  while (digitalRead(irPin[0]) == HIGH) {
  digitalWrite(2, HIGH);              //Establishes forward direction of Left Motor
  digitalWrite(3, LOW);  
  analogWrite(9, 40);                  
  digitalWrite(8, LOW);               //Establishes reverse direction of Right Motor 
  digitalWrite(7, HIGH);   
  analogWrite(10, 47);
  }
  digitalWrite(2, LOW);               //Brake Left Motor
  digitalWrite(3, HIGH);  
  analogWrite(9, 255);                  
  digitalWrite(8, HIGH);              //Brake Right Motor 
  digitalWrite(7, LOW);   
  analogWrite(10, 255);
  delay(1);
  //halt();
}

/*
FORWARD
##########################################################################################
##########################################################################################
*/
void forward() {
  digitalWrite(2, HIGH);               //Establishes forward direction of Left Motor
  digitalWrite(3, LOW);   
  digitalWrite(8, HIGH);               //Establishes forward direction of Right Motor
  digitalWrite(7, LOW);    
}

/*
CALIBRATE
##########################################################################################
##########################################################################################
*/
void calibrateSensors() {
  initialMillis = millis();
  while (millis() - initialMillis < 5000) { 
    int i;
    for (i = 1; i < 3; i ++) {
       irV[i]  = analogRead(irPin[i]); 
       if (irV[i]  > irVMax[i]) {
         irVMax[i] = irV[i];
         }
       if (irV[i]  < irVMin[i]) {
        irVMin[i] = irV[i];
         }
    }
  }
}

/*
BUTTON FUNCTION
##########################################################################################
##########################################################################################
*/
void waitForButton () {
buttonState = digitalRead(buttonPin);
  while (buttonState == LOW) {
    digitalWrite(led3Pin, HIGH);   
    delay(200);                   
    digitalWrite(led3Pin, LOW);    
    delay(200);                   
    buttonState = digitalRead(buttonPin);
  }
}

/*
GET DIRECTIONS
##########################################################################################
##########################################################################################
*/    
void getDirection() {
 delay(20);
  irV[0] = digitalRead(irPin[0]);
  irV[1] = analogRead(irPin[1]);
  irV[2] = analogRead(irPin[2]);
  irV[3] = digitalRead(irPin[3]);
  irV[4] = digitalRead(irPin[4]);
  irV[5] = digitalRead(irPin[5]);

  dVal = irV[0] * 100 + irV[3] * 10 + irV[5] * 1 ; 
   
//  Serial.print(dVal);
//  Serial.print(" ");
//  Serial.print(irV[1]);
//  Serial.print(" ");
//  Serial.println(irV[2]);
  
  if (dVal == 110) {                   //  right turn
    digitalWrite(led1Pin, HIGH); 
    turnRight();
    digitalWrite(led1Pin, LOW); 
    }  
  else if (dVal == 101) {              //left turn
    digitalWrite(led2Pin, HIGH); 
    turnLeft();
    digitalWrite(led2Pin, LOW);
    }   
  else if (dVal == 100) {              //turn left on T
    turnLeft();
    path[pathLength] = 'L';
//  Serial.print(path[pathLength]);
    pathLength ++;
    if(path[pathLength-2]=='B') {
      correctPath();
      }   
    }
  else if (dVal == 10 && irV[4] == LOW){                //straight on right branch
    straight();
    path[pathLength] = 'S';
//  Serial.print(path[pathLength]);
    pathLength ++;
    if(path[pathLength-2]=='B') {
      correctPath();
      }
    }
  else if (dVal == 1 && irV[4] == LOW){                 //left on left branch
    turnLeft();
    path[pathLength] = 'L';
//  Serial.print(path[pathLength]);
    pathLength ++;
    if(path[pathLength-2]=='B') {
      correctPath();
      }
    }
  else if (dVal == 0  && irV[4] == LOW && (irV[1] + irV[2] >1000)) {  
    turnLeft();                        //left on Cross
    path[pathLength] = 'L';
//    Serial.print(path[pathLength]);
    pathLength ++;
    if(path[pathLength-2]=='B') {
      correctPath();
      }  
    }
  else if ((dVal == 111) && (irV[4] == HIGH) && (irV[1] + irV[2] > 1500)) {  
    rotateRight180();                  // dead end
    path[pathLength] = 'B';
//  Serial.print(path[pathLength]);
    pathLength ++;
    if(path[pathLength-2]=='B') {
      correctPath();
      }
    }       
  else if ((dVal == 0)  && (irV[4] == LOW) && (irV[1] + irV[2] < 200)) {
  digitalWrite(2, LOW);               //Brake Left Motor
  digitalWrite(3, HIGH);  
  analogWrite(9, 255);                  
  digitalWrite(8, HIGH);              //Brake Right Motor 
  digitalWrite(7, LOW);   
  analogWrite(10, 255);
  delay(30);
    halt();                             // end of maze
    digitalWrite(led3Pin, LOW);
    printPath(); 
    turn = 1;
    waitForButton();
    delay(3000);
    }    
}     


/*
GIVE DIRECTIONS
##########################################################################################
##########################################################################################
*/    
void giveDirection() {
  delay(20);                      
  irV[0] =  digitalRead(irPin[0]);
  irV[1] =  analogRead(irPin[1]);
  irV[2] =  analogRead(irPin[2]);
  irV[3] =  digitalRead(irPin[3]);
  irV[4] =  digitalRead(irPin[4]);
  irV[5] =  digitalRead(irPin[5]);
      
  dVal = irV[0] * 100 + irV[3] * 10 + irV[5] * 1 ; 
  
//   Serial.println(dVal);
    
  if (dVal == 110) {                   // right turn
    digitalWrite(led1Pin, HIGH); 
    turnRight();
    digitalWrite(led1Pin, LOW); 
  }  
  else if (dVal == 101) {              // left turn
    digitalWrite(led2Pin, HIGH);
    turnLeft();
    digitalWrite(led2Pin, LOW); 
  }   
  else if (dVal == 100) {              // turn on T
    if (path[readLength]== 'L') {
//      Serial.print(path[readLength]);
      turnLeft();
    }
    if (path[readLength]== 'R') {
//      Serial.print(path[readLength]);
      turnRight();
    } 
    readLength++;
  }      
  else if (dVal == 10 && irV[4] == LOW) {               // right branch
    if (path[readLength]== 'S') {
//   Serial.print(path[readLength]);
      straight();
    }
    if (path[readLength]== 'R') {
//      Serial.print(path[readLength]);
      turnRight();
    } 
    readLength++;
  }      
  else if (dVal == 1 && irV[4] == LOW) {                // left branch
    if (path[readLength]== 'S') {
//      Serial.print(path[readLength]);
      //digitalWrite(led3Pin, HIGH);
      straight();
      //digitalWrite(led3Pin, LOW);
      }
    if (path[readLength]== 'L') {
//      Serial.print(path[readLength]);
      turnLeft();
      } 
    readLength++;
  } 
  else if ((dVal == 0) && (irV[4] == LOW) && (irV[1] + irV[2] > 1000)) {  // Cross 
    digitalWrite(led3Pin, HIGH);
    if (path[readLength]== 'L') {
//      Serial.print(path[readLength]);
      turnLeft();
      } 
    if (path[readLength]== 'R') {
//      Serial.print(path[readLength]);
      turnRight();
      } 
    if (path[readLength]== 'S') {
 //     Serial.print(path[readLength]);
 //     digitalWrite(led3Pin, HIGH);
      straight();
  //    digitalWrite(led3Pin, LOW);
      }
    readLength++;
    digitalWrite(led3Pin, LOW);           
    }
  else if ((dVal == 0)  && (irV[4] == LOW) && (irV[1] + irV[2] < 200)) {  // end of maze

  delay(30);
    halt();
    readLength = 0;
    //turn++;
    spMax = spMax+(spMax *.1);              // increase speed 10% each turn  
    waitForButton();                        //  100, 110, 121, 133, 146, 160 ect.
    delay(3000);
//    finished();
    }  
} 

/*
OPTIMIZE PATH
##########################################################################################
##########################################################################################
*/
void correctPath() {                   
  int correctionDone = 0;
  if(path[pathLength-3]=='L' && path[pathLength-1]=='R' && correctionDone==0) { //LBR
    pathLength-=3;
    path[pathLength]='B';
    correctionDone=1;
  }
  if(path[pathLength-3]=='L' && path[pathLength-1]=='S' && correctionDone==0) { //LBS
    pathLength-=3;
    path[pathLength]='R';
    correctionDone=1;
  }
  if(path[pathLength-3]=='R' && path[pathLength-1]=='L' && correctionDone==0) { //RBL
    pathLength-=3;
    path[pathLength]='B';
    correctionDone=1;
  }
  if(path[pathLength-3]=='S' && path[pathLength-1]=='L' && correctionDone==0) { //SBL
    pathLength-=3;
    path[pathLength]='R';
    correctionDone=1;
  }
  if(path[pathLength-3]=='S' && path[pathLength-1]=='S' && correctionDone==0) { //SBS
    pathLength-=3;
    path[pathLength]='B';
    correctionDone=1;
  }
  if(path[pathLength-3]=='L' && path[pathLength-1]=='L' && correctionDone==0) { //LBL
    pathLength-=3;
    path[pathLength]='S';
    correctionDone=1;
  }
  pathLength++;
}

/*
PRINT PATH
##########################################################################################
##########################################################################################
*/
void printPath() {
  Serial.print(" PathLength ");
  Serial.print(pathLength);  
  int i;
  Serial.println(" Path ");
  for (i = 0; i <= pathLength; i++) {  
    Serial.print(path[i]);
    }
}

/*
FINISHED
##########################################################################################
##########################################################################################
*/
void finished () {
  int i;
  i=1;
  while(i=1){
  digitalWrite(led1Pin, HIGH);   
  delay(200);                   
  digitalWrite(led1Pin, LOW);    
  delay(200);
  digitalWrite(led2Pin, HIGH);     
  delay(200);                   
  digitalWrite(led2Pin, LOW);    
  delay(200); 
  }  
}


