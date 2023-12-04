
#ifndef SPI_H_
#define SPI_H_ 

#include "board.h"
#include "UART.h"


uint8_t round_up_pwr2(uint8_t n);
uint8_t get_spi_prescaler_mask(uint8_t n);
void SPI_master_init(volatile SPI_t * SPI_addr, uint32_t clock_rate);
uint8_t SPI_transmit(volatile SPI_t* SPI_addr, uint8_t send_value, uint8_t *data);
uint8_t SPI_receive(volatile SPI_t *SPI_addr, uint8_t* data);
uint8_t SPI_transfer(volatile SPI_t *SPI_addr, uint8_t send_value, uint8_t *data);
void display_error(volatile UART_t * UART_addr, enum ErrorTypes error);

#endif 