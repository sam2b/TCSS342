/*
 *  slc3.c
 *
 *  Date Due: June 1, 2018
 *  Author:  Sam Brendel
 *  Final Project
 *  version: 5.31c
 */

#include "slc3.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>

// Represents system memory outside of the CPU.
unsigned short memory[MEMORY_SIZE];
// A symbolic value representing a position in memory if the user has set a breakpoint.
char breakPoint[MEMORY_SIZE];

/**
 * Simulates trap table lookup.
 * @param vector the area in memory that is simulated to be looked up to
 *        execute the requested TRAP routine.
 * @param cpu the cpu object that contains data.
 */
void trap(unsigned short vector, CPU_p *cpu, WINDOW *theWindow) {
    switch (vector) {
        case TRAP_VECTOR_X20: ;// GETC
            noecho(); //turn echo off
            char *input = (char*) malloc(sizeof(char));
            cursorAtInput(theWindow, input);
            cpu->reg[0][0] = *input;
            cpu->reg[0][1] = false;
            //printf("\nDOING TRAP X20\n");
            free(input);
            //echo(); //turn echo back on.
            break;
        case TRAP_VECTOR_X21: ;// OUT
            /* put R0 value into char variable, then send to "cursor" function */
            char *output = (char*) malloc(sizeof(char) * 2);
            output[0] = cpu->reg[0][0];
            output[1] = '\0'; // null-terminator
            cursorAtOutput(theWindow, output);
            //printf("\nDOING TRAP X21\n");
            free(output);
            break;
        case TRAP_VECTOR_X22: ;//PUTS trap command
            //printf("\nDOING TRAP X22\n");
            char *outputString = (char*) malloc(sizeof(char) * 40);
            short memCounter = cpu->reg[0][0];
            short outCounter = 0;
            /* store characters in string until character is null pointer,
            * starting with character stored in memory at reg[0][0] and incrementing until
            * null pointer is reached. */
            while (memory[memCounter] != 0) {
                outputString[outCounter] = memory[memCounter];
                memCounter++;
                outCounter++;
            }
            outputString[outCounter] = '\0';
            cursorAtOutput(theWindow, outputString);
            free(outputString);
            break;
        case TRAP_VECTOR_X25: // HALT
            cursorAtPrompt(theWindow, "========HALT========");
            //cpu->pc = 0; // reset to zero as per Prof Mobus.
            isHalted = true;
            isRun = false;
            break;
        default: 
            cursorAtPrompt(theWindow, "Error: Unknown Trap vector");
            break;
    }
    echo(); //turn echo back on.
}

/**
 * The controller component of the LC-3 simulator.
 * @param cpu the cpu object to contain data.
 */
int controller(CPU_p *cpu, WINDOW *theWindow) {

    // check to make sure both pointers are not NULL
    // do any initializations here
    unsigned int opcode, dr=0, sr1=0, sr2=0, bit5, bit11, state, nzp;
    short offset, immed;
    unsigned short vector8, vector16;
    unsigned short *regDrPointer, *regSr1Pointer, *regSr2Pointer; // used for nested subroutines.
    bool isCycleComplete = false;
    unsigned short *stackPointer = (unsigned short *)cpu->reg[REGISTER_6];

    state = FETCH;
    while (!isHalted) {
        switch (state) {
            case FETCH: // microstates 18, 33, 35 in the book.
                //printf("Now in FETCH---------------\n");
                cpu->mar = cpu->pc;           // Step 1: MAR is loaded with the contends of the PC,
                cpu->pc++;                    //         and also increment PC. Only done in the FETCH phase.
                cpu->mdr = memory[cpu->mar];  // Step 2: Interrogate memory, resulting in the instruction placed into the MDR.
                cpu->ir  = cpu->mdr;          // Step 3: Load the IR with the contents of the MDR.
                state    = DECODE;
                break;

            case DECODE: // microstate 32
                //printf("Now in DECODE---------------\n");
                opcode = (cpu->ir & MASK_OPCODE) >> BITSHIFT_OPCODE; // Input is the four-bit opcode IR[15:12]. The output line asserted is the one corresponding to the opcode at the input.
                //opcode = opcode  >> BITSHIFT_OPCODE;
                state = EVAL_ADDR;
                break;

            case EVAL_ADDR:
                //printf("Now in EVAL_ADDR---------------\n");
                // This phase computes the address of the memory location that is needed to process the instruction.
                // NOTE: Study each opcode to determine what all happens this phase for that opcode.
                // Look at the LD instruction to see microstate 2 example.
                switch (opcode) {
                    // different opcodes require different handling
                    // compute effective address, e.g. add sext(immed7) to
                    // register
                case OP_ADD:
                case OP_AND:
                    dr   = (cpu->ir & MASK_DR)   >> BITSHIFT_DR;
                    sr1  = (cpu->ir & MASK_SR1)  >> BITSHIFT_SR1;
                    bit5 = (cpu->ir & MASK_BIT5) >> BITSHIFT_BIT5;
                    //setPointer(cpu, regSr1Pointer, sr1, isPopped stackPointer);
                    // Account for nested subroutines using their own R0 has a returned result.
                    if (sr1 == 0 && isPopped) {
                        regSr1Pointer = regRzeroPointer;
                    } else {
                        regSr1Pointer = (unsigned short *)cpu->reg[sr1]; //regPointer = (unsigned short *)cpu->reg[reg];
                    }
                    if (bit5 == 0) {
                        sr2 = cpu->ir & MASK_SR2; // no shift needed.
                        //setPointer(cpu, regSr2Pointer, sr2, isPopped stackPointer);
                        // Account for nested subroutines using their own R0 has a returned result.
                        if (sr2 == 0 && isPopped) {
                            regSr2Pointer = regRzeroPointer;
                        } else {
                            regSr2Pointer = (unsigned short *)cpu->reg[sr2]; //regPointer = (unsigned short *)cpu->reg[reg];
                        }
                    } else { // bit5 == 1
                        immed = cpu->ir & MASK_IMMED5; // no shift needed.
                        immed = sext(immed, BIT_IMMED);
                    }
                    // The book page 106 says current microprocessors can be done simultaneously during fetch, but this simulator is old skool.
                    break;
                case OP_NOT:
                    dr  = (cpu->ir & MASK_DR)  >> BITSHIFT_DR;
                    sr1 = (cpu->ir & MASK_SR1) >> BITSHIFT_SR1;
                    //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                    // Account for nested subroutines using their own R0 has a returned result.
                    if (sr1 == 0 && isPopped) {
                        regSr1Pointer = regRzeroPointer;
                    } else {
                        regSr1Pointer = (unsigned short *)cpu->reg[sr1]; //regPointer = (unsigned short *)cpu->reg[reg];
                    }
                    break;
                    case OP_LD:
                        dr       = (cpu->ir     & MASK_DR) >> BITSHIFT_DR;
                        offset   = sext(cpu->ir & MASK_PCOFFSET9, BIT_PCOFFSET9);
                        cpu->mar = cpu->pc + offset; // microstate 2.
                        cpu->mdr = memory[cpu->mar]; // microstate 25.
                        break;
                    case OP_LDR:
                        dr       = (cpu->ir      & MASK_DR)  >> BITSHIFT_DR;
                        sr1      = (cpu->ir      & MASK_SR1) >> BITSHIFT_SR1;
                        //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                        // Account for nested subroutines using their own R0 has a returned result.
                        if (sr1 == 0 && isPopped) {
                            regSr1Pointer = regRzeroPointer;
                        } else {
                            regSr1Pointer = (unsigned short *)cpu->reg[sr1]; //regPointer = (unsigned short *)cpu->reg[reg];
                        }
                        offset   = sext(cpu->ir  & MASK_PCOFFSET6, BIT_PCOFFSET6);
                        //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                        cpu->mar = *regSr1Pointer + offset;
                        cpu->mdr = memory[cpu->mar];
                        break;
                    case OP_LDI:
                        dr       = (cpu->ir     & MASK_DR) >> BITSHIFT_DR;
                        offset   = sext(cpu->ir & MASK_PCOFFSET9, BIT_PCOFFSET9);
                        cpu->mar = cpu->pc + offset; // microstate 2.
                        cpu->mdr = memory[memory[cpu->mar] - ADDRESS_START]; // microstate 25.
                        break;
                    case OP_ST:
                    case OP_STI: // Same as ST.
                        dr       = (cpu->ir     & MASK_DR) >> BITSHIFT_DR; // Actually a source register, but still use dr.
                        offset   = sext(cpu->ir & MASK_PCOFFSET9, BIT_PCOFFSET9);
                        cpu->mar = cpu->pc + offset; // microstate 2.
                        break;
                    case OP_STR:
                        dr       = (cpu->ir      & MASK_DR)  >> BITSHIFT_DR;  // Actually source register.
                        sr1      = (cpu->ir      & MASK_SR1) >> BITSHIFT_SR1; // Base register.
                        //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                        // Account for nested subroutines using their own R0 has a returned result.
                        if (sr1 == 0 && isPopped) {
                            regSr1Pointer = regRzeroPointer;
                        } else {
                            regSr1Pointer = (unsigned short *)cpu->reg[sr1]; //regPointer = (unsigned short *)cpu->reg[reg];
                        }
                        offset   = sext(cpu->ir  & MASK_PCOFFSET6, BIT_PCOFFSET6);
                        //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                        cpu->mar = *regSr1Pointer + offset;
                        break;
                    case OP_LEA:
                        dr       = (cpu->ir & MASK_DR) >> BITSHIFT_DR;
                        offset   = sext(cpu->ir  & MASK_PCOFFSET9, BIT_PCOFFSET9);
                        break;
                    case OP_JSR: // includes JSRR.
                        // R5 is expected to contain the result from a subroutine.
                        offset = sext(cpu->ir & MASK_PCOFFSET11, BIT_PCOFFSET11);
                        break;
                    case OP_PP:
                        // R6 is exclusively used as the stack pointer, and is already initialized to the last memory[] element.
                        // The stack shall begin at the last element in memory[].
                        // The first stack item begins at one element before the last element.
                        bit5 = (cpu->ir & MASK_BIT5) >> BITSHIFT_BIT5;
                        *stackPointer = cpu->ir & MASK_PP; // TODO is this a problem?
                        if (bit5 == 0) {
                            // Push.
                            sr1 = (cpu->ir & MASK_SR1) >> BITSHIFT_SR1;
                            //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                            // Account for nested subroutines using their own R0 has a returned result.
                            if (sr1 == 0 && isPopped) {
                                regSr1Pointer = regRzeroPointer;
                            } else {
                                regSr1Pointer = (unsigned short *)cpu->reg[sr1]; //regPointer = (unsigned short *)cpu->reg[reg];
                            }
                        } else {
                            // Pop.
                            dr = (cpu->ir & MASK_DR) >> BITSHIFT_DR;
                        }
                        break;
                }
                state = FETCH_OP;
                break;
                
            case FETCH_OP:
                //printf("Now in FETCH_OP---------------\n");
                switch (opcode) {
                    // get operands out of registers into A, B of ALU
                    // or get memory for load instr.
                    case OP_TRAP:
                        vector8 = cpu->ir & MASK_TRAPVECT8; // No shift needed.
                        break;
                    case OP_ST:
                    case OP_STI:
                    case OP_STR: // Book page 124.
                        cpu->mdr = cpu->reg[dr][0];//*regDrPointer;
                        break;
                    //case OP_RET:
                    case OP_JMP: // includes RET.
                        sr1 = (cpu->ir & MASK_SR1) >> BITSHIFT_SR1;
                        //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                        // Account for nested subroutines using their own R0 has a returned result.
                        if (sr1 == 0 && isPopped) {
                            regSr1Pointer = regRzeroPointer;
                        } else {
                            regSr1Pointer = (unsigned short *)cpu->reg[sr1]; //regPointer = (unsigned short *)cpu->reg[reg];
                        }
                        break;
                    case OP_BR:
                        nzp = (cpu->ir & MASK_NZP) >> BITSHIFT_CC;
                        offset = cpu->ir & MASK_PCOFFSET9;
                        break;
                    case OP_JSR:  // includes JSRR.
                        bit11 = (cpu->ir & MASK_BIT11) >> BITSHIFT_BIT11;
                        cpu->reg[REGISTER_7][0] = cpu->pc;
                        cpu->reg[REGISTER_7][1] = true;
                        jsrStackPush(cpu, memory);
                        if (bit11 == 0) { //JSRR
                            sr1 = (cpu->ir & MASK_SR1) >> BITSHIFT_SR1;
                            //setPointer(cpu, regSr1Pointer, sr1, isPopped *stackPointer);
                            // Account for nested subroutines using their own R0 has a returned result.
                            if (sr1 == 0 && isPopped) {
                                regSr1Pointer = regRzeroPointer;
                            } else {
                                regSr1Pointer = (unsigned short *)cpu->reg[sr1]; //regPointer = (unsigned short *)cpu->reg[reg];
                            }
                            cpu->pc = *regSr1Pointer;
                        } else { //JSR
                            cpu->pc += offset;
                        }
                        break;
                }
                state = EXECUTE;
                break;

            case EXECUTE: // Note that ST does not have an execute microstate.
                //printf("Now in EXECUTE---------------\n");
                switch (opcode) {
                    case OP_ADD:
                        if (bit5 == 0) {
                            cpu->mdr = (*regSr2Pointer + *regSr1Pointer);
                        } else if (bit5 == 1) {
                            cpu->mdr = *regSr1Pointer + immed;
                        }
                        cpu->cc = getCC(cpu->mdr);
                        break;
                    case OP_AND:
                        if (bit5 == 0) {
                            cpu->mdr = *regSr2Pointer & *regSr1Pointer;
                        } else if (bit5 == 1) {
                            cpu->mdr = *regSr1Pointer & immed;
                        }
                        cpu->cc = getCC(cpu->mdr);
                        break;
                    case OP_NOT:
                        cpu->mdr = ~(*regSr1Pointer); // Interpret as a negative if the leading bit is a 1.
                        cpu->cc = getCC(cpu->mdr);
                        break;
                    case OP_TRAP:
                        // Book page 222.
                        cpu->reg[REGISTER_7][0] = cpu->pc; // Store the PC in R7 before loading PC with the starting address of the service routine.
                        cpu->reg[REGISTER_7][1] = true;
                        trap(vector8, cpu, theWindow);
                        break;
                    //case OP_RET:
                    case OP_JMP: // includes RET.
                        if (sr1 == REGISTER_7) { // RET instruction.
                            memory[(*stackPointer)-1] = cpu->reg[0][0]; // store the result, but do NOT increment the stackPointer.
                            jsrStackPop(cpu, memory);
                            isPopped = 2;
                            cpu->pc = cpu->reg[REGISTER_7][0]; // Restore the PC.
                        }
                        break;
                    case OP_BR:
                        offset = sext(offset, BIT_PCOFFSET9);
                        if (branchEnabled(nzp, cpu)) {
                            cpu->pc += (offset);
                        }
                        break;
                    case OP_PP:
                        if (bit5 == 0) {
                            // Push.
                            cpu->reg[REGISTER_6][0]--; // Grow the stack.
                            // Store phase does the write-back into memory.
                        } else {
                            // Pop.
                            cpu->reg[dr][0] = memory[cpu->reg[*stackPointer][0]];
                            if (*stackPointer < (MEMORY_SIZE-ADDRESS_START)) {
                                (*stackPointer)++; // Shrink the stack.
                                // Do not increment when the stack is empty.
                            } else {
                                cursorAtOutput(theWindow, "Error: cannot pop an empty stack.");
                            }
                        }
                        break;
                }
                state = STORE;
                break;
                
            case STORE: // Look at ST. Microstate 16 is the store to memory
                //printf("Now in STORE---------------\n");
                switch (opcode) {
                    // write back to register or store MDR into memory
                    case OP_ADD:
                    case OP_AND: // Same as ADD
                    case OP_NOT: // Same as AND and ADD.
                        cpu->reg[dr][0] = cpu->mdr;
                        cpu->cc = getCC(cpu->reg[dr][0]);
                        break;
                    case OP_LD:
                    case OP_LDR: // Same as LD.
                        cpu->reg[dr][0] = cpu->mdr; // Load into the register.
                        cpu->cc = getCC(cpu->reg[dr][0]);
                        break;
                    case OP_LDI:
                        cpu->reg[dr][0] = cpu->mdr;
                        break;
                    case OP_STI:
                        memory[memory[cpu->mar] - ADDRESS_START] = cpu->mdr; // Store into memory.
                        break;
                    case OP_ST:
                    case OP_STR: // Same as ST.
                        memory[cpu->mar] = cpu->mdr; // Store into memory.
                        break;
                    case OP_LEA:
                        cpu->reg[dr][0] = cpu->pc + offset;
                        cpu->reg[dr][1] = true;
                        cpu->cc = getCC(cpu->reg[dr][0]);
                        break;
                    case OP_PP:
                        // R6 is exclusively used as the stack pointer, and is already initialized to 0xFFFF.
                        // The stack shall begin at the last element in memory[].
                        // The first stack item begins at one element before 0xFFFF.
                        if (bit5 == 0) {
                            // Push onto the stack.
                            memory[cpu->reg[REGISTER_6][0]] = cpu->reg[sr1][0];
                        }
                        break;
                }
                isCycleComplete = true;
                state = FETCH;
                break;
        } // end switch (state)
        if (isHalted || isCycleComplete) {
            if (isPopped) {
                isPopped--;
                // One time use of the result from the previous subroutine.
                regRzeroPointer = (unsigned short *)(memory + (*stackPointer - STACK_OFFSET_R0 - 1));
            } else {
                // The real R0 in this present routine.
                regRzeroPointer = (unsigned short *)cpu->reg[0];
            }
            break;
        }
    } // end while() loop.
    return 0;
} // end controller()

/*void setPointer(CPU_p *cpu, unsigned short *regPointer, unsigned short reg
                  , bool isPopped unsigned short *stackPointer) {
    // Account for nested subroutines using their own R0 has a returned result.
    if(reg == 0 && isPopped) {
        *regPointer = *(unsigned short *)(memory + (*stackPointer-STACK_OFFSET_R0));
    } else if(reg == 0 && *stackPointer == (MEMORY_SIZE-ADDRESS_START)) {
        regPointer = (unsigned short *)(memory + (*stackPointer));
    } else {
        *regPointer = *cpu->reg[reg]; //regPointer = (unsigned short *)cpu->reg[reg];
    }
}*/

/**
 * Saves R0..R7 and PC onto the stack.
 */
void jsrStackPush(CPU_p *cpu, unsigned short *mem) {
    // A one-time increment when the stack is empty.
    if (cpu->reg[REGISTER_6][0] == (MEMORY_SIZE - ADDRESS_START - 1)) {
        cpu->reg[REGISTER_6][0]++;
    }

    int i;
    for(i=0; i<=REGISTER_7; i++) {
        memory[cpu->reg[REGISTER_6][0]] = cpu->reg[i][0];
        cpu->reg[REGISTER_6][0]--;
    }
}

/**
 * Restores R0..R7 and PC.
 * Since R0 stores the result (from computation or exit code) for a given subroutine,
 * and since all registers are restored during RET, after RET is called the parent
 * routine's R0 can only be accessed from within the stack via memory[*stackPointer-STACK_OFFSET_R0].
 */
void jsrStackPop(CPU_p *cpu, unsigned short *mem) {
    int i;
    for(i=REGISTER_7; i>=0; i--) {
        cpu->reg[REGISTER_6][0]++;
        if (i != REGISTER_6) {
            cpu->reg[i][0] = memory[cpu->reg[REGISTER_6][0]];
            // Do not overwrite R6.
        }
    }
}

/**
 * Sets the condition code resulting by the resulting computer value.
 * @param value the value that was recently computed.
 * @return the condition code that represents the 3bit NZP as binary.
 */
bool branchEnabled(unsigned short nzp, CPU_p *cpu) {
    bool result = false;
    switch(nzp) {
        case CONDITION_NZP:
            result = true;
            break;
        case CONDITION_NP:
            if (cpu->cc == CONDITION_N || cpu->cc == CONDITION_P)
                result = true;
            break;
        case CONDITION_NZ:
            if (cpu->cc == CONDITION_N || cpu->cc == CONDITION_Z)
                result = true;
            break;
        case CONDITION_ZP:
            if (cpu->cc == CONDITION_Z || cpu->cc == CONDITION_P)
                result = true;
            break;
        case CONDITION_N:
            if (cpu->cc == CONDITION_N)
                result = true;
            break;
        case CONDITION_Z:
            if (cpu->cc == CONDITION_Z)
                result = true;
            break;
        case CONDITION_P:
            if (cpu->cc == CONDITION_P)
                result = true;
            break;
    }    
    return result;
}

/**
* This function will determine the condition code based on the value passed
*/
unsigned short getCC(unsigned short value) {
    short signedValue = value;
    unsigned short code;
    if (signedValue < 0) {
        code = CONDITION_N;
    }
    else if (signedValue == 0) {
            code = CONDITION_Z;
    }
    else {
        code = CONDITION_P;
    }
    return code;
}

/**
 * This returns the same value except it is converted to a signed short instead.
 */
short toSign(unsigned short value) {
    short signedValue = value;
    return signedValue;
}

/**
* This function will take the loaction of the high order bit of the immediate value
* and sign extend it so that if the high order bit is a 1, then it will be converted to
* negative value.
* 
* @param value is the number to be sign extended.
* @param instance determines what the high order bit is of the value.
*/
short sext(unsigned short theValue, int highOrderBit) {
    short value = (short) theValue;
    switch(highOrderBit) {
        case BIT_IMMED:
            if (((value & BIT_IMMED) >> BITSHIFT_NEGATIVE_IMMEDIATE) == 1)
                    value = value | MASK_NEGATIVE_IMMEDIATE;
            break;
        case BIT_PCOFFSET11:
            if (((value & BIT_PCOFFSET11) >> BITSHIFT_NEGATIVE_PCOFFSET11) == 1)
                value = value | MASK_NEGATIVE_PCOFFSET11;
            break;
        case BIT_PCOFFSET9:
            if (((value & BIT_PCOFFSET9) >> BITSHIFT_NEGATIVE_PCOFFSET9) == 1)
                value = value | MASK_NEGATIVE_PCOFFSET9;
            break;
        case BIT_PCOFFSET6:
            if (((value & BIT_PCOFFSET6) >> BITSHIFT_NEGATIVE_PCOFFSET6) == 1)
                value = value | MASK_NEGATIVE_PCOFFSET6;
            break;
    }
    return value;
}

/**
 * Simulating a ZEXT operation.
 */
unsigned short ZEXT(unsigned short value) {
    // Simulated ZEXT.
    return value;
}

/**
 * Print out fields to the console for the CPU_p object.
 * @param cpu the cpu object containing the data.
 */
void displayCPU(CPU_p *cpu, int memStart) {
    int c, hexExit, menuSelection = 0;
    isHalted = false;
    bool rePromptUser = true;
    bool rePromptHex = true;
    char *fileName; // = malloc(FILENAME_SIZE * sizeof(char)); //char fileName[FILENAME_SIZE];
    char breakPointMark;
    initscr();
    cbreak();
    clear();
    WINDOW *main_win = newwin(WINDOW_LEGTH, WINDOW_WIDTH, 0, 0); //(32, 49, 0, 0)
    box(main_win, 0, 0);
    refresh();

    while(1) {
        rePromptUser = true;
        rePromptHex = true;
        menuSelection = 0;
        mvwprintw(main_win, 1, 1,  "Welcome to the LC-3 Samulator Simulator");
        mvwprintw(main_win, 2, 1,  "Registers");
        mvwprintw(main_win, 2, 31, "Memory");

        // First 8 lines of Registers and Memory.
        int i = 0, offsetMemoryAddress = 0;
        for(i = 0; i < 8; i++) {
            if (cpu->reg[i][1] == true) {
                offsetMemoryAddress = ADDRESS_START;
            } else {
                offsetMemoryAddress = 0;
            }
            mvwprintw(main_win, 3+i, 1, "R%u: x%04X", i, cpu->reg[i][0]+offsetMemoryAddress);   // Registers.
            mvwprintw(main_win, 3+i, 26, "%c x%04X: x%04X"
                    , breakPoint[i+(memStart-ADDRESS_START)]
                    , i+memStart
                    , memory[i+(memStart-ADDRESS_START)]); // Memory.
        }

        // Next 3 lines of Memory.
        int j = 0;
        for (j = 0; j < 3; j++) {
            mvwprintw(main_win, 11+j, 26, "%c x%04X: x%04X"
                    , breakPoint[i+(memStart-ADDRESS_START)]
                    , i+memStart
                    , memory[i+(memStart-ADDRESS_START)]);
            i++;
        }

        // Next 4 lines of Registers and Memory.
        mvwprintw(main_win, 14, 1, "PC:  x%04X    IR: x%04X  %c x%04X: x%04X"
                , cpu->pc+ADDRESS_START
                , cpu->ir, breakPoint[i+(memStart-ADDRESS_START)]
                , i+memStart
                , memory[i+(memStart-ADDRESS_START)]);
        i++;
        mvwprintw(main_win, 15, 1, "A:   x%04X     B: x%04X  %c x%04X: x%04X"
                , cpu->A, cpu->B, breakPoint[i+(memStart-ADDRESS_START)], i+memStart, memory[i+(memStart-ADDRESS_START)]);
        i++;
        mvwprintw(main_win, 16, 1, "MAR: x%04X   MDR: x%04X  %c x%04X: x%04X"
                , cpu->mar+ADDRESS_START
                , cpu->ir, breakPoint[i+(memStart-ADDRESS_START)]
                , i+memStart
                , memory[i+(memStart-ADDRESS_START)]);
        i++;
        mvwprintw(main_win, 17, 1, "CC:  N:%d Z:%d P:%d         %c x%04X: x%04X",
                  (cpu->cc >> BITSHIFT_CC_BIT3) & MASK_CC_N
                , (cpu->cc >> BITSHIFT_CC_BIT2) & MASK_CC_Z
                , cpu->cc  & MASK_CC_P
                , breakPoint[i+(memStart-ADDRESS_START)]
                , i+memStart //ADDRESS_START
                , memory[i+(memStart-ADDRESS_START)]);
        i++;

        // Last 2 lines of Memory.
        mvwprintw(main_win, 18, 26, "%c x%04X: x%04X"
                , breakPoint[i+(memStart-ADDRESS_START)]
                , i+memStart
                , memory[i+(memStart-ADDRESS_START)]);
        mvwprintw(main_win, 19, 1, "Select: 1)Load  2)Save  3)Step  5)DisplayMem  6)Edit  7)Run  8)SetBrkpt  9)Exit");
        cursorAtPrompt(main_win, "");
        if (cpu->pc == 0 && !isHalted) {
            // Only do a single time, else what you want to display gets obliterated.
            mvwprintw(main_win, 23, 1, "Input                                                                           ");
            mvwprintw(main_win, 24, 1, "Output                                                                         ");
            outputColCounter = 0;
        }
        cursorAtPrompt(main_win, ""); // twice necessary to prevent overwrite.

        while(rePromptUser) {
            rePromptUser = false;
            CPU_p cpuTemp;
            move(21, 1);
            clrtoeol();
            move(22, 1);
            clrtoeol();
            move(23, 1);
            clrtoeol();
            move(24, 1);
            clrtoeol();
            noecho();

            //c = '3'; // debugging within IDE.  Uncomment this line, and comment the entire next block.
            if (isRun && !isHalted && (breakPoint[cpu->pc] != '*')) {
                c = '3'; // keep stepping until TRAP x25 is hit.
            } else {
                isRun = false;
                if (breakPoint[cpu->pc] == '*') {
                    cursorAtPrompt(main_win, "Press Step to proceed.");
                    box(main_win, 0, 0);
                    refresh();
                }
                c = wgetch(main_win); // This is what stops to prompt the user for an Option input.
            }

            echo();
            box(main_win, 0, 0); // Do not remove this line, else display anomalies happen.
            refresh();
            switch(c){
                case '1': // Load.
                    cpuTemp = initialize();
                    clearOutput(main_win);
                    isHalted = false;
                    cpu = &cpuTemp;
                    cursorAtPrompt(main_win, "Specify file name: ");
                    fileName = malloc(FILENAME_SIZE * sizeof(char));
                    wgetstr(main_win, fileName);
                    loadProgramInstructions(openFileText(fileName, main_win), main_win);
                    free(fileName);
                    box(main_win, 0, 0);
                    refresh();
                    break;
                case '2': // Save
                    cursorAtPrompt(main_win, "Type the filename to write to: ");
                    fileName = malloc(FILENAME_SIZE * sizeof(char));
                    wgetstr(main_win, fileName);
                    writeToFile(main_win, fileName);
                    free(fileName);
                    break;
                case '3': // Step.
                    controller(cpu, main_win); // invoke exclusively in case 3.
                    break;
                case '5': // DisplayMem.
                    clearOutput(main_win);
                    while (rePromptHex) {
                        clearPrompt(main_win);
                       box(main_win, 0, 0);
                        char inputMemAddress[4];
                        int newStart = 0;
                        //mvwprintw(main_win, 21, 1, "Push Q to return to main menu.");
                        //mvwprintw(main_win, 22, 1, "New Starting Address: x");
                        cursorAtPrompt(main_win, "New Starting Address: ");
                        refresh();
                        wgetstr(main_win, inputMemAddress);
                        box(main_win, 0, 0);
                        refresh();
                        if (inputMemAddress[0] == 'q' || inputMemAddress[0] == 'Q') {
                            cursorAtPrompt(main_win, "                                                                                ");
                            //rePromptUser = true;
                            break;
                        }
                        if (hexCheckAddress(inputMemAddress)) {
                            newStart = strtol(inputMemAddress, NULL, HEX_BITS);
                            displayCPU(cpu, newStart);
                            break;
                        } else {
                            clearOutput(main_win);
                            cursorAtOutput(main_win, "You must enter a 4-digit hex value. Try again.");
                            continue;
                        }
                    }
                    clearPrompt(main_win);
                    //clearOutput(main_win);
                    rePromptHex = true;
                    break;
                case '6': // Edit.
                    clearOutput(main_win);
                    while (rePromptHex) {
                        clearPrompt(main_win);
                        box(main_win, 0, 0);
                        char inputMemAddress[4], inputMemValue[4];
                        int editAddress = 0, editValue = 0;
                        cursorAtPrompt(main_win, "Edit Memory Address: ");
                        wgetstr(main_win, inputMemAddress);
                        refresh();
                        if (inputMemAddress[0] == 'q' || inputMemAddress[0] == 'Q') {
                            cursorAtPrompt(main_win, "                                                                                ");
                            rePromptUser = true;
                            break;
                        }
                        if (hexCheck(inputMemAddress)) {
                            editAddress = strtol(inputMemAddress, NULL, HEX_BITS);
                        } else {
                            clearOutput(main_win);
                            cursorAtOutput(main_win, "You must enter a 4-digit hex value. Try again.");
                            rePromptHex = true;
                            continue;
                        }

                        clearOutput(main_win);
                        mvwprintw(main_win, OUTPUT_LINE_NUMBER, OUTPUT_COL_NUMBER, "Old value for address: x%04X"
                                , memory[editAddress - ADDRESS_START]);
                        outputColCounter++;
                        //cursorAtOutput(main_win, "Old value for address: blah");
                        clearPrompt(main_win);
                        cursorAtPrompt(main_win, "Enter new hex value: ");
                        refresh();
                        wgetstr(main_win, inputMemValue);
                        if (inputMemValue[0] == 'q' || inputMemValue[0] == 'Q') {
                            //cursorAtPrompt(main_win, "                                                                                ");
                            break;
                        }
                        if (hexCheck(inputMemValue)) {
                            editValue = strtol(inputMemValue, NULL, HEX_BITS);
                        } else {
                            clearOutput(main_win);
                            cursorAtOutput(main_win, "You must enter a 4-digit hex value. Try again.");
                            clearOutput(main_win);
                            continue;
                        }
                        clearPrompt(main_win);
                        clearOutput(main_win);
                        memory[editAddress - ADDRESS_START] = editValue;
                        //cursorAtOutput(main_win, "Done.");
                        refresh();
                        break;
                    }
                    rePromptHex = true;
                    break;
                case '7': // Run.
                    isRun = true;
                    break;
                case '8': // Break Point.
                    clearOutput(main_win);
                    while (rePromptHex) {
                        box(main_win, 0, 0);
                        char inputMemAddress[4];
                        int editAddress = 0;
                        char *breakPointStatus = (char*) malloc(sizeof(char) * 3);
                        cursorAtPrompt(main_win, "Toggle Breakpoint at Memory Address: ");
                        wgetstr(main_win, inputMemAddress);
                        refresh();
                        if (inputMemAddress[0] == 'q' || inputMemAddress[0] == 'Q') {
                            cursorAtPrompt(main_win, "                                                                                ");
                            rePromptUser = true;
                            break;
                        }
                        if (hexCheckAddress(inputMemAddress)) {
                            editAddress = strtol(inputMemAddress, NULL, HEX_BITS);
                        } else {
                            clearOutput(main_win);
                            cursorAtOutput(main_win, "You must enter a 4-digit hex value. Try again.");
                            rePromptHex = true;
                            continue;
                        }

                        if (breakPoint[editAddress - ADDRESS_START] == '*') {
                            breakPoint[editAddress - ADDRESS_START] = ' ';
                        } else {
                            breakPoint[editAddress - ADDRESS_START] = '*';
                        }

                        if (breakPoint[editAddress - ADDRESS_START] == '*') {
                            breakPointStatus = "ON ";
                        } else {
                            breakPointStatus = "OFF";
                        }
                        mvwprintw(main_win, OUTPUT_LINE_NUMBER, OUTPUT_COL_NUMBER, "Breakpoint set to %s for address: x%04X"
                                , breakPointStatus, memory[editAddress - ADDRESS_START]);
                        outputColCounter++;
                        //free(breakPointStatus); // BUG this causes a seg fault for some unknown reason.
                        //cursorAtOutput(main_win, "Done.");
                        refresh();
                        break;
                    }
                    rePromptHex = true;
                    break;
                case '9': // Exit.
                    echo(); //turn echo back on.
                    endwin();
                    printf("Bubye\n");
                    exit(0);
                    break;
                default:
                    cursorAtPrompt(main_win, "---Invalid selection                                                             ");
                    box(main_win, 0, 0);
                    refresh();
                    rePromptUser = true;
                    break;
            }
            wrefresh(main_win);
        }
    }
}

void cursorAtPrompt(WINDOW *theWindow, char *theText) {
    if (!isHalted) {
         // First wipe out what ever is there.
        mvwprintw(theWindow, 21, 1, "                                                                               ");
    }
    mvwprintw(theWindow, 22, 1, "--------------------------------------------------------------------------------");
    refresh();
    mvwprintw(theWindow, 21, 1, theText); //The last place the cursor will sit.
    refresh();
}

void clearPrompt(WINDOW *theWindow) {
    mvwprintw(theWindow, 21, 1, "                                                                               ");
    mvwprintw(theWindow, 22, 1, "--------------------------------------------------------------------------------");
    refresh();
    wrefresh(theWindow);
}

void cursorAtInput(WINDOW *theWindow, char *theText) {
    int input = mvwgetch(theWindow, 23, 8);
    theText[0] = input;
    refresh();
}

void cursorAtOutput(WINDOW *theWindow, char *theText) {
    int i;
    char *text = (char*) malloc(sizeof(char) * 2);
    text[1] = '\0';
    for (i = 0; i < strlen(theText); i++) {
        text[0] = theText[i];
        mvwprintw(theWindow, OUTPUT_LINE_NUMBER + outputLineCounter, OUTPUT_COL_NUMBER + outputColCounter, text);
        outputColCounter++;
        if (theText[i] == 75) {
            outputLineCounter++;
            outputColCounter = 0;
        }
    }
    //mvwprintw(theWindow, OUTPUT_LINE_NUMBER + outputLineCounter, 8, theText);
    //outputLineCounter++;
    refresh();
}

void clearOutput(WINDOW *theWindow) {
    int i;
    mvwprintw(theWindow, OUTPUT_LINE_NUMBER, 1
            , "Output                                                                              ");
    for (i = 1; i <= OUTPUT_AREA_DEPTH; i++) {
        mvwprintw(theWindow, OUTPUT_LINE_NUMBER + i, 1
                , "                                                                                ");
    }
    box(theWindow, 0, 0);
    refresh();
    outputLineCounter = 0;
    outputColCounter = 0;
}

void cursorAtCustom(WINDOW *theWindow, int theRow, int theColumn, char *theText) {
    mvwprintw(theWindow, theRow, theColumn, theText);
    refresh();
}

/**
 * Validites of a hex number of a memory address.
 * Returns 1 if true, 0 if false.
 */
int hexCheckAddress(char num[]) {
    int counter = 0;
    int valid = 0;
    int i;
    int value = strtol(num, NULL, HEX_BITS);
    for (i = 0; i < 4; i++) {
        if (isxdigit(num[i])) {
            counter++;
        }
    }
    if (counter == 4 && value >= ADDRESS_START && value <= MEMORY_SIZE) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Validites of a hex number.
 * Returns 1 if true, 0 if false.
 */
int hexCheck(char num[]) {
    int counter = 0;
    int valid = 0;
    int i;
    int value = strtol(num, NULL, HEX_BITS);
    for (i = 0; i < 4; i++) {
        if (isxdigit(num[i])) {
            counter++;
        }
    }
    if (counter == 4) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Sets all elements to zero.
 */
void zeroOutMemory(unsigned short *array) {
    int i;
    for (i = 0; i < MEMORY_SIZE; i++) {
        array[i] = 0;
    }
}

/**
 * Sets all elements to false.
 */
void resetBreakPoints(unsigned char *array) {
    int i;
    for (i = 0; i < MEMORY_SIZE; i++) {
        breakPoint[i] = ' ';
    }
}

void writeToFile(WINDOW *theWindow, char *fileName) {
    FILE *outputFile, *outputFileLst;
    char fileNameLst[FILENAME_SIZE] = "";
    strcat(fileNameLst, fileName);
    strcat(fileNameLst, ".lst");
    strcat(fileName, ".hex");

    // Check if file exists
    outputFile = fopen(fileName, "r");
    if(outputFile == NULL) {
        outputFileLst = fopen(fileNameLst, "w+");
        int i;
        for(i = 0; i <= MEMORY_SIZE-ADDRESS_START; i++) {
            fprintf(outputFileLst, "x%04X: ", (i+ADDRESS_START));
            fprintf(outputFileLst, "x%04X", memory[i]);
            fprintf(outputFileLst, "%c", '\n');
            fflush(outputFileLst);
        }
        fclose(outputFileLst);

        outputFile = fopen(fileName, "w+");
        for(i = 0; i <= MEMORY_SIZE-ADDRESS_START; i++) {
            fprintf(outputFile, "%04X", memory[i]);
            fprintf(outputFile, "%c", '\n');
            fflush(outputFile);
        }
        cursorAtPrompt(theWindow, "File written.                                                                   ");
    } else {
        // do nothing, and warn the user the file already exists.
        // Perhaps prompt to overwrite, and handle appropriately.
        cursorAtPrompt(theWindow, "File already exists. Not written.                                               ");
    }
    refresh();
    fclose(outputFile);
}

/*void zeroRegisters(unsigned short *array[8][2]) {
    int i;
    for (i = 0; i < REGISTER_SIZE; i++) {
        array[i][0] = 0;
        array[i][1] = false;
    }
}*/

/**
 * Initializes a CPU_p object and its fields.
 * Removes the junk from these memory locations.
 */
CPU_p initialize() {
    CPU_p cpu = { 0           // pc
                , CONDITION_Z // cc
                , { {0,false}, {0,false}, {0,false}, {0,false}, {0,false}
                    , {0,false}, {MEMORY_SIZE-ADDRESS_START,true}, {0,false}} // Registers 0-7.
                , 0           // ir
                , 0           // mar
                , 0           // mdr
                , 0           // alu.a
                , 0};         // alu.b

    zeroOutMemory(memory);
    resetBreakPoints(breakPoint);

    // R6 is exclusively used as the stack pointer, and always contains memory address.
    cpu.reg[REGISTER_6][0] = MEMORY_SIZE - ADDRESS_START;
    cpu.reg[REGISTER_6][1] = true;
    return cpu;
}

/**
 * Opens a text file in read only mode.
 * @param theFileName the file name to open, in this present working directory.
 * @return the pointer to the file now opened.
 */
FILE* openFileText(char *theFileName, WINDOW *theWindow) {
    //printf("You said %s", theFileName); // debugging, remove me.
    FILE *dataFile;
    dataFile = fopen(theFileName, "r");
    //if ((dataFile = fopen(theFileName, "r")) == NULL) {
    if (dataFile == NULL) {
    //  printf("\n---ERROR: File %s could not be opened.\n\n", theFileName);
        char temp;
        if(theWindow == NULL) {
            printf("Error, File not found.  Press <ENTER> to continue.");
            fflush(stdin);
            temp = getchar(); // BUG this does not work as expected in option #1.
            printf("\n");
        } else {
            cursorAtPrompt(theWindow, "Error, File not found.");
            isHalted = true;
        }
    } else {
        isHalted = false;
        //printf("\nSUCCESS: File Found: %s\n\n", theFileName); // debugging
    }
    return dataFile;
}

/**
 * Loads the the instructions from the text file into memory[].
 * Expects one instruction per line, in hex number form, and a new line character.
 * @param inputFile the file to read.
 */
void loadProgramInstructions(FILE *inputFile, WINDOW *theWindow) {
    if (inputFile != NULL){
        char instruction [5]; // includes room for the carriage return character.
        int length = sizeof(instruction);
        int i = 0;
        unsigned short startingAddress = 0;

        /* The first line in the inputfile is actually the starting address of
           where to begin loading the instructions into memory.  This is
           a result of the .ORIG instruction in assembly. */
        if(!feof(inputFile)) {
            fgets(instruction, length, inputFile);
            startingAddress = strtol(instruction, NULL, HEX_BITS);
            fgets(instruction, length, inputFile); // processes the carriage return character.
        }

        // In this simulator, we start at ADDRESS_MIN which is the zero'th element in memory[].
        if (startingAddress >= ADDRESS_START) {
            i = (startingAddress - ADDRESS_START);
            while(!feof(inputFile)) {
                fgets(instruction, length, inputFile);
                memory[i] = strtol(instruction, NULL, HEX_BITS);
                //printf("\n %04X", memory[i]); // debugging, confirms the memory[] does have the data.
                fgets(instruction, length, inputFile); // processes the carriage return character.
                i++;
            }
        } else {
            if(theWindow == NULL) {
                printf("Error, address must be between %x and %x\n"
                    , ADDRESS_START, (ADDRESS_START + MEMORY_SIZE));
            } else {
                cursorAtPrompt(theWindow, "Error, address "); // TODO specify min and max address.
                isHalted = true;
            }
        }
        fclose(inputFile);
    }
}

/**
 * Driver for the program.
 */
int main(int argc, char* argv[]) {
    char *fileName = argv[1]; //char *fileName = "./HW3.hex";
    CPU_p cpu = initialize();
    if(fileName != NULL) {
        loadProgramInstructions(openFileText(fileName, NULL), NULL);
    }
    displayCPU(&cpu, ADDRESS_START); // send the address of the object.
}
