#include <Arduino.h>



// =====================================================================

//  LINE FOLLOWER - rewritten to follow the paper logic

//

//  read(IRfront); read(IRside);

//

//  1. if (IRside == 1)                    -> while (IRfront != 1):

//                                              wheel_L += u; wheel_R -= u;

//                                              (pivot RIGHT in steps until

//                                               the front sensor finds the line)

//  2. if (IRfront == 1)                   -> wheel_L += u; wheel_R += u;

//                                              (one step FORWARD)

//  3. if (IRfront == 0 && IRside == 0)    -> wheel_L -= u; wheel_R += u;

//                                              (one step pivot LEFT)

//

//  "u" = one incremental move: drive the wheel for a short time, then stop.

//  +u = wheel moves forward, -u = wheel moves backward.

// =====================================================================



// ---------------- Pins ----------------

const int frontIRPin = 16;   // centre / front IR sensor

const int sideIRPin  = 12;   // right-side IR sensor



// Left motor (Motor A)

const int enA = 4;   // PWM speed

const int in1 = 2;

const int in2 = 3;



// Right motor (Motor B)

const int enB = 8;   // PWM speed

const int in3 = 6;

const int in4 = 7;



// ---------------- Sensor polarity ----------------

// Your sensors read HIGH when they are over the black line.

// If yours are the other way round, change this to LOW.

const int ON_LINE = HIGH;



// ---------------- Tuning ("u") ----------------

const int forwardSpeed = 225;  // PWM for a forward step

const int turnSpeed    = 190;  // PWM for a turning step



const int forwardStepTime = 50; // ms the motors run for one forward "u"

const int turnStepTime    = 40; // ms the motors run for one turning "u"

const int pauseTime       = 5;  // ms stopped after each "u" (the delay())



// ---------------- Wheel directions ----------------

enum WheelDir { FWD, REV };   // FWD = +u, REV = -u



// ---------------- Sensor helpers ----------------

bool frontOnLine() { return digitalRead(frontIRPin) == ON_LINE; }

bool sideOnLine()  { return digitalRead(sideIRPin)  == ON_LINE; }



// ---------------- Motor helpers ----------------

void setLeftWheel(WheelDir d) {

  // Forward for your wiring: in1 LOW, in2 HIGH

  digitalWrite(in1, d == FWD ? LOW  : HIGH);

  digitalWrite(in2, d == FWD ? HIGH : LOW);

}



void setRightWheel(WheelDir d) {

  // Forward for your wiring: in3 LOW, in4 HIGH

  digitalWrite(in3, d == FWD ? LOW  : HIGH);

  digitalWrite(in4, d == FWD ? HIGH : LOW);

}



void stopMotors() {

  analogWrite(enA, 0);

  analogWrite(enB, 0);

}



// One incremental move "u": set directions, run briefly, stop, short pause.

void stepU(WheelDir left, WheelDir right, int speed, int runTime) {

  setLeftWheel(left);

  setRightWheel(right);

  analogWrite(enA, speed);

  analogWrite(enB, speed);

  delay(runTime);

  stopMotors();

  delay(pauseTime);

}



// ---------------- Movements from the paper ----------------

// wheel_L += u; wheel_R += u;

void stepForward()   { stepU(FWD, FWD, forwardSpeed, forwardStepTime); }



// wheel_L += u; wheel_R -= u;

void stepTurnRight() { stepU(FWD, REV, turnSpeed, turnStepTime); }



// wheel_L -= u; wheel_R += u;

void stepTurnLeft()  { stepU(REV, FWD, turnSpeed, turnStepTime); }



// =====================================================================

void setup() {

  Serial.begin(115200);



  pinMode(frontIRPin, INPUT);

  pinMode(sideIRPin, INPUT);



  pinMode(enA, OUTPUT);

  pinMode(in1, OUTPUT);

  pinMode(in2, OUTPUT);

  pinMode(enB, OUTPUT);

  pinMode(in3, OUTPUT);

  pinMode(in4, OUTPUT);



  stopMotors();

  digitalWrite(in1, LOW);

  digitalWrite(in2, LOW);

  digitalWrite(in3, LOW);

  digitalWrite(in4, LOW);

}



void loop() {

  // read(IRfront); read(IRside);

  bool front = frontOnLine();

  bool side  = sideOnLine();



  // ---- Rule 1: side sensor sees the line -> pivot right until front finds it

  if (side) {

    if (!front) Serial.println("RIGHT: turning until front sees line");

    while (!frontOnLine()) {       // re-read the sensor every step

      stepTurnRight();

    }

    front = true;                  // loop only exits when front is on the line

  }



  // ---- Rule 2: front sensor sees the line -> one step forward

  if (front) {

    Serial.println("FWD");

    stepForward();

  }

  // ---- Rule 3: neither sensor sees the line -> one step left

  else if (!side) {

    Serial.println("LEFT");

    stepTurnLeft();

  }

}
