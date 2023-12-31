/*
 * isr.c
 *
 * Created: 21.04.2021 21:00:00
 * Author: Dag
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdbool.h>

#include "serial.h"

volatile uint8_t bytesReceived;


// NEW: Holds the input characters received from the terminal other that carriage return & line feed
extern char termInputBuffer[80];
extern bool cmdComplete;
//extern bool driveON;
extern uint8_t cmdLength;
uint8_t counter;
uint16_t interrupts;
/*
ISR(INT0_vect)
{
   // ** Micro switch indicate that the door is about to OPEN.
   // IF IMMEDIATE NOTIFICATION WANTED ISSUE A ITN (Incident Triggered Notification).
   // ... by setting a ITN FLAG.
   //
   // MAKE SURE THAT ALL THE REGISTERES ARE SET CORRECTLY...
   //
   // Setting the I-bit in the SREG register is usually done by calling sei();
   // The MCUCR can be left untouched.
   // GIMSK's INT0 bit needs to be set to '1'. (I would assume that this is also set by sei()).
   // ...this all... so maybe nothing needs to be done to arm this interrupt.
   printf("INT0\n");
   // Should mask away this interrupt so that it will be kept silent if triggered for a long period.(?)
}
*/

/************************************************************************
USART Receive interrupt service routine                              

Will handle command-line input from the serial communication terminal
reading byte by byte and waiting for a cr-lf that will complete the 
input

Every input character will be echoed back to the sending terminal.
*************************************************************************/
ISR(USART0_RXC_vect) // will execute for every received  byte from PC terminal.
{	
   int n = bytesReceived; // global byte counter...
   char rxChar;
   rxChar = USART_ReceiveByte(0);
   //PORTC |= (1<<PORTC2);
   if (rxChar == 13) {
      cmdComplete = true;
      cmdLength = n;
      bytesReceived = 0;
      return;
   }
   // Use ^C to terminate any ongoing operation...
   /***
   else if (rxChar == 3) {
      driveON = false;
   }
   ***/
   else {
      termInputBuffer[n] = rxChar;
      // echo input char
      usart_putchar(rxChar, 0);
   }
   n++;
   bytesReceived = n;
}
/******************************************************************************
ISR(USART1_RXC_vect) // will execute for every received  byte from PC terminal.
{
   int n = bytesReceived; // global byte counter...
   char rxChar;
   rxChar = USART_ReceiveByte(1);
   PORTC |= (1<<PORTC2);
   if (rxChar == 13) {
      cmdComplete = true;
      cmdLength = n;
      bytesReceived = 0;
      return;
   }
   
   // Use ^C to terminate any ongoing operation...
   //else if (rxChar == 3) {
      //driveON = false;
   //}
   
   else {
      termInputBuffer[n] = rxChar;
      // echo input char
      usart_putchar(rxChar, 1);
   }
   n++;
   bytesReceived = n;
}
*****************************************************************************/

uint8_t phyStateTime[255];
uint8_t intNumber;
bool bitPacketComplete;
bool rxOn;

ISR(PCINT0_vect) {
   // PIN change interrupt 1 has been triggered by any logical change on the 
   // actual pin(s), see the initialization in main.
   //
   // NEW STRATEGY
   // Initiate data collection by terminal command, which will enable the pin change interrupt.
   // Store all timer data for every pin change interrupt.
   // When timer interrupt triggers, disable pin change interrupt and analyze the collected data,
   // and print it to the command line terminal.
   uint8_t stateTime = 0;
   interrupts++;
   
   if (!rxOn) {
      if (TCNT0 == 130) {
      //if ((TCNT0 > 132) && (TCNT0 < 135)) {
         counter = TCNT0;      
         rxOn = true;
         intNumber = 0;
         memset(&phyStateTime[0], 0, 255);
      }
      else {         
         TCNT0 = 0;
         return;
      }
   }
   //interrupts++;
         
   stateTime = TCNT0;
   TCNT0 = 0;
   if ((PINA&0x01) == 1) { // low-to-high
       stateTime &= ~0x80;      
       // The most significant bit holds the bit-value that lasted for the time
       // registered by the timer register, which is the value of the rest of 
       // the byte. (Should not exceed the value of 127 which equals to 2.7 ms.  
       // If it does it might indicate that the interrupt registered a preamble 
       // indication.)
   }
   else { // high-to-low
       stateTime |= 0x80;      
   }   
   if (intNumber < 132) {
      phyStateTime[intNumber++] = stateTime;
   }
   else {
      rxOn = false;
      bitPacketComplete = true;
      MCUCR &= ~0x1;    // Disabling Any logical change on INT0 generates an interrupt request
      GIMSK &= ~0x8;   // Disabling Pin Change Interrupt Enable 1 - for all pins of port B.
      PCMSK0 &= ~0x1;   // Disabling Pin Change Enable Mask on PORTB => 0b000 0010 = (PB1)
   }
   //TCNT0 = 0;
   //printf(".");
   // DONE!
}

ISR(TIM0_COMPA_vect) {
   // This interrupt service routine will be triggered when the counter has reached the value in the timer-compare register.
   //if ((rxOn) && (intNumber > 128)) {
   if (rxOn){
      // Disabling the pin change interrupt
      MCUCR &= ~0x1;    // Disabling Any logical change on INT0 generates an interrupt request
      GIMSK &= ~0x8;   // Disabling Pin Change Interrupt Enable 1 - for all pins of port B.
      PCMSK0 &= ~0x1;   // Disabling Pin Change Enable Mask on PORTB => 0b000 0010 = (PB1)
      bitPacketComplete = true;
      rxOn = false;
      TCNT0 = 0;
      printf("TIM0_COMPA_vect\n");
   }
}
