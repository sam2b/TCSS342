/*
 *  slc3.h
 *
 *  Date Due: Apr 22, 2018
 *  Authors:  Sam Brendel, Tyler Shupack
 *  Problem 3,4
 *  version: 4.16g
 */
#include <stdio.h>
#include <stdbool.h>

#ifndef SLC3_H_
#define SLC3_H_

#define MEMORY_SIZE         100
#define FILENAME_SIZE       200
#define SIMULATOR_OFFSET 0x3000

#define FETCH     0
#define DECODE    1
#define EVAL_ADDR 2
#define FETCH_OP  3
#define EXECUTE   4
#define STORE     5

#define OP_ADD   1 // 0001 0000 0000 0000
#define OP_AND   5 // 0101 0000 0000 0000
#define OP_NOT   9 // 1001 0000 0000 0000
#define OP_TRAP 15 // 1111 0000 0000 0000
#define OP_LD    2 // 0010 0000 0000 0000
#define OP_ST    3 // 0011 0000 0000 0000
#define OP_JMP  12 // 1100 0000 0000 0000
#define OP_BR    0 // 0000 0000 0000 0000

#define MASK_OPCODE  61440 // 1111 0000 0000 0000
#define MASK_DR       3584 // 0000 1110 0000 0000
#define MASK_SR1       448 // 0000 0001 1100 0000
#define MASK_SR2         7 // 0000 0000 0000 0111
#define MASK_PCOFFSET9 511 // 0000 0001 1111 1111
#define MASK_TRAPVECT8 255 // 0000 0000 1111 1111
#define MASK_BIT5       32 // 0000 0000 0010 0000
#define MASK_IMMED5     31 // 0000 0000 0001 1111
#define MASK_NZP      3584 // 0000 1110 0000 0000
#define MASK_CC_N        7
#define MASK_CC_Z        5
#define MASK_CC_P        1

#define CONDITION_N   4 // 0000 1000 0000 0000
#define CONDITION_Z   2 // 0000 0100 0000 0000
#define CONDITION_P   1 // 0000 0010 0000 0000
#define CONDITION_NZ  6 // 0000 1100 0000 0000
#define CONDITION_NP  5 // 0000 1010 0000 0000
#define CONDITION_ZP  3 // 0000 0110 0000 0000
#define CONDITION_NZP 7 // 0000 1110 0000 0000

#define BITSHIFT_OPCODE 12
#define BITSHIFT_DR      9
#define BITSHIFT_CC      9
#define BITSHIFT_SR1     6
#define BITSHIFT_BIT5    5
#define BITSHIFT_CC_BIT3 2
#define BITSHIFT_CC_BIT2 1

struct CPUType {
	unsigned short int pc;     // program counter.
	unsigned short cc;         // condition code for BR instruction.
	unsigned short int reg[8]; // registers.
	unsigned short int ir;     // instruction register.
	unsigned short mar;        // memory address register.
	unsigned short mdr;        // memory data register.
	unsigned short microState;
	unsigned short A;
	unsigned short B;
};

typedef struct CPUType CPU_p;

int controller (CPU_p *);
void displayCPU(CPU_p *, int);
void zeroOut(unsigned short *array, int);
CPU_p initialize();
unsigned short ZEXT(unsigned short);
short SEXT(unsigned short);
void TRAP(unsigned short, CPU_p *);
unsigned short  getCC(short);
bool doBen(unsigned short, CPU_p *);
void displayHeader();
FILE* openFileText(char *);
void loadProgramInstructions(FILE *);

#endif /* SLC3_H_ */
