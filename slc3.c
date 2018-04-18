/*
 *  slc3.c
 *
 *  Date Due: Apr 22, 2018
 *  Authors:  Sam Brendel, Tyler Shupack
 *  Problem 3,4
 *  version: 4.17a
 */

#include "slc3.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ncurses.h>

unsigned short memory[MEMORY_SIZE];
bool isHalted = false;

/**
 * Simulates trap table lookup for now
 * @param vector the area in memory that is simulated to be looked up to
 *        execute the requested TRAP routine.
 * @param cpu the cpu object that contains data.
 */
void trap(unsigned short vector, CPU_p *cpu) {
    switch (vector) {
    case 0x25:
        printf("==========HALT==========\n");
        cpu->pc = 0; // reset to zero as per Prof Mobus.
        isHalted = true;
        break;
    default: 
        printf("Err: Unknown Trap vector?\n");
        break;
    }
}

/**
 * The controller component of the LC-3 simulator.
 * @param cpu the cpu object to contain data.
 */
int controller(CPU_p *cpu) {

    // check to make sure both pointers are not NULL
    // do any initializations here
    unsigned int opcode, dr, sr1, sr2, bit5, immed, offset, state, nzp;    // fields for the IR
    unsigned short vector8, vector16;
    bool isCycleComplete = false;
    short signedShort = 0;

    state = FETCH;
    while (!isHalted) { // efficient endless loop to be used in the next problem
        switch (state) {
            case FETCH: // microstates 18, 33, 35 in the book.
                //printf("Now in FETCH---------------\n");
                cpu->mar = cpu->pc;           // Step 1: MAR is loaded with the contends of the PC,
                cpu->pc++;                   //         and also increment PC. Only done in the FETCH phase.
                cpu->mdr = memory[cpu->mar];  // Step 2: Interrogate memory, resulting in the instruction placed into the MDR.
                cpu->ir  = cpu->mdr;          // Step 3: Load the IR with the contents of the MDR.
                state    = DECODE;
                break;

            case DECODE: // microstate 32
                //printf("Now in DECODE---------------\n");
                opcode = cpu->ir  & MASK_OPCODE; // Input is the four-bit opcode IR[15:12]. The output line asserted is the one corresponding to the opcode at the input.
                opcode = opcode  >> BITSHIFT_OPCODE;
                switch (opcode) {
                // different opcodes require different handling
                // compute effective address, e.g. add sext(immed7) to register.
                case OP_LD:
                    dr     = cpu->ir >> BITSHIFT_DR;
                    dr     = dr       & MASK_SR2;
                    offset = cpu->ir  & MASK_PCOFFSET9;
                    break;
            }
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
                case OP_LD:
                    cpu->mar = cpu->pc + offset; // microstate 2.
                    cpu->mdr = memory[cpu->mar]; // microstate 25.
                    break;
                case OP_ST:
                    dr      = cpu->ir  & MASK_DR;         // This is actually a source register, but still use dr.
                    dr      = dr     >> BITSHIFT_DR;
                    offset  = cpu->ir  & MASK_PCOFFSET9;
                    cpu->mar = cpu->pc  + offset; // microstate 2.
                    break;
            }
            state = FETCH_OP;
            break;

        case FETCH_OP:
            //printf("Now in FETCH_OP---------------\n");
            switch (opcode) {
                // get operands out of registers into A, B of ALU
                // or get memory for load instr.
                case OP_ADD:
                case OP_AND:
                    dr   = cpu->ir  & MASK_DR;
                    dr   = dr      >> BITSHIFT_DR;
                    sr1  = cpu->ir  & MASK_SR1;
                    sr1  = sr1     >> BITSHIFT_SR1;
                    bit5 = cpu->ir  & MASK_BIT5;
                    bit5 = bit5    >> BITSHIFT_BIT5;
                    if (bit5 == 0) {
                        sr2 = cpu->ir & MASK_SR2; // no shift needed.
                    } else if (bit5 == 1) {
                        immed = cpu->ir & MASK_IMMED5; // no shift needed.
                    }
                    // The book page 106 says current microprocessors can be done simultaneously during fetch, but this simulator is old skool.
                    break;
                case OP_NOT:
                    dr  = cpu->ir  & MASK_DR;
                    dr  = dr      >> BITSHIFT_DR;
                    sr1 = cpu->ir  & MASK_SR1;
                    sr1 = sr1     >> BITSHIFT_SR1;
                    break;
                case OP_TRAP:
                    vector8 = cpu->ir & MASK_TRAPVECT8; // No shift needed.
                    break;
                case OP_LD:
                    dr      = cpu->ir & MASK_DR;
                    dr      = dr     >> BITSHIFT_DR;
                    offset  = cpu->ir & MASK_PCOFFSET9;
                    offset  = SEXT(offset);
                    cpu->mar = cpu->pc + offset;
                    cpu->mdr = memory[cpu->mar];
                    break;
                case OP_ST: // Same as LD.
                    // Book page 124.
                    cpu->mdr = cpu->reg[dr];
                    break;
                case OP_JMP:
                    sr1 = cpu->ir & MASK_SR1;
                    sr1 = sr1    >> BITSHIFT_SR1;
                    break;
                case OP_BR:
                    nzp = cpu->ir    & MASK_NZP;
                    nzp = nzp       >> BITSHIFT_CC;
                    offset = cpu->ir & MASK_PCOFFSET9;
                    break;
                default:
                    break;
            }
            state = EXECUTE;
            break;

        case EXECUTE: // Note that ST does not have an execute microstate.
            //printf("Now in EXECUTE---------------\n");
            switch (opcode) {
                case OP_ADD:
                    if (bit5 == 0) {
                        cpu->mdr = cpu->reg[sr2] + cpu->reg[sr1];
                    } else if (bit5 == 1) {
                        cpu->mdr = cpu->reg[sr1] + immed;
                    }
                    signedShort = SEXT(cpu->mdr);
                    cpu->cc = getCC(signedShort); // TODO should this be set in this phase?
                    break;
                case OP_AND:
                    if (bit5 == 0) {
                        cpu->mdr = cpu->reg[sr2] & cpu->reg[sr1];
                    } else if (bit5 == 1) {
                        cpu->mdr = cpu->reg[sr1] & immed;
                    }
                    signedShort = SEXT(cpu->mdr);
                    cpu->cc = getCC(signedShort); // TODO should this be set in this phase?
                    break;
                case OP_NOT:
                    cpu->mdr = ~cpu->reg[sr1]; // Interpret as a negative if the leading bit is a 1.
                    signedShort = SEXT(cpu->mdr);
                    cpu->cc = getCC(signedShort); // TODO should this be set in this phase?
                    break;
                case OP_TRAP:
                    // Book page 222.
                    vector16   = ZEXT(vector8); // TODO: should we make this actually do a zero extend to 16 bits?
                    cpu->mar    = vector16;
                    cpu->reg[7] = cpu->pc; // Store the PC in R7 before loading PC with the starting address of the service routine.
                    cpu->mdr    = memory[cpu->mar]; // read the contents of the register.
                    cpu->pc     = cpu->mdr; // The contents of the MDR are loaded into the PC.  Load the PC with the starting address of the service routine.
                    trap(vector8, cpu);
                    break;
                case OP_JMP:
                    cpu->pc = cpu->reg[sr1];
                    break;
                case OP_BR:
                    if (doBen(nzp, cpu)) {
                        cpu->pc += (offset);
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
            case OP_NOT: // Sam as AND and AND.
                cpu->reg[dr] = cpu->mdr;
                break;
            case OP_LD:
                cpu->reg[dr] = cpu->mdr; // Load into the register.
                break;
            case OP_ST:
                memory[cpu->mar] = cpu->mdr;     // Store into memory.
                break;
            case OP_BR:
                // cpu->pc = cpu->mar; // TODO does this really happen in store phase?
                break;
            }

            // do any clean up here in prep for the next complete cycle
            isCycleComplete = true;
            state = FETCH;
            break;
        } // end switch (state)

        if (isHalted || isCycleComplete) {
           break;
        }
    } // end for()
    return 0;
} // end controller()

/**
 * Gets the condition code of the resulting computer value.
 * @param value the value that was recently computed.
 * @return the condition code that represents the 3bit NZP as binary.
 */
bool doBen(unsigned short nzp, CPU_p *cpu) {
    return (
               (cpu->cc == CONDITION_N   &&  nzp == CONDITION_N)
            || (cpu->cc == CONDITION_Z   &&  nzp == CONDITION_Z)
            || (cpu->cc == CONDITION_P   &&  nzp == CONDITION_P)
            || (cpu->cc == CONDITION_NZ  && (nzp == CONDITION_N || nzp == CONDITION_Z))
            || (cpu->cc == CONDITION_ZP  && (nzp == CONDITION_Z || nzp == CONDITION_P))
            || (cpu->cc == CONDITION_NP  && (nzp == CONDITION_N || nzp == CONDITION_P))
            || (cpu->cc == CONDITION_NZP && (nzp == CONDITION_N || nzp == CONDITION_Z || nzp == CONDITION_P))
            );
}

unsigned short getCC(short value) {
    unsigned short code;
    if (value < 0)
        code = CONDITION_N;
    else if (value == 0)
            code = CONDITION_Z;
    else
        code = CONDITION_P;
    return code;
}

/**
 * Sets the PC to the designated index in memory[].
 * @param cpu the cpu object containing data.
 * @param whereTo the address where to go to.
 */
void JUMP(CPU_p *cpu, unsigned short whereTo) {
    cpu->pc = whereTo;
}

/**
 * This returns the same value except it is converted to a signed short instead.
 */
short SEXT(unsigned short value) {
    short signedValue = value;
    return signedValue;
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
void displayCPU(CPU_p *cpu) {
    for(;;) {
        //printf("---displayCPU()\n"); // debugging
        bool rePromptUser = true;
        int menuSelection = 0;
        char *fileName[FILENAME_SIZE];
        printf("Welcome to the LC-3 Simulator Simulator\n");
        printf("Registers                     Memory\n");

        // First 8 lines
        int i = 0;
        for(i = 0; i < 8; i++) {
            printf("R%u: %4X", i, cpu->reg[i]);   // Registers.
            printf("%26X: %4X\n", i+SIMULATOR_OFFSET, memory[i]); // Memory.
        }

        // Next 3 lines
        int j;
        for (j = 0; j < 3; j++ & i++) {
            printf("%34X: %4X\n", i+SIMULATOR_OFFSET, memory[i]);
        }

        // Next 4 lines.
        printf("PC:  %4X    IR: %4X         %4X: %4X\n", cpu->pc, cpu->ir, i+SIMULATOR_OFFSET, memory[i++]);
        printf("A:   %4X     B: %4X         %4X: %4X\n", cpu->A, cpu->B, i+SIMULATOR_OFFSET, memory[i++]);
        printf("MAR: %4X   MDR: %4X         %4X: %4X\n", cpu->pc, cpu->ir, i+SIMULATOR_OFFSET, memory[i++]);
        printf("CC:  N:%d Z:%d P:%d              %4X: %4X\n",
                cpu->cc >> BITSHIFT_CC_BIT3 & MASK_CC_N,
                cpu->cc >> BITSHIFT_CC_BIT2 & MASK_CC_Z,
                cpu->cc & MASK_CC_P,
                i+SIMULATOR_OFFSET,
                memory[i++]);

        // Last 2 lines.
        printf("%34X: %4X\n", i+SIMULATOR_OFFSET, memory[i++]);
        while(rePromptUser) {
            rePromptUser = false;
            printf("Select: 1) Load,  3) Step,  5) Display Mem,  9) Exit\n");
            fflush(stdout);

            scanf("%d", &menuSelection); // TODO put this back in.  Just debugging v4.16d
            //menuSelection = 3; //TODO debugging, remove me.

            switch(menuSelection) {
                case 1:
                    printf("Specify file name: ");
                    fflush(stdout);
                    scanf("%s", fileName);
                    loadProgramInstructions(openFileText(fileName));
                    break;
                case 3:
                    //printf("CASE3\n"); // do nothing.  Just let the PC run the next instruction.
                    controller(cpu); // invoke exclusively in case 3.
                    break;
                case 5:
                    printf("CASE5\n"); // Update the window for the memory registers.
                    break;
                case 9:
                    //printf("CASE9\n");
                    //cpu->IR = 0xF025; // TRAP x25
                    printf("\nBubye\n");
                    exit(0);
                    break;
                default:
                    printf("---Invalid selection\n.");
                    rePromptUser = true;
                    break;
            }
            //fflush(stdout);
        }
    }
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
                , { 0, 0, 0, 0, 0, 0, 0, 0 }
                , 0    // ir
                , 0    // mar
                , 0
                , 0
                , 0
                , 0}; // mdr

    zeroOut(memory, 100);

    // Intentionally hard coding these values into two memory registers.
    //cpu.reg[0] = 3;
    //cpu.reg[7] = 4;
    //cpu->reg[3] = 0xB0B0;   // Intentional simulated data.
    //memory[4]  = 0xA0A0;   // Intentional simulated data.
    //cpu->reg[0] = 0xD0E0;   // Intentional simulated data.
    return cpu;
}

/**
 * Opens a text file in read only mode.
 * @param theFileName the file name to open, in this present working directory.
 * @return the pointer to the file now opened.
 */
FILE* openFileText(char *theFileName) {
    FILE *dataFile;
    dataFile = fopen(theFileName, "r");
    //if ((dataFile = fopen(theFileName, "r")) == NULL) {
    if (dataFile == NULL) {
        printf("\n---ERROR: File %s could not be opened.\n\n", theFileName);
    } else {
        printf("\nSUCCESS: File Found: %s\n\n", theFileName); // debugging
    }
    return dataFile;
}

/**
 * Loads the the instructions from the text file into memory[].
 * Expects one instruction per line, in hex number form, and a new line character.
 * @param inputFile the file to read.
 */
void loadProgramInstructions(FILE *inputFile) {
        char instruction [5]; // includes room for the null terminating character.
        int length = sizeof(instruction);
        int i = 0;
        while(!feof(inputFile)) {
            fgets(instruction, length, inputFile);
            memory[i++] = strtol(instruction, NULL, 16);
            fgets(instruction, length, inputFile); // FIXME: hackaround for getting a zero instead of the next line.
        }
        fclose(inputFile);

        if (memory[0] == 0) {
            printf("\n---ERROR, no instructions were loaded in memory!\n\n");
        }
}

/**
 * Driver for the program.
 */
int main(int argc, char* argv[]) {
char *fileName = argv[1];
    CPU_p cpu = initialize();
    if(fileName != NULL) {
        loadProgramInstructions(openFileText(fileName));
    }
    displayCPU(&cpu); // send the address of the object.
}
