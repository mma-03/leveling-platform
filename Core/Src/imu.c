#include "imu.h"
#include "main.h"
#include <stdint.h>
#include <math.h>

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
#define IMU_BUFFER_SIZE 6
#define IMU_RADIAN_TO_DEGREES_FACTOR (180/M_PI)

extern I2C_HandleTypeDef hi2c1;
// private helpers
static int16_t imu_combine_bytes(uint8_t byte_high, uint8_t byte_low){
    uint16_t combined = (byte_high << 8) | byte_low;
    return (int16_t)combined;
}

static float imu_raw_to_g(int16_t reading) {
    return (reading / IMU_ACCEL_LSB_PER_G);
}

// public functions
bool imu_init(void){
    uint8_t who_am_i;
    if ((HAL_I2C_Mem_Read(&hi2c1, IMU_ADDRESS_SHIFTED, IMU_WHO_AM_I_ADDR, I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, IMU_TIMEOUT_MS) != HAL_OK) || (who_am_i != IMU_WHO_AM_I_EXPECTED)) {
        return false;
    }

    uint8_t pwr_mgmt_1_awake = IMU_PWR_MGMT_1_AWAKE;
    uint8_t accel_config_2g = IMU_ACCEL_CONFIG_2G;

    if ((HAL_I2C_Mem_Write(&hi2c1, IMU_ADDRESS_SHIFTED, IMU_PWR_MGMT_1_ADDRESS, I2C_MEMADD_SIZE_8BIT, &pwr_mgmt_1_awake, 1, IMU_TIMEOUT_MS) != HAL_OK) ||
    (HAL_I2C_Mem_Write(&hi2c1, IMU_ADDRESS_SHIFTED, IMU_ACCEL_CONFIG_ADDRESS, I2C_MEMADD_SIZE_8BIT, &accel_config_2g, 1, IMU_TIMEOUT_MS) != HAL_OK)) {
        return false;
    }

    return true;
}

bool imu_get_reading(imu_reading_g_t *reading){
    uint8_t buffer[IMU_BUFFER_SIZE];
    if (HAL_I2C_Mem_Read(&hi2c1, IMU_ADDRESS_SHIFTED, IMU_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, buffer, sizeof(buffer), IMU_TIMEOUT_MS) != HAL_OK) {
        return false;
    }

    reading->x = imu_raw_to_g(imu_combine_bytes(buffer[0], buffer[1]));
    reading->y = imu_raw_to_g(imu_combine_bytes(buffer[2], buffer[3]));
    reading->z = imu_raw_to_g(imu_combine_bytes(buffer[4], buffer[5]));

    return true;
}

float imu_get_angle(const imu_reading_g_t *reading){
    float angle_radian = atan2f(reading->y, reading->z);
    float angle_degrees = (angle_radian)/IMU_RADIAN_TO_DEGREES_FACTOR;
    return angle_degrees;
}