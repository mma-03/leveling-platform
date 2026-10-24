#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>

void serial_init(void);

bool serial_printf(const char* format, ...) __attribute__((format(printf, 1, 2)));

#endif