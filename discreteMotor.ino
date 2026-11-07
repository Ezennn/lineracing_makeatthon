#include <Arduino.h>
 
// ---------- PINS ----------
const int centreIRPin = 16;
const int sideIRPin   = 12;   // right side sensor
const int trigPin = 15, echoPin = 14;
const int enA = 4, in1 = 2, in2 = 3;   // left motor
const int enB = 8, in3 = 6, in4 = 7;   // right motor
 
// ---------- TUNE THESE ----------
const int baseSpeed   = 225;   // straights
const int cruiseSpeed = 175;   // after a loss / near corners (less overshoot)
const int arcOuter    = 200;
const int arcInner    = 0;     // lower = tighter
const int pivotSpeed  = 190;
 
const unsigned long BRAKE_MS  = 25;    // active brake when line lost / found
const unsigned long SLOW_MS   = 400;   // stay at cruiseSpeed this long after recovering
const unsigned long ARC_MS    = 100;   // soft arc (skipped if side sensor fired)
const unsigned long PIVOT_MS  = 700;   // pivot same way until here
const unsigned long GIVEUP_MS = 4000;  // then pivot other way (dead-end U-turn)
const long SIDE_MEMORY_MS = 400;
 
const float WALL_CM = 7;
const bool  WALL_TURN_RIGHT = true;
const unsigned long WALL_MIN_TURN_MS = 250;
 
// ---------- STATE ----------
unsigned long lostSince = 0;
unsigned long slowUntil = 0;
unsigned long wallTurnUntil = 0;
unsigned long lastPing = 0;
long lastSideSeen = -100000;
bool preferRight = false;
bool sharp = false;
float dist = 999;
 
// ---------- MOTORS ----------
void setMotor(int en, int a, int b, int speed) {
  digitalWrite(a, speed >= 0 ? LOW : HIGH);
  digitalWrite(b, speed >= 0 ? HIGH : LOW);
  analogWrite(en, abs(speed));
}
void drive(int l, int r) {
  setMotor(enA, in1, in2, l);
  setMotor(enB, in3, in4, r);
}
void brake() {   // both inputs HIGH + enable on = active brake (L298N)
  digitalWrite(in1, HIGH); digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, HIGH);
  analogWrite(enA, 255);
  analogWrite(enB, 255);
}
void arc(bool right)   { right ? drive(arcOuter, arcInner) : drive(arcInner, arcOuter); }
void pivot(bool right) { right ? drive(pivotSpeed, -pivotSpeed) : drive(-pivotSpeed, pivotSpeed); }
 
float readCm() {
  digitalWrite(trigPin, LOW);  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long d = pulseIn(echoPin, HIGH, 5000);
  return d ? d * 0.0343f / 2 : 999;
}
 
void setup() {
  Serial.begin(115200);
  pinMode(centreIRPin, INPUT);
  pinMode(sideIRPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(enA, OUTPUT); pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT); pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  analogWriteFreq(20000);
  analogWriteRange(255);
  drive(0, 0);
}
 
void loop() {
  unsigned long now = millis();
  bool c = digitalRead(centreIRPin) == HIGH;
  bool s = digitalRead(sideIRPin) == HIGH;
  if (s) {
    lastSideSeen = now;
    slowUntil = now + 150;      // side fired = corner/junction likely, slow down
  }
 
  if (now - lastPing >= 60) { lastPing = now; dist = readCm(); }
 
  // --- WALL ---
  if (now < wallTurnUntil) { pivot(WALL_TURN_RIGHT); return; }
  if (c && dist < WALL_CM) {
    brake(); delay(BRAKE_MS);
    wallTurnUntil = millis() + WALL_MIN_TURN_MS;
    lastSideSeen = WALL_TURN_RIGHT ? (long)millis() : -100000;
    lostSince = 0;
    return;
  }
 
  // --- ON LINE ---
  if (c) {
    if (lostSince) {                         // just found it again
      brake(); delay(BRAKE_MS);              // kill spin momentum
      if (now - lostSince > PIVOT_MS) lastSideSeen = -100000;   // dead-end U-turn: forget right
      slowUntil = millis() + SLOW_MS;
      lostSince = 0;
    }
    int sp = (millis() < slowUntil) ? cruiseSpeed : baseSpeed;
    drive(sp, sp);
    return;
  }
 
  // --- LOST ---
  if (!lostSince) {
    lostSince = now;
    preferRight = s || ((long)now - lastSideSeen < SIDE_MEMORY_MS);
    sharp = s;
  }
  unsigned long lost = now - lostSince;
 
  if (lost < BRAKE_MS) { brake(); return; }  // stop forward momentum first
  unsigned long t = lost - BRAKE_MS;
 
  if (!sharp && t < ARC_MS)   arc(preferRight);
  else if (t < PIVOT_MS)      pivot(preferRight);
  else if (t < GIVEUP_MS)     pivot(!preferRight);
  else                        drive(0, 0);
}
