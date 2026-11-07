#include <Arduino.h>

// wheel diameter: 3.3
// define pins
const int centreIRPin = 0;
const int sideIRPin = 1;
const int leftUltraPin = 2;
const int rightUltraPin = 3;

// Motor Driver Pins
// Left Motor (Motor A)
const int enA = 5;  // Speed control (Needs PWM ~ pin)
const int in1 = 6;  // Direction Pin 1
const int in2 = 7;  // Direction Pin 2

// Right Motor (Motor B)
const int enB = 10; // Speed control (Needs PWM ~ pin)
const int in3 = 8;  // Direction Pin 1
const int in4 = 9;  // Direction Pin 2

// --- Added for IR History ---
const int IRHistory = 10;
int sideIRHistory[historySize] = {0}; // Array to hold the last 10 readings
int historyIndex = 0;                 // Tracks where to write the next reading
bool foundHigh = false;

void setup() {
  Serial.begin(115200);
  pinMode(centreIRPin, INPUT);
  pinMode(sideIRPin, INPUT);

  // Set all motor control pins to outputs
  // Motor 1
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  
  // Motor 2
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  // Optional: Ensure motors are completely off at startup
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void loop() {
  // log side IR
  sideIRHistory[historyIndex] = digitalRead(sideIRPin);
  historyIndex = (historyIndex + 1) % historySize; 

  // Check center sensor
  if (digitalRead(centreIRPin) == HIGH) {
    // Set left motor forward
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(enA, 255); // Speed: 0 (stop) to 255 (full speed)

    // Set right motor forward
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    analogWrite(enB, 255);
  } 
  
  else if (digitalRead(centreIRPin) == LOW) {
    foundHigh = false;
    analogWrite(enA, 25);
    analogWrite(enB, 25)
    
    // Search the array of recent side IR sensor values for a HIGH value
    for (int i = 0; i < historySize; i++) {
      if (sideIRHistory[i] == HIGH) {
        foundHigh = true;
        break;
      }
    }

    if (foundHigh) {
      // found recent HIGH values - turn in that direction
      
    } else {
      // no recent HIGH values - turn opposite direction
      
    }
  }
  
  // Small delay to prevent the buffer from filling instantly in a single millisecond
  delay(100); 
}