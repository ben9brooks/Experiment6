/*
 * sEOS.c
 *
 * Created: 11/30/2023 10:16:37 AM
 *  Author: Ben
 */ 
#include <avr/interrupt.h>
#include <avr/io.h>

uint8_t Timer2_Interrupt_Init(uint8_t interval_ms)
{
	uint8_t OCR_value = 0;
	uint8_t error = 0;
	OCR_value = (uint8_t) (((interval_ms*(F_CPU/OSC_DIV)+(512000UL))/(1024000UL))-1);
	if(OCR_value <= 255)
	{
		OCR2A = OCR_value;
		TIMSK2 = 0x02; //enables ocra match interrupt
		TCCR2A = 0x02; //auto-reload counter with ocra
		TCCR2B = 0x07; //starts counter with prescale 1024
	}
	else
	{
		error=1;
	}
	return error;
}