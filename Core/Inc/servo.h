#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>

void servo_init(void);

bool servo_set_angle(float angle_degrees);

float servo_get_last_angle(void);

#endif