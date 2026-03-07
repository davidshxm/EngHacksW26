/*
 * ============================================================================
 * ICE DETECTION & AUTO-BRAKING SYSTEM FOR WALKER
 * ============================================================================
 * 
 * Target Board:  Arduino UNO R4 Minima
 * Sensors:       2x MH-B Infrared Sensors (analog output)
 * Actuators:     2x DC Motors (via L298N dual H-bridge driver)
 * Alerts:        Buzzer + LED (Green/Red)
 * 
 * WIRING DIAGRAM:
 * -----------------------------------------------------------------------
 * MH-B Sensor #1 (Left):
 *   VCC  -> 5V
 *   GND  -> GND
 *   AO   -> A0  (Analog output)
 *   DO   -> D2  (Digital threshold output)
 *
 * MH-B Sensor #2 (Right):
 *   VCC  -> 5V
 *   GND  -> GND
 *   AO   -> A1  (Analog output)
 *   DO   -> D3  (Digital threshold output)
 *
 * L298N Motor Driver:
 *   IN1  -> D5  (Motor A direction 1) [PWM]
 *   IN2  -> D6  (Motor A direction 2) [PWM]
 *   IN3  -> D9  (Motor B direction 1) [PWM]
 *   IN4  -> D10 (Motor B direction 2) [PWM]
 *   ENA  -> D11 (Motor A enable/speed) [PWM] - remove jumper
 *   ENB  -> D12 (Motor B enable)
 *   12V  -> External 9-12V supply
 *   GND  -> Common GND with Arduino
 *   5V   -> Can power Arduino (or separate supply)
 *
 * Alert Outputs:
 *   Buzzer    -> D7  (Active buzzer, low-side via transistor recommended)
 *   Green LED -> D8  (with 220Ω resistor)
 *   Red LED   -> D4  (with 220Ω resistor)
 *
 * -----------------------------------------------------------------------
 * 
 * DETECTION LOGIC:
 *   The MH-B IR sensors output an analog voltage that changes based on
 *   the infrared reflectivity of the surface ahead. Ice/wet surfaces
 *   reflect IR differently than dry concrete/asphalt.
 *   
 *   - We read both sensors and compute a rolling average.
 *   - We compare current readings against a calibrated "dry surface" baseline.
 *   - A significant deviation (configurable threshold) triggers ice alert.
 *   - Both sensors must agree (or one must deviate strongly) to reduce
 *     false positives.
 *   - On detection: motors reverse briefly to brake, buzzer + red LED activate.
 *   - On safe surface: motors are unpowered (coast), green LED is on.
 * 
 * ============================================================================
 */

// =========================== PIN DEFINITIONS ===============================

// --- IR Sensors ---
const int IR_SENSOR_1_ANALOG  = A0;   // Left MH-B analog out
const int IR_SENSOR_1_DIGITAL = 2;    // Left MH-B digital out
const int IR_SENSOR_2_ANALOG  = A1;   // Right MH-B analog out
const int IR_SENSOR_2_DIGITAL = 3;    // Right MH-B digital out

// --- Motor Driver (L298N) ---
const int MOTOR_A_IN1 = 5;    // Motor A forward  (PWM)
const int MOTOR_A_IN2 = 6;    // Motor A reverse   (PWM)
const int MOTOR_B_IN3 = 9;    // Motor B forward  (PWM)
const int MOTOR_B_IN4 = 10;   // Motor B reverse   (PWM)
const int MOTOR_A_EN  = 11;   // Motor A enable    (PWM speed)
const int MOTOR_B_EN  = 12;   // Motor B enable

// --- Alert Outputs ---
const int BUZZER_PIN   = 7;
const int GREEN_LED    = 8;
const int RED_LED      = 4;

// --- Calibration Button ---
const int CALIBRATE_BTN = 13; // Momentary push button to GND (internal pullup)

// ========================= CONFIGURATION ===================================

// Detection thresholds
const int   SAMPLE_COUNT          = 10;     // Readings per sensor per cycle
const int   ROLLING_WINDOW        = 5;      // Rolling average window size
const float ICE_DEVIATION_THRESH  = 0.15;   // 15% deviation from baseline = ice
const float HIGH_CONFIDENCE_THRESH = 0.25;  // 25% deviation = high confidence
const int   DIGITAL_AGREE_REQUIRED = 1;     // Min digital pins that must trigger (1 or 2)

// Motor braking parameters
const int   BRAKE_PWM_SPEED       = 180;    // Reverse motor PWM (0-255)
const unsigned long BRAKE_DURATION_MS = 1500; // How long to reverse-brake (ms)
const unsigned long BRAKE_COOLDOWN_MS = 3000; // Min time between brake activations

// Timing
const unsigned long SENSOR_READ_INTERVAL_MS = 100;  // Read sensors every 100ms
const unsigned long ALERT_BEEP_INTERVAL_MS  = 300;  // Buzzer beep interval
const unsigned long CALIBRATION_DURATION_MS = 3000;  // 3 seconds of readings to calibrate

// ========================= STATE VARIABLES =================================

// Sensor baselines (set during calibration)
float baseline_sensor1 = 512.0;  // Default midpoint
float baseline_sensor2 = 512.0;

// Rolling average buffers
float readings_s1[5];  // sized to ROLLING_WINDOW
float readings_s2[5];
int   reading_index = 0;
bool  buffer_filled = false;

// Current processed values
float current_avg_s1 = 0.0;
float current_avg_s2 = 0.0;
float deviation_s1   = 0.0;
float deviation_s2   = 0.0;

// System state
enum SystemState {
  STATE_SAFE,         // Normal operation, no ice
  STATE_WARNING,      // Possible ice, low confidence
  STATE_ICE_DETECTED, // Ice confirmed, braking active
  STATE_BRAKING,      // Currently executing brake maneuver
  STATE_CALIBRATING   // Calibration in progress
};

SystemState currentState = STATE_SAFE;
SystemState previousState = STATE_SAFE;

// Timing trackers
unsigned long lastSensorRead   = 0;
unsigned long lastBeepToggle   = 0;
unsigned long brakeStartTime   = 0;
unsigned long lastBrakeTime    = 0;
bool          beepState        = false;
bool          systemCalibrated = false;

// ========================= SETUP ===========================================

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000); // Wait for serial (R4 Minima USB)
  
  // --- Pin Modes ---
  // IR Sensors
  pinMode(IR_SENSOR_1_ANALOG, INPUT);
  pinMode(IR_SENSOR_2_ANALOG, INPUT);
  pinMode(IR_SENSOR_1_DIGITAL, INPUT);
  pinMode(IR_SENSOR_2_DIGITAL, INPUT);
  
  // Motors
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  pinMode(MOTOR_A_EN, OUTPUT);
  pinMode(MOTOR_B_EN, OUTPUT);
  
  // Alerts
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  // Calibration button
  pinMode(CALIBRATE_BTN, INPUT_PULLUP);
  
  // --- Initialize motors OFF ---
  motorsStop();
  
  // --- Initialize LEDs ---
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // --- Initialize rolling average buffers ---
  for (int i = 0; i < ROLLING_WINDOW; i++) {
    readings_s1[i] = 0;
    readings_s2[i] = 0;
  }
  
  // --- Startup sequence ---
  printHeader();
  startupBlink();
  
  Serial.println(F("[SYSTEM] Ready. Press calibration button on DRY surface."));
  Serial.println(F("[SYSTEM] Or send 'C' via Serial to calibrate."));
  Serial.println();
}

// ========================= MAIN LOOP =======================================

void loop() {
  unsigned long now = millis();
  
  // --- Check for calibration trigger ---
  if (digitalRead(CALIBRATE_BTN) == LOW || checkSerialCommand()) {
    runCalibration();
  }
  
  // --- Periodic sensor reading ---
  if (now - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
    lastSensorRead = now;
    
    // Read and process sensors
    readSensors();
    
    // Determine surface state
    SystemState detected = evaluateSurface();
    
    // State machine transition
    handleStateTransition(detected, now);
  }
  
  // --- Handle ongoing state behaviors ---
  switch (currentState) {
    case STATE_SAFE:
      handleSafeState();
      break;
      
    case STATE_WARNING:
      handleWarningState(now);
      break;
      
    case STATE_ICE_DETECTED:
      handleIceDetectedState(now);
      break;
      
    case STATE_BRAKING:
      handleBrakingState(now);
      break;
      
    case STATE_CALIBRATING:
      // Handled in runCalibration()
      break;
  }
}

// ========================= SENSOR FUNCTIONS ================================

/*
 * Read both MH-B sensors with oversampling for noise reduction.
 * Updates the rolling average buffers.
 */
void readSensors() {
  long sum1 = 0, sum2 = 0;
  
  // Oversample to reduce noise
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum1 += analogRead(IR_SENSOR_1_ANALOG);
    sum2 += analogRead(IR_SENSOR_2_ANALOG);
    delayMicroseconds(200);  // Small delay between samples
  }
  
  float avg1 = (float)sum1 / SAMPLE_COUNT;
  float avg2 = (float)sum2 / SAMPLE_COUNT;
  
  // Store in rolling buffer
  readings_s1[reading_index] = avg1;
  readings_s2[reading_index] = avg2;
  reading_index = (reading_index + 1) % ROLLING_WINDOW;
  
  if (reading_index == 0) buffer_filled = true;
  
  // Compute rolling average
  int count = buffer_filled ? ROLLING_WINDOW : reading_index;
  if (count == 0) count = 1;
  
  float total1 = 0, total2 = 0;
  for (int i = 0; i < count; i++) {
    total1 += readings_s1[i];
    total2 += readings_s2[i];
  }
  
  current_avg_s1 = total1 / count;
  current_avg_s2 = total2 / count;
  
  // Compute deviation from baseline (as fraction)
  if (baseline_sensor1 > 0) {
    deviation_s1 = fabs(current_avg_s1 - baseline_sensor1) / baseline_sensor1;
  }
  if (baseline_sensor2 > 0) {
    deviation_s2 = fabs(current_avg_s2 - baseline_sensor2) / baseline_sensor2;
  }
}

/*
 * Evaluate surface condition based on sensor data.
 * Uses both analog deviation and digital threshold outputs.
 * Returns the detected state.
 */
SystemState evaluateSurface() {
  if (!systemCalibrated) return STATE_SAFE;
  
  // Read digital outputs (LOW = obstacle/reflective surface detected on most MH-B)
  bool digital1 = (digitalRead(IR_SENSOR_1_DIGITAL) == LOW);
  bool digital2 = (digitalRead(IR_SENSOR_2_DIGITAL) == LOW);
  int  digitalCount = (digital1 ? 1 : 0) + (digital2 ? 1 : 0);
  
  // --- Decision Logic ---
  
  // HIGH CONFIDENCE: Both analog sensors show large deviation
  if (deviation_s1 >= HIGH_CONFIDENCE_THRESH && deviation_s2 >= HIGH_CONFIDENCE_THRESH) {
    return STATE_ICE_DETECTED;
  }
  
  // HIGH CONFIDENCE: One analog sensor + both digital triggers
  if ((deviation_s1 >= ICE_DEVIATION_THRESH || deviation_s2 >= ICE_DEVIATION_THRESH) 
      && digitalCount >= 2) {
    return STATE_ICE_DETECTED;
  }
  
  // MEDIUM CONFIDENCE: Both analog sensors show moderate deviation
  if (deviation_s1 >= ICE_DEVIATION_THRESH && deviation_s2 >= ICE_DEVIATION_THRESH) {
    return STATE_ICE_DETECTED;
  }
  
  // LOW CONFIDENCE: One sensor deviates
  if (deviation_s1 >= ICE_DEVIATION_THRESH || deviation_s2 >= ICE_DEVIATION_THRESH) {
    return STATE_WARNING;
  }
  
  // LOW CONFIDENCE: Digital triggers without analog confirmation
  if (digitalCount >= DIGITAL_AGREE_REQUIRED) {
    return STATE_WARNING;
  }
  
  return STATE_SAFE;
}

// ========================= STATE MACHINE ===================================

void handleStateTransition(SystemState detected, unsigned long now) {
  previousState = currentState;
  
  switch (currentState) {
    case STATE_SAFE:
      if (detected == STATE_ICE_DETECTED) {
        currentState = STATE_ICE_DETECTED;
        Serial.println(F(">>> TRANSITION: SAFE -> ICE DETECTED <<<"));
      } else if (detected == STATE_WARNING) {
        currentState = STATE_WARNING;
        Serial.println(F(">>> TRANSITION: SAFE -> WARNING <<<"));
      }
      break;
      
    case STATE_WARNING:
      if (detected == STATE_ICE_DETECTED) {
        currentState = STATE_ICE_DETECTED;
        Serial.println(F(">>> TRANSITION: WARNING -> ICE DETECTED <<<"));
      } else if (detected == STATE_SAFE) {
        currentState = STATE_SAFE;
        Serial.println(F(">>> TRANSITION: WARNING -> SAFE <<<"));
      }
      break;
      
    case STATE_ICE_DETECTED:
      // Initiate braking if cooldown has passed
      if (now - lastBrakeTime >= BRAKE_COOLDOWN_MS) {
        currentState = STATE_BRAKING;
        brakeStartTime = now;
        lastBrakeTime = now;
        motorsReverse(BRAKE_PWM_SPEED);
        Serial.println(F(">>> TRANSITION: ICE DETECTED -> BRAKING <<<"));
      }
      break;
      
    case STATE_BRAKING:
      // Check if brake duration has elapsed
      if (now - brakeStartTime >= BRAKE_DURATION_MS) {
        motorsStop();
        // Re-evaluate surface
        if (detected == STATE_SAFE) {
          currentState = STATE_SAFE;
          Serial.println(F(">>> TRANSITION: BRAKING -> SAFE <<<"));
        } else {
          currentState = STATE_ICE_DETECTED;
          Serial.println(F(">>> TRANSITION: BRAKING -> ICE DETECTED (still icy) <<<"));
        }
      }
      break;
      
    default:
      break;
  }
}

// ========================= STATE HANDLERS ==================================

void handleSafeState() {
  motorsStop();
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Print status periodically (every ~1 second, controlled by read interval)
  static int safeCounter = 0;
  if (++safeCounter >= 10) {
    safeCounter = 0;
    printStatus("SAFE");
  }
}

void handleWarningState(unsigned long now) {
  motorsStop();  // Don't brake yet, just warn
  digitalWrite(GREEN_LED, HIGH);  // Both LEDs on = yellow effect
  digitalWrite(RED_LED, HIGH);
  
  // Slow beep pattern
  if (now - lastBeepToggle >= ALERT_BEEP_INTERVAL_MS * 2) {
    lastBeepToggle = now;
    beepState = !beepState;
    digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
  }
  
  printStatus("WARNING");
}

void handleIceDetectedState(unsigned long now) {
  // Motors stay stopped until braking initiates
  motorsStop();
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  
  // Rapid beep pattern
  if (now - lastBeepToggle >= ALERT_BEEP_INTERVAL_MS) {
    lastBeepToggle = now;
    beepState = !beepState;
    digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
  }
  
  printStatus("ICE!");
}

void handleBrakingState(unsigned long now) {
  // Motors are already reversing (set in transition)
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);  // Continuous tone while braking
  
  // Print braking progress
  unsigned long elapsed = now - brakeStartTime;
  Serial.print(F("[BRAKING] "));
  Serial.print(elapsed);
  Serial.print(F("ms / "));
  Serial.print(BRAKE_DURATION_MS);
  Serial.println(F("ms"));
}

// ========================= MOTOR FUNCTIONS =================================

/*
 * Stop both motors (coast - no power applied).
 * This is the normal state on safe surfaces.
 */
void motorsStop() {
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 0);
  digitalWrite(MOTOR_A_EN, LOW);
  digitalWrite(MOTOR_B_EN, LOW);
}

/*
 * Run both motors in reverse at given PWM speed.
 * This creates the braking/reversing effect.
 */
void motorsReverse(int speed) {
  speed = constrain(speed, 0, 255);
  
  digitalWrite(MOTOR_A_EN, HIGH);
  digitalWrite(MOTOR_B_EN, HIGH);
  
  // Reverse direction: IN2/IN4 active, IN1/IN3 off
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, speed);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, speed);
  
  Serial.print(F("[MOTORS] Reversing at PWM: "));
  Serial.println(speed);
}

/*
 * Run both motors forward at given PWM speed.
 * (Used for testing only - normally walker is human-powered)
 */
void motorsForward(int speed) {
  speed = constrain(speed, 0, 255);
  
  digitalWrite(MOTOR_A_EN, HIGH);
  digitalWrite(MOTOR_B_EN, HIGH);
  
  analogWrite(MOTOR_A_IN1, speed);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, speed);
  analogWrite(MOTOR_B_IN4, 0);
  
  Serial.print(F("[MOTORS] Forward at PWM: "));
  Serial.println(speed);
}

/*
 * Hard brake - short both motor terminals (active braking).
 * More aggressive than coast stop.
 */
void motorsHardBrake() {
  digitalWrite(MOTOR_A_EN, HIGH);
  digitalWrite(MOTOR_B_EN, HIGH);
  
  // Both inputs HIGH = brake mode on L298N
  analogWrite(MOTOR_A_IN1, 255);
  analogWrite(MOTOR_A_IN2, 255);
  analogWrite(MOTOR_B_IN3, 255);
  analogWrite(MOTOR_B_IN4, 255);
  
  Serial.println(F("[MOTORS] Hard brake engaged"));
}

// ========================= CALIBRATION =====================================

/*
 * Calibration routine: reads sensors for CALIBRATION_DURATION_MS on a
 * known DRY surface and stores the average as baseline.
 */
void runCalibration() {
  currentState = STATE_CALIBRATING;
  motorsStop();
  
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("   CALIBRATION MODE"));
  Serial.println(F("   Place walker on DRY surface"));
  Serial.println(F("   Collecting baseline readings..."));
  Serial.println(F("========================================"));
  
  // Visual indicator: alternating LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  long sum1 = 0, sum2 = 0;
  int totalReadings = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < CALIBRATION_DURATION_MS) {
    // Flash LEDs alternately during calibration
    bool phase = ((millis() / 250) % 2 == 0);
    digitalWrite(GREEN_LED, phase ? HIGH : LOW);
    digitalWrite(RED_LED, phase ? LOW : HIGH);
    
    // Read sensors
    long s1 = 0, s2 = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
      s1 += analogRead(IR_SENSOR_1_ANALOG);
      s2 += analogRead(IR_SENSOR_2_ANALOG);
      delayMicroseconds(200);
    }
    
    sum1 += s1 / SAMPLE_COUNT;
    sum2 += s2 / SAMPLE_COUNT;
    totalReadings++;
    
    delay(50);
  }
  
  // Compute baselines
  baseline_sensor1 = (float)sum1 / totalReadings;
  baseline_sensor2 = (float)sum2 / totalReadings;
  systemCalibrated = true;
  
  // Reset rolling buffers
  reading_index = 0;
  buffer_filled = false;
  for (int i = 0; i < ROLLING_WINDOW; i++) {
    readings_s1[i] = baseline_sensor1;
    readings_s2[i] = baseline_sensor2;
  }
  
  // Report results
  Serial.println();
  Serial.println(F("--- Calibration Complete ---"));
  Serial.print(F("  Sensor 1 baseline: "));
  Serial.println(baseline_sensor1, 2);
  Serial.print(F("  Sensor 2 baseline: "));
  Serial.println(baseline_sensor2, 2);
  Serial.print(F("  Total samples: "));
  Serial.println(totalReadings);
  Serial.print(F("  Ice threshold (15%): S1="));
  Serial.print(baseline_sensor1 * ICE_DEVIATION_THRESH, 2);
  Serial.print(F("  S2="));
  Serial.println(baseline_sensor2 * ICE_DEVIATION_THRESH, 2);
  Serial.println(F("--- System Armed ---"));
  Serial.println();
  
  // Confirm with double beep
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
  
  currentState = STATE_SAFE;
}

// ========================= UTILITY FUNCTIONS ===============================

bool checkSerialCommand() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    // Flush remaining
    while (Serial.available()) Serial.read();
    
    if (cmd == 'C' || cmd == 'c') {
      return true;  // Trigger calibration
    }
    if (cmd == 'S' || cmd == 's') {
      printDetailedStatus();
    }
    if (cmd == 'R' || cmd == 'r') {
      // Raw reading dump
      Serial.print(F("[RAW] S1_analog="));
      Serial.print(analogRead(IR_SENSOR_1_ANALOG));
      Serial.print(F("  S2_analog="));
      Serial.print(analogRead(IR_SENSOR_2_ANALOG));
      Serial.print(F("  S1_digital="));
      Serial.print(digitalRead(IR_SENSOR_1_DIGITAL));
      Serial.print(F("  S2_digital="));
      Serial.println(digitalRead(IR_SENSOR_2_DIGITAL));
    }
  }
  return false;
}

void printStatus(const char* state) {
  Serial.print(F("["));
  Serial.print(state);
  Serial.print(F("] S1="));
  Serial.print(current_avg_s1, 1);
  Serial.print(F(" ("));
  Serial.print(deviation_s1 * 100, 1);
  Serial.print(F("%) S2="));
  Serial.print(current_avg_s2, 1);
  Serial.print(F(" ("));
  Serial.print(deviation_s2 * 100, 1);
  Serial.println(F("%)"));
}

void printDetailedStatus() {
  Serial.println();
  Serial.println(F("===== DETAILED STATUS ====="));
  Serial.print(F("  State:        ")); Serial.println(stateToString(currentState));
  Serial.print(F("  Calibrated:   ")); Serial.println(systemCalibrated ? "YES" : "NO");
  Serial.print(F("  Baseline S1:  ")); Serial.println(baseline_sensor1, 2);
  Serial.print(F("  Baseline S2:  ")); Serial.println(baseline_sensor2, 2);
  Serial.print(F("  Current S1:   ")); Serial.println(current_avg_s1, 2);
  Serial.print(F("  Current S2:   ")); Serial.println(current_avg_s2, 2);
  Serial.print(F("  Deviation S1: ")); Serial.print(deviation_s1 * 100, 1); Serial.println(F("%"));
  Serial.print(F("  Deviation S2: ")); Serial.print(deviation_s2 * 100, 1); Serial.println(F("%"));
  Serial.print(F("  Dig S1:       ")); Serial.println(digitalRead(IR_SENSOR_1_DIGITAL));
  Serial.print(F("  Dig S2:       ")); Serial.println(digitalRead(IR_SENSOR_2_DIGITAL));
  Serial.println(F("==========================="));
  Serial.println();
}

const char* stateToString(SystemState s) {
  switch (s) {
    case STATE_SAFE:         return "SAFE";
    case STATE_WARNING:      return "WARNING";
    case STATE_ICE_DETECTED: return "ICE_DETECTED";
    case STATE_BRAKING:      return "BRAKING";
    case STATE_CALIBRATING:  return "CALIBRATING";
    default:                 return "UNKNOWN";
  }
}

void printHeader() {
  Serial.println();
  Serial.println(F("╔══════════════════════════════════════════════╗"));
  Serial.println(F("║   ICE DETECTION & AUTO-BRAKING SYSTEM v1.0  ║"));
  Serial.println(F("║   Walker Safety Device - Arduino UNO R4     ║"));
  Serial.println(F("╚══════════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Serial Commands:"));
  Serial.println(F("  C = Calibrate on dry surface"));
  Serial.println(F("  S = Detailed status report"));
  Serial.println(F("  R = Raw sensor reading"));
  Serial.println();
}

void startupBlink() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(80);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
  }
}
