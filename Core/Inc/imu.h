#ifndef IMU_H
#define IMU_h

#include <stdbool.h>

typedef struct {
    float x;
    float y;
    float z;
} imu_reading_g_t;

bool imu_init(void);

bool imu_get_reading(imu_reading_g_t *reading);

float imu_get_angle(const imu_reading_g_t *reading);

#endif