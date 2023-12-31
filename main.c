/*
* main.c
*
* Created: 07.11.2022 17:22:30
* Author : Dag
*
* Source code for the tinyWatcher board based on the ATtiny1634 MCU from Atmel
* (now Microchip).
*
* The code is to send nexa-commands to a wireless socket switch by the use of a single GPIO
* (General Purpose Input Output) port.
*
* PB3(output): D100 (BLUE LED)
*
* GPIO Port C
* PC0(output): LED1
* PC2(output): LED2
*
*
*
*/
#ifndef F_CPU
// define cpu clock speed if not defined
#define F_CPU 12000000UL
#endif

#include <avr/io.h>

#include <string.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>

#include "appver.h"
#include "serial.h"
#include "cmd.h"

#define UNIT_STATUS_LED_ON() PORTB |= (1<<PORTB3)
#define UNIT_STATUS_LED_OFF() PORTB &= ~(1<<PORTB3)

#define UNIT_STATUS_LED1_ON() PORTC |= (1<<PORTC0)
#define UNIT_STATUS_LED1_OFF() PORTC &= ~(1<<PORTC0)

#define UNIT_STATUS_LED2_ON() PORTC |= (1<<PORTC2)
#define UNIT_STATUS_LED2_OFF() PORTC &= ~(1<<PORTC2)

// Global variables for use with the UART terminal interface
char termInputBuffer[80];
bool cmdComplete;
uint8_t cmdLength = 0;

extern uint8_t intNumber;
extern bool bitPacketComplete;
extern bool rxOn;
extern uint8_t counter;
extern uint16_t interrupts;

extern uint8_t phyStateTime[255];
const uint8_t bitWidth = 12;

void ledBlink()
{
   UNIT_STATUS_LED_ON();
   _delay_ms(150);
   UNIT_STATUS_LED_OFF();
   _delay_ms(150);
}
// NOTE! LED1 NOT MOUNTED!
void led1Blink()
{
   UNIT_STATUS_LED1_ON();
   _delay_ms(150);
   UNIT_STATUS_LED1_OFF();
   _delay_ms(150);
}

void led2Blink()
{
   UNIT_STATUS_LED2_ON();
   _delay_ms(150);
   UNIT_STATUS_LED2_OFF();
   _delay_ms(150);
}

void decode(uint8_t* bitStream, int len) {
   uint8_t data;
   uint8_t bits;
   //uint16_t totalNumOfBits = 0;
   
   for (int i = 0; i < len; i++) {
      data = *bitStream++;
      bits = (data & 0x7f)/bitWidth;
      if ((data & 0x10) == 0x10) {
         for (int j = 0; j < bits; j++) {
            printf("1");
            //totalNumOfBits++;
            //if (totalNumOfBits%10) printf(" ");
         }
      }
      else {
         for (int j = 0; j < bits; j++) {
            printf("0");
            //totalNumOfBits++;
            //if (totalNumOfBits%10) printf(" ");
         }
      }
   }
}

//111111111111111111111111111111111111111111111111111111111111111111111111
void init() {
   //DDRA |= (1<<PORTA0)|(1<<PORTA2)|(1<<PORTA3)|(1<<PORTA4)|(1<<PORTA5)|(1<<PORTA6);
   DDRA |= (1<<PORTA2);
   DDRA &= ~(1<<PORTA1); // Making PA1 as input for the manual-/PC-operation switch 
   PUEA |= (1<<PORTA1); // Using pull-up on PA1
   //PUEA |= (1<<PORTA0)|(1<<PORTA1); // Using pull-up on PA1
   
   //put these lines in setup
   //CCP=0xD8; //write key to configuration change protection register
   //WDTCSR=(1<<WDP3)|(1<<WDP0)|(1<<WDIE); //enable WDT interrupt with longest prescale option (8 seconds)
   
   // Using PB1 for reading OOK data on the 433MHz band
   //DDRB &= ~(1<<PORTB1);
   DDRB |= (1<<PORTB3); // Blue LED

   // Using PA0 for reading OOK data on the 433MHz band
   DDRA &= ~(1<<PORTA0);
   //PUEA |= (1<<PORTA0); // Using pull-up on PA0

   // Not sure if a pull-up is needed on the input pin... 
   //PUEB |= (1<<PORTB1)
   
   // PC2 as output: LED2.
   DDRC |= (1<<PORTC2);

   // Prescale table for the ATtiny1634
   // =================================
   // |CS02|CS01|CS00| Description
   // |  0 |  0 |  0 | No clock source 
   // |  0 |  0 |  1 | clock I/O no prescaleing 
   // |  0 |  1 |  0 | (clock I/O)/8 from prescaler 
   // |  0 |  1 |  1 | (clock I/O)/64 from prescaler 
   // |  1 |  0 |  0 | (clock I/O)/256 from prescaler 
   // |  1 |  0 |  1 | (clock I/O)/1024 from prescaler 
   // |  1 |  1 |  0 | External clock source T0. Clock on falling edge 
   // |  1 |  1 |  1 | External clock source T0. Clock on rising edge 
   
   //preScale = 256;
   TCCR0B |= 0x4; // div 256 ==> each Timer tick = 256*1 / 12 000 000 = 21,33us ==> 256 Timer ticks = 5,46ms

   // We'll arm the timer-interrupt to help detect the data packets preamble.
   // - excluding the timer interrupt here...
   //TIMSK |= 0x1; // Set the interrupt mask register bit for OCIE0A (Timer/Counter0 Compare Match Interrupt Enable)
   //TIFR |= 0x1; // Clearing the interrupt Output Compare Flag 0 A AND triggering the interrupt handler for this.
   // Set Compare register value..
   // TO BE DEFINED
   
   // Assign that address for my stdout function...
   stdout = &mystdout;
   USART_Init(0);
   sei();
   
   // Boot printout...
   printf("Version.%d.%d[build:%d]\n", MAJOR_VERSION, MINOR_VERSION, BUILD);
   for(int8_t i=5; i>0; i--)
   {
      if (i==1) {
         printf("\n");
      }
      else {
         printf(".");
      }
      ledBlink();
   }   
   promt();
}

int main(void)
{
   bitPacketComplete = false;
   // Initialize the controller with ISRs, UART and other MCU configuration.
   init();

   // Command-loop
   while(1) {
      if (cmdComplete) {
         cmdComplete = false;         
         executeCmd(&termInputBuffer[0], cmdLength);
         memset(termInputBuffer, 0, 80);
      }
      if (bitPacketComplete) {
         
         if (intNumber > 0) {
            
            for (int i = 0; i < intNumber; i++){
               printf("%02x ", phyStateTime[i]);
            }
            printf("\n%d Symbols received.\n", intNumber);
            printf("\nCounter=%d\n", counter);
            printf("\nInterrupts=%d\n", interrupts);
            //decode(&phyStateTime[0], intNumber);
            bitPacketComplete = false;
         }
         else {
            printf("NO DATA...");
            
         }
         promt();
      }
      _delay_ms(1000);
      //promt();                
   }
}