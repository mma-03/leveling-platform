# Designing Servo Module

## Convention
- Angles run -90° to +90°, with 0 = center (1500 µs)
- Lower pulse (- angle) = clockwise, Higher pulse (+ angle) = counterclockwise

## Converting angle to pulse width
- at 0 degrees, the pulse is 1500 µs
- each degree adds about 11.1 µs
- CCR only accepts whole numbers, so the conversion has to end with a whole number pulse width, so it needs rounding
- rounding uses lroundf, and the pulse is kept signed, so a large negative angle clamps to the minimum instead of wrapping around to the maximum

**Why float?** Angles come in decimals. The IMU reports tilts like 12.7°, and an int would throw away the fraction before the servo module sees it. The STM32F401 has a hardware FPU, so float math is fast.

## Clamping
- convert the requested angle to pulse width
- the pulse is clamped right before the CCR, as the safety net
- write it to CCR
- convert the clamped pulse back to an angle and store it as the last angle
- return false if the clamp changed the pulse

**Why clamp the pulse right before CCR?** It is the last step before hardware, so it protects the servo no matter how the pulse was calculated: a bug in math, bad sensor reading, etc.

## Functions
**Private (servo.c):**
```c
static int32_t angle_to_pulse(float angle_degrees); // helper to convert angle in degrees to pulse width
static float pulse_to_angle(int32_t pulse_us); // helper to convert pulse width to angle in degrees
```

**Public (servo.h):**
```c
void servo_init(void); // starts the servo signal and moves to center
bool servo_set_angle(float angle_degrees); // takes an angle parameter from the caller and sets it on the servo, returns true = set exactly as requested by caller, false = had to clamp
float servo_get_last_angle(void); // returns the last angle for callers that need it such as the oled in the future, if clamped, report the clamped degrees
```

**Why servo starts at center?** At power-up, the servo could be in any position. Center is a known and safe starting point in the middle of its range.

## Constants
```c
#define SERVO_CENTER_US 1500
#define SERVO_MAX_CLAMP_US 2400
#define SERVO_MIN_CLAMP_US 600
#define SERVO_US_PER_DEGREE 11.1f
```

## Encapsulation
**Private (inside servo.c):**
- timer handle
- channel
- calibration numbers
- `angle_to_pulse()`
- `pulse_to_angle()`

**Public:** 
- `servo_init()`
- `servo_set_angle()`
- `servo_get_last_angle()`

## Verification
**Setup:** NUCLEO-F401RE, MG90S servo on PA0 (TIM2 CH1), powered from a separate 5V 3A supply with a common ground. Results printed over UART at 115200 baud.

| Requested angle | Expected | Observed angle | Within range |
|---|---|---|---|
| 0° | center, in range | 0.000 | 1 |
| +45° | ~ 45°, counterclockwise | 45.045 | 1 |
| -45° | ~ -45°, clockwise | -44.955 | 1 |
| +120° | clamped to ~ +81° | 81.081 | 0 |
| -120° | clamped to ~ -81° | -81.081 | 0 |

**Note on the +/- 45° assymetry:** 45° x 11.1 µs/° = 499.5 µs, which isn't a whole number so the pulse has to be rounded. `lroundf` rounds a positive value ending in .5 up, so +45° becomes 1999.5 -> 2000 µs and -45° becomes 1000.5 -> 1001 µs. Rounding up moves +45° slightly away from center and -45° slightly towards center. Convering the pulses back gives +45.045° and -44.955°.