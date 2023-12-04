/*
 * gpio_output.c
 *
 * Created: 9/11/2023 1:38:42 PM
 *  Author: tim tony ben
 */
#include "gpio_output.h"

//Sets DDRX bit to make port pin an output
void GPIO_Output_Init(volatile GPIO_port_t *port_addr, uint8_t pin_mask)
{
  
  port_addr->DDR_REG |= (pin_mask);
}

// Sets the GPIO output to 1
void GPIO_Output_Set(volatile GPIO_port_t *port_addr, uint8_t pin_mask)
{
  //dereferences port address' port register via struct, sets to 1 with |=
  port_addr->PORT_REG |= (pin_mask);
}

// Clears the GPIO output to 0
void GPIO_Output_Clear(volatile GPIO_port_t *port_addr, uint8_t pin_mask)
{
  ////dereferences port address' port register via struct, sets to 0 with &= ~()
  port_addr->PORT_REG &= ~(pin_mask);
} 


