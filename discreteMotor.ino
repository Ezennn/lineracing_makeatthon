#include <Arduino.h>

// Pins
const int centreIRPin = 16;   // IR_front
const int sideIRPin   = 12;   // IR_side (right)

const int enA = 4, in1 = 2, in2 = 3;   // left motor
const int enB = 8, in3 = 6, in4 = 7;   // right motor

// "n" = one small increment
const int speedPWM = 225;
const int stepTime = 40;    // ms motors run per increment
const int pauseTime = 10;   // ms stopped between increments

// dir: +1 = forward, -1 = backward
void setWheel(int en, int a, int b, int dir) {
  digitalWrite(a, dir > 0 ? LOW : HIGH);
  digitalWrite(b, dir > 0 ? HIGH : LOW);
  analogWrite(en, speedPWM);
}

void stopMotors() {
  analogWrite(enA, 0);
  analogWrite(enB, 0);
}

// one increment: wheelL += n (dirL), wheelR += n (dirR)
void step(int dirL, int dirR) {
  setWheel(enA, in1, in2, dirL);
  setWheel(enB, in3, in4, dirR);
  delay(stepTime);
  stopMotors();
  delay(pauseTime);   // "delay()" in your pseudocode
}

void setup() {
  Serial.begin(115200);
  pinMode(centreIRPin, INPUT);
  pinMode(sideIRPin, INPUT);
  pinMode(enA, OUTPUT); pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT); pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  analogWriteFreq(20000);   // Pico: quieter, smaller deadband
  analogWriteRange(255);
  stopMotors();
}

void loop() {
  int irFront = digitalRead(centreIRPin);
  int irSide  = digitalRead(sideIRPin);

  if (irFront == 1) {                    // go straight
    step(+1, +1);
  }

  if (irSide == 1) {                     // go right
    step(+1, -1);
  }

  if (irFront == 0 && irSide == 0) {     // go left
    step(-1, +1);
  }
}
