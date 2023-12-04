/*
** Author: youngerr bby
** Date: 10/05/1492
*/
#include "SPI.h"
#include "board.h"
#include "GPIO_Outputs.h"
#include "UART_Print.h"


uint8_t round_up_pwr2(uint8_t n) {
    if (n <= 1) {
        return 1;
    }
    if (n > 128) {
        return 128;
    }
    n--; // the -- ensures that you don't round up when it's already a power of 2.
    n |= n >> 1; // ORs with 1 shift right
    n |= n >> 2; // ORs with 2-bit shift right
    n |= n >> 4; // ORs with 4-bit shift right
    n |= n >> 7; // ORs with 7-bit shit right
    return n + 1; // inc to power of 2 
}

uint8_t get_spi_prescaler_mask(uint8_t n) {
    uint8_t rounded_value = round_up_pwr2(n);
    
    switch (rounded_value) {
        case 4:   return 0b000;
        case 16:  return 0b001;
        case 64:  return 0b010;
        case 128: return 0b011;
        case 2:   return 0b100;
        case 8:   return 0b101;
        case 32:  return 0b110;
        default:  return 0b000; // Default case
    }
}

void SPI_master_init(volatile SPI_t * SPI_addr, uint32_t clock_rate)
{
    // CPOL CPHA - Clear CPOL and CPHA (0)
	// These are preference-based but must be unanimous with other code
    SPI_addr->control_reg &= (~(3<<2));
    // SPE - Enable SPI (1)
    SPI_addr->control_reg |= (1<<6);
    // MSTR - Set to master mode (1)
    SPI_addr->control_reg |= (1<<4);
    // DORD - Clear to make MSB first (0)
    SPI_addr->control_reg &= (~(1<<5));
    
    // Set clock rate based on the given `clock_rate`. You can use a series of if-else conditions to check which prescaler value to use.
    uint8_t divider = (F_CPU / OSC_DIV ) / (clock_rate);
    uint8_t mask = get_spi_prescaler_mask(divider);
    SPI_addr->control_reg |= (mask%4); // takes bottom 2 bits or mask & 0x3
    SPI_addr->status_reg |= (mask/4); // takes bit 2

    //add MOSI & SCK pins based on whether it's SPI0 or SPI1
    if(SPI_addr == SPI0)
    {
		GPIO_output_init(PB, (1<<5));
		GPIO_output_set(PB, (1<<5));
		
		GPIO_output_init(PB, (1<<7));
		GPIO_output_clear(PB, (1<<7));
		
        //PB->DDR_REG |= (1<<5) | (1<<7); //MOSI & SCK Output
        //PB->PORT_REG |= (1<<5);  //MOSI 1
        //PB->PORT_REG &= ~(1<<7); //SCK 0
    }
    else if (SPI_addr == SPI1)
    {
        PE->GPIO_DDR |= (1<<3);          // MOSI output
        PD->GPIO_DDR |= (1<<7);          // SCK output
        PE->GPIO_PORT |= (1<<3);         // MOSI 1
        PD->GPIO_PORT &= ~(1<<7);        // SCK 0
    }

}

uint8_t SPI_transmit(volatile SPI_t* SPI_addr, uint8_t send_value, uint8_t *data)
{
    // init var for loop
    uint8_t status;
    uint16_t timeout = 0;
    // write data to spider
    SPI_addr->data_reg = send_value;
    //wait for spif (bit 7) to be 0, this means SPDR can be written again
    do
    {
        status = (SPI_addr->status_reg);
        timeout++;
    } while (((status&0x80) == 0) && timeout != 0 );

    if(timeout == 0)
    {
        *data = 0xFF;
        return ERROR_TIMEOUT;
    }
    else if ((status&0x40)!=0)
    {
        *data = (SPI_addr->data_reg);
        return ERROR_SPI;
    }
    else 
    {
        *data = (SPI_addr->data_reg);
        return 0;
    }
}
uint8_t SPI_receive(volatile SPI_t *SPI_addr, uint8_t* data)
{
    // init var for loop
    uint8_t status;
    uint16_t timeout = 0;
    // write data to spider
    SPI_addr->data_reg = 0xFF;
    //wait for spif (bit 7) to be 0, this means SPDR can be written again
    do
    {
        status = (SPI_addr->status_reg);
        timeout++;
    } while (((status&0x80) == 0) && timeout != 0 );

    if(timeout == 0)
    {
        *data = 0xFF;
        return ERROR_TIMEOUT;
    }
    else if ((status&0x40)!=0)
    {
        *data = (SPI_addr->data_reg);
        return ERROR_SPI;
    }
    else 
    {
        *data = (SPI_addr->data_reg);
        return 0;
    }
}
uint8_t SPI_transfer(volatile SPI_t *SPI_addr, uint8_t send_value, uint8_t *data)
{

    // init var for loop
    uint8_t status;
    uint16_t timeout = 0;
    // write data to spider
    SPI_addr->data_reg = send_value;
    //wait for spif (bit 7) to be 0, this means SPDR can be written again
    do
    {
        status = (SPI_addr->status_reg);
        timeout++;
    } while (((status&0x80) == 0) && timeout != 0 );

    if(timeout == 0)
    {
        *data = 0xFF;
        return ERROR_TIMEOUT;
    }
    else if ((status&0x40)!=0)
    {
        *data = (SPI_addr->data_reg);
        return ERROR_SPI;
    }
    else 
    {
        *data = (SPI_addr->data_reg);
        return 0;
    }
}

void display_error(volatile UART_t * UART_addr, enum ErrorTypes error)
{
	switch(error)
	{
		case ERROR_TIMEOUT:
			UART_transmit_string(UART1, "timeout\n", 8);
			break;
		case ERROR_SPI:
			UART_transmit_string(UART1, "SPI\n", 4);
			break;
		case ERROR_SD:
			UART_transmit_string(UART1, "SD\n", 3);
			break;
		case ERROR_VOLTAGE:
			UART_transmit_string(UART1, "VOLTAGE!!!!\n", 12);
			break;
		case ERROR_CMD0:
			UART_transmit_string(UART1, "CMD0\n", 5);
			break;
		case ERROR_CMD8:
			UART_transmit_string(UART1, "CMD8\n", 5);
			break;
        case ERROR_CMD58:
			UART_transmit_string(UART1, "CMD58\n", 6);
			break;
        case ERROR_CMD55:
			UART_transmit_string(UART1, "CMD55\n", 6);
			break;
        case ERROR_CMD41:
			UART_transmit_string(UART1, "CMD41\n", 6);
			break;
        case ERROR_CMD41_TIMEOUT:
			UART_transmit_string(UART1, "CMD41TIME\n", 10);
			break;
		case ERROR_TOKEN:
			UART_transmit_string(UART1, "TOKEN\n", 6);
			break;
		case ERROR_CMD16:
			UART_transmit_string(UART1, "CMD16\n", 6);
			break;
		default:
			UART_transmit_string(UART1, "impossible\n", 11);
			
	}
}