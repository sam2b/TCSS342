/*
 *  slc3.h
 *
 *  Date Due: Apr 22, 2018
 *  Authors:  Sam Brendel, Tyler Richard Shupack
 *  Problem 3,4
 *  version: 4.11a
 */

#ifndef SLC3_H_
#define SLC3_H_

struct CPUType {
	unsigned short int PC;     // program counter.
	unsigned short cc;         // condition code for BR instruction.
	unsigned short int reg[8]; // registers.
	unsigned short int ir;     // instruction register.
	unsigned short mar;        // memory address register.
	unsigned short mdr;        // memory data register.
	unsigned short A;
	unsigned short B;
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
void displayHeader();

#endif /* SLC3_H_ */
