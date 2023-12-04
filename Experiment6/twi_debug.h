/*
 * twi_debug.h
 *
 * Created: 11/2/2023 10:33:37 AM
 *  Author: Ben
 */ 


#ifndef TWI_DEBUG_H_
#define TWI_DEBUG_H_


uint8_t initialize_sta013();
uint8_t sta_debug_test();
uint8_t write_sta013(volatile TWI_t* TWI_addr, uint32_t base_addr, uint8_t value);

void printError(uint8_t err);


#endif /* TWI_DEBUG_H_ */