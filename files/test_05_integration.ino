/*
 * ============================================================================
 * TEST 5: FULL INTEGRATION TEST
 * ============================================================================
 * 
 * Purpose:   End-to-end system test combining sensors, motors, and alerts.
 *            Verifies the complete detection -> alert -> brake pipeline.
 * 
 * Wiring:    Full system wiring (same as main.ino)
 * 
 * Usage:
 *   1. Upload to Arduino UNO R4 Minima
 *   2. Open Serial Monitor at 115200 baud
 *   3. Commands:
 *      'c' = Calibrate on current surface (must do first)
 *      'r' = Run continuous monitoring (same as main, but with extra logging)
 *      's' = Stop monitoring
 *      '1' = Simulate SAFE reading (print expected behavior)
 *      '2' = Simulate WARNING reading
 *      '3' = Simulate ICE DETECTED reading
 *      'w' = Wiring check (tests all pins for connectivity)
 *      'l' = Latency test (measures sensor-to-motor response time)
 * 
 * ============================================================================
 */

// --- All pin definitions (same as main.ino) ---
const int IR1_ANALOG  = A0;
const int IR1_DIGITAL = 2;
const int IR2_ANALOG  = A1;
const int IR2_DIGITAL = 3;
const int MOTOR_A_IN1 = 5;
const int MOTOR_A_IN2 = 6;
const int MOTOR_B_IN3 = 9;
const int MOTOR_B_IN4 = 10;
const int MOTOR_A_EN  = 11;
const int MOTOR_B_EN  = 12;
const int BUZZER_PIN  = 7;
const int GREEN_LED   = 8;
const int RED_LED     = 4;
const int CAL_BTN     = 13;

// Config
const int   SAMPLES      = 10;
const float THRESH_LOW   = 0.15;
const float THRESH_HIGH  = 0.25;
const int   BRAKE_PWM    = 180;
const unsigned long BRAKE_TIME = 1500;

// State
float baseline1 = 0, baseline2 = 0;
bool  calibrated = false;
bool  monitoring = false;
int   testsPassed = 0;
int   testsFailed = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  
  // Configure all pins
  pinMode(IR1_ANALOG, INPUT);
  pinMode(IR1_DIGITAL, INPUT);
  pinMode(IR2_ANALOG, INPUT);
  pinMode(IR2_DIGITAL, INPUT);
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  pinMode(MOTOR_A_EN, OUTPUT);
  pinMode(MOTOR_B_EN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(CAL_BTN, INPUT_PULLUP);
  
  allOff();
  
  Serial.println(F("╔══════════════════════════════════════════════════╗"));
  Serial.println(F("║      TEST 5: FULL INTEGRATION TEST              ║"));
  Serial.println(F("╚══════════════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Commands: c=Calibrate r=Monitor s=Stop w=WiringCheck l=Latency"));
  Serial.println(F("          1=TestSafe  2=TestWarn  3=TestIce"));
  Serial.println();
  
  // Auto-run wiring check
  Serial.println(F("Running automatic wiring check..."));
  wiringCheck();
}

void loop() {
  if (monitoring) {
    runMonitorCycle();
  }
  
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();
    
    switch (cmd) {
      case 'c': case 'C': runCalibration(); break;
      case 'r': case 'R': monitoring = true; Serial.println(F("[Monitor ON]")); break;
      case 's': case 'S': monitoring = false; allOff(); Serial.println(F("[Monitor OFF]")); break;
      case '1': testSafeResponse(); break;
      case '2': testWarningResponse(); break;
      case '3': testIceResponse(); break;
      case 'w': case 'W': wiringCheck(); break;
      case 'l': case 'L': latencyTest(); break;
    }
  }
  
  delay(10);
}

// ========================= WIRING CHECK ====================================

void wiringCheck() {
  testsPassed = 0;
  testsFailed = 0;
  
  Serial.println();
  Serial.println(F("═══ WIRING VERIFICATION ═══"));
  
  // Test IR sensor 1 analog (should read something, not stuck at 0 or 1023)
  int r1 = analogRead(IR1_ANALOG);
  check("IR Sensor 1 Analog (A0)", r1 > 10 && r1 < 1013, r1);
  
  // Test IR sensor 2 analog
  int r2 = analogRead(IR2_ANALOG);
  check("IR Sensor 2 Analog (A1)", r2 > 10 && r2 < 1013, r2);
  
  // Test digital pins respond (read state, it's OK either way as long as not floating)
  // We check by reading multiple times - floating pins will vary
  bool d1_stable = true;
  int d1_first = digitalRead(IR1_DIGITAL);
  for (int i = 0; i < 10; i++) {
    if (digitalRead(IR1_DIGITAL) != d1_first) { d1_stable = false; break; }
    delayMicroseconds(100);
  }
  check("IR Sensor 1 Digital (D2)", d1_stable, d1_first);
  
  bool d2_stable = true;
  int d2_first = digitalRead(IR2_DIGITAL);
  for (int i = 0; i < 10; i++) {
    if (digitalRead(IR2_DIGITAL) != d2_first) { d2_stable = false; break; }
    delayMicroseconds(100);
  }
  check("IR Sensor 2 Digital (D3)", d2_stable, d2_first);
  
  // Test LEDs (visual check required)
  Serial.println(F("  [VISUAL] Green LED (D8) should blink..."));
  for (int i = 0; i < 3; i++) {
    digitalWrite(GREEN_LED, HIGH); delay(200);
    digitalWrite(GREEN_LED, LOW); delay(200);
  }
  
  Serial.println(F("  [VISUAL] Red LED (D4) should blink..."));
  for (int i = 0; i < 3; i++) {
    digitalWrite(RED_LED, HIGH); delay(200);
    digitalWrite(RED_LED, LOW); delay(200);
  }
  
  // Test buzzer
  Serial.println(F("  [AUDIO] Buzzer (D7) should beep..."));
  digitalWrite(BUZZER_PIN, HIGH); delay(150);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Test motors briefly
  Serial.println(F("  [MOTOR] Motor A (D5/D6) brief pulse..."));
  digitalWrite(MOTOR_A_EN, HIGH);
  analogWrite(MOTOR_A_IN1, 100);
  analogWrite(MOTOR_A_IN2, 0);
  delay(300);
  analogWrite(MOTOR_A_IN1, 0);
  digitalWrite(MOTOR_A_EN, LOW);
  
  Serial.println(F("  [MOTOR] Motor B (D9/D10) brief pulse..."));
  digitalWrite(MOTOR_B_EN, HIGH);
  analogWrite(MOTOR_B_IN3, 100);
  analogWrite(MOTOR_B_IN4, 0);
  delay(300);
  analogWrite(MOTOR_B_IN3, 0);
  digitalWrite(MOTOR_B_EN, LOW);
  
  // Calibration button
  int btn = digitalRead(CAL_BTN);
  check("Calibration Button (D13)", btn == HIGH, btn); // Should be HIGH (pullup, not pressed)
  
  Serial.println();
  Serial.print(F("Results: "));
  Serial.print(testsPassed); Serial.print(F(" passed, "));
  Serial.print(testsFailed); Serial.println(F(" failed"));
  
  if (testsFailed == 0) {
    Serial.println(F("All automated checks PASSED. Verify LEDs/buzzer/motors visually."));
  } else {
    Serial.println(F("Some checks FAILED - review wiring!"));
  }
  Serial.println();
}

void check(const char* name, bool passed, int value) {
  Serial.print(F("  "));
  Serial.print(passed ? F("[PASS]") : F("[FAIL]"));
  Serial.print(F(" "));
  Serial.print(name);
  Serial.print(F(" = "));
  Serial.println(value);
  
  if (passed) testsPassed++;
  else testsFailed++;
}

// ========================= CALIBRATION =====================================

void runCalibration() {
  Serial.println(F("\n>>> Calibrating on current surface (3 seconds)..."));
  
  long sum1 = 0, sum2 = 0;
  int count = 0;
  unsigned long start = millis();
  
  while (millis() - start < 3000) {
    long s1 = 0, s2 = 0;
    for (int i = 0; i < SAMPLES; i++) {
      s1 += analogRead(IR1_ANALOG);
      s2 += analogRead(IR2_ANALOG);
      delayMicroseconds(200);
    }
    sum1 += s1 / SAMPLES;
    sum2 += s2 / SAMPLES;
    count++;
    
    // Progress indicator
    if (count % 10 == 0) Serial.print('.');
    delay(50);
  }
  
  baseline1 = (float)sum1 / count;
  baseline2 = (float)sum2 / count;
  calibrated = true;
  
  Serial.println();
  Serial.print(F("  Baseline S1: ")); Serial.println(baseline1, 1);
  Serial.print(F("  Baseline S2: ")); Serial.println(baseline2, 1);
  Serial.println(F("  Calibration complete. Start monitoring with 'r'."));
  Serial.println();
}

// ========================= MONITORING ======================================

void runMonitorCycle() {
  // Read sensors
  long s1 = 0, s2 = 0;
  for (int i = 0; i < SAMPLES; i++) {
    s1 += analogRead(IR1_ANALOG);
    s2 += analogRead(IR2_ANALOG);
    delayMicroseconds(200);
  }
  float avg1 = (float)s1 / SAMPLES;
  float avg2 = (float)s2 / SAMPLES;
  
  if (!calibrated) {
    Serial.println(F("[!] Not calibrated - press 'c' first"));
    monitoring = false;
    return;
  }
  
  float dev1 = fabs(avg1 - baseline1) / baseline1;
  float dev2 = fabs(avg2 - baseline2) / baseline2;
  
  bool dig1 = (digitalRead(IR1_DIGITAL) == LOW);
  bool dig2 = (digitalRead(IR2_DIGITAL) == LOW);
  int digCount = (dig1 ? 1 : 0) + (dig2 ? 1 : 0);
  
  // Evaluate
  const char* state = "SAFE";
  
  if (dev1 >= THRESH_HIGH && dev2 >= THRESH_HIGH) {
    state = "ICE!";
    activateIceResponse();
  } else if ((dev1 >= THRESH_LOW || dev2 >= THRESH_LOW) && digCount >= 2) {
    state = "ICE!";
    activateIceResponse();
  } else if (dev1 >= THRESH_LOW && dev2 >= THRESH_LOW) {
    state = "ICE!";
    activateIceResponse();
  } else if (dev1 >= THRESH_LOW || dev2 >= THRESH_LOW || digCount >= 1) {
    state = "WARN";
    activateWarning();
  } else {
    activateSafe();
  }
  
  // Log
  Serial.print('['); Serial.print(state); Serial.print(F("] "));
  Serial.print(F("S1=")); Serial.print(avg1, 0);
  Serial.print(F("(")); Serial.print(dev1 * 100, 1); Serial.print(F("%) "));
  Serial.print(F("S2=")); Serial.print(avg2, 0);
  Serial.print(F("(")); Serial.print(dev2 * 100, 1); Serial.print(F("%) "));
  Serial.print(F("D1=")); Serial.print(dig1);
  Serial.print(F(" D2=")); Serial.println(dig2);
  
  delay(200);
}

// ========================= RESPONSE FUNCTIONS ==============================

void activateSafe() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  motorsOff();
}

void activateWarning() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  bool beep = ((millis() / 600) % 2 == 0);
  digitalWrite(BUZZER_PIN, beep);
  motorsOff();
}

void activateIceResponse() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  
  // Brake!
  digitalWrite(MOTOR_A_EN, HIGH);
  digitalWrite(MOTOR_B_EN, HIGH);
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, BRAKE_PWM);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, BRAKE_PWM);
}

void motorsOff() {
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 0);
  digitalWrite(MOTOR_A_EN, LOW);
  digitalWrite(MOTOR_B_EN, LOW);
}

void allOff() {
  motorsOff();
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

// ========================= COMPONENT TESTS =================================

void testSafeResponse() {
  Serial.println(F("\n>>> TEST: SAFE state response <<<"));
  Serial.println(F("  Expected: Green LED ON, Red OFF, Buzzer OFF, Motors OFF"));
  activateSafe();
  delay(2000);
  
  // Verify
  bool pass = (digitalRead(GREEN_LED) == HIGH &&
               digitalRead(RED_LED) == LOW);
  Serial.print(F("  Result: "));
  Serial.println(pass ? F("PASS") : F("FAIL - check LED wiring"));
  allOff();
  Serial.println();
}

void testWarningResponse() {
  Serial.println(F("\n>>> TEST: WARNING state response <<<"));
  Serial.println(F("  Expected: Both LEDs ON, Buzzer slow beep, Motors OFF"));
  
  unsigned long start = millis();
  while (millis() - start < 3000) {
    activateWarning();
    delay(10);
  }
  
  Serial.println(F("  Did you see both LEDs and hear slow beeping? (visual check)"));
  allOff();
  Serial.println();
}

void testIceResponse() {
  Serial.println(F("\n>>> TEST: ICE DETECTED response <<<"));
  Serial.println(F("  Expected: Red LED ON, Buzzer ON, Motors REVERSE for 1.5s"));
  
  activateIceResponse();
  Serial.println(F("  Motors reversing..."));
  delay(BRAKE_TIME);
  
  allOff();
  Serial.println(F("  Brake complete. Motors stopped."));
  Serial.println(F("  Did motors run in reverse? Did you hear buzzer? (visual check)"));
  Serial.println();
}

// ========================= LATENCY TEST ====================================

void latencyTest() {
  if (!calibrated) {
    Serial.println(F("[!] Calibrate first with 'c'!"));
    return;
  }
  
  Serial.println(F("\n═══ LATENCY TEST ═══"));
  Serial.println(F("Measuring sensor-read-to-motor-activation time..."));
  Serial.println(F("(Place a reflective surface in front of sensors NOW)"));
  delay(3000);
  
  unsigned long t_start = micros();
  
  // Read sensors
  long s1 = 0, s2 = 0;
  for (int i = 0; i < SAMPLES; i++) {
    s1 += analogRead(IR1_ANALOG);
    s2 += analogRead(IR2_ANALOG);
    delayMicroseconds(200);
  }
  
  unsigned long t_read = micros();
  
  // Compute deviation
  float avg1 = (float)s1 / SAMPLES;
  float dev1 = fabs(avg1 - baseline1) / baseline1;
  
  unsigned long t_compute = micros();
  
  // Activate motors
  digitalWrite(MOTOR_A_EN, HIGH);
  digitalWrite(MOTOR_B_EN, HIGH);
  analogWrite(MOTOR_A_IN2, BRAKE_PWM);
  analogWrite(MOTOR_B_IN4, BRAKE_PWM);
  
  unsigned long t_motor = micros();
  
  // Stop motors
  delay(200);
  allOff();
  
  Serial.print(F("  Sensor read time:  ")); Serial.print(t_read - t_start); Serial.println(F(" µs"));
  Serial.print(F("  Compute time:      ")); Serial.print(t_compute - t_read); Serial.println(F(" µs"));
  Serial.print(F("  Motor start time:  ")); Serial.print(t_motor - t_compute); Serial.println(F(" µs"));
  Serial.print(F("  TOTAL LATENCY:     ")); Serial.print(t_motor - t_start); Serial.println(F(" µs"));
  Serial.print(F("                     ")); Serial.print((t_motor - t_start) / 1000.0, 2); Serial.println(F(" ms"));
  Serial.println();
  
  if ((t_motor - t_start) < 10000) {
    Serial.println(F("  Latency < 10ms — EXCELLENT for safety application"));
  } else if ((t_motor - t_start) < 50000) {
    Serial.println(F("  Latency < 50ms — Good, acceptable response time"));
  } else {
    Serial.println(F("  Latency > 50ms — Consider reducing SAMPLES count"));
  }
  Serial.println();
}
