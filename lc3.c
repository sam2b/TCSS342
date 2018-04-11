/*
 * lc3.c
 *
 *  Date Due: Apr 8, 2018
 *  Authors:  Sam Brendel, and Samantha Anderson
 *  version: 408d
 */

#include "lc3.h"
#include <stdio.h>
#include <stdlib.h>

#define FETCH     0
#define DECODE    1
#define EVAL_ADDR 2
#define FETCH_OP  3
#define EXECUTE   4
#define STORE     5

#define OP_ADD             1 // 0001 0000 0000 0000
#define OP_AND             5 // 0101 0000 0000 0000
#define OP_NOT             9 // 1001 0000 0000 0000
#define OP_TRAP           15 // 1111 0000 0000 0000
#define OP_LD              2 // 0010 0000 0000 0000
#define OP_ST              3 // 0011 0000 0000 0000
#define OP_JMP            12 // 1100 0000 0000 0000
#define OP_BR              0 // 0000 0000 0000 0000

#define MASK_OPCODE    61440 // 1111 0000 0000 0000
#define MASK_DR         3584 // 0000 1110 0000 0000
#define MASK_SR1         448 // 0000 0001 1100 0000
#define MASK_SR2           7 // 0000 0000 0000 0111
#define MASK_PCOFFSET9   511 // 0000 0001 1111 1111
#define MASK_TRAPVECT8   255 // 0000 0000 1111 1111
#define MASK_BIT5         32 // 0000 0000 0010 0000
#define MASK_IMMED5       31 // 0000 0000 0001 1111
#define MASK_NZP        3584 // 0000 1110 0000 0000

#define CONDITION_N        4 // 0000 1000 0000 0000
#define CONDITION_Z        2 // 0000 0100 0000 0000
#define CONDITION_P        1 // 0000 0010 0000 0000
#define CONDITION_NZ       6 // 0000 1100 0000 0000
#define CONDITION_NP       5 // 0000 1010 0000 0000
#define CONDITION_ZP       3 // 0000 0110 0000 0000
#define CONDITION_NZP      7 // 0000 0110 0000 0000


unsigned short memory[32]; // 32 words of memory enough to store simple program
unsigned short reg[8];
unsigned short op1;
unsigned short op2;
unsigned short result;
char brCC[3];

//***Simulates trap table lookup for now***
void trap(unsigned short vector, CPU_p cpu)
{
    switch (vector)
    {
    case 0x25:
        displayCPU(cpu);
        printf("==========HALT==========\n");
        //exit(0);
        
    default: 
        printf("Err: Unknown Trap vector?\n");
        break;
    }
}

void displayMem(CPU_p cpu)
{
    printf("\nMemory:\n");
    int j;
    for(j=0; j<32; j++) {
        printf("%2u  %#x \n", j, memory[j]);
    }
}

int controller(CPU_p cpu) {

    // check to make sure both pointers are not NULL
    // do any initializations here
    unsigned int opcode, dr, sr1, sr2, bit5, immed, offset, state, condition;    // fields for the IR
    unsigned short vector8; //uint8_t vector8;
    unsigned short vector16;

    state = FETCH;
    for (;;) { // efficient endless loop to be used in the next problem
        switch (state) {
            case FETCH: // microstates 18, 33, 35 in the book.
                cpu.mar = cpu.PC;          // Step 1: MAR is loaded with the contends of the PC,
                cpu.PC++;                  //         and also increment PC. Only done in the FETCH phase.
                cpu.mdr = memory[cpu.mar]; // Step 2: Interrogate memory, resulting in the instruction placed into the MDR.
                cpu.ir = cpu.mdr;          // Step 3: Load the IR with the contents of the MDR.
                state = DECODE;
                break;

            case DECODE: // microstate 32
                opcode = cpu.ir & MASK_OPCODE; // Input is the four-bit opcode IR[15:12]. The output line asserted is the one corresponding to the opcode at the input.
                opcode = opcode >> 12;
                switch (opcode) {
                // different opcodes require different handling
                // compute effective address, e.g. add sext(immed7) to register.
                case OP_LD:
                    dr = cpu.ir >> 9 & 0x7;
                    offset = cpu.ir & 0x1FF;
                    break;
            }
            state = EVAL_ADDR;
            break;

        case EVAL_ADDR:
            // This phase computes the address of the memory location that is needed to process the instruction.
            // NOTE: Study each opcode to determine what all happens this phase for that opcode.
            // Look at the LD instruction to see microstate 2 example.
            switch (opcode) {
                // different opcodes require different handling
                // compute effective address, e.g. add sext(immed7) to
                // register
                case OP_LD:
                    cpu.mar = cpu.PC + offset; // microstate 2.
                    cpu.mdr = memory[cpu.mar]; // microstate 25.
                    break;
                case OP_ST:
                    dr = cpu.ir & MASK_DR;         // This is actually a source register, but still use dr.
                    dr = dr >> 9;
                    offset = cpu.ir & MASK_PCOFFSET9;
                    cpu.mar = cpu.PC + offset; // microstate 2.
                    break;
            }
            state = FETCH_OP;
            break;

        case FETCH_OP: // sam B.
            switch (opcode) {
                // get operands out of registers into A, B of ALU
                // or get memory for load instr.
                case OP_ADD:
                case OP_AND:
                    dr = cpu.ir & MASK_DR;
                    dr = dr >> 9;
                    sr1 = cpu.ir & MASK_SR1;
                    sr1 = sr1 >> 6;
                    bit5 = cpu.ir & MASK_BIT5;
                    bit5 = bit5 >> 5;
                    if (bit5 == 0) {
                        sr2 = cpu.ir & MASK_SR2; // no shift needed.
                    } else if (bit5 == 1) {
                        immed = cpu.ir & MASK_IMMED5; // no shift needed.
                    }
                    // The book page 106 says current microprocessors can be done simultaneously during fetch, but this simulator is old skool.
                    break;
                case OP_NOT:
                    dr = cpu.ir & MASK_DR;
                    dr = dr >> 9;
                    sr1 = cpu.ir & MASK_SR1;
                    sr1 = sr1 >> 6;
                    break;
                case OP_TRAP:
                    vector8 = cpu.ir & MASK_TRAPVECT8; // No shift needed.
                    break;
                case OP_LD:
                    dr = cpu.ir & MASK_DR;
                    dr = dr >> 9;
                    offset = cpu.ir & MASK_PCOFFSET9;
                    
                    offset = SEXT(offset);
                    cpu.mar = cpu.PC + offset;
                    cpu.mdr = memory[cpu.mar];
                    break;
                case OP_ST: // Same as LD.
                    // Book page 124.
                    cpu.mdr = memory[cpu.reg[dr]];
                    break;
                case OP_JMP:
                    sr1 = cpu.ir & MASK_SR1;
                    sr1 = sr1 >> 6;
                    break;
                case OP_BR:
                    cpu.cc = cpu.ir & MASK_NZP;
                    cpu.cc = cpu.cc >> 9;
                    offset = cpu.ir & MASK_PCOFFSET9;
                    break;
                default:
                    break;
            }
            state = EXECUTE;
            break;

        case EXECUTE: // Note that ST does not have an execute microstate.
            switch (opcode) {
                case OP_ADD:
                    if (bit5 == 0) {
                        cpu.mdr = memory[cpu.reg[sr2]] + memory[cpu.reg[sr1]]; // Is result actually the MDR?
                    } else if (bit5 == 1) {
                        cpu.mdr = memory[cpu.reg[sr1]] + immed;
                    }
                    condition = getConditionCode(cpu.mdr);
                    break;
                case OP_AND:
                    if (bit5 == 0) {
                        cpu.mdr = memory[cpu.reg[sr2]] & memory[cpu.reg[sr1]];
                    } else if (bit5 == 1) {
                        cpu.mdr = memory[cpu.reg[sr1]] & immed;
                    }
                    break;
                case OP_NOT:
                    cpu.mdr = ~memory[cpu.reg[sr1]];
                    break;
                case OP_TRAP:
                    // Book page 222.
                    vector16 = ZEXT(vector8); // TODO: should we make this actually do a zero extend to 16 bits?
                    cpu.mar = vector16;
                    memory[cpu.reg[7]] = cpu.PC; // Store the PC in R7 before loading PC with the starting address of the service routine.
                    cpu.mdr = memory[cpu.mar]; // read the contents of the register.
                    cpu.PC = cpu.mdr; // The contents of the MDR are loaded into the PC.  Load the PC with the starting address of the service routine.
                    trap(vector8, cpu);
                    break;
                case OP_JMP:
                    cpu.PC = memory[cpu.reg[sr1]];
                    break;
                case OP_BR:
                     if ((cpu.cc == condition) || (cpu.cc == CONDITION_NZP)) {
                        cpu.mar = cpu.PC + offset;
                    }
                    break;
            }

            state = STORE;
            break;

        case STORE: // Look at ST. Microstate 16 is the store to memory //Sam B.
            switch (opcode) {
            // write back to register or store MDR into memory
            case OP_ADD:
            case OP_AND: // Same as ADD
            case OP_NOT: // Sam as AND and AND.
                memory[cpu.reg[dr]] = cpu.mdr;
                break;
            case OP_LD:
                memory[cpu.reg[dr]] = cpu.mdr; // Load into the register.
                break;
            case OP_ST:
                memory[cpu.mar] = cpu.mdr;     // Store into memory.
                break;
            case OP_BR:
                cpu.PC = cpu.mar;
                break;
            }

            // do any clean up here in prep for the next complete cycle

            state = FETCH;
            break;
        } // end switch (state)

        if ((state == FETCH))
            break;

    } // end for()
    displayCPU(cpu);
    displayMem(cpu);
    return 0;
} // end controller()

unsigned short getConditionCode(unsigned short value) {
    unsigned short code;

    if (value <= 0) {
        if (value < 0)
            code = CONDITION_Z;
        else if (value == 0)
                code = CONDITION_Z;
        else
            code = CONDITION_NZ;
    } else if (value >= 0) {
        if (value > 0)
            code = CONDITION_P;
        else if (value == 0)
                code = CONDITION_Z;
        else
            code = CONDITION_ZP;
    }
    return code;
}

void JUMP(CPU_p *cpu, unsigned short whereTo) {
    cpu->PC = whereTo;
}

unsigned short SEXT(unsigned short value) {
    // Simulated SEXT.
    return value;
}

unsigned short ZEXT(unsigned short value) {
    // Simulated ZEXT.
    return value;
}

/**
 * Print out fields to the console for the CPU_p object.
 */
void displayCPU(CPU_p cpu) {
    int i;
    printf("Registers: ");
    for (i=0; i<8; i++) {
        printf("R%d=%x   ", i, memory[i+24]);
    }
    printf("\nPC=%#x   cc=%#x   ir=%#x   mar=%#x   mdr=%#x\n", cpu.PC, cpu.cc, cpu.ir,
            cpu.mar, cpu.mdr);

    
}

/**
 * Sets all elements to zero.
 */
void zeroOut(unsigned short *array, int quantity) {
    int i;
    for (i = 0; i <= quantity; i++) {
        array[i] = 0;
    }
}

/**
 * Initializes a CPU_p object and its fields.
 * Removes the junk from these memory locations.
 */
CPU_p initialize() {
    CPU_p cpu = { 0    // PC
                , 0    // cc
                , { 24, 25, 26, 27, 28, 29, 30, 31 } // The last 8 elements of the memory[] array.
                , 0    // ir
                , 0    // mar
                , 0 }; // mdr

    zeroOut(memory, 32);
    int i = 0;
    
    for (; i < 24; i++)
    {
        memory[i] = i;
    }

    // Intentionally hard coding these values into two memory registers.
    memory[cpu.reg[1]] = 3;
    memory[cpu.reg[2]] = 4;
    memory[cpu.reg[3]] = 0xB0B0;   // Intentional simulated data.
    memory[4] = 0xA0A0;            // Intentional simulated data.
    memory[cpu.reg[0]] = 0xD0E0;     // Intentional simulated data.
    return cpu;
}

/**
 * Driver for the program.
 */
int main(int argc, char* argv[]) {
    CPU_p cpu = initialize();
    memory[0] = strtol(argv[1], NULL, 16);
    controller(cpu);
}
