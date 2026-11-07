#include <Arduino.h>

// VERSION STATUS
// Two parallel front IR sensors: left-of-centre and right-of-centre
// Same pins as before, only their position on the bot changed

// define pins
const int leftIRPin  = 16;   // front-LEFT sensor  (was "centre")
const int rightIRPin = 12;   // front-RIGHT sensor (was "side")
const int trigPin = 15; 
const int echoPin = 14;

// Left Motor (Motor A)
const int enA = 4;
const int in1 = 2;
const int in2 = 3;

// Right Motor (Motor B)
const int enB = 8;
const int in3 = 6;
const int in4 = 7;

// --- Memory: which side the line was last seen on ---
int lastSide = 0;                 // -1 = left, +1 = right, 0 = unknown
unsigned long lastSideTime = 0;
const unsigned long sideMemoryMs = 500;

float duration, distance;

// --- Movement Settings (unchanged) ---
const int motorSpeed = 225; 
const int forwardStepTime = 75;  
const int forwardPauseTime = 30;  // clear stop between forward steps
const int turnStepTime = 40;  
const int turnPauseTime = 25;     // clear stop between turn steps
const int turnOvershoot = 30;
const int longTurnThreshold = 500;
const int longTurnOvershoot = 200;

// --- Helpers ---
void setForward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void setPivot(bool right) {
  digitalWrite(in1, right ? LOW : HIGH);
  digitalWrite(in2, right ? HIGH : LOW);
  digitalWrite(in3, right ? HIGH : LOW);
  digitalWrite(in4, right ? LOW : HIGH);
}

void pulse(int onMs, int offMs) {
  analogWrite(enA, motorSpeed); 
  analogWrite(enB, motorSpeed);
  delay(onMs);
  analogWrite(enA, 0); 
  analogWrite(enB, 0);
  delay(offMs);
}

void setup() {
  Serial.begin(115200);
  pinMode(leftIRPin, INPUT);
  pinMode(rightIRPin, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void loop() {
  // 1. READ SENSORS
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 5000);   // timeout so steps stay evenly timed
  distance = (duration*.0343)/2;

  int L = digitalRead(leftIRPin);
  int R = digitalRead(rightIRPin);

  // 2. DECIDE
  if (L == HIGH && R == HIGH) {
    // line under both: straight
    Serial.println("FWD");
    setForward();
    pulse(forwardStepTime, forwardPauseTime);

  } else if (R == HIGH) {
    // line drifting right: small right correction
    Serial.println("NUDGE RIGHT");
    lastSide = +1; lastSideTime = millis();
    setPivot(true);
    pulse(turnStepTime, turnPauseTime);

  } else if (L == HIGH) {
    // line drifting left: small left correction
    Serial.println("NUDGE LEFT");
    lastSide = -1; lastSideTime = millis();
    setPivot(false);
    pulse(turnStepTime, turnPauseTime);

  } else {
    // both LOW: lost. Sweep toward last side seen (default left)
    bool goRight = (lastSide == +1) && (millis() - lastSideTime < sideMemoryMs);
    Serial.println(goRight ? "LOST - SWEEP RIGHT" : "LOST - SWEEP LEFT");
    setPivot(goRight);

    unsigned long start = millis();
    while (digitalRead(leftIRPin) == LOW && digitalRead(rightIRPin) == LOW) {
      pulse(turnStepTime, turnPauseTime);
    }
    unsigned long dur = millis() - start;

    // overshoot so the line ends up under both sensors
    analogWrite(enA, motorSpeed); 
    analogWrite(enB, motorSpeed);
    delay(dur > longTurnThreshold ? longTurnOvershoot : turnOvershoot);
    analogWrite(enA, 0); 
    analogWrite(enB, 0);

    lastSide = 0;
  }
}
