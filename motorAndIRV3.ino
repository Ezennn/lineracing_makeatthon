#include <Arduino.h>

// VERSION STATUS
// centreIR and distance thingy working

// ==== TO DO ====
// need to fiddle with historySize array
// determine how many IR counts are taken per second.
// lots of testing

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

// --- Added for IR History ---
const int historySize = 100;          
int sideIRHistory[historySize] = {0}; // Array to hold the last 10 readings
int historyIndex = 0;                 // Tracks where to write the next reading
bool foundHigh = false;
float duration, distance;

void setup() {
  Serial.begin(115200);
  pinMode(centreIRPin, INPUT);
  pinMode(sideIRPin, INPUT);

  // Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set all motor control pins to outputs
  // Motor 1
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  
  // Motor 2
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void loop() {
  // Ultrasonic Test
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;
  // Serial.print("Distance: ");
  // Serial.println(distance);

  // log side IR
  sideIRHistory[historyIndex] = digitalRead(sideIRPin);
  historyIndex = (historyIndex + 1) % historySize; 

  // Check center sensor
  if (digitalRead(centreIRPin) == HIGH) {
    // Set left motor forward
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(enA, 215); 

    // Set right motor forward
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    analogWrite(enB, 215);
    Serial.println("FWD - LINE DETECTED");
  } 
  
  else if (digitalRead(centreIRPin) == LOW) {
    // slow it down
    foundHigh = false;
    analogWrite(enA, 0); 
    analogWrite(enB, 0); 
    // Serial.println("UNDETECTED");
    
    // search recent side IR values for HIGH
    for (int i = 0; i < historySize; i++) {
      if (sideIRHistory[i] == HIGH) {
        foundHigh = true;
        break;
      }
    }

    if (foundHigh) {
      // found recent HIGH values - turn in that direction
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);

      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      Serial.println("RIGHT");
      
    } else {
      // no recent HIGH values - turn opposite direction
      digitalWrite(in1, HIGH); 
      digitalWrite(in2, LOW); 

      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      Serial.println("LEFT");
    }
  }
  delay(20); 
}