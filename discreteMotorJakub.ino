#include <Arduino.h>

// Pin Definitions
const int centreIRPin = 16;
const int sideIRPin   = 12;
const int trigPin     = 15;
const int echoPin     = 14;

// Left Motor (Motor A)
const int enA = 4;
const int in1 = 2;
const int in2 = 3;

// Right Motor (Motor B)
const int enB = 8;
const int in3 = 6;
const int in4 = 7;

// --- Tunable Speeds (0 - 255) ---
const int forwardSpeed   = 185; // Slightly lower than 225 so it doesn't overshoot curves
const int curveOuterSpd  = 180; // Left wheel speed when arcing smoothly around a right curve
const int pivotFastSpd   = 165; // Outer wheel speed during sweep recovery
const int pivotInsideSpd = 125; // Inner reverse wheel speed (slower so pivot stays on the line)

// --- Timing & Memory ---
unsigned long lastRightSeenTime = 0;
const unsigned long rightMemoryWindow = 400; // Remember right line for 400 ms
const unsigned long leftSearchMaxTime = 160; // Max ms to check left before sweeping right

unsigned long lastSonarTime = 0;
float distance = 999.0;

// Helper function to set both motors cleanly (-255 to +255)
void setMotors(int leftSpd, int rightSpd) {
  // Left Motor
  if (leftSpd >= 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(enA, leftSpd);
  } else {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(enA, -leftSpd);
  }

  // Right Motor
  if (rightSpd >= 0) {
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    analogWrite(enB, rightSpd);
  } else {
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    analogWrite(enB, -rightSpd);
  }
}

// Micro-pulse the motors so high static friction doesn't stall them at lower speeds
void updateUltrasonic() {
  if (millis() - lastSonarTime >= 100) { // Check sonar only 10x/sec so IR stays fast
    lastSonarTime = millis();
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // 15,000 us timeout (~2.5m) prevents loop freezes
    unsigned long dur = pulseIn(echoPin, HIGH, 15000);
    if (dur > 0) {
      distance = (dur * 0.0343f) / 2.0f;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(centreIRPin, INPUT);
  pinMode(sideIRPin, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  setMotors(0, 0);
}

void loop() {
  updateUltrasonic();

  int centerState = digitalRead(centreIRPin);
  int rightState  = digitalRead(sideIRPin);
  unsigned long now = millis();

  // Always latch the timestamp whenever the right sensor sees black
  if (rightState == HIGH) {
    lastRightSeenTime = now;
  }

  // CASE 1: Both Center and Right see black -> Curve is bending RIGHT right now!
  // Steer smoothly to the right immediately instead of driving straight past it.
  if (centerState == HIGH && rightState == HIGH) {
    Serial.println("ARCING RIGHT ON CURVE");
    setMotors(curveOuterSpd, 0); // Left wheel pushes forward, right wheel acts as pivot
    delay(8);
  }
  // CASE 2: Only Center sees black -> Straight ahead
  else if (centerState == HIGH && rightState == LOW) {
    Serial.println("FWD - ON LINE");
    setMotors(forwardSpeed, forwardSpeed);
    delay(8);
  }
  // CASE 3: Center lost the line, but Right sensor sees it NOW (or saw it within 400ms)
  else if (rightState == HIGH || (now - lastRightSeenTime <= rightMemoryWindow)) {
    Serial.println("SWEEPING RIGHT (FAST 1ms POLLING)");

    // Spin right continuously while polling sensors every 1 ms
    while (digitalRead(centreIRPin) == LOW) {
      // If right sensor is currently on the tape, we are close: do a smoother arc
      if (digitalRead(sideIRPin) == HIGH) {
        lastRightSeenTime = millis();
        setMotors(pivotFastSpd, -90);
      } else {
        // Right sensor passed it, pivot sharper until center catches up
        setMotors(pivotFastSpd, -pivotInsideSpd);
      }
      delay(1);
    }

    // Brief 15ms counter-brake to kill rotational momentum right on the line
    setMotors(-120, 120);
    delay(15);
    setMotors(0, 0);
    lastRightSeenTime = 0; // Clear memory once centered
  }
  // CASE 4: Neither sensor sees black -> Try a quick Left check first; if not found, sweep Right!
  else {
    Serial.println("SEARCHING LEFT FIRST");
    unsigned long searchStart = millis();
    bool foundLine = false;

    // 4A: Sweep left for up to 160ms checking every 1ms
    while (millis() - searchStart < leftSearchMaxTime) {
      if (digitalRead(centreIRPin) == HIGH) {
        foundLine = true;
        break;
      }
      if (digitalRead(sideIRPin) == HIGH) {
        // The line actually showed up on the right! Break out and let Case 3 handle it
        lastRightSeenTime = millis();
        foundLine = true;
        break;
      }
      setMotors(-pivotInsideSpd, pivotFastSpd);
      delay(1);
    }

    if (foundLine) {
      // Brief counter-brake to stop left spin right on the tape
      setMotors(120, -120);
      delay(12);
      setMotors(0, 0);
    } else {
      // 4B: Line wasn't on the left -> It slipped into the gap between Center and Right!
      Serial.println("GAP DETECTED -> SWEEPING RIGHT");
      while (digitalRead(centreIRPin) == LOW) {
        setMotors(pivotFastSpd, -pivotInsideSpd);
        delay(1);
      }
      setMotors(-120, 120);
      delay(15);
      setMotors(0, 0);
    }
  }
}