/*
 * ============================================================================
 * TEST 4: DUAL SENSOR COMPARISON & THRESHOLD TUNING
 * ============================================================================
 * 
 * Purpose:   Help determine optimal detection thresholds by comparing both
 *            sensors side-by-side on different surfaces. Outputs CSV-formatted
 *            data for analysis.
 * 
 * Wiring:    Same as main sketch (sensors on A0/D2 and A1/D3)
 * 
 * Usage:
 *   1. Upload to Arduino
 *   2. Open Serial Monitor at 115200 baud
 *   3. Commands:
 *      'd' = Record DRY surface samples (10 seconds)
 *      'w' = Record WET surface samples (10 seconds)
 *      'i' = Record ICE surface samples (10 seconds)
 *      'g' = Record GLASS surface samples (use as ice proxy)
 *      'f' = Free-run mode (continuous CSV output)
 *      'x' = Stop free-run
 *      'p' = Print summary of all recorded surfaces
 *      't' = Compute recommended thresholds
 * 
 *   4. Copy the CSV output to a spreadsheet for analysis
 *   5. Use the computed thresholds in the main sketch
 * 
 * ============================================================================
 */

const int IR1_ANALOG  = A0;
const int IR1_DIGITAL = 2;
const int IR2_ANALOG  = A1;
const int IR2_DIGITAL = 3;

const int OVERSAMPLE = 20;
const unsigned long RECORD_DURATION_MS = 10000;

// Surface recording storage (store aggregate stats to save RAM)
struct SurfaceStats {
  float s1_sum;
  float s2_sum;
  float s1_sq_sum;  // For std deviation
  float s2_sq_sum;
  float s1_min, s1_max;
  float s2_min, s2_max;
  int   count;
  bool  recorded;
};

SurfaceStats surfaces[4];  // 0=dry, 1=wet, 2=ice, 3=glass
const char* surfaceNames[] = {"DRY", "WET", "ICE", "GLASS"};

bool freeRunning = false;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  
  pinMode(IR1_ANALOG, INPUT);
  pinMode(IR1_DIGITAL, INPUT);
  pinMode(IR2_ANALOG, INPUT);
  pinMode(IR2_DIGITAL, INPUT);
  
  // Initialize stats
  for (int i = 0; i < 4; i++) {
    memset(&surfaces[i], 0, sizeof(SurfaceStats));
    surfaces[i].s1_min = 1023;
    surfaces[i].s2_min = 1023;
  }
  
  Serial.println(F("╔════════════════════════════════════════════╗"));
  Serial.println(F("║  TEST 4: DUAL SENSOR THRESHOLD TUNING     ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F("  d=Record DRY  w=Record WET  i=Record ICE  g=Record GLASS"));
  Serial.println(F("  f=Free-run    x=Stop        p=Summary     t=Thresholds"));
  Serial.println();
}

void loop() {
  if (freeRunning) {
    float s1, s2;
    readSensorPair(&s1, &s2);
    bool d1 = (digitalRead(IR1_DIGITAL) == LOW);
    bool d2 = (digitalRead(IR2_DIGITAL) == LOW);
    
    // CSV output: timestamp, s1_analog, s2_analog, s1_dig, s2_dig, diff, ratio
    Serial.print(millis());
    Serial.print(',');
    Serial.print(s1, 1);
    Serial.print(',');
    Serial.print(s2, 1);
    Serial.print(',');
    Serial.print(d1);
    Serial.print(',');
    Serial.print(d2);
    Serial.print(',');
    Serial.print(fabs(s1 - s2), 1);
    Serial.print(',');
    if (s2 > 0) Serial.print(s1 / s2, 4);
    else Serial.print(F("NaN"));
    Serial.println();
    
    delay(100);
  }
  
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) Serial.read();
    
    switch (c) {
      case 'd': case 'D': recordSurface(0); break;
      case 'w': case 'W': recordSurface(1); break;
      case 'i': case 'I': recordSurface(2); break;
      case 'g': case 'G': recordSurface(3); break;
      
      case 'f': case 'F':
        freeRunning = true;
        Serial.println(F("time_ms,s1_analog,s2_analog,s1_dig,s2_dig,diff,ratio"));
        break;
        
      case 'x': case 'X':
        freeRunning = false;
        Serial.println(F("[Stopped]"));
        break;
        
      case 'p': case 'P':
        printSummary();
        break;
        
      case 't': case 'T':
        computeThresholds();
        break;
    }
  }
}

// ========================= SENSOR READING ==================================

void readSensorPair(float* s1, float* s2) {
  long sum1 = 0, sum2 = 0;
  for (int i = 0; i < OVERSAMPLE; i++) {
    sum1 += analogRead(IR1_ANALOG);
    sum2 += analogRead(IR2_ANALOG);
    delayMicroseconds(300);
  }
  *s1 = (float)sum1 / OVERSAMPLE;
  *s2 = (float)sum2 / OVERSAMPLE;
}

// ========================= SURFACE RECORDING ===============================

void recordSurface(int idx) {
  Serial.print(F("\n>>> Recording "));
  Serial.print(surfaceNames[idx]);
  Serial.println(F(" surface for 10 seconds..."));
  Serial.println(F("    Hold sensors steady over the surface."));
  Serial.println(F("    CSV: time_ms, s1, s2"));
  
  // Reset stats for this surface
  SurfaceStats* s = &surfaces[idx];
  s->s1_sum = 0; s->s2_sum = 0;
  s->s1_sq_sum = 0; s->s2_sq_sum = 0;
  s->s1_min = 1023; s->s1_max = 0;
  s->s2_min = 1023; s->s2_max = 0;
  s->count = 0;
  
  unsigned long start = millis();
  while (millis() - start < RECORD_DURATION_MS) {
    float v1, v2;
    readSensorPair(&v1, &v2);
    
    s->s1_sum += v1;
    s->s2_sum += v2;
    s->s1_sq_sum += v1 * v1;
    s->s2_sq_sum += v2 * v2;
    if (v1 < s->s1_min) s->s1_min = v1;
    if (v1 > s->s1_max) s->s1_max = v1;
    if (v2 < s->s2_min) s->s2_min = v2;
    if (v2 > s->s2_max) s->s2_max = v2;
    s->count++;
    
    // Output CSV
    Serial.print(millis() - start);
    Serial.print(',');
    Serial.print(v1, 1);
    Serial.print(',');
    Serial.println(v2, 1);
    
    delay(100);
  }
  
  s->recorded = true;
  
  float avg1 = s->s1_sum / s->count;
  float avg2 = s->s2_sum / s->count;
  float std1 = sqrt((s->s1_sq_sum / s->count) - (avg1 * avg1));
  float std2 = sqrt((s->s2_sq_sum / s->count) - (avg2 * avg2));
  
  Serial.println();
  Serial.print(F("--- ")); Serial.print(surfaceNames[idx]); Serial.println(F(" Recording Complete ---"));
  Serial.print(F("  Samples: ")); Serial.println(s->count);
  Serial.print(F("  S1: avg=")); Serial.print(avg1, 1);
  Serial.print(F("  std=")); Serial.print(std1, 1);
  Serial.print(F("  [" )); Serial.print(s->s1_min, 1);
  Serial.print(F("-")); Serial.print(s->s1_max, 1); Serial.println(F("]"));
  Serial.print(F("  S2: avg=")); Serial.print(avg2, 1);
  Serial.print(F("  std=")); Serial.print(std2, 1);
  Serial.print(F("  [")); Serial.print(s->s2_min, 1);
  Serial.print(F("-")); Serial.print(s->s2_max, 1); Serial.println(F("]"));
  Serial.println();
}

// ========================= ANALYSIS ========================================

void printSummary() {
  Serial.println();
  Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
  Serial.println(F("║                    SURFACE COMPARISON                       ║"));
  Serial.println(F("╠══════════╦═══════════╦═══════════╦═══════════╦══════════════╣"));
  Serial.println(F("║ Surface  ║  S1 Avg   ║  S2 Avg   ║  S1 StdDv ║  S2 StdDv   ║"));
  Serial.println(F("╠══════════╬═══════════╬═══════════╬═══════════╬══════════════╣"));
  
  for (int i = 0; i < 4; i++) {
    if (!surfaces[i].recorded) continue;
    
    float avg1 = surfaces[i].s1_sum / surfaces[i].count;
    float avg2 = surfaces[i].s2_sum / surfaces[i].count;
    float std1 = sqrt((surfaces[i].s1_sq_sum / surfaces[i].count) - (avg1 * avg1));
    float std2 = sqrt((surfaces[i].s2_sq_sum / surfaces[i].count) - (avg2 * avg2));
    
    Serial.print(F("║ "));
    Serial.print(surfaceNames[i]);
    // Pad to 8 chars
    for (int p = strlen(surfaceNames[i]); p < 8; p++) Serial.print(' ');
    Serial.print(F("║ "));
    printPadded(avg1, 9);
    Serial.print(F("║ "));
    printPadded(avg2, 9);
    Serial.print(F("║ "));
    printPadded(std1, 9);
    Serial.print(F("║ "));
    printPadded(std2, 12);
    Serial.println(F("║"));
  }
  
  Serial.println(F("╚══════════╩═══════════╩═══════════╩═══════════╩══════════════╝"));
  Serial.println();
}

void computeThresholds() {
  if (!surfaces[0].recorded) {
    Serial.println(F("[ERROR] Must record DRY surface first! Press 'd'."));
    return;
  }
  
  float dry_avg1 = surfaces[0].s1_sum / surfaces[0].count;
  float dry_avg2 = surfaces[0].s2_sum / surfaces[0].count;
  float dry_std1 = sqrt((surfaces[0].s1_sq_sum / surfaces[0].count) - (dry_avg1 * dry_avg1));
  float dry_std2 = sqrt((surfaces[0].s2_sq_sum / surfaces[0].count) - (dry_avg2 * dry_avg2));
  
  Serial.println();
  Serial.println(F("═══════════════════════════════════════════════════"));
  Serial.println(F("         RECOMMENDED THRESHOLD ANALYSIS"));
  Serial.println(F("═══════════════════════════════════════════════════"));
  Serial.println();
  
  Serial.println(F("DRY baseline:"));
  Serial.print(F("  S1 = ")); Serial.print(dry_avg1, 1);
  Serial.print(F(" ± ")); Serial.println(dry_std1, 1);
  Serial.print(F("  S2 = ")); Serial.print(dry_avg2, 1);
  Serial.print(F(" ± ")); Serial.println(dry_std2, 1);
  Serial.println();
  
  // For each recorded hazardous surface, compute deviation
  for (int i = 1; i < 4; i++) {
    if (!surfaces[i].recorded) continue;
    
    float avg1 = surfaces[i].s1_sum / surfaces[i].count;
    float avg2 = surfaces[i].s2_sum / surfaces[i].count;
    
    float dev1 = fabs(avg1 - dry_avg1) / dry_avg1;
    float dev2 = fabs(avg2 - dry_avg2) / dry_avg2;
    
    Serial.print(surfaceNames[i]);
    Serial.println(F(" deviation from DRY:"));
    Serial.print(F("  S1: ")); Serial.print(dev1 * 100, 1); Serial.println(F("%"));
    Serial.print(F("  S2: ")); Serial.print(dev2 * 100, 1); Serial.println(F("%"));
    Serial.println();
  }
  
  // Recommend threshold
  Serial.println(F("RECOMMENDED SETTINGS for main.ino:"));
  Serial.println(F("  (Set ICE_DEVIATION_THRESH to ~60% of smallest ice deviation)"));
  Serial.println(F("  (Set HIGH_CONFIDENCE_THRESH to ~80% of smallest ice deviation)"));
  
  // If ice or glass is recorded, give specific recommendation
  for (int i = 2; i < 4; i++) {
    if (!surfaces[i].recorded) continue;
    
    float avg1 = surfaces[i].s1_sum / surfaces[i].count;
    float avg2 = surfaces[i].s2_sum / surfaces[i].count;
    float dev1 = fabs(avg1 - dry_avg1) / dry_avg1;
    float dev2 = fabs(avg2 - dry_avg2) / dry_avg2;
    float minDev = min(dev1, dev2);
    
    Serial.println();
    Serial.print(F("  Based on ")); Serial.print(surfaceNames[i]); Serial.println(F(" data:"));
    Serial.print(F("    ICE_DEVIATION_THRESH  = "));
    Serial.println(minDev * 0.6, 3);
    Serial.print(F("    HIGH_CONFIDENCE_THRESH = "));
    Serial.println(minDev * 0.8, 3);
  }
  
  // Noise floor warning
  float noise_margin_1 = (3 * dry_std1) / dry_avg1;  // 3-sigma noise as fraction
  float noise_margin_2 = (3 * dry_std2) / dry_avg2;
  
  Serial.println();
  Serial.println(F("NOISE FLOOR (3-sigma):"));
  Serial.print(F("  S1: ")); Serial.print(noise_margin_1 * 100, 2); Serial.println(F("%"));
  Serial.print(F("  S2: ")); Serial.print(noise_margin_2 * 100, 2); Serial.println(F("%"));
  Serial.println(F("  Threshold MUST be above noise floor to avoid false positives!"));
  Serial.println();
}

void printPadded(float val, int width) {
  char buf[16];
  dtostrf(val, width, 1, buf);
  Serial.print(buf);
}
