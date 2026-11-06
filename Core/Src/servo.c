#include "servo.h"
#include "main.h"
#include <stdint.h>
#include <math.h>

#define SERVO_CENTER_US 1500
#define SERVO_MAX_CLAMP_US 2400
#define SERVO_MIN_CLAMP_US 600
#define SERVO_US_PER_DEGREE 11.1f

extern TIM_HandleTypeDef htim2;

static float s_last_angle_degrees = 0.0f;
// private helper functions
static int32_t angle_to_pulse(float angle_degrees) {
    return lroundf(SERVO_CENTER_US + (angle_degrees * SERVO_US_PER_DEGREE));
}

static float pulse_to_angle(int32_t pulse_us) {
    return (pulse_us - SERVO_CENTER_US) / SERVO_US_PER_DEGREE;
}

// public functions
void servo_init(void) {
    servo_set_angle(0.0f); 

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
}

bool servo_set_angle(float angle_degrees) {
    int32_t pulse_us = angle_to_pulse(angle_degrees);

    bool within_range = true;

    if (pulse_us > SERVO_MAX_CLAMP_US) {
        pulse_us = SERVO_MAX_CLAMP_US;
        within_range = false;
    } else if (pulse_us < SERVO_MIN_CLAMP_US) {
        pulse_us = SERVO_MIN_CLAMP_US;
        within_range = false;
    }

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse_us);

    s_last_angle_degrees = pulse_to_angle(pulse_us);

    return within_range;
}

float servo_get_last_angle(void) {
    return s_last_angle_degrees;
}