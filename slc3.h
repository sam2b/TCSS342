/*
 *  slc3.h
 *
 *  Date Due: June 1, 2018
 *  Author:  Sam Brendel
 *  Final Project
 *  version: 5.31d
 */

#include <stdio.h>
#include <stdbool.h>
#include <ncurses.h>

#ifndef SLC3_H_
#define SLC3_H_

#define MEMORY_SIZE       0xFFFF //0x03E8 // 1000 lines, Displayed as x33E8
#define REGISTER_SIZE          8
#define FILENAME_SIZE        200
#define STRING_SIZE          200
#define OUTPUT_LINE_NUMBER    24
#define OUTPUT_COL_NUMBER      8
#define OUTPUT_AREA_DEPTH      6
#define ADDRESS_START     0x3000
#define MAX_HEX_BITS           4
#define HEX_BITS              16
#define REGISTER_5             5 // Can store the result from a sub routine.
#define REGISTER_6             6 // Exclusively used for the stack.
#define REGISTER_7             7
#define WINDOW_WIDTH          82
#define WINDOW_LEGTH          50
#define COLUMN_LABEL_MEMORY   31
#define COLUMN_LABEL_REGISTERS 1
#define COLUMN_LABEL_TITLE     1
#define STACK_OFFSET_R0        8

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
#define OP_LDR   6 // 0110 0000 0000 0000
#define OP_ST    3 // 0011 0000 0000 0000
#define OP_STR   7 // 0111 0000 0000 0000
#define OP_STI  11 // 1011 0000 0000 0000
#define OP_JMP  12 // 1100 0000 0000 0000
#define OP_RET  12 // 1100 0000 0000 0000
#define OP_BR    0 // 0000 0000 0000 0000
#define OP_JSR   4 // 0100 0000 0000 0000
#define OP_JSRR  4 // 0100 0000 0000 0000
#define OP_LEA  14 // 1110 0000 0000 0000
#define OP_LDI  10 // 1010 0000 0000 0000
#define OP_PP   13 // 1101 0000 0000 0110 Custom for stack push and pop. R6 is the stack pointer.

#define MASK_OPCODE               61440 // 1111 0000 0000 0000
#define MASK_DR                    3584 // 0000 1110 0000 0000
#define MASK_SR1                    448 // 0000 0001 1100 0000
#define MASK_SR2                      7 // 0000 0000 0000 0111
#define MASK_PCOFFSET11            2047 // 0000 0111 1111 1111
#define MASK_PCOFFSET9              511 // 0000 0001 1111 1111
#define MASK_PCOFFSET6               63 // 0000 0000 0011 1111
#define MASK_TRAPVECT8              255 // 0000 0000 1111 1111
#define MASK_BIT11                 2048 // 0000 1000 0000 0000
#define MASK_BIT5                    32 // 0000 0000 0010 0000
#define MASK_IMMED5                  31 // 0000 0000 0001 1111
#define MASK_NZP                   3584 // 0000 1110 0000 0000
#define MASK_CC_N                     7 // 0000 1000 0000 0000
#define MASK_CC_Z                     5 // 0000 0100 0000 0000
#define MASK_CC_P                     1 // 0000 0010 0000 0000
#define MASK_NEGATIVE_IMMEDIATE  0xFFE0 // 1111 1111 1110 0000
#define MASK_NEGATIVE_PCOFFSET11 0xF800 // 1111 1000 0000 0000
#define MASK_NEGATIVE_PCOFFSET9  0xFE00 // 1111 1110 0000 0000
#define MASK_NEGATIVE_PCOFFSET6  0xFFC0 // 1111 1111 1100 0000
#define MASK_STACK_POINTER            7 // 0000 0000 0000 0111

#define CONDITION_N   4 // 0000 1000 0000 0000
#define CONDITION_Z   2 // 0000 0100 0000 0000
#define CONDITION_P   1 // 0000 0010 0000 0000
#define CONDITION_NZ  6 // 0000 1100 0000 0000
#define CONDITION_NP  5 // 0000 1010 0000 0000
#define CONDITION_ZP  3 // 0000 0110 0000 0000
#define CONDITION_NZP 7 // 0000 1110 0000 0000

// How many times to shift the bits.
#define BITSHIFT_OPCODE              12
#define BITSHIFT_DR                   9
#define BITSHIFT_CC                   9
#define BITSHIFT_SR1                  6
#define BITSHIFT_BIT5                 5
#define BITSHIFT_BIT11               11
#define BITSHIFT_CC_BIT3              2
#define BITSHIFT_CC_BIT2              1
#define BITSHIFT_NEGATIVE_IMMEDIATE   4
#define BITSHIFT_NEGATIVE_PCOFFSET11 10
#define BITSHIFT_NEGATIVE_PCOFFSET9   8
#define BITSHIFT_NEGATIVE_PCOFFSET6   5

#define TRAP_VECTOR_X20  0x20
#define TRAP_VECTOR_X21  0x21
#define TRAP_VECTOR_X22  0x22
#define TRAP_VECTOR_X25  0x25

#define BIT_IMMED        16 // 0000 0000 0001 0000
#define BIT_PCOFFSET11 1024 // 0000 0100 0000 0000
#define BIT_PCOFFSET9   256 // 0000 0001 0000 0000
#define BIT_PCOFFSET6    32 // 0000 0000 0010 0000

struct CPUType {
    unsigned short int pc;     // program counter.
    unsigned short cc;         // condition code for BR instruction.
    unsigned short int reg[8][2]; // registers. Element [n][1] boolean denotes if the value is a memory address.
    unsigned short int ir;     // instruction register.
    unsigned short mar;        // memory address register.
    unsigned short mdr;        // memory data register.
    unsigned short A;
    unsigned short B;
};
typedef struct CPUType CPU_p;

bool isHalted = false;
bool isRun = false;
int outputLineCounter = 0;
int outputColCounter = 0;
int isPopped = 0;
unsigned short *regRzeroPointer;

bool  branchEnabled(unsigned short, CPU_p *);
void  clearOutput(WINDOW *);
void  clearPrompt(WINDOW *);
int   controller(CPU_p *, WINDOW *);
void  cursorAtPrompt(WINDOW *, char *);
void  cursorAtInput(WINDOW *, char *);
void  cursorAtOutput(WINDOW *, char *);
void  cursorAtCustom(WINDOW *, int, int, char *);
void  writeToFile(WINDOW *theWindow, char *);
void  displayCPU(CPU_p *, int);
void  stackPush(CPU_p *);
void  displayHeader();
int   hexCheck(char num[]);
int   hexCheckAddress(char num[]);
CPU_p initialize();
void  loadProgramInstructions(FILE *, WINDOW *);
FILE* openFileText(char *, WINDOW *);
short sext(unsigned short, int);
short toSign(unsigned short);
void  trap(unsigned short, CPU_p *, WINDOW *);
void  zeroOutMemory(unsigned short *);
void  resetBreakPoints(unsigned char *);
//void  zeroRegisters(unsigned short *[8][2]);
unsigned short getCC(unsigned short);
unsigned short ZEXT(unsigned short);
void jsrStackPush(CPU_p *, unsigned short *);
void jsrStackPop(CPU_p *, unsigned short *);
void setPointer(CPU_p *, unsigned short *, unsigned short, bool, unsigned short *);

#endif /* SLC3_H_ */
