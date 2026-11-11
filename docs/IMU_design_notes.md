# Design Notes for the IMU module

## MPU-6500 datasheet analysis

### I2C
- address = 0x68
- WHO_AM_I address = 0x75
- WHO_AM_I expected value = 0x70

### PWR_MGMT_1
- address: 0x6B
- reset value: 0x01
- bit[6] is SLEEP, when set (1), the chip is set to sleep mode.

The chip starts awake, but the driver still sets this register, because resetting or reflashing the STM32 doesn't reset the IMU, so there could be a left over state from a previous program.

### Accelerometer Registers
| Register | Address |
|---|---|
| ACCEL_XOUT_H | 0x3B |
| ACCEL_XOUT_L | 0x3C |
| ACCEL_YOUT_H | 0x3D |
| ACCEL_YOUT_L | 0x3E |
| ACCEL_ZOUT_H | 0x3F |
| ACCEL_ZOUT_L | 0x40 |

Note: all 6 registers are consecutive, so one 6-byte read gets all three axes.

### Reading Acceleration
Each X, Y, Z measurement is a 16-bit number, but the registers only hold 8 bits, so each reading is split across two registers:
- a high byte (_H): the top 8 bits
- a low byte (_L): the bottom 8 bits
- to rebuild: shift the high byte left by 8 bits and combine it with the low byte.

### ACCEL_CONFIG
- address: 0x1C
- reset value: 0x00
- ACCEL_FS_SEL is bits 4 and 3 of ACCEL_CONFIG
- 0x00 = 0000 0000, so bits 4:3 = 00
- ACCEL_FS_SEL description: 00 = +/-2 g
- AFS_SEL = 0 -> 16,384 LSB/g
- by default, 16,384 raw counts = 1 g, 8,192 = 0.5 g

## Acceleration to tilt angle
When the sensor is still, the only acceleration it gets is gravity, 1 g pointing straight down.
- Lying flat: all of gravity falls on the Z axis, so Z reads about 1 g, and Y reads 0.
- Tilted by angle θ around the X axis: gravity is now split between Y and Z.
```
Y = 1 g x sin θ
Z = 1 g x cos θ
Y/Z = sin θ / cos θ = tan θ
θ = arctan(Y/Z)
```
Use `atan2f(y, z)` from `<math.h>`, which returns radians. Convert to degrees by multiplying 180/π.

The angle is only valid when the acceleration is gravity, since a sideways push also looks like a tilt. Future upgrade using gyroscope.

## Design

**What it needs to do:**
- At startup: prove it's talking to the right chip and put the chip into a known state (awake, +/- 2 g).
- Read the accelerometer: all three axes in one 6 byte read.
- Give back a tilt angle for the platform.


`bool imu_init(void)`:
Prove the ID, write PWR_MGMT_1 (PWR_MGMT_1_AWAKE, awake) and write ACCEL_CONFIG (ACCEL_CONFIG_2G, +/- 2 g).
It should return true if working, false if something goes wrong: no device answers, wrong ID comes back, or a write fails. Main checks this and prints a failure message, keeps the servo centered, and doesn't level.

**Returning three values:** A C function can only return one value, but a reading has three values (X, Y , Z).
- a struct that groups the values
- an output parameter. The caller passes the address of a variable, and the function fills it in.

**Read all three axes in one conversation with the I2C.** This way, all three axes get data from one read, instead of two different readings being reported when you tilt the board between two separate reads. One conversion also costs one START, one address, and one register number instead of three each.

**Reading vs. calculating:** Reading the sensor talks directly to the hardware and should be a separate function. Calculating is pure math that could be tested without any hardware at all.

## Functions

**Public:**
```c
typedef struct {
    float x;
    float y;
    float z;
} imu_reading_g_t;

bool imu_init(void);

bool imu_get_reading(imu_reading_g_t *reading);

float imu_get_angle(const imu_reading_g_t *reading);
```

**Private:**
```c
static int16_t imu_combine_bytes(uint8_t byte_high, uint8_t byte_low);

static float imu_raw_to_g(int16_t raw_count);
```

**Constants:**
```c
#define IMU_ADDRESS 0x68
#define IMU_ADDRESS_SHIFTED (IMU_ADDRESS << 1)
#define IMU_WHO_AM_I_ADDR 0x75
#define IMU_WHO_AM_I_EXPECTED 0x70
#define IMU_PWR_MGMT_1_ADDRESS 0x6B
#define IMU_PWR_MGMT_1_AWAKE 0x01
#define IMU_ACCEL_CONFIG_ADDRESS 0x1C
#define IMU_ACCEL_CONFIG_2G 0x00
#define IMU_ACCEL_LSB_PER_G 16384.0f
#define IMU_TIMEOUT_MS 100
#define IMU_ACCEL_XOUT_H 0x3B
#define IMU_ACCEL_DATA_LEN 6
#define IMU_RADIAN_TO_DEGREES_FACTOR (180.0f/3.14159265f)
```

## Verification

**Setup:** NUCLEO-F401RE, GY-521 (MPU-6500) on I2C1 (PB8 = SCL, PB9 = SDA) at 100 kHz, powered from the Nucleo's 3.3 V. Readings printed through UART every 200 ms.

**Procedure:** the board was held flat, then tilted by lifting each end of the breadboard, then returned flat.

### Results
| Position | X (g) | Y (g) | Z (g) | Angle | Expected |
| --- | --- | --- | --- | --- | --- |
| Flat | ~ -0.03 | ~ 0.01 | ~1.01 | ~ +0.5° | ~ 0°, Z = 1 |
| Top end lifted | ~ -0.04 | ~ -0.23 | ~ 0.98 | ~ -13° | (-) angle |
| Bottom end lifted | ~ 0.01 | ~ 0.42 | ~0.91 | ~ +24.7° | (+) angle |

**Axis identification:** X stayed roughly constant during both tilts, so the board was rotating around the X-axis. Y and Z changed, confirming `atan2f(y, z)` as the correct axes for this tilt direction.

**Consistency:** at the largest tilt (24.7°), sin(24.7°) = 0.418 and cos(24.7°) = 0.909, matching the measured Y = 0.42 g and Z = 0.91 g. The total magnitude √(0.42^2 + 0.91^2) = 1.0 g. This confirms that the I2C read -> byte combining -> raw to g conversion -> atan2f is all consistent and correct.

### Observations
- When flat, the angle reads about +0.5° rather than 0°, and Z reads about 1.015g rather than 1.000 g. This comes from the surface not being perfectly level and from the sensor's own internal bias. Planned fix: a calibration step that records the levelled angle and subtracts it from future readings.
- While not moving, the angle varies by about +/- 0.4°. If given directly to the servo, this would cause visible movement. Planned fix: a low-pass filter on the angle.
- The angle measures tilt around an axis across the width of the breadboard, so the servo shaft must be mounted along that same axis.

