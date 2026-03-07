// 1. Define the possible states for your system
enum SystemState {
  CALIBRATION,
  DRIVE,
  BRAKE
};

// Define Motor States to track if they are running
enum MotorState {
  M_STOPPED,
  M_RUNNING
};

// Set the initial states
SystemState currentState = CALIBRATION;
MotorState currentMotorState = M_STOPPED; 

// 2. Define your pins
// IR Pins
const int ir_Sensor_Pin1 = A0; 
const int ir_Sensor_Pin2 = A1; 

// Motor Pins
const int motor_A_IN1 = 5; 
const int motor_A_IN2 = 6; 
const int motor_B_IN3 = 9; 
const int motor_B_IN4 = 10; 
const int motor_A_EN = 11; 
const int motor_B_EN = 12; 

// LED Pin
const int ledPin = 2;

int currentSpeed = 255;  

// Threshold Values (Removed 'const' so we can update them in calibration)
int ir_ice_detected = 0;
int ir_no_ice_detected = 0;
int calibrationStep = 0;

void setup() {
  Serial.begin(9600);
  
  // Initialize IR pins
  pinMode(ir_Sensor_Pin1, INPUT);
  pinMode(ir_Sensor_Pin2, INPUT);

  // Initialize Motor pins
  pinMode(motor_A_IN1, OUTPUT);
  pinMode(motor_A_IN2, OUTPUT);
  pinMode(motor_B_IN3, OUTPUT);
  pinMode(motor_B_IN4, OUTPUT);
  pinMode(motor_A_EN, OUTPUT);
  pinMode(motor_B_EN, OUTPUT);

  // Initialize Led pins
  pinMode(ledPin, OUTPUT);
  
  Serial.println("System Initialized. Starting CALIBRATION.");
  checkMotorStatus(); // Check initial status
}

void loop() {
  // 3. The State Machine
  switch (currentState) {
    case CALIBRATION:
      handleCalibration();
      break;

    case DRIVE:
      handleDrive();
      break;

    case BRAKE:
      handleBrake();
      break;
  }
}

// --- State Handler Functions ---

void handleCalibration() {
  if (Serial.available() > 0) {
    char incomingChar = Serial.read(); 

    if (incomingChar == '\n' || incomingChar == '\r') {
      
      // --- STEP 0: Take the ICE reading ---
      if (calibrationStep == 0) {
        // Updated to average Pin1 and Pin2 instead of Pin1 twice
        ir_ice_detected = (analogRead(ir_Sensor_Pin1) + analogRead(ir_Sensor_Pin2)) / 2;
          
        Serial.print("Ice reading taken: ");
        Serial.println(ir_ice_detected);
        Serial.println("Please remove the ice and press Enter again...");
        
        calibrationStep = 1; 
      }
        
      // --- STEP 1: Take the NO ICE reading ---
      else if (calibrationStep == 1) {
        ir_no_ice_detected = (analogRead(ir_Sensor_Pin1) + analogRead(ir_Sensor_Pin2)) / 2;
          
        Serial.print("No Ice reading taken: ");
        Serial.println(ir_no_ice_detected); 

        Serial.println("Calibration complete. Transitioning to DRIVE.");
        currentState = DRIVE;
      }
    }
  }
}

void handleDrive() {
  digitalWrite(ledPin, LOW);
  // 1. Output commands to drive motors forward
  // If motors aren't already running, start them
  if (currentMotorState != M_RUNNING) {
     setMotors(currentSpeed, 0, currentSpeed, 0); 
  }

  // 2. Read the IR sensor (Fixed missing semicolons here)
  int irValue1 = analogRead(ir_Sensor_Pin1); 
  int irValue2 = analogRead(ir_Sensor_Pin2);
  int irAverage = (irValue1 + irValue2) / 2;

  // 3. Check for trigger condition
  if (irAverage >= (ir_ice_detected - 50) && irAverage <= (ir_ice_detected + 50)) {
    Serial.println("Ice detected! Transitioning to BRAKE.");
    currentState = BRAKE;
  }
}

void handleBrake() {
  digitalWrite(ledPin, HIGH);
  // 1. Output commands to stop the motors
  if (currentMotorState != M_STOPPED) {
    stopMotors();
    delay(200); // Give physical motors time to halt
  }

  // 2. Read the IR sensor (Fixed missing semicolons here)
  int irValue1 = analogRead(ir_Sensor_Pin1); 
  int irValue2 = analogRead(ir_Sensor_Pin2);
  int irAverage = (irValue1 + irValue2) / 2;

  // 3. Check if the trigger condition has cleared
  if (irAverage >= (ir_no_ice_detected - 50) && irAverage <= (ir_no_ice_detected + 50)) {
    Serial.println("Path clear. Transitioning back to DRIVE.");
    currentState = DRIVE;
  }
}

// --- Motor Control & Status Functions ---

void setMotors(int a1, int a2, int b3, int b4) {
  // Note: Fixed variables to match your lowercase definitions
  digitalWrite(motor_A_EN, HIGH);
  digitalWrite(motor_B_EN, HIGH);
  analogWrite(motor_A_IN1, a1);
  analogWrite(motor_A_IN2, a2);
  analogWrite(motor_B_IN3, b3);
  analogWrite(motor_B_IN4, b4);
  
  // Update state and print
  currentMotorState = M_RUNNING;
  checkMotorStatus();
}

void stopMotors() {
  analogWrite(motor_A_IN1, 0);
  analogWrite(motor_A_IN2, 0);
  analogWrite(motor_B_IN3, 0);
  analogWrite(motor_B_IN4, 0);
  digitalWrite(motor_A_EN, LOW);
  digitalWrite(motor_B_EN, LOW);
  
  // Update state and print
  currentMotorState = M_STOPPED;
  checkMotorStatus();
}

// Custom function to print the motor status at any time
void checkMotorStatus() {
  Serial.print(">>> MOTOR STATUS: ");
  if (currentMotorState == M_RUNNING) {
    Serial.println("RUNNING");
  } else {
    Serial.println("STOPPED");
  }
}