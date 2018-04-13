/*
 *  slc3.c
 *
 *  Date Due: Apr 22, 2018
 *  Authors:  Sam Brendel, Tyler Shupack
 *  Problem 3,4
 *  version: 4.11a
 */

#include "slc3.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


unsigned short memory[100]; // 32 words of memory enough to store simple program
bool isTrap = false;

//***Simulates trap table lookup for now***
void trap(unsigned short vector, CPU_p cpu) {
    switch (vector) {
    case 0x25:
        printf("==========HALT==========\n");
        isTrap = true;
        break;
    default: 
        printf("Err: Unknown Trap vector?\n");
        break;
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

        case FETCH_OP:
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

        case STORE: // Look at ST. Microstate 16 is the store to memory
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

        //if (state == FETCH)
        if (isTrap)
           break;

    } // end for()

    if (isTrap) {
        displayCPU(cpu);
    }

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


void displayHeader() {
    printf("Welcome to the LC-3 Simulator Simulator\n");
}

/**
 * Print out fields to the console for the CPU_p object.
 */
void displayCPU(CPU_p cpu) {
    displayHeader();
    printf("Registers                     Memory\n");

    // First 8 lines
    int i = 0;
    for(i = 0; i < 8; i++) {
        printf("R%u: %#.4x", i, memory[cpu.reg[i]]);   // Registers.
        printf("%#26x: %#.4x\n", i+0x3000, memory[i]); // Memory.
    }

    // Next 3 lines
    int j;
    for (j = 0; j < 3; j++ & i++) {
        printf("%#36x: %#.4x\n", i+0x3000, memory[i]);
    }

    // Next 4 lines.
    printf("PC:  %#.4x    IR: %#.4x     %#.4x: %#.4x\n", cpu.PC, cpu.ir, i+0x3000, memory[i]);
    printf("A:   %#.4x       B: %#.4x       %#.4x: %#.4x\n", cpu.A, cpu.B, i+0x3000, memory[i++]);
    printf("MAR: %#.4x   MDR: %#.4x     %#.4x: %#.4x\n", cpu.PC, cpu.ir, i+0x3000, memory[i++]);
    printf("CC:  N:%d Z:%d P:%d              %#.4x: %#.4x\n", cpu.cc >> 2 & 7, cpu.cc >> 1 & 5, cpu.cc & 1, i+0x3000, memory[i++]);

    // Last 2 lines.
    printf("%#36x: %#.4x\n", i+0x3000, memory[i++]);
    printf("Select: 1) Load,  3) Step,  5) Display Mem,  9) Exit\n");




//    int i;
//    //printf("Registers: ");
//    for (i=0; i<8; i++) {
//        printf("R%d=%x   ", i, memory[i+24]);
//    }
//    printf("\nPC=%#x   cc=%#x   ir=%#x   mar=%#x   mdr=%#x\n", cpu.PC, cpu.cc, cpu.ir, cpu.mar, cpu.mdr);
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
                , { 92, 93, 94, 95, 96, 97, 98, 99 } // The last 8 elements of the memory[] array.
                , 0    // ir
                , 0    // mar
                , 0 }; // mdr

    int i;
    for (i = 0; i < 100; i++) {
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
