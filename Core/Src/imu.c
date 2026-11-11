#include "imu.h"
#include <stdint.h>

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

// private helpers
static int16_t imu_combine_bytes(uint8_t byte_high, uint8_t byte_low){

}

static float imu_raw_to_g(int16_t reading) {
    
}