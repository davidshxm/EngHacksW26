// 1. Define the possible states for your system
enum SystemState {
  CALIBRATION,
  DRIVE,
  BRAKE
};

// Set the initial state
SystemState currentState = CALIBRATION;

// 2. Define your pins
const int irSensorPin = 2; // Example IR sensor pin
const int irSesnsorPin2 = 3; // Example second IR sensor pin
// Add your motor control pins here...

void setup() {
  Serial.begin(9600);
  
  // Initialize pins
  pinMode(irSensorPin, INPUT);
  // pinMode(motorPin, OUTPUT);
  
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
  // 1. Execute calibration routine (e.g., spinning to find line, reading ambient light)
  // ... your calibration code here ...

  // 2. Check if calibration is finished
  bool isCalibrationDone = true; // Replace with your actual completion condition

  // 3. Transition to the next state
  if (isCalibrationDone) {
    Serial.println("Calibration complete. Transitioning to DRIVE.");
    currentState = DRIVE;
  }
}

void handleDrive() {
  // 1. Output commands to drive motors forward
  // ... your motor driving code here ...

  // 2. Read the IR sensor
  int irReading = digitalRead(irSensorPin); // Use analogRead() if it's an analog sensor

  // 3. Check for trigger condition (e.g., obstacle detected or line lost)
  // Assuming HIGH means a trigger condition was met
  if (irReading == HIGH) {
    Serial.println("Trigger detected. Transitioning to BRAKE.");
    currentState = BRAKE;
  }
}

void handleBrake() {
  // 1. Output commands to stop the motors
  // ... your braking code here ...

  // 2. Read the IR sensor to check if it's safe to move again
  int irReading = digitalRead(irSensorPin);

  // 3. Check if the trigger condition has cleared
  // Assuming LOW means the path is clear
  if (irReading == LOW) {
    Serial.println("Path clear. Transitioning back to DRIVE.");
    currentState = DRIVE;
  }
}