# Integration Notes

## Leveling the loop
1. Read the IMU. If the read fails, skip this cycle.
2. Compute the tilt angle.
3. Command the servo to the negative of the tilt.
4. If the servo had to clamp, report "out of range"

## Decisions
- **Sign:** the servo is commanded to the **negative** of the tilt, so it rotates opposite to the base and cancels the tilt.
- **Failed read:** skip the cycle. The PWM timer keeps sending the last pulse, so the servo holds its last position instead of jumping.
- **Out of range:** `servo_set_angle()` clamps to the safe limits and returns `false`. Report it over the serial for now (OLED later).

## Additional notes:
- The servo's mounting reverses its direction relative to the IMU, so I captured that in a single mounting constant rather than hiding it in the control logic.