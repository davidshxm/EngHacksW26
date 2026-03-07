/*
 * ============================================================================
 * TEST 2: DC MOTOR & L298N DRIVER TEST
 * ============================================================================
 * 
 * Purpose:   Verify both DC motors work correctly with the L298N driver.
 *            Tests forward, reverse, stop, hard brake, and PWM speed control.
 * 
 * Wiring:    Same as main sketch (L298N on D5,D6,D9,D10,D11,D12)
 * 
 * Usage:     1. Upload to Arduino UNO R4 Minima
 *            2. Open Serial Monitor at 115200 baud
 *            3. Use serial commands to control motors:
 *               'f' = Forward (both motors)
 *               'r' = Reverse (both motors)
 *               's' = Stop (coast)
 *               'b' = Hard brake
 *               'l' = Test left motor only
 *               'k' = Test right motor only  (k for right, avoids 'r' conflict)
 *               '+' = Increase speed
 *               '-' = Decrease speed
 *               'a' = Run full automated test sequence
 *               'p' = Pulse brake test (simulates ice detection response)
 * 
 * SAFETY:    Keep fingers away from wheels. Start with low PWM values.
 *            Secure the walker model before testing.
 * 
 * ============================================================================
 */

// Motor pins (L298N)
const int MOTOR_A_IN1 = 5;
const int MOTOR_A_IN2 = 6;
const int MOTOR_B_IN3 = 9;
const int MOTOR_B_IN4 = 10;
const int MOTOR_A_EN  = 11;
const int MOTOR_B_EN  = 12;

// Test parameters
int currentSpeed = 128;      // Current PWM speed (0-255)
const int SPEED_STEP = 32;   // Speed adjustment per +/- press

// Motor state tracking
enum MotorState { M_STOPPED, M_FORWARD, M_REVERSE, M_BRAKE };
MotorState motorAState = M_STOPPED;
MotorState motorBState = M_STOPPED;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  pinMode(MOTOR_A_EN, OUTPUT);
  pinMode(MOTOR_B_EN, OUTPUT);
  
  // Start with motors off
  stopMotors();
  
  printMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();
    
    switch (cmd) {
      case 'f': case 'F':
        Serial.print(F("[CMD] Forward at PWM "));
        Serial.println(currentSpeed);
        setMotors(currentSpeed, 0, currentSpeed, 0);
        motorAState = M_FORWARD;
        motorBState = M_FORWARD;
        break;
        
      case 'r': case 'R':
        Serial.print(F("[CMD] Reverse at PWM "));
        Serial.println(currentSpeed);
        setMotors(0, currentSpeed, 0, currentSpeed);
        motorAState = M_REVERSE;
        motorBState = M_REVERSE;
        break;
        
      case 's': case 'S':
        Serial.println(F("[CMD] Stop (coast)"));
        stopMotors();
        break;
        
      case 'b': case 'B':
        Serial.println(F("[CMD] Hard brake"));
        hardBrake();
        break;
        
      case 'l': case 'L':
        Serial.println(F("[CMD] Left motor (A) only - forward"));
        stopMotors();
        delay(100);
        digitalWrite(MOTOR_A_EN, HIGH);
        analogWrite(MOTOR_A_IN1, currentSpeed);
        analogWrite(MOTOR_A_IN2, 0);
        motorAState = M_FORWARD;
        break;
        
      case 'k': case 'K':
        Serial.println(F("[CMD] Right motor (B) only - forward"));
        stopMotors();
        delay(100);
        digitalWrite(MOTOR_B_EN, HIGH);
        analogWrite(MOTOR_B_IN3, currentSpeed);
        analogWrite(MOTOR_B_IN4, 0);
        motorBState = M_FORWARD;
        break;
        
      case '+': case '=':
        currentSpeed = min(255, currentSpeed + SPEED_STEP);
        Serial.print(F("[CMD] Speed increased to "));
        Serial.println(currentSpeed);
        updateRunningMotors();
        break;
        
      case '-': case '_':
        currentSpeed = max(0, currentSpeed - SPEED_STEP);
        Serial.print(F("[CMD] Speed decreased to "));
        Serial.println(currentSpeed);
        updateRunningMotors();
        break;
        
      case 'a': case 'A':
        runAutomatedTest();
        break;
        
      case 'p': case 'P':
        runPulseBrakeTest();
        break;
        
      case 'h': case 'H': case '?':
        printMenu();
        break;
    }
  }
}

// ========================= MOTOR CONTROL ===================================

void setMotors(int a1, int a2, int b3, int b4) {
  digitalWrite(MOTOR_A_EN, HIGH);
  digitalWrite(MOTOR_B_EN, HIGH);
  analogWrite(MOTOR_A_IN1, a1);
  analogWrite(MOTOR_A_IN2, a2);
  analogWrite(MOTOR_B_IN3, b3);
  analogWrite(MOTOR_B_IN4, b4);
}

void stopMotors() {
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 0);
  digitalWrite(MOTOR_A_EN, LOW);
  digitalWrite(MOTOR_B_EN, LOW);
  motorAState = M_STOPPED;
  motorBState = M_STOPPED;
}

void hardBrake() {
  digitalWrite(MOTOR_A_EN, HIGH);
  digitalWrite(MOTOR_B_EN, HIGH);
  analogWrite(MOTOR_A_IN1, 255);
  analogWrite(MOTOR_A_IN2, 255);
  analogWrite(MOTOR_B_IN3, 255);
  analogWrite(MOTOR_B_IN4, 255);
  motorAState = M_BRAKE;
  motorBState = M_BRAKE;
}

void updateRunningMotors() {
  // Re-apply current speed to running motors without changing direction
  if (motorAState == M_FORWARD) {
    analogWrite(MOTOR_A_IN1, currentSpeed);
  } else if (motorAState == M_REVERSE) {
    analogWrite(MOTOR_A_IN2, currentSpeed);
  }
  
  if (motorBState == M_FORWARD) {
    analogWrite(MOTOR_B_IN3, currentSpeed);
  } else if (motorBState == M_REVERSE) {
    analogWrite(MOTOR_B_IN4, currentSpeed);
  }
}

// ========================= AUTOMATED TESTS =================================

void runAutomatedTest() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════════╗"));
  Serial.println(F("║     AUTOMATED MOTOR TEST SEQUENCE          ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println();
  
  // Test 1: Motor A forward
  Serial.println(F("[TEST 1/8] Motor A forward..."));
  stopMotors();
  delay(300);
  digitalWrite(MOTOR_A_EN, HIGH);
  analogWrite(MOTOR_A_IN1, 150);
  analogWrite(MOTOR_A_IN2, 0);
  delay(1500);
  stopMotors();
  Serial.println(F("  DONE - Did Motor A spin forward? (left wheel)"));
  delay(1000);
  
  // Test 2: Motor A reverse
  Serial.println(F("[TEST 2/8] Motor A reverse..."));
  digitalWrite(MOTOR_A_EN, HIGH);
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 150);
  delay(1500);
  stopMotors();
  Serial.println(F("  DONE - Did Motor A spin backward?"));
  delay(1000);
  
  // Test 3: Motor B forward
  Serial.println(F("[TEST 3/8] Motor B forward..."));
  digitalWrite(MOTOR_B_EN, HIGH);
  analogWrite(MOTOR_B_IN3, 150);
  analogWrite(MOTOR_B_IN4, 0);
  delay(1500);
  stopMotors();
  Serial.println(F("  DONE - Did Motor B spin forward? (right wheel)"));
  delay(1000);
  
  // Test 4: Motor B reverse
  Serial.println(F("[TEST 4/8] Motor B reverse..."));
  digitalWrite(MOTOR_B_EN, HIGH);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 150);
  delay(1500);
  stopMotors();
  Serial.println(F("  DONE - Did Motor B spin backward?"));
  delay(1000);
  
  // Test 5: Both forward
  Serial.println(F("[TEST 5/8] Both motors forward..."));
  setMotors(150, 0, 150, 0);
  delay(1500);
  stopMotors();
  Serial.println(F("  DONE - Both wheels should have gone forward"));
  delay(1000);
  
  // Test 6: Both reverse (brake simulation)
  Serial.println(F("[TEST 6/8] Both motors REVERSE (brake mode)..."));
  setMotors(0, 180, 0, 180);
  delay(1500);
  stopMotors();
  Serial.println(F("  DONE - Both wheels should have reversed"));
  delay(1000);
  
  // Test 7: Speed ramp test
  Serial.println(F("[TEST 7/8] Speed ramp test (Motor A forward)..."));
  digitalWrite(MOTOR_A_EN, HIGH);
  for (int pwm = 60; pwm <= 255; pwm += 15) {
    analogWrite(MOTOR_A_IN1, pwm);
    analogWrite(MOTOR_A_IN2, 0);
    Serial.print(F("  PWM: ")); Serial.println(pwm);
    delay(400);
  }
  stopMotors();
  Serial.println(F("  DONE - Speed should have gradually increased"));
  delay(1000);
  
  // Test 8: Hard brake test
  Serial.println(F("[TEST 8/8] Hard brake test..."));
  Serial.println(F("  Running forward..."));
  setMotors(200, 0, 200, 0);
  delay(1500);
  Serial.println(F("  HARD BRAKE NOW!"));
  hardBrake();
  delay(500);
  stopMotors();
  Serial.println(F("  DONE - Motors should have stopped abruptly"));
  
  Serial.println();
  Serial.println(F("═══ ALL TESTS COMPLETE ═══"));
  Serial.println(F("If any motor didn't respond, check:"));
  Serial.println(F("  1. Wiring connections"));
  Serial.println(F("  2. L298N external power supply (9-12V)"));
  Serial.println(F("  3. L298N enable jumpers (removed for PWM control)"));
  Serial.println(F("  4. Common GND between Arduino and L298N"));
  Serial.println();
}

void runPulseBrakeTest() {
  Serial.println();
  Serial.println(F("═══ PULSE BRAKE TEST (Ice Detection Simulation) ═══"));
  Serial.println(F("Simulating: walker rolling forward -> ice detected -> brake"));
  Serial.println();
  
  // Phase 1: Simulate forward motion
  Serial.println(F("[Phase 1] Walker rolling forward (motors off - human powered)"));
  stopMotors();
  delay(2000);
  
  // Phase 2: Ice detected! Reverse brake
  Serial.println(F("[Phase 2] ICE DETECTED! Engaging reverse brake..."));
  setMotors(0, 180, 0, 180);
  unsigned long brakeStart = millis();
  while (millis() - brakeStart < 1500) {
    Serial.print(F("  BRAKING: "));
    Serial.print(millis() - brakeStart);
    Serial.println(F("ms"));
    delay(250);
  }
  
  // Phase 3: Stop
  stopMotors();
  Serial.println(F("[Phase 3] Brake released. Motors off."));
  delay(1000);
  
  // Phase 4: Resume (motors off for human-powered movement)
  Serial.println(F("[Phase 4] Safe surface - motors off (coast mode)"));
  stopMotors();
  
  Serial.println();
  Serial.println(F("═══ PULSE BRAKE TEST COMPLETE ═══"));
  Serial.println();
}

// ========================= UI =============================================

void printMenu() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════════╗"));
  Serial.println(F("║      TEST 2: DC MOTOR & L298N TEST        ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F("  f = Forward (both)     r = Reverse (both)"));
  Serial.println(F("  s = Stop (coast)       b = Hard brake"));
  Serial.println(F("  l = Left motor only    k = Right motor only"));
  Serial.println(F("  + = Speed up           - = Speed down"));
  Serial.println(F("  a = Automated test     p = Pulse brake test"));
  Serial.println(F("  h = This menu"));
  Serial.print(  F("  Current speed: PWM ")); Serial.println(currentSpeed);
  Serial.println();
}
