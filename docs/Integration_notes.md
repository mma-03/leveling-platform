# Integration Notes

## Stage 1: Leveling loop
1. Read the IMU. If the read fails, skip this cycle.
2. Compute the tilt angle.
3. Command the servo to the negative of the tilt x mounting constant
4. If the servo had to clamp, report "out of range"

### Decisions
- **Sign:** the servo counter-rotates (-tilt), multiplied by the mounting constant, because of how the servo is mounted.
- **Failed read:** skip the cycle. The PWM timer keeps sending the last pulse, so the servo holds its last position instead of jumping.
- **Out of range:** `servo_set_angle()` clamps to the safe limits and returns `false`. Report it over the serial for now (OLED later).

### Test results
- Predicted negative, but the platform doubled the tilt, fixed with the mounting constant.
- The servo's mounting reverses its direction relative to the IMU, which was captured in a single mounting constant rather than hiding it in the control logic.

## Stage 2: Fixed-rate loop
- Stage 1 loop's timing with HAL_Delay, period = work + delay. So the timing changes whenever the amount of work within the loop changes.
- Print only every 10th pass, so 10 lines a second.

### Timer
- TIM3: PSC = 83 (1 µs steps), ARR = 9,999 (10,000 steps = 10 ms)
- Timing budget: I2C read = ~0.7 ms + one print = ~2.2 ms = ~ 3 ms, which fits in the 10 ms period

### Test Results
- Servo reacts faster and moves more smoothly
- A line prints 10 times a second
- Slight jitter still present

## Satge 3: Calibration
1. An offset that lives in imu.c which makes imu_get_angle() return calibrated angles.
2. Average 100 readings, which cuts the noise about 10x less, and at 10ms per reading, it takes about 1 second, which is a reasonable time to hold the board still at calibration.
3. Calibrate on button press, as the board maybe held tilted at startup. At startup, the offset is 0 until the button is pressed.