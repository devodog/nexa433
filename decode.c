/*
 * decode.c
 *
 * Created: 05.11.2023 11:51:31
 *  Author: dagak
 */ 
enum RF_REC_STATE {
   PREAMBLE_WAIT0,
   PREAMBLE_WAIT1,
   PREAMBLE_WAIT2,
   PREAMBLE_REC,
   BIT_SYMBOL_READ_START,
   READ_BITS,
   BIT_SYMBOL_READ_COMPLETE,
   IDLE
};
enum RF_REC_STATE rxState = IDLE;

#define MAX_SYMBOLS 64

const uint8_t preambleShort 130; // 2773 us
const uint8_t preambleLong 150;  // 3350 us
const uint8_t bitWidth = 9;
const uint8_t symbolWidth = 8;
uint8_t symbolBitCount = 0;
uint8_t symbolByte = 0;
uint8_t symbolByteIndex = 0;
uint8_t data[MAX_SYMBOLS];
uint8_t symbolCount;
bool packetComplete = false;
uint8_t duration;




   
   // State 0 [PREAMBLE_WAIT0]: waiting for a preamble (no timer interrupt enabled)
   if (rxState == PREAMBLE_WAIT0) {
      if ((PINB&0x02)>>1 == 1) { // low-to-high
         TCNT0 = 0;
         rxState = PREAMBLE_WAIT1;
         return;
      }
   }
   
   if (rxState == PREAMBLE_WAIT1) {
      if ((PINB&0x02)>>1 == 0) { // high-to-low
         if ((TCNT0/bitWidth) == 1) {
            rxState = PREAMBLE_WAIT2;
            TCNT0 = 0;
         }
         else {
            rxState = PREAMBLE_WAIT0;
            TCNT0 = 0;
         }
      }
      return;
   }

   // PIN Change interrupt from low-to-high (read TCNT0 register and IF preamble detected, clear the TCNT0 register)
   // state 2 -> State 3 [PREAMBLE_REC]
   if (rxState == PREAMBLE_WAIT2) {
      if ((PINB&0x02)>>1 == 1) { // low-to-high
         if ((TCNT0 > preambleShort && (TCNT0 < preambleLong)) {
            rxState = PREAMBLE_REC;
            TCNT0 = 0;
         }
         else {
            rxState = PREAMBLE_WAIT0;
            TCNT0 = 0;
         }
      }
      return;
   }

   // PIN Change interrupt from high-to-low (read TCNT0 register and IF it is a single bit, clear the TCNT0 register)
   // state 3 -> State 4 [BIT_SYMBOL_READ_START]
   if (rxState == PREAMBLE_REC) {
      if ((PINB&0x02)>>1 == 0) { // high-to-low
         if ((TCNT0/bitWidth) == 1) {
            rxState = BIT_SYMBOL_READ_START;
            symbolByte = 0;
            symbolByteIndex = 0;
            TCNT0 = 0;
         }
         else {
            rxState = PREAMBLE_WAIT0;
            TCNT0 = 0;
         }
      }
      return;
   }

   // State 4 -> State 5 [READ_BITS]
   if (rxState == BIT_SYMBOL_READ_START) {
      bitCount = TCNT0/bitWidth;
      if (bitCount <= 0) {
         rxState = PREAMBLE_REC;
         TCNT0 = 0;
         return;
      }
      if ((PINB&0x02)>>1 == 1) { // low-to-high => reading the number of '0's (zeros)
         // PIN Change interrupt from low-to-high (read TCNT0 register and IF it is (10 - READ_BITS > 0) bits, clear the TCNT0 register)
         // state 4 -> State 5 [READ_BITS]
         
         if ( bitCount <= (symbolWidth-symbolBitCount) ) {
            symbolByte = symbolByte<<bitCount; // Expecting that zeros are shifted inn from the right to the left...
            symbolBitCount += bitCount;
            
            if (symbolBitCount == symbolWidth) {
               data[symbolCount] = symbolByte;
               
               if (symbolCount >= MAX_SYMBOLS) {
                  packetComplete = true;
                  rxState = PREAMBLE_WAIT0;
                  return;
               }
               symbolCount++;
               rxState = BIT_SYMBOL_READ_COMPLETE;
            }
            else {
               rxState = READ_BITS;
            }
            TCNT0 = 0;
         }
      }
      return;
   }

   // state 5 -> State 5 [READ_BITS]
   if (rxState == READ_BITS) {
      bitCount = TCNT0/bitWidth;
      if ((PINB&0x02)>>1 == 0) { // high-to-low => reading the number of '1's
         // PIN Change interrupt from high-to-low (read TCNT0 register and IF it is (10 - READ_BITS > 0) bits, clear the TCNT0 register)
         // state 5 -> State 5 [READ_BITS]
         
         if ( bitCount <= (symbolWidth-symbolBitCount) ) {
            uint16_t temp = 0x0ff; temp <<= bitCount; temp &= 0xff00; temp >>= 8;
            
            symbolByte = symbolByte<<bitCount; // Expecting that zeros are shifted inn from the right to the left...
            symbolByte |= temp;
            symbolBitCount += bitCount;

            if (symbolBitCount == symbolWidth) {
               data[symbolCount] = symbolByte;
               
               if (symbolCount >= MAX_SYMBOLS) {
                  packetComplete = true;
                  rxState = PREAMBLE_WAIT0;
                  return;
               }
               symbolCount++;
               rxState = BIT_SYMBOL_READ_COMPLETE;
            }
            else {
               // --- IF symbolBitCount < symbolWidth => rxState = READ_more_BITS.
               // IF symbolBitCount > symbolWidth could indicate that the last symbol bit was '1' and
               // that the start symbol indication was part of this read...
               // This is NOT handled here so far...- the bitCount test prevents this possibility...
               rxState = READ_BITS;
            }
            TCNT0 = 0;
         }
         return;
      }
      else {
         // PIN Change interrupt from low-to-high (read TCNT0 register and IF it is (10 - READ_BITS > 0) bits, clear the TCNT0 register)
         // state 5 -> State 5 [READ_BITS]
         
         if ( bitCount <= (symbolWidth-symbolBitCount) ) {
            symbolByte = symbolByte<<bitCount; // Expecting that zeros are shifted inn from the right to the left...
            symbolBitCount += bitCount;
            
            if (symbolBitCount == symbolWidth) {
               data[symbolCount] = symbolByte;
               
               if (symbolCount >= MAX_SYMBOLS) {
                  packetComplete = true;
                  rxState = PREAMBLE_WAIT0;
                  return;
               }
               symbolCount++;
               rxState = BIT_SYMBOL_READ_COMPLETE;
            }
            else {
               rxState = READ_BITS;
            }
            TCNT0 = 0;
         }
      }
      return;
   }
   
   // state 5 -> State 5 [READ_BITS]
   if (rxState == READ_BITS) {
      bitCount = TCNT0/bitWidth;
      if ((PINB&0x02)>>1 == 1) { // low-to-high => reading the number of '0's (zeros)
         // PIN Change interrupt from low-to-high (read TCNT0 register and IF it is (10 - READ_BITS > 0) bits, clear the TCNT0 register)
         // state 4 -> State 5 [READ_BITS]
         if ( bitCount <= (symbolWidth-symbolBitCount) ) {
            symbolByte = symbolByte<<bitCount; // Expecting that zeros are shifted inn from the right to the left...
            symbolBitCount += bitCount;
            rxState = READ_BITS;
            TCNT0 = 0;
         }
         else {
            //
            rxState = PREAMBLE_WAIT0;
            TCNT0 = 0;
         }
      }
      else {
         // PIN Change interrupt from high-to-low (read TCNT0 register and IF it is (10 - READ_BITS > 0) bits, clear the TCNT0 register)
         // state 4 -> State 5 [READ_BITS]
         if ( bitCount <= (symbolWidth-symbolBitCount) ) {
            symbolByte = symbolByte<<bitCount; // Expecting that zeros are shifted inn from the right to the left...
            symbolBitCount += bitCount;
            rxState = READ_BITS;
            TCNT0 = 0;
         }
         else {
            
            rxState = PREAMBLE_WAIT0;
            TCNT0 = 0;
         }
      }
      return;
   }