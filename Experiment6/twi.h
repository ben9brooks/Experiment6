#ifndef TWI_H_
#define TWI_H_ 
#include "board.h"

#define TWEN_on (1<<2)
#define TWSTA_on (1<<5)
#define TWSTO_on (1<<4)
#define TWEA_ACK (1<<6)
#define TWEA_NACK (0<<6)
//#define TWINT (1<<7)

typedef struct TWI
{
    volatile uint8_t TWBR;
    volatile uint8_t TWSR;
    volatile uint8_t TWAR;
    volatile uint8_t TWDR;
    volatile uint8_t TWCR;
    volatile uint8_t TWAMR;
} TWI_t;

#define TWI0 ((volatile TWI_t *) 0xB8)
#define TWI1 ((volatile TWI_t *) 0xD8)

void nackCond(volatile TWI_t* TWI_addr);
void fullStopCond(volatile TWI_t* TWI_addr);
void startCond(volatile TWI_t* TWI_addr);
void ackCond(volatile TWI_t* TWI_addr);
uint8_t TWI_master_init(volatile TWI_t *TWI_addr, uint32_t I2C_freq);
uint8_t TWI_master_receive(volatile TWI_t *TWI_addr, uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t* arr);
uint8_t TWI_master_transmit(volatile TWI_t *TWI_addr, uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t* arr);

typedef enum 
{
    TWI_OK = 0,
    TWI_ERROR = 1 ,
	TWI_ERROR_TWO = 2,
	TWI_ERROR_TIMEOUT = 3,
	TWI_ERROR_BUS_BUSY = 4,
	TWI_ERROR_NACK = 5

}TWI_ERROR_CODES;

typedef enum
{
	TWSR_START_Cond = 0x08,
	TWSR_START_Cond_repeat = 0x10,
	TWSR_ARB = 0x38,
	TWSR_W_ACK_rcvd_int = 0x18,
	TWSR_W_NACK_rcvd_int = 0x20,
	TWSR_W_ACK_rcvd_data = 0x28,
	TWSR_W_NACK_rcvd_data = 0x30,
	TWSR_R_ACK_rcvd = 0x40,
	TWSR_R_NACK_rcvd = 0x48,
	TWSR_R_ACK_rtrnd = 0x50,  //data byte received, ack returned
	TWSR_R_NACK_rtrnd = 0x58  //data byte received, nack returned
	
}TWSR_ERROR_CODES;

#endif 