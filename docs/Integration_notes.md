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

### Test Results
- Predicted negative, but the platform doubled the tilt, fixed with the mounting constant.
- The servo's mounting reverses its direction relative to the IMU, which was captured in a single mounting constant rather than hiding it in the control logic.

## Stage 2: Fixed-rate loop
- Stage 1 loop's timing with HAL_Delay, period = work + delay. So the timing changes whenever the amount of work within the loop changes.
- Print only every 10th pass, so 10 lines a second.

### Timer
- TIM3: PSC = 83 (1 µs steps), ARR = 9,999 (10,000 steps = 10 ms)
- Timing budget: I2C read = ~0.7 ms + one print = ~2.2 ms = ~ 3 ms, which fits in the 10 ms period

### Test Results
- Servo reacts faster and moves more smoothly.
- A line prints 10 times a second.
- Slight jitter still present.

## Stage 3: Calibration
1. An offset that lives in imu.c which makes imu_get_angle() return calibrated angles.
2. Average 100 readings, which cuts the noise by about 10x, and at 10ms per reading, it takes about 1 second, which is a reasonable time to hold the board still at calibration.
3. Calibrate on button press, as the board may be held tilted at startup. At startup, the offset is 0 until the button is pressed.
4. Calibrate on a truly level surface, the platform treats the calibration surface as level.

### Test Results
- Calibration centres the tilt reading on ~0.0°.
- Recalibrating gives the same result instead of shifting, confirming the raw-angle design.
- Noise of about +/- 0.5° remains, to be addressed by the stage 4 filter.

## Stage 4: Filter
1. static variable, at file scope in imu.c.
2. The filtered value starts at the first reading. A static bool that indicates if the filter has started at the beginning.
3. Reset the filter after calibrating (mark it as "not started").
4. **Choosing α:** `filtered_new = filtered_old + α x (reading - filtered_old)`.
Each 10 ms pass moves the filtered value a fraction α towards the new reading.
- reading: calibrated angle from the IMU this pass.
- filtered_old: filter's output from the previous pass.
- filtered_new: this pass's output. Sent to the servo, and becomes filtered_old next pass.
- noise left = noise x √(α / (2 - α))
- lag = ~ (1 / α) x 10 ms

| α | Noise left (from +/- 0.5°) | Lag |
|---|---|---|
| 0.5 | +/- 0.29 ° | 20 ms |
| 0.2 | +/- 0.17 ° | 50 ms |
| 0.1 | +/- 0.11 ° | 100 ms |

Starting with α = 0.2 which removes about 2/3 of the noise with only 50 ms of lag.

### Test Results
- First version never started the filter (the "started" flag was set in the wrong branch), found in review and fixed.
- Still test, 10s each: unfiltered (α = 1.0) spread = ~0.72° (+/- 0.36°), filtered (α = 0.2) spread = ~0.37° (+/- 0.19°). Most filtered readings stay within +/- 0.06° of the mean.
- While still, the unfiltered servo command jumped between ~9 positions, with the filter, it stays at one position most of the time.
