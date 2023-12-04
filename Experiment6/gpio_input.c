/*
 * gpio_input.c
 *
 * Created: 9/11/2023 1:38:42 PM
 *  Author: tim tony ben
 *  Author 2: younger
 */ 

#include "gpio_input.h"

// Initializes GPIO Input by setting the bit's DDR value to 0 and adjusting the pull-up resistor.
void GPIO_Input_Init(volatile GPIO_port_t *port_addr, uint8_t pin_mask, uint8_t pull_up)
{
  //set ddrx bit to make port pin an Input
  // set or clear port pin to enable/disable pull-up 
  port_addr->DDR_REG &= ~(pin_mask);
  if (pull_up == ENABLE )
  {
    port_addr->PORT_REG |= (pin_mask);
  } 
  else 
  {
    port_addr->PORT_REG &= ~(pin_mask);
  }
}

// Reads individual bits in the GPIO register.
uint8_t GPIO_Input_Read(volatile GPIO_port_t *port_addr, uint8_t pin_mask)
{
  if (((port_addr->PIN_REG) & pin_mask ) > 0)
  {
    return 1;   
  } 
  else
  {
    return 0;
  }
}
