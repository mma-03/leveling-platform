#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "serial.h"

extern UART_HandleTypeDef huart2;

void serial_init(void) {
    // if needed later
}

bool serial_printf(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    return (HAL_UART_Transmit(&huart2, (const uint8_t *)buffer, strlen(buffer), 100) == HAL_OK);
}


