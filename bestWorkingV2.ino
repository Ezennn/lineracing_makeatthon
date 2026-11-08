#include <Arduino.h>

// VERSION STATUS
// centreIR and distance thingy working
// Quick-left check before right sweep (halved check time), extended overshoot for long right turns

// wheel diameter: 3.3
// define pins
const int centreIRPin = 16; 
const int sideIRPin = 12;   
const int trigPin = 15; 
const int echoPin = 14;

// Motor Driver Pins
// Left Motor (Motor A)
const int enA = 4;  // Speed control (Needs PWM ~ pin)
const int in1 = 2;  // Direction Pin 1
const int in2 = 3;  // Direction Pin 2

// Right Motor (Motor B)
const int enB = 8; // Speed control (Needs PWM ~ pin)
const int in3 = 6;  // Direction Pin 1
const int in4 = 7;  // Direction Pin 2

// --- Memory Flag for Line Following ---
bool sawLineOnRight = false; // Remembers if the line was last seen on the right

float duration, distance;

// --- Movement Settings ---
const int motorSpeed = 225; 

// Forward movement settings
const int forwardStepTime = 75;  
const int forwardPauseTime = 3;  

// Turn sweep settings 
const int turnStepTime = 40;  
const int turnPauseTime = 20; 
const int turnOvershoot = 30; // Standard overshoot to comfortably center

// Advanced Turn Settings
const int leftCheckTime = 40;        // HALVED: Quick left twitch before a right turn
const int longTurnThreshold = 500;   // Milliseconds before a turn is considered "long"
const int longTurnOvershoot = 200;   // Extended overshoot if the turn was long

void setup() {
  Serial.begin(115200);
  pinMode(centreIRPin, INPUT);
  pinMode(sideIRPin, INPUT);

  // Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set all motor control pins to outputs
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

  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;

  int centerState = digitalRead(centreIRPin);
  int rightState = digitalRead(sideIRPin);

  // 2. DETERMINE NEXT MOVEMENT
  
  if (centerState == HIGH) {
    // === PATH DIRECTLY FORWARD EXISTS === 
    
    if (rightState == HIGH) {
      sawLineOnRight = true; // Remember fake/real side paths just in case
    } else {
      sawLineOnRight = false; // Passed the junction, forget it!
    }
    
    // Drive forward
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    
    analogWrite(enA, motorSpeed); 
    analogWrite(enB, motorSpeed);
    Serial.println("FWD - LINE DETECTED");
    
    // Execute smooth forward step
    delay(forwardStepTime); 
    analogWrite(enA, 0); 
    analogWrite(enB, 0); 
    delay(forwardPauseTime); 
    
  } else {
    // === NO PATH DIRECTLY FORWARD (LOST THE LINE) ===
    
    if (sawLineOnRight || rightState == HIGH) {
      
      // -- NEW: QUICK LEFT CHECK --
      Serial.println("QUICK LEFT CHECK...");
      digitalWrite(in1, HIGH); 
      digitalWrite(in2, LOW); 
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      
      analogWrite(enA, motorSpeed); 
      analogWrite(enB, motorSpeed);
      delay(leftCheckTime);
      
      analogWrite(enA, 0); 
      analogWrite(enB, 0);
      delay(turnPauseTime);
      
      // If we found the line during the left check, skip the right sweep
      if (digitalRead(centreIRPin) == HIGH) {
        Serial.println("LINE FOUND ON LEFT CHECK");
        sawLineOnRight = false;
      } 
      else {
        // -- PROCEED TO RIGHT SWEEP --
        Serial.println("SWEEPING RIGHT TO RECOVER LINE");
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        digitalWrite(in3, HIGH);
        digitalWrite(in4, LOW);
        
        // Start a timer to see how long this turn takes
        unsigned long turnStartTime = millis();
        
        // Trap the robot in this loop until the sensor sees black again
        while (digitalRead(centreIRPin) == LOW) {
          analogWrite(enA, motorSpeed); 
          analogWrite(enB, motorSpeed);
          delay(turnStepTime); 
          
          analogWrite(enA, 0); 
          analogWrite(enB, 0);
          delay(turnPauseTime); 
        }
        
        // Calculate how long it took to find the line
        unsigned long turnDuration = millis() - turnStartTime;
        
        // Execute overshoot based on how long the turn was
        analogWrite(enA, motorSpeed); 
        analogWrite(enB, motorSpeed);
        
        if (turnDuration > longTurnThreshold) {
           delay(longTurnOvershoot); // 200ms extra push
           Serial.println("LONG TURN OVERSHOOT APPLIED");
        } else {
           delay(turnOvershoot); // Standard 30ms push
        }
        
        // Stop and reset the memory flag
        analogWrite(enA, 0); 
        analogWrite(enB, 0); 
        sawLineOnRight = false; 
      }
      
    } else {
      // TURN LEFT IN INCREMENTS UNTIL LINE IS FOUND
      Serial.println("SWEEPING LEFT TO RECOVER LINE");
      digitalWrite(in1, HIGH); 
      digitalWrite(in2, LOW); 
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      
      // Trap the robot in this loop until the sensor sees black again
      while (digitalRead(centreIRPin) == LOW) {
        analogWrite(enA, motorSpeed); 
        analogWrite(enB, motorSpeed);
        delay(turnStepTime); 
        
        analogWrite(enA, 0); 
        analogWrite(enB, 0);
        delay(turnPauseTime); 
      }
      
      // Comfortably center it
      analogWrite(enA, motorSpeed); 
      analogWrite(enB, motorSpeed);
      delay(turnOvershoot); 
      
      // Stop
      analogWrite(enA, 0); 
      analogWrite(enB, 0); 
    }
  }
}