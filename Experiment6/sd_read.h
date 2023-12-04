/*
 * sd_read.h
 *
 * Created: 11/14/2023 4:39:23 PM
 *  Author: Ben
 */ 
#ifndef SD_READ_H_
#define SD_READ_H_


#include "UART.h"

uint8_t read_sector( uint32_t sector_number, uint16_t sector_size, uint8_t* data_array);
uint8_t read_value_8 (uint16_t offset, uint8_t array[]);
uint16_t read_value_16 (uint16_t offset, uint8_t array[]);
uint32_t read_value_32 (uint16_t offset, uint8_t array[]);


#endif /* SD_READ_H_ */