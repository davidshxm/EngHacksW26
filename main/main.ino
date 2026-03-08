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
const int motor_B_IN3 = 5; 
const int motor_B_IN4 = 6; 
const int motor_A_EN = 9; 
const int motor_B_EN = 4; 

// LED Pin
const int LED_Pin = 2;

// BUtton Pin
const int button_Pin = 3;

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
  
  // Initialize Button Pin
  pinMode(button_Pin, INPUT);

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
  int currentAvg = 0;

  // --- STEP 1: ICE READING ---
  Serial.println("Setting ICE Live Value. Press button to lock in.");
  
  // The loop actively checks the physical pin every time it repeats
  while (digitalRead(button_Pin) != LOW) {
    currentAvg = (analogRead(ir_Sensor_Pin1) + analogRead(ir_Sensor_Pin2)) / 2;
  }
  
  ir_ice_detected = currentAvg; // Save the reading
  calibrationStep = 1;
  Serial.print("ICE value locked! Value is: ");
  Serial.println(ir_ice_detected);

  // --- CRITICAL STEP: WAIT FOR RELEASE ---
  delay(50); // Small delay to debounce the physical metal contacts
  
  // Trap the code here as long as the button is still being held down
  while (digitalRead(button_Pin) == LOW) {
    // Do nothing, just wait for the user to lift their finger
  }

  // --- STEP 2: NO ICE READING ---
  Serial.println("Setting NO ICE Live Value. Press button to lock in.");
  
  while (digitalRead(button_Pin) != LOW) {
    currentAvg = (analogRead(ir_Sensor_Pin1) + analogRead(ir_Sensor_Pin2)) / 2;
  }
  
  ir_no_ice_detected = currentAvg; // Save the reading
  Serial.print("NO ICE value locked! Value is: ");
  Serial.println(ir_no_ice_detected);

  // --- CRITICAL STEP: WAIT FOR RELEASE (AGAIN) ---
  delay(50); 
  while (digitalRead(button_Pin) == LOW) {
    // Wait for release again so we don't accidentally trigger something in DRIVE
  }


  // --- STEP 3: TRANSITION ---
  if (calibrationStep == 1) {
    Serial.println("Calibration complete. Transitioning to DRIVE.");
    currentState = DRIVE;
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
  Serial.print(irAverage);
  Serial.print(" - ");
  Serial.println(ir_no_ice_detected);

  // 3. Check for trigger condition (e.g., obstacle detected or line lost)
  // Assuming HIGH means a trigger condition was met
  if (irAverage >= (ir_ice_detected - 200) && irAverage <= (ir_ice_detected + 200)) {
    Serial.println("Trigger detected. Transitioning to BRAKE.");
    currentState = BRAKE;
  }

  // Check if the key pressed was 'Enter' (\n or \r)
  if (digitalRead(button_Pin) == LOW) {
    // Reset the program
    Serial.println("Resetting program. Returning to Calibration.");
    calibrationStep = 0;
    currentState = CALIBRATION;
    delay(250);
  }
}

void handleBrake() {

  digitalWrite(LED_Pin, HIGH);
  // 1. Output commands to stop the motors
  setMotors(currentSpeed, 0, currentSpeed, 0);
  delay(100);
  stopMotors();
  delay(300);

  // 2. Read the IR sensor to check if it's safe to move again
  int irValue1 = analogRead(ir_Sensor_Pin1);
  int irValue2 = analogRead(ir_Sensor_Pin2);

  int irAverage = (irValue1 + irValue2)/2;
  Serial.print(irAverage);
  Serial.print(" - ");
  Serial.println(ir_ice_detected);

  // 3. Check if the trigger condition has cleared
  // Assuming LOW means the path is clear
  if (irAverage >= (ir_no_ice_detected - 200) && irAverage <= (ir_no_ice_detected + 200)) {
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