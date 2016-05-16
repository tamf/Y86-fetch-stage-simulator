


#include <stdio.h>
#include <unistd.h>
#include "printInternalReg.h"

//
//  printReg() takes the input parameters described below and formats them for 
//           consistent printing. 
//
//  PC  - the address that the program counter is at when the instruction is retrieved
//  icode - the instruction code
//  ifun  - the function code for the instruction
//  regsValid - Not all instructions set the rA and rB values. If an instruction
//              has an rA and rB value this is set to a non-zero value and to 
//              0 if rA and rB is not valid.                 
//  rA - the rA value, if regsValid is zero this can be anything. 
//  rB - the rB value, if regsValid is zero this can be anything. 
//  valCValid - Not all instructions have a valC value. If an instruction has a 
//              a valC value this is set to any non-zero value otherwise it is set
//              to zero. 
//  valC - if valCValid is true (i.e. non-zero)  then this contains the value of valC such that 
//         when it is printed as an unsigned integer it has the expected value. For example if the 
//         the instruction were mrmovq 32(%r9), %r10 then valC in memory, in little endian 
//         format would be  20 00 00 00 00 00 00 00, but when we print it as an integer, in hex, 
//         it would print 0000000000000020 
//         if valCvalid is false (i.e. 0) then valC can have any value
//  byte0 - byte8  These are the 8 individual bytes that make up valC, assuming 
//          valCvalid is true. Byte0 is the least significant byte 
//          and byte8 the most significant byte. When these bytes 
//          are printed in the order from byte0 to byte8 it will reproduce the in 
//          memory representation of valC. 
// valP     This is the value of valP as deteremined during the fetch stage. 
// instr    This is the string representation of the instruction mnemonic. Note that if the 
//          instruction is an unconditional jump then it is reported as jmp, and if it is 
//          an unconditional move then it is reported as rrmovq. The mnemonics are to
//          be the ones from the text book. 
   
void printReg(uint64_t PC, nibble icode, nibble ifun, 
	     int regsValid, nibble rA, nibble rB, 
             int  valCValid, uint64_t valC,
	     uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3,  
	     uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7,  
	     int64_t valP, char * instr)  {


  static int currentLine = 0; // This variable is used to determine the line number being printed so that 
                              // a header line can be periodically printed.

  // Print the header line every 20 lines
  if (!(currentLine++ % 20)) {
   
    printf("\nPC           icode ifun  rA rB        valC           ValClsb  valCmsb       valP         Instruction\n"); 

  }


  // print the program counter (only the low order 4 bytes are printed) 
  printf("%08llX :    %2X    %2X ", PC, icode, ifun);
  

  // If required print the register values
  if (regsValid) {
    printf("  %2X %2X    ", rA, rB);
  } else {
    printf("   -  -    ");
  }

  // Print valC, if present
  if (valCValid) {
    printf("%016llX  %02X%02X%02X%02X %02X%02X%02X%02X ", valC, 
	   byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7);	   
  } else {
    printf("%36s", " "); 
  }

  // Print valP and the instruction mnemonic.
  printf("%016llX     %s\n", valP, instr);
}



