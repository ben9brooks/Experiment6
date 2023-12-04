/*
 * SD.h
 *
 * Created: 10/16/2023 5:03:46 PM
 *  Author: Ben
 */ 
#ifndef SD_H_
#define SD_H_

#include <stdint.h>
#include "board.h"
#include "Directory_Functions_struct.h"

#define CMD0  (0x00)
#define CMD8  (0x08)
#define CMD58 (58U)
#define CMD55 (55U)
#define CMD41 (41U)
#define CMD16 (16U)
#define CMD17 (17U)

uint8_t send_command (volatile SPI_t *SPI_addr, uint8_t command, uint32_t argument);
uint8_t receive_response (volatile SPI_t *SPI_addr, uint8_t number_of_bytes, uint8_t * array_name);
uint8_t SD_init(volatile SPI_t *SPI_addr);
uint8_t read_block (volatile SPI_t *SPI_addr, uint16_t number_of_bytes, uint8_t * array);

//exp 5
uint8_t mount_drive(FS_values_t* fs);
uint32_t first_sector(uint32_t cluster_num);
uint32_t find_next_clus(uint32_t cluster_num, uint8_t array[]);
void print_file(uint32_t first_cluster, uint8_t *buffer);

#endif /* SD_H_ */