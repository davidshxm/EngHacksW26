/*
 * ============================================================================
 * TEST 1: MH-B IR SENSOR INDIVIDUAL TEST
 * ============================================================================
 * 
 * Purpose:   Verify each MH-B sensor works independently. Reads analog
 *            and digital outputs, prints values for surface comparison.
 * 
 * Wiring:    Same as main sketch (sensors on A0/D2 and A1/D3)
 * 
 * Usage:     1. Upload to Arduino UNO R4 Minima
 *            2. Open Serial Monitor at 115200 baud
 *            3. Point sensors at different surfaces:
 *               - Dry concrete / asphalt
 *               - Wet surface
 *               - Ice / frozen surface
 *               - Reflective surface (glass, mirror) to simulate ice
 *            4. Record the analog values for each surface type
 *            5. Use recorded values to tune thresholds in main sketch
 * 
 * Expected Results:
 *   - Dry surface:  Stable analog reading (your baseline)
 *   - Wet surface:  Slightly different from dry (higher reflectivity)
 *   - Ice/glass:    Noticeably different (much more IR reflective)
 *   - Digital pin:  LOW when object/reflective surface detected
 * 
 * ============================================================================
 */

const int IR1_ANALOG  = A0;
const int IR1_DIGITAL = 2;
const int IR2_ANALOG  = A1;
const int IR2_DIGITAL = 3;

const int NUM_SAMPLES = 20;  // Oversampling count

// Statistics tracking
float s1_min = 1023, s1_max = 0, s1_sum = 0;
float s2_min = 1023, s2_max = 0, s2_sum = 0;
int   totalCycles = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  
  pinMode(IR1_ANALOG, INPUT);
  pinMode(IR1_DIGITAL, INPUT);
  pinMode(IR2_ANALOG, INPUT);
  pinMode(IR2_DIGITAL, INPUT);
  
  Serial.println(F("╔════════════════════════════════════════════╗"));
  Serial.println(F("║   TEST 1: MH-B IR SENSOR INDIVIDUAL TEST  ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Point sensors at different surfaces and observe values."));
  Serial.println(F("Press 'r' to reset statistics."));
  Serial.println();
  Serial.println(F("CYCLE | S1_Analog | S1_Dig | S2_Analog | S2_Dig | S1_Noise | S2_Noise"));
  Serial.println(F("------+-----------+--------+-----------+--------+----------+---------"));
}

void loop() {
  // --- Oversampled reading ---
  long sum1 = 0, sum2 = 0;
  int  min1 = 1023, max1 = 0;
  int  min2 = 1023, max2 = 0;
  
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int r1 = analogRead(IR1_ANALOG);
    int r2 = analogRead(IR2_ANALOG);
    
    sum1 += r1;
    sum2 += r2;
    
    if (r1 < min1) min1 = r1;
    if (r1 > max1) max1 = r1;
    if (r2 < min2) min2 = r2;
    if (r2 > max2) max2 = r2;
    
    delayMicroseconds(500);
  }
  
  float avg1 = (float)sum1 / NUM_SAMPLES;
  float avg2 = (float)sum2 / NUM_SAMPLES;
  int noise1 = max1 - min1;  // Peak-to-peak noise
  int noise2 = max2 - min2;
  
  bool dig1 = (digitalRead(IR1_DIGITAL) == LOW);
  bool dig2 = (digitalRead(IR2_DIGITAL) == LOW);
  
  // Update running statistics
  totalCycles++;
  s1_sum += avg1;
  s2_sum += avg2;
  if (avg1 < s1_min) s1_min = avg1;
  if (avg1 > s1_max) s1_max = avg1;
  if (avg2 < s2_min) s2_min = avg2;
  if (avg2 > s2_max) s2_max = avg2;
  
  // --- Print formatted output ---
  char buf[100];
  snprintf(buf, sizeof(buf), "%5d | %9.1f | %6s | %9.1f | %6s | %8d | %7d",
           totalCycles,
           avg1, dig1 ? "TRIG" : "---",
           avg2, dig2 ? "TRIG" : "---",
           noise1, noise2);
  Serial.println(buf);
  
  // Print statistics every 20 cycles
  if (totalCycles % 20 == 0) {
    Serial.println();
    Serial.println(F("--- Running Statistics ---"));
    Serial.print(F("  S1: min=")); Serial.print(s1_min, 1);
    Serial.print(F("  max=")); Serial.print(s1_max, 1);
    Serial.print(F("  avg=")); Serial.print(s1_sum / totalCycles, 1);
    Serial.print(F("  range=")); Serial.println(s1_max - s1_min, 1);
    Serial.print(F("  S2: min=")); Serial.print(s2_min, 1);
    Serial.print(F("  max=")); Serial.print(s2_max, 1);
    Serial.print(F("  avg=")); Serial.print(s2_sum / totalCycles, 1);
    Serial.print(F("  range=")); Serial.println(s2_max - s2_min, 1);
    
    // Cross-sensor comparison
    float diff = fabs((s1_sum / totalCycles) - (s2_sum / totalCycles));
    Serial.print(F("  Sensor difference (avg): "));
    Serial.print(diff, 1);
    if (diff > 50) {
      Serial.println(F("  ⚠ LARGE - check sensor alignment!"));
    } else {
      Serial.println(F("  ✓ OK"));
    }
    Serial.println(F("--------------------------"));
    Serial.println();
  }
  
  // Check for reset command
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) Serial.read();
    if (c == 'r' || c == 'R') {
      s1_min = 1023; s1_max = 0; s1_sum = 0;
      s2_min = 1023; s2_max = 0; s2_sum = 0;
      totalCycles = 0;
      Serial.println(F(">>> Statistics reset <<<"));
    }
  }
  
  delay(250);  // 4 readings per second
}
