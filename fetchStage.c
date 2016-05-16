
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include "printInternalReg.h"

#define ERROR_RETURN -1
#define SUCCESS 0

#define TRUE 1
#define FALSE 0

uint64_t fileLen; // length of file (number of bytes)
uint64_t PC = 0; // program counter

unsigned char instrBuffer[10]; // instruction buffer 

unsigned char icode_ifun; // char containing icode and ifun
nibble* icode_ifun_arr; // array containing icode and ifun
nibble icode;
nibble ifun;

unsigned char rA_rB; // char containing rA, rB
nibble* rA_rB_arr; // array containing rA, rB
int regsValid = 0;
nibble rA = 0;
nibble rB = 0;
	
int valCValid = 0;
uint64_t valC = 0;
	
uint8_t b0 = 0; // byte0 of valC, which is LSB
uint8_t b1 = 0;
uint8_t b2 = 0;
uint8_t b3 = 0;
uint8_t b4 = 0;
uint8_t b5 = 0;
uint8_t b6 = 0;
uint8_t b7 = 0;
	
uint64_t valP;

char* instr;

int bytesToRead; // bytes we need to read from memory for this instruction
int numBytesCanRead; // number of bytes we are able to read
int notEnoughBytes = FALSE; // flag for whether there are enough bytes in file to read

char* fileContents; // char array of file contents
unsigned int numConsecutiveHalts = 0; // number of consecutive halt instructions encountered

// Set instruction buffer to zeros
void resetInstrBuffer() {
	for (int i = 0; i < 10; i++) {
		instrBuffer[i] = 0;
	}
}

// Returns number of bytes needed to read given the icode.
int getBytesToRead(nibble icode) {
	switch (icode) {
		case 0:
			return 1;
		case 1:
			return 1;
		case 2:
			return 2;
		case 3:
			return 10;
		case 4:
			return 10;
		case 5:
			return 10;
		case 6:
			return 2;
		case 7:
			return 9;
		case 8:
			return 9;
		case 9:
			return 1;
		case 0xA:
			return 2;
		case 0xB:
			return 2;
		default:
			// icode not recognized. Print message and exit.
			printf("Invalid opcode %.2x at 0x%016llX\n", icode_ifun, PC);
			exit(0);
			break;				
	}
	return 0;
}

// Converts unsigned char into two hex digits, then returns them in array. output[0] has most significant hex digit.
nibble* decToHex(unsigned char* ch) {
	static nibble output[2];
	output[0] = *ch / 16;
	output[1] = *ch % 16;
	return output;
}


// Get valC for printReg function. Reads eight bytes from instrBuffer starting from offsetFromPC and uses bit manipulations to combine them into one int.
uint64_t getValC(int offsetFromPC) {
	uint64_t valCByteArray[8];
	for (int i = offsetFromPC; i <= offsetFromPC + 8; i++) {
		valCByteArray[i - offsetFromPC] = instrBuffer[i];
	}

	uint64_t new_valC = 0;
	
	for (int i = 0; i < 8; i++){
		valCByteArray[i] = (valCByteArray[i] << (i*8));
		new_valC = new_valC | valCByteArray[i];
	}
	return new_valC;
}

// If ifun is non-zero, prints out where the function code is invalid and exits. This is called if the instruction expects ifun to be zero.
void assertZeroiFunCode(nibble ifun, unsigned char icode_ifun) {
	if (ifun != 0) {
		printf("Invalid function code %.2x at 0x%016llX\n",icode_ifun,PC);
		exit(0);
	}
}

// Sets rA and rB for printReg function
void set_rA_rB() {
	regsValid = 1;
	rA_rB = instrBuffer[1];
	rA_rB_arr = decToHex(&rA_rB);
	rA = (nibble) rA_rB_arr[0];
	rB = (nibble) rA_rB_arr[1];
}

// Sets valC and byte0 to byte7 for printReg. Only used by 9 or 10 byte instructions.
void setValC_and_bytes(int numBytesInInstr) {
	int PC_offset = numBytesInInstr - 8;
	
	valCValid = 1;
	valC = getValC(PC_offset);
	
	b0 = instrBuffer[PC_offset];
	b1 = instrBuffer[PC_offset + 1];
	b2 = instrBuffer[PC_offset + 2];
	b3 = instrBuffer[PC_offset + 3];
	b4 = instrBuffer[PC_offset + 4];
	b5 = instrBuffer[PC_offset + 5];
	b6 = instrBuffer[PC_offset + 6];
	b7 = instrBuffer[PC_offset + 7];	
}

// Sets parameters for printReg for 10 byte instructions except instr.
void setParams10ByteInstr() {
	set_rA_rB();	
	setValC_and_bytes(10);	
	valP = PC + 10;
}

// Sets parameters for printReg for 9 byte instructions except instr.
void setParams9ByteInstr() {
	regsValid = 0;
	setValC_and_bytes(9);		
	valP = PC + 9;
}

// Sets parameters for printReg for 1 byte instructions except instr.
void setParams1ByteInstr() {
	regsValid = 0;
	valCValid = 0;
	valP = PC + 1;
}
// Sets parameters for printReg for 2 byte instructions except instr.
void setParams2ByteInstr() {
	set_rA_rB();
	valCValid = 0;			
	valP = PC + 2;	
}	

// Processes the instruction, calls printReg with correct params
void processInstr() {
	if (icode != 0) {
		numConsecutiveHalts = 0;
	}
	switch (icode) {
		case 0: 
			instr = "halt";
			
			// if invalid ifun, exit
			assertZeroiFunCode(ifun, icode_ifun);
			
			// set params
			setParams1ByteInstr();
			
			// increment number of consecutive halts
			numConsecutiveHalts++;
			
			break;
		case 1:
			instr = "nop";
			assertZeroiFunCode(ifun, icode_ifun);
			setParams1ByteInstr();
			
			break;
		case 2: 
			// get instr based on ifun
			switch (ifun) {
				case 0:
					instr = "rrmovq";
					break;
				case 1:
					instr = "cmovle";
					break;
				case 2:
					instr = "cmovl";	
					break;
				case 3:
					instr = "cmove";
					break;
				case 4:
					instr = "cmovne";
					break;
				case 5:
					instr = "cmovge";
					break;
				case 6:
					instr = "cmovg";
					break;
				default:
					// Invalid function code
					printf("Invalid function code %.2x at 0x%016llX\n",icode_ifun,PC);
					exit(0);
			}			
			setParams2ByteInstr();	
			
			break;
		case 3:
			instr = "irmovq";			
			assertZeroiFunCode(ifun, icode_ifun);
			setParams10ByteInstr();		
			
			break;
		case 4: 
			instr = "rmmovq";
			assertZeroiFunCode(ifun, icode_ifun);
			setParams10ByteInstr();
		
			break;
		case 5: 
			instr = "mrmovq";
			assertZeroiFunCode(ifun, icode_ifun);
			setParams10ByteInstr();

			break;
		case 6: 
			// get instr based on ifun
			switch (ifun) {
				case 0:
					instr = "addq";
					break;
				case 1:
					instr = "subq";
					break;
				case 2:
					instr = "andq";	
					break;
				case 3:
					instr = "xorq";
					break;
				case 4:
					instr = "mulq";
					break;
				case 5:
					instr = "divq";
					break;
				case 6:
					instr = "modq";
					break;
				default:
					// Invalid function code
					printf("Invalid function code %.2x at 0x%016llX\n",icode_ifun,PC);
					exit(0);
			}
			setParams2ByteInstr();

			break;
		case 7:
			switch (ifun) {
				case 0:
					instr = "jmp";
					break;
				case 1:
					instr = "jle";
					break;
				case 2:
					instr = "jl";
					break;
				case 3:
					instr = "je";
					break;
				case 4:
					instr = "jne";
					break;
				case 5:
					instr = "jge";
					break;
				case 6:
					instr = "jg";
					break;
				default:
					// Invalid function code
					printf("Invalid function code %.2x at 0x%016llX\n",icode_ifun,PC);
					exit(0);
			}
			setParams9ByteInstr();
		
			break;
		case 8: 
			instr = "call";
			assertZeroiFunCode(ifun, icode_ifun);
			setParams9ByteInstr();

			break;
		case 9:
			instr = "ret";
			assertZeroiFunCode(ifun, icode_ifun);
			setParams1ByteInstr();
			
			break;
		case 0xA: 
			instr = "pushq";
			assertZeroiFunCode(ifun, icode_ifun);
			setParams2ByteInstr();

			break;
		case 0xB: 
			instr = "popq";
			assertZeroiFunCode(ifun, icode_ifun);	
			setParams2ByteInstr();
			
			break;
		default:
			// icode not recognized. Print message and exit.
			printf("Invalid opcode %.2x at 0x%016llX", icode_ifun, PC);
			exit(0);
			break;
	}
	// handling case where insufficient bytes to complete instruction fetch
	if (notEnoughBytes) {
		printf("Memory access error at %.X, required %d bytes, read %d bytes.\n", PC, bytesToRead, numBytesCanRead);
		exit(0);
	}	
	// for halt instructions, do not print out more than 5 consecutive.
	if (icode != 0 || numConsecutiveHalts <= 5) {
		printReg(PC,icode,ifun,regsValid,rA,rB,valCValid,valC,b0,b1,b2,b3,b4,b5,b6,b7,valP,instr);
	}
	// set new PC
	PC = valP;
}

int main(int argc, char **argv) {

  int machineCodeFD = -1;       // File descriptor of file with object code
  PC = 0;                		// The program counter

  // Verify that the command line has an appropriate number
  // of arguments

  if (argc < 2 || argc > 3) {
    printf("Usage: %s InputFilename [startingOffset]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to open, attempt to open it 
  // for reading and verify that the open did occur.
  machineCodeFD = open(argv[1], O_RDONLY);

  if (machineCodeFD < 0) {
    printf("Failed to open: %s\n", argv[1]);
    return ERROR_RETURN;
  }

  // If there is a 2nd argument present it is an offset so
  // convert it to a value. This offset is the initial value the 
  // program counter is to have. The program will seek to that location
  // in the object file and begin fetching instructions from there.  
  if (3 == argc) {
    // See man page for strtol() as to why
    // we check for errors by examining errno
    errno = 0;
    PC = strtol(argv[2], NULL, 0);
    if (errno != 0) {
      perror("Invalid offset on command line");
      return ERROR_RETURN;
    }
  }

  printf("Opened %s, starting offset 0x%016llX\n", argv[1], PC);

  // Start adding your code here and comment out the line the #define EXAMPLESON line
  
  FILE *fileStream;
  fileStream = fopen(argv[1], "rb"); // open file in binary read mode
  fseek(fileStream, 0, SEEK_END);
  fileLen = ftell(fileStream); // determine file length
  fseek(fileStream, 0, SEEK_SET);
  
  fileContents = malloc(fileLen * sizeof(char)); // initialize char array of file contents
  fgets(fileContents, fileLen, fileStream);
  
  while (PC < fileLen) {
	resetInstrBuffer(); // set instruction buffer to zeros
	
	fseek(fileStream, PC, SEEK_SET); // set read offset to PC
	int test = fread(instrBuffer, 1, 1, fileStream); // read one byte from fileStream
	
	icode_ifun = instrBuffer[0]; // set icode_ifun var
	icode_ifun_arr = decToHex(&icode_ifun); // array containing icode and ifun
	icode = icode_ifun_arr[0]; // extract icode
	ifun = icode_ifun_arr[1]; // extract ifun
	
	fseek(fileStream, PC, SEEK_SET);
	bytesToRead = getBytesToRead(icode); // get num bytes to read based on icode. Will terminate if invalid icode
	
	numBytesCanRead = fread(instrBuffer, 1, bytesToRead, fileStream); // read bytesToRead into instrBuffer
	
	// if not enough bytes, set exit condition
	if (numBytesCanRead < bytesToRead) {
		notEnoughBytes = TRUE;
	}
	processInstr();
	
  } 
  
  // if PC strictly exceeds length of file, normal termination
  printf("Normal termination\n");
  exit(0);

  return SUCCESS;

}


