#include <Arduino.h>

// Define pins
const int centreIRPin = 16; 
const int sideIRPin = 12;   
const int trigPin = 15; 
const int echoPin = 14;

// Motor Driver Pins
// Left Motor (Motor A)
const int enA = 4;  // Speed control (PWM)
const int in1 = 2;  // Direction Pin 1
const int in2 = 3;  // Direction Pin 2

// Right Motor (Motor B)
const int enB = 8;  // Speed control (PWM)
const int in3 = 6;  // Direction Pin 1
const int in4 = 7;  // Direction Pin 2

// --- Memory Counter for Right Line Following ---
int rightMemorySteps = 0; 

float duration, distance;

// --- Movement Settings ---
const int forwardSpeed = 210; // Straight forward PWM (Original: 225)
const int turnSpeed = 180;    // Sweep turn PWM (Original: 190)

// Forward movement step-and-pause settings
const int forwardStepTime = 45;       // Normal straight step duration (ms)
const int intersectionCheckTime = 60; // How long to go straight forward when right sensor triggers to check if forward path exists
const int forwardPauseTime = 15;      // Pause time with motors OFF between steps (ms)

// Turn sweep settings (incremental stepping)
const int turnStepTime = 25;  // Duration of each sweep increment (ms)
const int turnPauseTime = 15; // Pause between sweep increments (ms)
const int turnOvershoot = 15; // Extra centering time after finding the line (ms)

// Helper to run a single turn increment while watching the center sensor
void runTurnStep(int stepMs) {
  analogWrite(enA, turnSpeed); 
  analogWrite(enB, turnSpeed);
  for (int i = 0; i < stepMs; i++) {
    if (digitalRead(centreIRPin) == HIGH) {
      break; // Stop the step early the exact millisecond the line is found
    }
    delay(1);
  }
  analogWrite(enA, 0); 
  analogWrite(enB, 0);
  delay(turnPauseTime); // Pause cleanly with motors OFF
}

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

  duration = pulseIn(echoPin, HIGH, 25000);
  distance = (duration * 0.0343) / 2;

  int centerState = digitalRead(centreIRPin);
  int rightState = digitalRead(sideIRPin);

  // 2. INTERSECTION / RIGHT-TURN VERIFICATION
  // When the right sensor outputs HIGH, go a little forward first and re-check if a forward path exists
  if (rightState == HIGH) {
    Serial.println("RIGHT DETECTED -> STEPPING FORWARD TO CHECK IF STRAIGHT PATH EXISTS");
    rightMemorySteps = 3; // Remember the right line in case the straight path ends right after this step

    // Drive straight forward a little bit
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    analogWrite(enA, forwardSpeed); 
    analogWrite(enB, forwardSpeed);
    delay(intersectionCheckTime);

    // Stop motors cleanly before re-reading the front sensor
    analogWrite(enA, 0); 
    analogWrite(enB, 0);
    delay(forwardPauseTime);

    // Re-read sensors at the new position slightly further forward
    centerState = digitalRead(centreIRPin);
    rightState = digitalRead(sideIRPin);

    // If a path forward STILL exists and we cleared the right branch, cancel the right turn
    if (centerState == HIGH && rightState == LOW) {
      Serial.println("STRAIGHT PATH EXISTS PAST INTERSECTION -> CONTINUING FORWARD");
      rightMemorySteps = 0;
    }
  }

  // 3. DETERMINE NEXT MOVEMENT
  if (centerState == HIGH) {
    // === PATH DIRECTLY FORWARD EXISTS === 
    Serial.println("FWD - LINE DETECTED");
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    
    analogWrite(enA, forwardSpeed); 
    analogWrite(enB, forwardSpeed);

    if (rightState == LOW && rightMemorySteps > 0) {
      rightMemorySteps--; // Count down memory while moving straight
    }
    
    // Execute pulsed forward step, then turn motors OFF
    delay(forwardStepTime); 
    analogWrite(enA, 0); 
    analogWrite(enB, 0); 
    delay(forwardPauseTime); 
    
  } else {
    // === NO PATH DIRECTLY FORWARD (LOST THE LINE) ===
    
    if (rightMemorySteps > 0 || rightState == HIGH) {
      // TURN RIGHT IN INCREMENTS UNTIL LINE IS FOUND
      Serial.println("NO FORWARD PATH -> SWEEPING RIGHT TO RECOVER LINE");
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      
      // Incremental step-and-pause loop
      while (digitalRead(centreIRPin) == LOW) {
        runTurnStep(turnStepTime);
      }
      
      // Small centering pulse
      analogWrite(enA, turnSpeed); 
      analogWrite(enB, turnSpeed);
      delay(turnOvershoot); 
      
      // Stop and reset the memory counter
      analogWrite(enA, 0); 
      analogWrite(enB, 0); 
      rightMemorySteps = 0; 
      
    } else {
      // TURN LEFT IN INCREMENTS UNTIL LINE IS FOUND
      Serial.println("SWEEPING LEFT TO RECOVER LINE");
      digitalWrite(in1, HIGH); 
      digitalWrite(in2, LOW); 
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      
      // Incremental step-and-pause loop (max 12 steps left; if not found, switch to right)
      int leftSteps = 0;
      while (digitalRead(centreIRPin) == LOW && leftSteps < 12) {
        if (digitalRead(sideIRPin) == HIGH) {
          rightMemorySteps = 3; // Spotted the line on the right!
          break;
        }
        runTurnStep(turnStepTime);
        leftSteps++;
      }

      // If 12 left steps didn't find the line, it slipped between the two sensors -> sweep right next
      if (digitalRead(centreIRPin) == LOW) {
        rightMemorySteps = 3;
      } else {
        // Comfortably center it on left recovery
        analogWrite(enA, turnSpeed); 
        analogWrite(enB, turnSpeed);
        delay(turnOvershoot); 
      }
      
      // Stop
      analogWrite(enA, 0); 
      analogWrite(enB, 0); 
    }
  }
}
