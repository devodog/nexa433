/*
 * cmd.c
 *
 */

#ifndef F_CPU
// define cpu clock speed if not defined
#define F_CPU 12000000UL
#endif

#include <avr/io.h>
#include <string.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>
#include "cmd.h"
#include "serial.h"
#include "appver.h"

static const double msd = MINIMUM_SIGNAL_DURATION;
bool setpMotorHalt = false;
bool driveON = false;
bool manOperation = false; // default manual operation - by use of a potentiometer
int refFrequency = 50;
int driveDirection = 1; // Clockwise...
int lastError = 0;

extern uint8_t intNumber;
extern bool bitPacketComplete;
//extern bool rxOn;
extern uint16_t interrupts;
char *cmdList[] = {"tx", "rx"};

enum commands {
   TX,
   RX
} COMMANDS;

char *argList[] = {"data", "addr", "repeats"};
//


void promt() {
   printf("\n433> ");
}
uint8_t sw1_ON[4] = {0b01001110, 0b11000011, 0b10010100, 0b00001001};
   
void txTestLED_PA2() {
   // Preamble
   PORTA = 0x04;
   _delay_ms(22);
   PORTA = 0x00;
   _delay_ms(300);
   
   for(int i=0; i<4; i++) {
      uint8_t bitMask = 1;
      for(int j=0; j<8; j++) {
         if(sw1_ON[i] & bitMask) { // '1'
            PORTA = 0x04;
            _delay_ms(22);
            PORTA = 0x00;
            _delay_ms(140);
            PORTA = 0x04;
            _delay_ms(22);
            PORTA = 0x00;
            _delay_ms(30);            
            printf("1");
         }
         else {
            PORTA = 0x04;
            _delay_ms(22);
            PORTA = 0x00;
            _delay_ms(30);
            PORTA = 0x04;
            _delay_ms(22);
            PORTA = 0x00;
            _delay_ms(140);
            printf("0");
         }
         bitMask = bitMask << 1;               
      }
   }
   printf("\nTX End");
}

void transmitt(uint8_t dev, uint8_t on) {
   sw1_ON[3] = dev;
   
   if (on==1)
      sw1_ON[3] |= 0x8;
   else
      sw1_ON[3] &= ~0x8;
   //printf("%x\n", sw1_ON[3]);
   // Preamble
   PORTA = 0x04;
   _delay_us(220);
   PORTA = 0x00;
   _delay_us(3000); // check this value...
   
   for(int i=0; i<4; i++) {
      uint8_t bitMask = 1;
      for(int j=0; j<8; j++) {
         if(sw1_ON[i] & bitMask) { // '1'
            PORTA = 0x04;
            _delay_us(220);
            PORTA = 0x00;
            _delay_us(1400);
            PORTA = 0x04;
            _delay_us(220);
            PORTA = 0x00;
            _delay_us(300);
         }
         else {
            PORTA = 0x04;
            _delay_us(220);
            PORTA = 0x00;
            _delay_us(300);
            PORTA = 0x04;
            _delay_us(220);
            PORTA = 0x00;
            _delay_us(1400);
         }
         bitMask = bitMask << 1;
      }
   }
   //printf("\nTX End");
}
/*
void transmit(char *args, int len) {
   //uint8_t addr;
   //uint8_t swState;
   //uint8_t swId = 0;
   uint8_t nextArgIndex;
   
   for (int i=0; i<len; i++) {
      if ((args[i] >= '0') && (args[i] <= '9')) {
         
      }
      else if (args[i] == ' ') {
         nextArgIndex = i+1;   
      }
   }      
}
*/

uint8_t executeCmd(char *termInput, int cmdLength) {
   int i = 0;
   
   // Check if the entered command is part of the command-list for this application.
   for (; i < sizeof(cmdList); i++) {
      if (strncmp(cmdList[i], termInput, strlen(cmdList[i])) == 0) {
         // The command entered is found in the command-list 
         break;
      }
   }

   // Execute the command if part of the command-list. 
   if (i >= sizeof(cmdList)) {
      printf("\n%s is not recognized", termInput);
      promt();
   }
   else {
      switch (i) {
            uint8_t devId = 1;
            uint8_t uData = 0;
         case TX: // Parsing 2 tx parameters, switch-id and switch-state... 
            
            
            /**
            for(int n=0; n<2; n++) {
               char digits[3] = {0,0,0};
               
               for (int i=0; i<3; i++){
                  if (termInput[3+i] != ' ') {
                     digits[i] = termInput[3+i];
                  }
                  else
                  break;
               }
               if (i > 0) {
                  uData = (digits[0]-30)*10;
                  uData += digits[1]-30;
               }
               else{
                  uData = digits[0]-30;               
               }
               if (n==0){
                  swData = uData;
               }
               else{
                  swData |= uData<<4;
               }
            }
            ***/
            if (termInput[5] == '1') {
               uData=1;
            }
            else {
               uData=0;
            }
            devId = termInput[3]-48;
            printf("\nTX Start: devId=%d State=%d\n", devId, uData);
            transmitt(devId, uData);
            transmitt(devId, uData);
            transmitt(devId, uData);           
            promt();
            break;
         
         case RX:
            bitPacketComplete = false;
            intNumber = 0;
            interrupts = 0;
            printf("\nWaiting for OOK signal packet!\n");
            // Initialize the pin-change interrupts... for handling the OOK data readout
            // MCUCR == MCU Control Register
            MCUCR |= 0x1; // Any logical change on INT0 generates an interrupt request
   
            // GIMSK == General Interrupt Mask Register
            GIMSK |= 0x8;  // Pin Change Interrupt Enable 1 - for port A.
   
            // PCMSK1 - Pin Change Mask Register 1
            PCMSK0 |= 0x1; // Pin Change Enable Mask on PORTB => 0b000 0001 = (PA1)
            TCNT0 = 0;
            break;
         
         default:
            printf("\nNOP[i=%d]", i);
            promt();
            break;
      }
   }
   return 0;
}

void updateConfig(char* param, char rw) {
   int i = 0;
   for (; i < sizeof(argList); i++) {
      if (strncmp(argList[i], param, strlen(argList[i])) == 0) {
         break;
      }
   }
}   

/*            
         case HELP:
            printf("\nCommands:\n");
            printf("run - Start the commutation process\n");
            printf("halt - Stop the commutation process\n");
            printf("status - Show current operation status (rotation dir. & frequency)\n");
            printf("get freq - Gets the current reference frequency\n");
            printf("get dir - Gets the current rotation direction\n");
            printf("get op - Get current operation mode\n");
            printf("get error - Gets the last error\n");
            printf("set freq - Sets the reference frequency\n");
            printf("set dir - Sets the rotation direction\n");
            break;
*/