#include "tick.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

static volatile bool tick_pending = false;

void tick_init(void) {
    if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        tick_pending = true;
    }
}

bool tick_elapsed(void) {
    __disable_irq();
    bool tick_status = tick_pending; // read flag
    tick_pending = false; // clear flag
    __enable_irq();
    return tick_status;
}

