/*
 * ============================================================================
 * TEST 3: ALERT SYSTEM TEST (LEDs + Buzzer)
 * ============================================================================
 * 
 * Purpose:   Verify all alert outputs work correctly before integration.
 *            Tests Green LED, Red LED, and active buzzer.
 * 
 * Wiring:
 *   Green LED -> D8 (with 220Ω resistor to GND)
 *   Red LED   -> D4 (with 220Ω resistor to GND)
 *   Buzzer    -> D7 (active buzzer, positive to D7, negative to GND)
 * 
 * Usage:     Upload, open Serial Monitor at 115200. Automated test runs,
 *            then interactive mode is available.
 * 
 * ============================================================================
 */

const int BUZZER_PIN = 7;
const int GREEN_LED  = 8;
const int RED_LED    = 4;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  // Start all off
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  Serial.println(F("╔════════════════════════════════════════════╗"));
  Serial.println(F("║      TEST 3: ALERT SYSTEM TEST            ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println();
  
  runAlertTest();
  
  Serial.println(F("Interactive mode:"));
  Serial.println(F("  g = Toggle green LED"));
  Serial.println(F("  r = Toggle red LED"));
  Serial.println(F("  b = Toggle buzzer"));
  Serial.println(F("  1 = SAFE pattern    (green on, red off, no buzzer)"));
  Serial.println(F("  2 = WARNING pattern (both LEDs, slow beep)"));
  Serial.println(F("  3 = ICE pattern     (red on, fast beep)"));
  Serial.println(F("  4 = BRAKE pattern   (red on, continuous buzzer)"));
  Serial.println(F("  0 = All off"));
  Serial.println();
}

int activePattern = 0;  // 0=none, 1=safe, 2=warn, 3=ice, 4=brake

void loop() {
  // Handle pattern animations
  unsigned long now = millis();
  
  switch (activePattern) {
    case 2: // Warning - slow beep
      {
        bool phase = ((now / 600) % 2 == 0);
        digitalWrite(BUZZER_PIN, phase);
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, HIGH);
      }
      break;
      
    case 3: // Ice detected - fast beep
      {
        bool phase = ((now / 300) % 2 == 0);
        digitalWrite(BUZZER_PIN, phase);
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, HIGH);
      }
      break;
  }
  
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) Serial.read();
    activePattern = 0;
    
    switch (c) {
      case 'g': case 'G':
        digitalWrite(GREEN_LED, !digitalRead(GREEN_LED));
        Serial.print(F("Green LED: "));
        Serial.println(digitalRead(GREEN_LED) ? "ON" : "OFF");
        break;
        
      case 'r': case 'R':
        digitalWrite(RED_LED, !digitalRead(RED_LED));
        Serial.print(F("Red LED: "));
        Serial.println(digitalRead(RED_LED) ? "ON" : "OFF");
        break;
        
      case 'b': case 'B':
        digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
        Serial.print(F("Buzzer: "));
        Serial.println(digitalRead(BUZZER_PIN) ? "ON" : "OFF");
        break;
        
      case '1':
        Serial.println(F("Pattern: SAFE"));
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        activePattern = 1;
        break;
        
      case '2':
        Serial.println(F("Pattern: WARNING (slow beep)"));
        activePattern = 2;
        break;
        
      case '3':
        Serial.println(F("Pattern: ICE DETECTED (fast beep)"));
        activePattern = 3;
        break;
        
      case '4':
        Serial.println(F("Pattern: BRAKING (continuous)"));
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        activePattern = 4;
        break;
        
      case '0':
        Serial.println(F("All OFF"));
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        break;
    }
  }
  
  delay(10);
}

void runAlertTest() {
  Serial.println(F("--- Automated Alert Test ---"));
  Serial.println();
  
  // Test green LED
  Serial.println(F("[1/6] Green LED ON..."));
  digitalWrite(GREEN_LED, HIGH);
  delay(800);
  digitalWrite(GREEN_LED, LOW);
  delay(300);
  
  // Test red LED
  Serial.println(F("[2/6] Red LED ON..."));
  digitalWrite(RED_LED, HIGH);
  delay(800);
  digitalWrite(RED_LED, LOW);
  delay(300);
  
  // Test buzzer
  Serial.println(F("[3/6] Buzzer ON..."));
  digitalWrite(BUZZER_PIN, HIGH);
  delay(400);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
  
  // Test SAFE pattern
  Serial.println(F("[4/6] SAFE pattern (2s)..."));
  digitalWrite(GREEN_LED, HIGH);
  delay(2000);
  digitalWrite(GREEN_LED, LOW);
  delay(300);
  
  // Test WARNING pattern
  Serial.println(F("[5/6] WARNING pattern (3s)..."));
  unsigned long start = millis();
  while (millis() - start < 3000) {
    bool phase = ((millis() / 600) % 2 == 0);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, phase);
    delay(10);
  }
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
  
  // Test ICE pattern
  Serial.println(F("[6/6] ICE DETECTED pattern (3s)..."));
  start = millis();
  while (millis() - start < 3000) {
    bool phase = ((millis() / 300) % 2 == 0);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, phase);
    delay(10);
  }
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println();
  Serial.println(F("--- Automated test complete ---"));
  Serial.println(F("Verify: Green LED, Red LED, and Buzzer all functioned."));
  Serial.println();
}
