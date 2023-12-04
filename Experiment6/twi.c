#include "twi.h"

void nackCond(volatile TWI_t* TWI_addr)
{
	TWI_addr->TWCR = ((1<<TWINT) | (0<<TWEA) | (1<<TWEN));
}

void fullStopCond(volatile TWI_t* TWI_addr)
{
	uint8_t status;
	uint8_t timeout = 200;
	TWI_addr->TWCR = ((1<<TWINT) | (1<<TWSTO) | (1<<TWEN));
	do 
	{
		status = TWI_addr->TWCR;
		//timeout--;
	} while (((status& 0x10) != 0) && (timeout != 0));
}

void startCond(volatile TWI_t* TWI_addr)
{
	TWI_addr->TWCR = ((1<<TWINT) | (1<<TWSTA) | (1<<TWEN));	
}

void ackCond(volatile TWI_t* TWI_addr)
{
	TWI_addr->TWCR = ((1<<TWINT) | (1<<TWEA) | (1<<TWEN));	
}
    
uint8_t TWI_master_init(volatile TWI_t *TWI_addr, uint32_t I2C_freq)
{
    //TWI_ERROR_CODES error = TWI_OK; 
    uint8_t twps_val;
	uint32_t prescale;
	prescale = (F_CPU/OSC_DIV);
	prescale = prescale/I2C_freq;
	prescale = prescale - 16UL;
	prescale = prescale/(2UL*255);
    //uint32_t prescale = (((F_CPU/OSC_DIV)/I2C_freq)-16UL)/(2UL*255);
    if (prescale < 1) {
		prescale = 1;
        twps_val = 0x00;
    } else if (prescale < 4) {
		prescale = 4;
        twps_val = 0x01;
    } else if (prescale < 16) {
		prescale = 16;
        twps_val = 0x02;
    } else if (prescale < 64) {
		prescale = 64;
        twps_val = 0x03;
    } else {
        return TWI_ERROR; // fail
    }

    TWI_addr->TWSR = twps_val;

    uint8_t TWBR=(((F_CPU/OSC_DIV)/I2C_freq)-16UL)/(2UL*prescale);
    if (TWBR >= 256)
    {
        return TWI_ERROR_TWO; // fail
    }

    TWI_addr->TWBR = TWBR;
    
    return 0;
}

uint8_t TWI_master_receive(volatile TWI_t *TWI_addr, uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t* arr)
{
	// 200us 5.00MS/s    D10 X
	// ???   10k points   1.49V
	// Type=Edge, Source=D10
	//general layout for a receive: START condition, Device Address, receive an ACK, then receive data byte(s) with an ACK after each one, except the last is a NACK, plus a STOP condition.
	uint8_t status;
	uint8_t temp8;
	uint8_t send_value;
	//uint8_t rcvd_arr[10];
	uint8_t index;
	//internal address is optional and a bonus, along with int_addr_sz
	
	
	//send device address with a 1 in LSB (SLA+R). LSB being a 1 means read, see lecture
	send_value = (device_addr<<1) | 0x01;
	
	//create start condition (writes to TWCR, TWINT set, TWSTA start cond set, write 1 to TWEN To enable TWI 
	startCond(TWI_addr);
	//TWI_addr->TWCR = ((1<<TWINT) | (1<<TWSTA) | (1<<TWEN));	
	
	//wait for TWINT (bit 7) to be set in TWCR
	do 
	{
		status = TWI_addr->TWCR;
	} while ((status&0x80)==0);
	
	//read status
	temp8 = ((TWI_addr->TWSR)&0xF8); //clear lower 3 bits
	
	//if start sent, then send SLA+R (temp8/status can be start or repeated start condition)
	if((temp8 == TWSR_START_Cond) || (temp8 == TWSR_START_Cond_repeat)) //0x08 0x10
	{
		TWI_addr->TWDR = send_value;
		TWI_addr->TWCR = ((1<<TWINT) | (1<<TWEN));
	}
	//can check for errors here?
	else
	{
		return TWI_ERROR_BUS_BUSY;
	}
	
	
	/************
	*
	* Read First Byte
	*
	************/
	
	// Wait for TWINT to be set indicating transmission of SLA+R and reception of ACK/NACK
	do 
	{
		status = TWI_addr->TWCR;
	} while ((status & 0x80) == 0);

	//read status
	temp8 = ((TWI_addr->TWSR)&0xF8); //clear lower 3 bits
	
	//receive ACK From slave (write 1 to TWEA, bit 6 of TWCR, when ACK should be sent after receiving data from slave)
	if(temp8 == TWSR_R_ACK_rcvd) //SLA+R sent, ACK received  0x40
	{
		//be prepped to send stop cond if only 1 bit received
		// if 1 byte received, send NACK to slave ( write 0 to TWEA)
		if(num_bytes == 1)
		{
			nackCond(TWI_addr);
		}
		// if >1 byte received, send ACK after all but the last byte.
		else
		{
			ackCond(TWI_addr);
		}

		//send all data bytes until all bytes sent or error
		index = 0;
		while((num_bytes != 0) && (1!=2)) //put error check here..
		{
			//wait for TWINT to be set
			do 
			{
				status = TWI_addr->TWCR;
			} while ((status&0x80)==0);

			//read status
			temp8 = ((TWI_addr->TWSR)&0xF8); // clear lower 3 bits
			
			//i think this is nested inside this loop?
			if(temp8 == TWSR_R_ACK_rtrnd) //data byte received, ack sent back  0x50
			{
				num_bytes--;
				arr[index] = TWI_addr->TWDR;
				index++;
				if(num_bytes == 1)
				{
					nackCond(TWI_addr);  //TWEA=0
				}
				else
				{
					ackCond(TWI_addr); //TWEA=1
				}
			}
			else if (temp8 == TWSR_R_NACK_rtrnd)
			{
				//save byte to array, dec num_bytes
				num_bytes--;
				arr[index] = TWI_addr->TWDR;
				
				//write 1 to TWSTO (bit 4) to request stop condition
				//fullStopCond(TWI_addr);
				
			}
			
		}
		fullStopCond(TWI_addr);
	}
	else //NACK at the start is not expected, we didn't get to receive anything.
	{
		
		if (temp8 == TWSR_R_NACK_rcvd)
		{
			fullStopCond(TWI_addr);
			return TWI_ERROR_NACK;
		}
		if (temp8 == TWSR_ARB)
		{
			TWI_addr->TWCR = ((1<<TWINT) | (1<<TWEN));
		}
	}
	return 0;
}

uint8_t TWI_master_transmit(volatile TWI_t *TWI_addr, uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t* arr)
{
	uint8_t status;
	uint8_t temp8;
	uint8_t send_value;
	uint8_t index;

	//send device address with a 0 in LSB (SLA+W). LSB being a 1 means write, see lecture
	send_value = (device_addr<<1);

	//create start condition (writes to TWCR, TWINT set, TWSTA start cond set, write 1 to TWEN To enable TWI 
	startCond(TWI_addr);

	//wait for TWINT (bit 7) to be set in TWCR
	do 
	{
		status = TWI_addr->TWCR;
	} while ((status&0x80)==0);

	//read status
	temp8 = ((TWI_addr->TWSR)&0xF8); //clear lower 3 bits

	//if start sent, then send SLA+W (temp8/status can be start or repeated start condition)
	if((temp8 == TWSR_START_Cond) || (temp8 == TWSR_START_Cond_repeat)) //0x08 0x10
	{
		TWI_addr->TWDR = send_value;
		TWI_addr->TWCR = ((1<<TWINT) | (1<<TWEN));
	}
	//can check for errors here?
	else
	{
		return TWI_ERROR_BUS_BUSY;
	}

	// Wait for TWINT to be set indicating transmission of SLA+W and reception of ACK/NACK
	do 
	{
		status = TWI_addr->TWCR;
	} while ((status & 0x80) == 0);

	//read status
	temp8 = ((TWI_addr->TWSR)&0xF8); //clear lower 3 bits

	/************
	*
	* INT ADDR
	*
	************/

	//receive ACK From slave (write 1 to TWEA, bit 6 of TWCR, when ACK should be sent after receiving data from slave)
	if(((TWI_addr->TWSR) &0xF8)== TWSR_W_ACK_rcvd_int) //SLA+W sent, ACK received 0x18
	{
		// send internal address to TWDR (0-4 bytes)
		for(uint8_t i = 0; i < int_addr_sz; i++)
		{
			//send byte(s), MSB first
			TWI_addr->TWDR = (int_addr >> (8 * (int_addr_sz - i - 1))) & 0xFF;
			TWI_addr->TWCR = ((1<<TWINT) | (1<<TWEN));

			//wait for TWINT
			do 
			{
				status = TWI_addr->TWCR;
			} while ((status & 0x80) == 0);

			//receive ACK from slave 0x28
			//read status
			temp8 = ((TWI_addr->TWSR)&0xF8); //clear lower 3 bits

			//receive ACK From slave -> break if NACK received
			if(temp8 == TWSR_W_NACK_rcvd_int)
			{
				fullStopCond(TWI_addr);
				return TWI_ERROR_NACK;
			}
			if(temp8 == TWSR_ARB)
			{
				return TWI_ERROR;
			}
		}

		/************
		*
		* SEND DATA 
		*
		************/


		//be prepped to send stop cond if only 1 bit received
		// if 1 byte received, send NACK to slave ( write 0 to TWEA)
		if(num_bytes == 0)
		{
			fullStopCond(TWI_addr);
			return 0;
		}
		if(num_bytes == 1)
		{
			ackCond(TWI_addr);
		}
		// if >1 byte received, send ACK after all but the last byte.
		else
		{
			ackCond(TWI_addr);
		}

		//send all data bytes until all bytes sent or error
		index = num_bytes-1;
		while((num_bytes != 0) && (1!=2)) //put error check here..
		{
			//wait for TWINT to be set
			do 
			{
				status = TWI_addr->TWCR;
			} while ((status&0x80)==0);

			//read status
			temp8 = ((TWI_addr->TWSR)&0xF8); // clear lower 3 bits
			
			//i think this is nested inside this loop?
			if(temp8 == TWSR_W_ACK_rcvd_data) //data byte received, ack sent back
			{
				num_bytes--;
				TWI_addr->TWDR = arr[index];
				index--;
				if(num_bytes == 0)
				{
					fullStopCond(TWI_addr);
					return 0;
					//nackCond(TWI_addr);
				}
				else
				{
					ackCond(TWI_addr);
				}
			}
			else if (temp8 == TWSR_W_NACK_rcvd_data)
			{
				//save byte to array, dec num_bytes
				num_bytes--;
				arr[index] = TWI_addr->TWDR;
				
				//write 1 to TWSTO (bit 4) to request stop condition
				fullStopCond(TWI_addr);
				
				//wait for twsto to return to 0
				do 
				{
					status = TWI_addr->TWCR;
				} while ((status&0x10) != 0);
				
			}
			else if (temp8 == TWSR_ARB)
			{
				return TWI_ERROR;
			}
			
		}
		fullStopCond(TWI_addr);
	}
	else //NACK at the start is not expected, we didn't get to receive anything.
	{
		fullStopCond(TWI_addr);
		if (temp8 == TWSR_W_NACK_rcvd_int)
		{
			return TWI_ERROR_NACK;
		}
		else
		{
			return TWI_ERROR;
		}
	}
	return 0;
}