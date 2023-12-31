/*
 * Serial.h
 *
 * Created: 28.09.2015 09:30:45
 *  Author: Dag
 */ 
#ifndef SERIAL_H_
#define SERIAL_H_

#ifndef F_CPU
// define cpu clock speed if not defined
#define F_CPU 12000000UL
#endif

#include <avr/io.h>
#include <stdio.h>

// ** Set desired baud rate
#define BAUDRATE 9600

// ** Calculate UBRR value
#define UBRRVAL ((F_CPU/(BAUDRATE*16UL))-1)

void USART_Init(uint8_t comport);
void USART_Flush( uint8_t comport );
uint8_t USART_ReceiveByte(uint8_t comport);

char usart_getchar(uint8_t comport);
void usart_putchar(char data, uint8_t comport);

void usart_pstr(char *s, uint8_t comport);
unsigned char kbhit(uint8_t comport);

//int usart_putchar_printf(char var, FILE *stream, uint8_t comport);
int usart_putchar_printf(char var, FILE *stream);

/********************************************************************************
Global Variables
********************************************************************************/
static FILE mystdout = FDEV_SETUP_STREAM(usart_putchar_printf, NULL, _FDEV_SETUP_WRITE);
/***
 If there is a need to use both USARTs in the Attiny1634 device, 
 we'll have to create two individual STREAMS, one for USART0 and one for USART1.
 
 This can be done in the following way:
 static FILE stdoutUSART0 = FDEV_SETUP_STREAM(usart0_putchar_printf, NULL, _FDEV_SETUP_WRITE);
 static FILE stdoutUSART1 = FDEV_SETUP_STREAM(usart1_putchar_printf, NULL, _FDEV_SETUP_WRITE);
 // ... where each stream is connected to each usart putchar implementation
 //
 // So, before using the printf function for the actual usart, assign the 
 // appropriated pointer to stdout in order the route the print-out to terminal.
 stdout = &stdoutUSART0; // for output on USART0
 stdout = &stdoutUSART1; // for output on USART1
 
 // ... or use the static files as input to fprintf() function.
***/ 

#endif /* SERIAL_H_ */
