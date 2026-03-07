// 1. Define the possible states for your system
enum SystemState {
  CALIBRATION,
  DRIVE,
  BRAKE
};

// Set the initial state
SystemState currentState = CALIBRATION;

// 2. Define your pins
// IR Pins
const int ir_Sensor_Pin1 = A0; 
const int ir_Sensor_Pin2 = A1; 

// Motor Pins
const int motor_A_IN1 = 8; 
const int motor_A_IN2 = 7; 
const int motor_B_IN3 = 4; 
const int motor_B_IN4 = 5; 
const int motor_A_EN = 9; 
const int motor_B_EN = 3; 

// LED Pin
const int LED_Pin = 2;

int currentSpeed = 255;  

// Threshold Values
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

  // Initialize LED pin
  pinMode(LED_Pin, OUTPUT);
  
  Serial.println("System Initialized. Starting CALIBRATION.");
}

void loop() {
  // 3. The State Machine: Directs the code to the current active state
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
  // 1. Check if there is data coming in from the Serial Monitor
  if (Serial.available() > 0) {
    
    // Read the incoming keystroke
    char incomingChar = Serial.read(); 

    // 2. Check if the key pressed was 'Enter' (\n or \r)
    if (incomingChar == '\n' || incomingChar == '\r') {
      
      // --- STEP 0: Take the ICE reading ---
      if (calibrationStep == 0) {
        Serial.println("Press Enter to calibrate IR for ice.");

        ir_ice_detected = (analogRead(ir_Sensor_Pin1) + analogRead(ir_Sensor_Pin1)) / 2;
          
        Serial.print("Ice reading taken: ");
        Serial.println(ir_ice_detected);
        Serial.println("Please remove the ice and press Enter again...");
        
        calibrationStep = 1; // Advance to the next step
      }
        
        // --- STEP 1: Take the NO ICE reading ---
      else if (calibrationStep == 1) {
        Serial.println("Press Enter to calibrate IR for no ice.");

        ir_no_ice_detected = (analogRead(ir_Sensor_Pin1) + analogRead(ir_Sensor_Pin1)) / 2;
          
        Serial.print("No Ice reading taken: ");
        Serial.println(ir_no_ice_detected); 

          // 3. Both readings are done! Transition to the next state
        Serial.println("Calibration complete. Transitioning to DRIVE.");
        currentState = DRIVE;
      }
    }
  }
}

void handleDrive() {
  // 1. Output commands to drive motors forward
  // ... your motor driving code here ...
  digitalWrite(LED_Pin, LOW);

  // 2. Read the IR sensor
  int irValue1 = analogRead(ir_Sensor_Pin1); // Use analogRead() if it's an analog sensor
  int irValue2 = analogRead(ir_Sensor_Pin2);

  int irAverage = (irValue1 + irValue2)/2;

  // 3. Check for trigger condition (e.g., obstacle detected or line lost)
  // Assuming HIGH means a trigger condition was met
  if (irAverage >= (ir_ice_detected - 50) && irAverage <= (ir_ice_detected + 50)) {
    Serial.println("Trigger detected. Transitioning to BRAKE.");
    currentState = BRAKE;
  }

  // Read the incoming keystroke
  char incomingChar = Serial.read(); 

  // Check if the key pressed was 'Enter' (\n or \r)
  if (incomingChar == '\n' || incomingChar == '\r') {
    // Reset the program
    Serial.println("Resetting program. Returning to Calibration.");
    calibrationStep = 0;
    currentState = CALIBRATION;
  }
}

void handleBrake() {

  digitalWrite(LED_Pin, HIGH);
  // 1. Output commands to stop the motors
  setMotors(currentSpeed, 0, currentSpeed, 0);
  delay(100);
  stopMotors();
  delay(100);

  // 2. Read the IR sensor to check if it's safe to move again
  int irValue1 = analogRead(ir_Sensor_Pin1);
  int irValue2 = analogRead(ir_Sensor_Pin2);

  int irAverage = (irValue1 + irValue2)/2;

  // 3. Check if the trigger condition has cleared
  // Assuming LOW means the path is clear
  if (irAverage >= (ir_no_ice_detected - 50) && irAverage <= (ir_no_ice_detected + 50)) {
    Serial.println("Path clear. Transitioning back to DRIVE.");
    currentState = DRIVE;
  }
}

void setMotors(int a1, int a2, int b3, int b4) {
  digitalWrite(motor_A_EN, HIGH);
  digitalWrite(motor_B_EN, HIGH);
  analogWrite(motor_A_IN1, a1);
  analogWrite(motor_A_IN2, a2);
  analogWrite(motor_B_IN3, b3);
  analogWrite(motor_B_IN4, b4);
}

void stopMotors() {
  analogWrite(motor_A_IN1, 0);
  analogWrite(motor_A_IN2, 0);
  analogWrite(motor_B_IN3, 0);
  analogWrite(motor_B_IN4, 0);
  digitalWrite(motor_A_EN, LOW);
  digitalWrite(motor_B_EN, LOW);
}