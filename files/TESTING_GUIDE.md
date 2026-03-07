# Testing Guide — Ice Detection System

## Testing Order

Run these tests in sequence. Each test validates a subsystem before you integrate.

---

## Test 1: IR Sensors (`test_01_ir_sensors.ino`)

**Goal**: Confirm both MH-B sensors give stable, distinct readings.

### Steps
1. Upload sketch, open Serial Monitor at 115200 baud
2. Hold sensors ~3-5cm above a dry table surface
3. Watch the analog values — they should be stable (noise < 20 counts)
4. Slowly move sensors over different materials: wood, paper, plastic, glass
5. Note how values change for each material
6. Press 'r' to reset statistics between surfaces

### Pass Criteria
- Both sensors report values between 50-950 (not stuck at 0 or 1023)
- Noise (peak-to-peak) is under 30 counts
- Both sensors show similar values on the same surface (difference < 50)
- Values change noticeably between dry and reflective surfaces

### Troubleshooting
- **Reading stuck at 0**: Check VCC/GND wiring
- **Reading stuck at 1023**: Sensor not detecting any surface (too far, or AO not connected)
- **Very noisy readings**: Add 0.1µF capacitor between AO and GND
- **Sensors disagree wildly**: Check they're at the same height/angle

---

## Test 2: DC Motors (`test_02_motors.ino`)

**Goal**: Confirm both motors respond to direction and speed commands.

### Steps
1. Upload sketch, open Serial Monitor
2. Press 'a' for automated test — watch/listen to each motor
3. Verify each of the 8 subtests produces the expected motion
4. Press 'p' for pulse brake test — simulates the ice detection braking
5. Use '+'/'-' to find the minimum PWM where motors spin reliably

### Pass Criteria
- Both motors spin forward and reverse on command
- Speed control works (motors speed up with '+')
- Hard brake stops motors noticeably faster than coast stop
- Pulse brake test produces a clear reverse pulse

### Troubleshooting
- **No motor movement**: Check L298N 12V input has external power
- **Only one motor works**: Check the specific IN/OUT pair and enable pin
- **Motor spins but won't reverse**: Swap motor wire polarity or check IN1/IN2 connections
- **Erratic speed**: Ensure Arduino GND is connected to L298N GND

---

## Test 3: Alerts (`test_03_alerts.ino`)

**Goal**: Confirm LEDs and buzzer work in all alert patterns.

### Steps
1. Upload sketch — automated test runs immediately
2. Verify green LED, red LED, and buzzer each activate individually
3. Use commands 1-4 to test each alert pattern
4. Verify pattern matches the expected behavior for each state

### Pass Criteria
- Green LED lights on D8
- Red LED lights on D4
- Buzzer sounds on D7
- Warning pattern: both LEDs + slow beep
- Ice pattern: red LED + fast beep

---

## Test 4: Threshold Tuning (`test_04_threshold_tuning.ino`)

**Goal**: Determine optimal detection thresholds for your specific surfaces.

### Steps
1. Upload sketch
2. Press 'd' — hold sensors over dry concrete/table for 10 seconds
3. Press 'w' — hold sensors over a wet surface for 10 seconds
4. Press 'i' — hold sensors over ice OR press 'g' for glass (ice proxy)
5. Press 'p' to see the comparison summary
6. Press 't' to get recommended threshold values

### Pass Criteria
- Dry surface has a stable, consistent reading
- Ice/glass shows at least 10% deviation from dry baseline
- Recommended threshold is above the noise floor (3-sigma line)
- If ice deviation is less than 10%, reposition sensors or adjust distance

### Using Results
Copy the recommended values from the 't' command output into `main.ino`:
```cpp
const float ICE_DEVIATION_THRESH   = <recommended value>;
const float HIGH_CONFIDENCE_THRESH = <recommended value>;
```

---

## Test 5: Integration (`test_05_integration.ino`)

**Goal**: Verify the complete sensor-to-motor pipeline works end-to-end.

### Steps
1. Upload sketch — automatic wiring check runs on startup
2. Review wiring check results (fix any FAIL items)
3. Press 'c' to calibrate on your dry surface
4. Press '1', '2', '3' to test each response mode individually
5. Press 'r' to start live monitoring
6. Move sensors between dry and icy/glass surfaces
7. Verify state transitions and motor activation
8. Press 'l' for latency measurement

### Pass Criteria
- All wiring checks pass
- State '1' (safe): green LED on, motors off
- State '2' (warning): both LEDs, slow beep, motors off
- State '3' (ice): red LED, buzzer, motors reverse briefly
- Live monitoring correctly transitions between states
- Latency < 10ms (sensor read to motor activation)

---

## Final: Main Sketch (`main.ino`)

After all tests pass, upload `main.ino` for production use.

1. Power on the system
2. Place walker on dry ground
3. Press calibration button (or send 'C' via serial)
4. Wait for double-beep confirmation
5. Green LED indicates system is armed and surface is safe
6. Walk toward an icy/reflective surface to test detection

### Serial Commands in Production
- `C` — Recalibrate on current surface
- `S` — Print detailed status
- `R` — Print raw sensor readings
