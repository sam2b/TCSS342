/*
 * lc3.h
 *
 *  Date Due: Apr 22, 2018
 *  Authors:  Sam Brendel, other
 *  Problem 3,4
 *  version: 4.10
 */

#ifndef LC3_H_
#define LC3_H_

struct CPUType {
	unsigned short int PC;     // program counter.
	unsigned short cc;         // condition code for BR instruction.
	unsigned short int reg[8]; // registers.
	unsigned short int ir;     // instruction register.
	unsigned short mar;        // memory address register.
	unsigned short mdr;        // memory data register.
};

typedef struct CPUType CPU_p;

int controller (CPU_p);
void displayCPU(CPU_p);
void zeroOut(unsigned short *array, int);
CPU_p initialize();
unsigned short ZEXT(unsigned short);
unsigned short SEXT(unsigned short);
void TRAP(unsigned short, CPU_p);
unsigned short getConditionCode(unsigned short);

#endif /* LC3_H_ */
