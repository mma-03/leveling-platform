#include "main.h"
#include "button.h"

static volatile bool press_pending = false; // flag

void button_init(void){
    // If needed later
}

// old: check if button is pressed
bool button_is_pressed(void){
    return (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET);
}

// callback function
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(GPIO_Pin ==  B1_Pin) {
        press_pending = true; // set flag
    }
}

// check if the button was pressed
bool button_was_pressed(void) {
    // Block interrupts so a press arriving between the read and the clear isn't lost
    __disable_irq(); // pause all interrupts
    bool press_status = press_pending; 
    press_pending = false;
    __enable_irq(); // turn interrupts back on
    return press_status;
}
