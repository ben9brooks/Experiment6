/*
 * main.c
 *
 * Created: 10/5/2021 3:06:09 PM
 * Author : grogu
 */ 

#include <avr/io.h>
#include "board.h"
#include "GPIO_Outputs.h"
#include "gpio_output.h"
#include "LEDS.h"
#include "UART.h"
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "UART_Print.h"
#include "print_memory.h"
#include "Long_Serial_In.h"
#include <stdio.h>

#include "twi.h"
#include "twi_debug.h"
#include "Config_Arrays.h"

#include "sd_read.h"
#include "SD.h"
#include "SPI.h"

#include <avr/interrupt.h>
#include "STA013_Config.h"


const char test_string[15] PROGMEM = "Hello World!\n\r";

int main(void)
{
	//TWI vars
	uint8_t error = 0;
	uint32_t i2c_freq = 50000; /* 100k */ 
	uint8_t array[3] = {0,0,0};
	uint8_t timer = 10;
	//SD/FAT vars
	uint8_t numEntries = 0;
	uint32_t userDirNum = 0;
	uint32_t userClusNum = 0;
	uint8_t mem_block[512];
	enum ErrorTypes typederror = 0;
	
	/************
	*
	* Exp #4 TWI - Initialize STA013
	*
	************/
	UART_init(UART1, 9600); /* baud = 9600 */ 

	error = TWI_master_init(TWI1, i2c_freq); 
	if (error != 0)
	{
		UART_transmit_string(UART1, "twi_init_fail\n", 14);
	}

	do 
	{
	 	error = TWI_master_receive(TWI1, 0x43, 0, 0, 3, array);
	 	timer--;
	} while ((error > 0) && (timer>0));
	printError(error);
	STA013_Init();
	
	sta_debug_test();
	_delay_ms(1000);

	UART_transmit_string(UART1, "\nEND\n", 5);

	/************
	*
	* Exp #5 FAT - Read SD card
	*
	************/
	
	SPI_master_init(SPI0, 400000U);
	
	/* initialize SS AKA CS */
	GPIO_Output_Init(PB, (1<<4));
	
	do
	{
		typederror = SD_init(SPI0);
		if (typederror != 0)
		{
			display_error(UART1, typederror);
		}
	}while(typederror != 0);
	
	FS_values_t* accessor_fileSystem = export_drive_values();
	FS_values_t file_system;
	mount_drive(&file_system);
	
	/* Fills the file system struct to match */
	*accessor_fileSystem = file_system;
	
	uint32_t FirstRootDirSector = first_sector(0);

	/* SPI can be reinitialized at a faster freq, now that the SD has been initialized. */
	SPI_master_init(SPI0, 8000000U);
	
	while (1)
	{
		typederror = read_sector(FirstRootDirSector, 512, mem_block);
		
		if(typederror != 0)
		{
			display_error(UART1, typederror);
			break;
		}
		
		
		userDirNum = FirstRootDirSector;
		while(1)
		{
			numEntries = print_directory(userDirNum, mem_block);
			UART_transmit_string(UART1, "Entry Number:\n", 14);
			userDirNum = long_serial_input(UART1);
			
			while(userDirNum > numEntries)
			{
				UART_transmit_string(UART1, "Invalid Entry Number. Provide a new one:\n", 41);
				userDirNum = long_serial_input(UART1);
			}
			
			userClusNum = read_dir_entry(FirstRootDirSector, userDirNum, mem_block);
			
			/* For Directory */
			if((userClusNum & 0x10000000) != 0)
			{
				userClusNum &= 0x0FFFFFFF; /* mask upper 4 off */
				userDirNum = first_sector(userClusNum);
				FirstRootDirSector = userDirNum;
			}
			/* For File */
			else
			{
				print_file(userClusNum, mem_block);
				userDirNum = FirstRootDirSector;
			}
		}
		
	}
}


