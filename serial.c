/*
 * serial.c
 *
 * Created: 28.09.2015 09:28:05
 *  Author: Dag
 *
 *
 * 
 *
 */ 
#include "serial.h"

void USART_Init(uint8_t comport) {
   if (comport==0) {
      //Set baud rate
      UBRR0L=(uint8_t)UBRRVAL;   // low byte
      UBRR0H=(UBRRVAL>>8);			// high byte
   
      // ** Enable Transmitter and Receiver
      UCSR0B = ((1<<RXEN0)|(1<<TXEN0));
      UCSR0B |= (1<<RXCIE0); //RX Complete Interrupt Enable
      // UCSZ02 must be '0' for 8 bit operation - this is the initial value of this bit in the UCSRnB register.
   }
   else {
      //Set baud rate
      UBRR1L=(uint8_t)UBRRVAL;   // low byte
      UBRR1H=(UBRRVAL>>8);			// high byte
   
      // ** Enable Transmitter and Receiver
      UCSR1B = ((1<<RXEN1)|(1<<TXEN1));
      //UCSR1B |= (1<<RXCIE1); //RX Complete Interrupt Enable
      // UCSZ02 must be '0' for 8 bit operation - this is the initial value of this bit in the UCSRnB register.    
   }   
}

uint8_t USART_ReceiveByte(uint8_t comport) {
   if (comport==0) {
      // Wait until a byte has been received
      while ((UCSR0A & (1<<RXC0)) == 0);
      // Return received data
      return UDR0;
   }
   else {
      // Wait until a byte has been received
      while ((UCSR1A & (1<<RXC1)) == 0);
      // Return received data
      return UDR1;      
   }   
}

void USART_Flush(uint8_t comport) {
   unsigned char dummy;
   if (comport==0) {
      while ( UCSR0A & (1<<RXC0) ) dummy = UDR0;
   }
   else {
      while ( UCSR1A & (1<<RXC1) ) dummy = UDR1;      
   }   
}

void USART_Transmit(unsigned char data, uint8_t comport)
{
   if (comport==0) {
      /* Wait for empty transmit buffer */
      while ( !( UCSR0A & (1<<UDRE0)) )
      ;
      /* Put data into buffer, sends the data */
      UDR0 = data;
   }
   else {
      /* Wait for empty transmit buffer */
      while ( !( UCSR1A & (1<<UDRE1)) )
      ;
      /* Put data into buffer, sends the data */
      UDR1 = data;      
   }   
}


void usart_putchar(char data, uint8_t comport) 
{
    if (comport==0) {
      // Wait for empty transmit buffer
      while ( !( UCSR0A & (1<<UDRE0)) )
      ;
   
      // Start transmission
      UDR0 = data;
   }
   else {
      // Wait for empty transmit buffer
      while ( !( UCSR1A & (1<<UDRE1)) )
      ;
   
      // Start transmission
      UDR1 = data;      
   }   
}

unsigned char usart_kbhit(uint8_t comport) {
   //return nonzero if char waiting polled version
   unsigned char b;
   b=0;
   
   if(UCSR0A & (1<<RXC0)) b=1;
   return b;
}

void usart_pstr(char *s, uint8_t comport) {
   // loop through entire string
   while (*s) {
      usart_putchar(*s, comport);
      s++;
   }
}

// this function is called by printf as a stream handler
int usart_putchar_printf(char var, FILE *stream) {
   // translate \n to \r for br@y++ terminal
   if (var == '\n')
      usart_putchar('\r', 0);   
   
   usart_putchar(var, 0);
   return 0;
}
