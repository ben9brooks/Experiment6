/*
 * gpio_output.h
 *
 * Created: 9/11/2023 1:38:42 PM
 *  Author: tim tony ben
 */
#include "board.h"

#ifndef GPIO_OUTPUT_H
#define GPIO_OUTPUT_H
//Sets DDRX bit to make port pin an output
void GPIO_Output_Init(volatile GPIO_port_t * port_addr, uint8_t pin_mask);
// Sets the GPIO port output to 1
void GPIO_Output_Set(volatile GPIO_port_t *port_addr, uint8_t pin_mask);
// Clears the GPIO port output to 0
void GPIO_Output_Clear(volatile GPIO_port_t *port_addr, uint8_t pin_mask); 

#endif
