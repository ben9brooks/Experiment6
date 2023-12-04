/*
 * gpio_input.h
 *
 * Created: 9/11/2023 1:38:42 PM
 *  Author: tim tony ben
 */

#include "board.h"

#ifndef GPIO_INPUT_H
#define GPIO_INPUT_H

#define ENABLE 1 
#define DISABLE 0 
// Initializes GPIO Input by setting the bit's DDR value to 0 and adjusting the pull-up resistor.
void GPIO_Input_Init(volatile GPIO_port_t *port_addr, uint8_t pin_mask, uint8_t pull_up);
// Reads individual bits in the GPIO register.
uint8_t GPIO_Input_Read(volatile GPIO_port_t *port_addr, uint8_t pin_mask); 


#endif 