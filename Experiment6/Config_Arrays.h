/*
 * Config_Arrays.h
 *
 * Created: 11/7/2023 6:28:15 PM
 *  Author: Ben
 */ 


#ifndef CONFIG_ARRAYS_H_
#define CONFIG_ARRAYS_H_

#include "twi.h"


uint8_t write_sta013_config(volatile TWI_t* TWI_addr);
uint8_t read_sta013_config(volatile TWI_t* TWI_addr);


#endif /* CONFIG_ARRAYS_H_ */