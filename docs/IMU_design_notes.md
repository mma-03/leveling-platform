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
Prove the ID, write PWR_MGMT_1 (PWR_MGMT_1_RESET, awake) and write ACCEL_CONFIG (ACCEL_CONFIG_RESET, +/- 2 g).
It should return true if working, false if something goes wrong: no device answers, wrong ID comes back, or a write fails. Main checks this and prints a failure message, keeps the servo centered, and doesn't level.

**Returning three values:** A C function can only return one value, but a reading has three values (X, Y , Z).
- a struct that groups the values
- an output parameter. The caller passes the address of a variable, and the function fills it in.

**Read all three axes in one conversation with the I2C.** This way, all three axes get data from one read, instead of two different readings being reported when you tilt the board between two separate reads. One conversion also costs one START one address and one register number instead of three each.

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
static int16_t imu_combine_bytes(uint8_t high, uint8_t low);

static float imu_raw_to_g(int16_t reading);
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
```