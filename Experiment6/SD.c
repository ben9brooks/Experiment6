/*
 * SD.c
 *
 * Created: 10/16/2023 5:03:32 PM
 *  Author: Ben
 */ 
#include "board.h"
#include "SPI.h"
#include "SD.h"
#include "gpio_output.h"
#include <util/delay.h>
#include "sd_read.h"
#include "Directory_Functions_struct.h"

#define SD_CS_port (PB)
#define SD_CS_pin (1<<4)

/**************************************
*		Global Variables
***************************************/
uint32_t g_fat_start_sector, g_first_data_sector, g_root_dir_sectors, g_secPerClus, g_resvdSecCnt, g_bytsPerSec;

void SD_CS_active(volatile GPIO_port_t *port, uint8_t pin);
void SD_CS_inactive(volatile GPIO_port_t *port, uint8_t pin);

void SD_CS_active(volatile GPIO_port_t *port, uint8_t pin)
{
	GPIO_Output_Clear(port,pin);
}
void SD_CS_inactive(volatile GPIO_port_t *port, uint8_t pin)
{
	GPIO_Output_Set(port,pin);
}

uint8_t send_command (volatile SPI_t *SPI_addr, uint8_t command, uint32_t argument)
{
	//Maybe we make a separate function call to return an error? seems like error-checking is common and there should be a clean solution.
	uint8_t errorStatus = 0; // No error by default
    uint8_t checksum = 0x01; // Default checksum value
    uint8_t data; // Placeholder for received SPI data

	//1: check if command is 6 bits (<= 63). If not, error flag & function exits.
	
	if (command > 63)
	{
		//change this?
		return ERROR_SPI;
	}
	//2: command OR'd with 0x40 to append start and transmission bits to the first byte to send.
	command |= 0x40;
	//3: Send first byte using SPI_transfer. If error found from transfer, exit.
	errorStatus = SPI_transfer(SPI_addr, command, &data);
	if (errorStatus != 0) return errorStatus;
	//4: 32-bit arg sent, MSB first. Exit if error occurs.
	for (uint8_t i = 4; i > 0; i--) // Start from the MSB, i starts high
    {
		//this shifts right in multiples of 8. Since we can only send 8 bits, the first one (i=3) is shifted right 24b, sending the 8 MSBs first.
        errorStatus = SPI_transfer(SPI_addr, (argument >> (8 * (i-1))) & 0xFF, &data);
        if (errorStatus != 0) return errorStatus;
    }
	//5: checksum byte, lsb set to 1. If cmd is 0 or 8, checksum must be sent, otherwise 0x01 can be sent.
	//CMD0: 01 000000  or 0x40 in the first byte. We OR'd 0x40 so it's 01 and then the remaining 6 determines the cmd.
	//CMD8: 01 001000  or 0x48
	if (command == 0x40)
	{
		//CMD 0
		checksum = 0x95;
	}
	if (command == 0x48)
	{
		//CMD 8
		checksum = 0x87; //found in notes
	}

	// data is overwritten here, does that matter?
	errorStatus = SPI_transfer(SPI_addr, checksum, &data);
	
	//6: return error status
	return errorStatus;
}

// !!! The array_name parameter, when used, must be an array of defined size!
uint8_t receive_response (volatile SPI_t *SPI_addr, uint8_t number_of_bytes, uint8_t * array)
{
	uint8_t errorStatus = 0;
	uint8_t timeout = 0;
	uint8_t data=0;
	//size of response varies, can be 1-5 bytes. Response has short delay, 
	// 1. send 0xFF repeatedly, and keep reading the received value. This is all done using SPI_transfer. 
	//    continue until msb of received byte is 0 or timeout on the loop. If timed out, return error and send 0xFF.
	do
	{
		errorStatus = SPI_transfer(SPI_addr, 0xFF, &data); //SPI receive?
		timeout++;
	} while ( (data == 0xFF) && (timeout != 0) ); //data as 0xFF is an error in SPI_transfer
	// handle timeout errors:
	//timeout =0; 
	//RETURN_IF_ERROR(timeout, 0, ERROR_TIMEOUT); 
	if (timeout == 0)
	{
			return ERROR_TIMEOUT;
		}
	else if ( (data & 0xFE)	!= 0x00 ) //0x00 and 0x01 are good values
	{
		*array = data; //return value to see error
		return ERROR_SD;
	}
	else
	{
		//receive the remainder of the bytes, if present.
		// 2. If more than one byte expected, 0xFF sent out and each received byte stored in array. Repeat until all bytes received.
		*array = data;
		if(number_of_bytes>1)
		{
			//start at 1 bc just got index 0, 3 lines above this
			for(uint8_t i = 1; i <= number_of_bytes; i++)
			{
				errorStatus = SPI_transfer(SPI_addr, 0xFF, &data);
				array[i] = data;
			}
		}
	}
	
	// 3. an additional 0xFF byte should be sent after the entire response. Received value is irrelevant.
	errorStatus = SPI_transfer(SPI_addr, 0xFF, &data);
	// 4. return error value
	return errorStatus;
}

 uint8_t SD_init(volatile SPI_t *SPI_addr)
 {	
	 //init spi to master mode (can this be done externally?)
	 
	 uint8_t errorStatus = 0;
	 uint8_t data = 0;
	 uint32_t arg = 0x00000000;
     uint8_t receive_array[8] = {0,0,0,0,0,0,0,0};
	 uint32_t ACMD41_arg = 0x00000000;
	 uint32_t CMD16_arg = 0x00000200;
	 uint16_t timeout = 0;
	 
	 //set CS to 1 (inactive) (which is PB4)
	 SD_CS_inactive(PB, (1<<4));
	 //send 80 clock-cycles worth of transmits 
	 for(uint8_t i = 0; i < 8; i++)
	 {
		 errorStatus = SPI_transmit(SPI_addr, 0xFF, &data);
	 }

	 
	 
	 /************
     *
     *  CMD0
     *
     *************/
	 //set SS to 0 (active)
	 SD_CS_active(PB, (1<<4));

	 errorStatus = send_command(SPI_addr, CMD0, arg);
	 if (errorStatus == 0)
	 {
		 errorStatus = receive_response(SPI_addr, 1, &receive_array[0]);
		 //set CS to 1 (inactive) (which is PB4)
		 SD_CS_inactive(PB, (1<<4));
	 }
	 if(receive_array[0] != 0x01)
	 {
		 return ERROR_CMD0;
	 }

	/************
     *
     *  CMD8
     *
     *************/
	 //STEP C) send CM8, expecting R7. If voltage val != 0x01 or if check byte doesn't match, stop here.
	 SD_CS_active(PB, (1<<4));
	 errorStatus = send_command(SPI_addr, CMD8, 0x000001AA);
	 if(errorStatus == 0)
	 {
		//loop at receive all 5 bytes, starting at MSB i think
		errorStatus = receive_response(SPI_addr, 5, &receive_array[0]);
	 	SD_CS_inactive(PB, (1<<4));
	 }
	 
	 // if response is 0x05 (illegal cmd), flag it for later, bc it can't be high capacity (SDHC).
	 //check for R1 reponse
	 if((receive_array[0] == 0x01) && (errorStatus == 0))
	 {
		//expecting echo back of 0x01000001AA (first byte is R1)
		if((receive_array[3] == 0x01 ) && (receive_array[4] == 0xAA))
		{
			ACMD41_arg = 0x40000000; //high voltage, v2.0
		}
		else
		{
			return ERROR_VOLTAGE;
		}
	 }
	 else if(receive_array[0] == 0x05) //old card
	 {
		ACMD41_arg = 0x00000000; //v1.x
		//sd_card_type = ??
	 }
	 else
	 {
		return ERROR_CMD8;
	 }

	/**************************
	*
	* 	CMD58
	*
	**************************/
	SD_CS_active(PB, (1<<4));
	errorStatus = send_command(SPI_addr, CMD58, arg); 	
	
	//check error
	if (errorStatus != 0x00)
	{
		return ERROR_CMD58;
	}

	//receive R3 - R1 plus 32bit OCR. bit 30 of OCR should be a 1 for high-capacity. SPI clock freq can be increased if that passes.
	errorStatus = receive_response(SPI_addr, 5, &receive_array[0]);
	SD_CS_inactive(PB, (1<<4));
	//check for error
	if (errorStatus != 0x00)
	{
		return ERROR_CMD58;
	}
	//check for R3
	//check R1 + 32 bit OCR
	if(receive_array[0] != 0x01)
	{
		return ERROR_CMD58;
	}
	if((receive_array[2] & 0xFC) != 0xFC)
	{
		return ERROR_CMD58;
	}

	/**************************
	*
	* 	ACMD41  -- try a new sampling rate, not 10
	*
	**************************/
	
	while(receive_array[0] != 0x00)
	{
		SD_CS_active(PB, (1<<4));
		//send cmd55 first, receive R1, 
		errorStatus = send_command(SPI_addr, CMD55, arg);
		if(errorStatus != 0x00)
		{
			return ERROR_CMD55;
		}
		errorStatus = receive_response(SPI_addr, 1, &receive_array[0]);
		
		if(receive_array[0] != 0x01)
		{
			return ERROR_CMD55;
		}
		//then ACMD41 sent as CMD41 and R1 received all while CS=0.	Send ACMD41 until R1 is actually 0x00
		errorStatus = send_command(SPI_addr, CMD41, ACMD41_arg);
		if(errorStatus != 0x00)
		{
			return ERROR_CMD41;
		}
		errorStatus = receive_response(SPI_addr, 1, receive_array);
		if(errorStatus != 0x00)
		{
			return ERROR_CMD41;
		}
		timeout++;
		if (timeout == 0)
		{
			return ERROR_CMD41_TIMEOUT;
		}
		SD_CS_inactive(PB, (1<<4));
	}
	
	
	/**************************
	*
	* 	CMD58 again
	*
	**************************/
	SD_CS_active(PB, (1<<4));
	errorStatus = send_command(SPI_addr, CMD58, arg); 	
	
	//check error
	if (errorStatus != 0x00)
	{
		return ERROR_CMD58;
	}

	//receive R3 - R1 + 32bit OCR
	errorStatus = receive_response(SPI_addr, 5, &receive_array[0]);
	SD_CS_inactive(PB, (1<<4));
	//check for error
	if (errorStatus != 0x00)
	{
		return ERROR_CMD58;
	}
	//check for R3
	//check R1 + 32 bit OCR
	if((receive_array[1] & 0x80) != 0x80)
	{
		return ERROR_CMD58;
	}
	else if((receive_array[1] & 0xC0) != 0xC0)
	{
		return ERROR_CMD58;
	}

	/**************************
	*
	* 	CMD16 
	*
	**************************/
	// send CMD16 to define block size (512?)
	SD_CS_active(PB, (1<<4));
	errorStatus = send_command(SPI_addr, CMD16, CMD16_arg);
	if (errorStatus != 0)
	{
		return ERROR_CMD16;
	}
	//listen for R1 maybe
	errorStatus = receive_response(SPI_addr, 1, &receive_array[0]);
	
	if(receive_array[0] != 0x00)
	{
		return ERROR_CMD16;
	}
	SD_CS_inactive(PB, (1<<4));
	return errorStatus;
 }
 
uint8_t read_block (volatile SPI_t *SPI_addr, uint16_t number_of_bytes, uint8_t * array)
{
	uint8_t errorStatus = 0;
	uint8_t timeout = 0;
	uint8_t data=0;
	// step a
	do
	{
		errorStatus = SPI_transfer(SPI_addr, 0xFF, &data); //SPI receive?
		timeout++;
	} while(((data & 0x80) == 0x80) && (errorStatus == 0) && (timeout != 0));
	//while ( (data == 0xFF) && (timeout != 0) );

	// step b
	if (errorStatus != 0)
	{
		return ERROR_CMD0;
	}
	if(data != 0)
	{
		return ERROR_CMD8;
	}

	do
	{
		errorStatus = SPI_transfer(SPI_addr, 0xFF, &data); //SPI receive?
		timeout++;
	} while((data == 0xFF) && (errorStatus == 0) && (timeout != 0));
	//while ( (data == 0xFF) && (timeout != 0) );


	// check for 0xFE (success) or 0b0000XXXX (error)
	if (data == 0xFE)
	{
		// take first byte of data
		SPI_receive(SPI_addr, &data);
		array[0] = data;
	}
	else
	{
		//error
		return ERROR_TIMEOUT;
	}

	// step c
	for(uint16_t i = 1; i < number_of_bytes; i++)
	{
		errorStatus = SPI_transfer(SPI_addr, 0xFF, &data);
		array[i] = data;
	}

	// step d

	for(uint8_t i = 0; i < 3; i++)
	{
		errorStatus = SPI_transmit(SPI_addr, 0xFF, &data);
	}

	// step e
	return errorStatus;
}

uint8_t mount_drive(FS_values_t* fs)
{
	uint8_t array[512];
	uint32_t mbr_relative_sectors = 0;
	// a - read sector 0 into array
	if(read_sector(0, 512, array) != 0)
	{
		return 1; //error
	}
	
	// determine if 0 is MBR or BPB
	if (array[0] != 0xEB && array[0] != 0xE9)
	{
		//likely the MBR, read relative sectors value at 0x01C6
		mbr_relative_sectors = read_value_32(0x01C6, array);
		//printf();
		
		//read bpb sector into array
		if(read_sector(mbr_relative_sectors, 512, array) != 0)
		{
			return 2; //error
		}
	}	
	// verify BPB
	if(array[0] != 0xEB && array[0] != 0xE9)
	{
		return 3; //error, BPB not found
	}
	
	// b - read values and determine FAT type
	fs->BytesPerSec = read_value_16(11, array);
	fs->SecPerClus = read_value_8(13, array);
	uint16_t reservedSectorCount = read_value_16(14, array);
	uint8_t numFATs = read_value_8(16, array);
	uint16_t rootEntCnt = read_value_16(17, array);
	uint32_t totalSectors = read_value_16(19, array);
	
	if (totalSectors == 0)
	{
		totalSectors = read_value_32(32, array);
	}
	uint32_t fatSize = read_value_16(22, array);
	if (fatSize == 0)
	{
		fatSize = read_value_32(36, array);
	}
	
	uint32_t totalClusters = totalSectors / fs->SecPerClus;
	if (totalClusters < 65525)
	{
		fs->FATtype = 16;
	}
	else
	{
		fs->FATtype = 32;
	}
	
	// c - calulate starting sector numbers for FAT, 1st data sector, 1st root dir. Global vars.
	fs->StartofFAT = reservedSectorCount;
	if (fs->FATtype == 32)
	{
		fs->FirstRootDirSec = fs->StartofFAT + (numFATs * fatSize) + read_value_32(44, array);
	}
	else
	{
		fs->RootDirSecs = ((read_value_16(17,array) * 32) + (fs->BytesPerSec - 1)) / fs->BytesPerSec;
		fs->FirstRootDirSec = fs->StartofFAT + (numFATs * fatSize);
	}
	fs->FirstDataSec = fs->FirstRootDirSec + fs->RootDirSecs;
	
	//assign globals
	g_fat_start_sector = reservedSectorCount + mbr_relative_sectors;
	g_root_dir_sectors = ((rootEntCnt * 32) + (fs->BytesPerSec-1)) / (fs->BytesPerSec);
	g_first_data_sector = reservedSectorCount + (numFATs * fatSize) + g_root_dir_sectors + mbr_relative_sectors;
	g_secPerClus = fs->SecPerClus;
	g_resvdSecCnt = reservedSectorCount;
	g_bytsPerSec = fs->BytesPerSec;
	
	
	return 0; //success
}

uint32_t first_sector(uint32_t cluster_num)
{
	if(cluster_num == 0)
	{
		return g_first_data_sector;
	}
	else if(cluster_num <= 2)
	{
		return g_first_data_sector + cluster_num;
	}
	return ((cluster_num-2) * g_secPerClus) + g_first_data_sector;
}

uint32_t find_next_clus(uint32_t cluster_num, uint8_t array[])
{
	uint32_t FATOffset = (cluster_num * 4);
	// a
	uint32_t ThisFATSecNum = g_resvdSecCnt + (FATOffset / g_bytsPerSec);
	// b
	read_sector(ThisFATSecNum, 512, array);
	// c
	uint32_t ThisFATEntOffset = FATOffset % g_bytsPerSec;
	// d
	uint32_t temp32 = read_value_32(ThisFATEntOffset, array);
	// e
	temp32 &= 0x0FFFFFFF;
	// f
	// blank
	
	return temp32;
}

void print_file(uint32_t first_cluster, uint8_t *buffer) {
	uint32_t current_cluster = first_cluster;
	uint32_t current_sector;
	uint32_t sector_in_cluster = 0;

	while (1) {
		// Calculate first sector of the current cluster
		current_sector = first_sector(current_cluster);

		// Read and print the sector
		read_sector(current_sector + sector_in_cluster, 512, buffer);
		print_memory(buffer, 512); 

		// Print sector and cluster information for debugging
		//printf("Cluster: %lu, Sector: %lu\n", current_cluster, current_sector + sector_in_cluster);

		// User interaction
		UART_transmit_string(UART1, "Enter 0 to stop, 1 to continue:\n", 32);
		uint8_t user_input = long_serial_input(UART1);
		if (user_input == 0) {
			return; // Exit the loop if the user chooses to exit
		}

		// Move to the next sector
		sector_in_cluster++;
		if (sector_in_cluster >= g_secPerClus) {
			// Find the next cluster if all sectors in the current cluster are printed
			current_cluster = find_next_clus(current_cluster, buffer);
			if ((current_cluster == 0x00000007) || (current_cluster == 0x0FFFFFFF)) {
				return; // Exit if end of the file is reached
			}
			sector_in_cluster = 0; // Reset the sector counter for the new cluster
		}
	}
	return;
}
