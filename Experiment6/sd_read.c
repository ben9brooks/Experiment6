/*
 * sd_read.c
 *
 * Created: 11/14/2023 4:39:10 PM
 *  Author: Ben
 */ 

#include "SD.h"

uint8_t read_sector( uint32_t sector_number, uint16_t sector_size, uint8_t* data_array)
{
	// set CS active (low)
	SD_CS_active(PB, (1<<4));
	
	// send CMD 17 and sector number
	send_command(SPI0, CMD17, sector_number);
	
	// write data into array
	uint8_t error = read_block(SPI0, sector_size, data_array);
	if(error != 0)
	{
		return error; //error
	}
	
	// set CS inactive (high)
	SD_CS_inactive(PB, (1<<4));
	
	return 0; //success
}

uint8_t read_value_8 (uint16_t offset, uint8_t array[])
{
	return array[offset];
}
uint16_t read_value_16 (uint16_t offset, uint8_t array[])
{
	uint16_t value = 0;
	value |= array[offset];
	value |= (uint16_t)array[offset+1]<<8;
	return value;
}
uint32_t read_value_32 (uint16_t offset, uint8_t array[])
{
	uint32_t value = 0;
	value |= array[offset];
	value |= (uint32_t)array[offset+1]<<8;
	value |= (uint32_t)array[offset+2]<<16;
	value |= (uint32_t)array[offset+3]<<24;
	return value;
}