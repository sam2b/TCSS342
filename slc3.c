/*
 *  slc3.c
 *
 *  Date Due: Apr 22, 2018
 *  Authors:  Sam Brendel, Tyler Shupack
 *  Problem 3,4
 *  version: 4.22d
 */

#include "slc3.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ncurses.h>
#include <ctype.h>

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
    case TRAP_VECTOR_X25:
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
    unsigned int opcode, dr, sr1, sr2, bit5, offset, state, nzp;    // fields for the IR
    unsigned short immed; // before using this value, blahblah = SEXTimmed(immed);
    unsigned short vector8, vector16;
    bool isCycleComplete = false;

    state = FETCH;
    while (!isHalted) {
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
                    offset  = toSign(offset);
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
                        cpu->mdr = cpu->reg[sr1] + SEXTimmed(immed);
                    }
                    cpu->cc = getCC(cpu->mdr); // TODO should this be set in this phase?
                    break;
                case OP_AND:
                    if (bit5 == 0) {
                        cpu->mdr = cpu->reg[sr2] & cpu->reg[sr1];
                    } else if (bit5 == 1) {
                        cpu->mdr = cpu->reg[sr1] & SEXTimmed(immed);
                    }
                    cpu->cc = getCC(cpu->mdr); // TODO should this be set in this phase?
                    break;
                case OP_NOT:
                    cpu->mdr = ~cpu->reg[sr1]; // Interpret as a negative if the leading bit is a 1.
                    cpu->cc = getCC(cpu->mdr); // TODO should this be set in this phase?
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
                //cpu->reg[dr] = cpu->mdr;
                cpu->reg[dr] = toSign(cpu->mdr);
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

        if (isHalted) {
            cpu->pc = 0;
        }

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

unsigned short getCC(unsigned short value) {
    short signedValue = value;
    unsigned short code;
    if (signedValue < 0)
        code = CONDITION_N;
    else if (signedValue == 0)
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
short toSign(unsigned short value) {
    short signedValue = value;
    return signedValue;
}

short SEXTimmed(unsigned short value) {
    if((value & NEGATIVE_IMMEDIATE >> BITSHIFT_NEGATIVE_IMMEDIATE) == 1) {
        return value | MASK_NEGATIVE_IMMEDIATE;
    }
    return (short)value;
}

/**
 * Simulating a ZEXT operation.
 */
unsigned short ZEXT(unsigned short value) {
    // Simulated ZEXT.
    return value;
}

// OLD FUNCTION BEFORE IMPLEMENTING NCURSES.
/*void displayCPU(CPU_p *cpu, int memStart) {
    for(;;) {
        //printf("---displayCPU()\n"); // debugging
        bool rePromptUser = true;
        int menuSelection = 0;
        int newStart = 0;
        char *fileName[FILENAME_SIZE];
        unsigned short tempMar = 0;
        //printf("isHalted=%d  ", isHalted);
        printf("Welcome to the LC-3 Simulator Simulator\n");
        printf("Registers                     Memory\n");

        // First 8 lines
        int i = 0;
        for(i = 0; i < 8; i++) {
            printf("R%u: x%04X", i, cpu->reg[i]);   // Registers.
            printf("                  x%04X: x%04X\n", i+memStart, memory[i + (memStart - ADDRESS_MIN)]); // Memory.
        }

        // Next 3 lines
        int j;
        for (j = 0; j < 3; j++) {
            printf("                           x%04X: x%04X\n", i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
            i++;
        }

        // Next 4 lines.
        printf("PC:  x%04X    IR: x%04X    x%04X: x%04X\n", cpu->pc+ADDRESS_MIN, cpu->ir, i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        i++;
        printf("A:   x%04X     B: x%04X    x%04X: x%04X\n", cpu->A, cpu->B, i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        i++;
        printf("MAR: x%04X   MDR: x%04X    x%04X: x%04X\n", cpu->mar+ADDRESS_MIN, cpu->ir, i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        i++;
        printf("CC:  N:%d Z:%d P:%d           x%04X: x%04X\n",
                cpu->cc >> BITSHIFT_CC_BIT3 & MASK_CC_N,
                cpu->cc >> BITSHIFT_CC_BIT2 & MASK_CC_Z,
                cpu->cc  & MASK_CC_P,
                i+ADDRESS_MIN,
                memory[i + (memStart - ADDRESS_MIN)]);

        i++;
        // Last 2 lines.
        printf("                           x%04X: x%04X\n", i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        while(rePromptUser) {
            rePromptUser = false;
            printf("Select: 1) Load,  3) Step,  5) Display Mem,  9) Exit\n");
            fflush(stdout);

            scanf("%d", &menuSelection); // TODO put this back in.  Just debugging.
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
                    printf("New Starting Address: x");
                    fflush(stdout);
                    scanf("%4X", &newStart);

                    displayCPU(cpu, newStart);
                    //printf("CASE5\n"); // Update the window for the memory registers.
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
}*/


/**
 * Print out fields to the console for the CPU_p object.
 * @param cpu the cpu object containing the data.
 */
void displayCPU(CPU_p *cpu, int memStart) {

    int c;
    int hexExit;

    initscr();
    cbreak();
    clear();

    WINDOW *main_win = newwin(28, 41, 0, 0);
    box(main_win, 0, 0);
    refresh();

    while(1) {
        bool rePromptUser = true;
        bool rePromptHex = true;
        int menuSelection = 0;
        int newStart = 0;
        char inStart[4];
        char *fileName = malloc(FILENAME_SIZE * sizeof(char)); //char fileName[FILENAME_SIZE];
        mvwprintw(main_win, 1, 1, "Welcome to the LC-3 Simulator Simulator");
        mvwprintw(main_win, 2, 1, "Registers");
        mvwprintw(main_win, 2, 31, "Memory");

        // First 8 lines
        int i = 0;
        for(i = 0; i < 8; i++) {
            mvwprintw(main_win, 3+i, 1, "R%u: x%04X", i, cpu->reg[i]);   // Registers.
            mvwprintw(main_win, 3+i, 28, "x%04X: x%04X", i+memStart, memory[i + (memStart - ADDRESS_MIN)]); // Memory.
        }

        // Next 3 lines
        int j = 0;
        for (j = 0; j < 3; j++) {
            mvwprintw(main_win, 11+j, 28, "x%04X: x%04X", i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
            i++;
        }

        // Next 4 lines.
        mvwprintw(main_win, 14, 1, "PC:  x%04X    IR: x%04X    x%04X: x%04X", cpu->pc+ADDRESS_MIN, cpu->ir, i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        i++;
        mvwprintw(main_win, 15, 1, "A:   x%04X     B: x%04X    x%04X: x%04X", cpu->A, cpu->B, i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        i++;
        mvwprintw(main_win, 16, 1, "MAR: x%04X   MDR: x%04X    x%04X: x%04X", cpu->mar+ADDRESS_MIN, cpu->ir, i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        i++;
        mvwprintw(main_win, 17, 1, "CC:  N:%d Z:%d P:%d           x%04X: x%04X",
                cpu->cc >> BITSHIFT_CC_BIT3 & MASK_CC_N,
                cpu->cc >> BITSHIFT_CC_BIT2 & MASK_CC_Z,
                cpu->cc  & MASK_CC_P,
                i+ADDRESS_MIN,
                memory[i + (memStart - ADDRESS_MIN)]);

        i++;
        // Last 2 lines.
        mvwprintw(main_win, 18, 28, "x%04X: x%04X", i+memStart, memory[i + (memStart - ADDRESS_MIN)]);
        mvwprintw(main_win, 19, 1, "Select: 1) Load,         3) Step");  
        mvwprintw(main_win, 20, 9, "5) Display Mem,  9) Exit");
        mvwprintw(main_win, 21, 1, " ------------------------------------- ");

        while(rePromptUser) {
            rePromptUser = false;
            CPU_p cpuTemp;
            move(23, 1);
            clrtoeol();
            move(24, 1);
            clrtoeol();
            move(25, 1);
            clrtoeol();
            c = wgetch(main_win);
            mvwprintw(main_win, 22, 1, "Input: %c", c);
            refresh();
            switch(c){
                case '1':
                    cpuTemp = initialize();
                    cpu = &cpuTemp;
                    mvwprintw(main_win, 23, 1, "Specify file name: ");
                    refresh();
                    wgetstr(main_win, fileName);
                    loadProgramInstructions(openFileText(fileName));
                    free(fileName);
                    refresh();
                    break;
                case '3':
                    //printf("CASE3\n"); // do nothing.  Just let the PC run the next instruction.
                    controller(cpu); // invoke exclusively in case 3.
                    break;
                case '5':
                    while (rePromptHex) {
                        mvwprintw(main_win, 23, 1, "Push Q to return to main menu.");
                        mvwprintw(main_win, 24, 1, "New Starting Address: x");
                        wgetstr(main_win, inStart);
                        refresh();
                        if (inStart[0] == 'q' || inStart[0] == 'Q') {
                            mvwprintw(main_win, 25, 1, "Returning to main menu.");
                            rePromptUser = true;
                            break;
                        }
                        if (hexCheck(inStart)) {
                            newStart = strtol(inStart, NULL, MAX_BIN_BITS);
                            displayCPU(cpu, newStart);
                        } else {
                            mvwprintw(main_win, 24, 1, "You must enter a 4-digit hex value.");
                            mvwprintw(main_win, 25, 1, "Try again.");
                            refresh();
                            rePromptHex = true;
                        }
                    }
                    //printf("CASE5\n"); // Update the window for the memory registers.
                    break;
                case '9':
                    //printf("CASE9\n");
                    //cpu->IR = 0xF025; // TRAP x25
                    endwin();
                    printf("Bubye\n");
                    exit(0);
                    break;
                default:
                    mvwprintw(main_win, 24, 1, "---Invalid selection.");
                    refresh();
                    rePromptUser = true;
                    break;
            }
            wrefresh(main_win);
        }
    }
}


/**
 * A function to check the validity of a hex number.
 * Returns 1 if true, 0 if false.
 */
int hexCheck(char num[]) {
    int counter = 0;
    int valid = 0;
    int i;

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
                , 0};  // mdr

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
    //printf("You said %s", theFileName); // debugging, remove me.
    FILE *dataFile;
    dataFile = fopen(theFileName, "r");
    //if ((dataFile = fopen(theFileName, "r")) == NULL) {
    if (dataFile == NULL) {
//        printf("\n---ERROR: File %s could not be opened.\n\n", theFileName);
        char temp;
        printf("Error, File not found.  Press <ENTER> to continue.", theFileName);
        fflush(stdin);
        temp = getchar(); // BUG this does not work as expected in option #1.
        printf("\n");
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
void loadProgramInstructions(FILE *inputFile) {
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
            startingAddress = strtol(instruction, NULL, MAX_BIN_BITS);
            fgets(instruction, length, inputFile); // processes the carriage return character.
        }

        // In this simulator, we start at ADDRESS_MIN which is the zero'th element in memory[].
        if (startingAddress >= ADDRESS_MIN) {
            i = (startingAddress - ADDRESS_MIN);
            while(!feof(inputFile)) {
                fgets(instruction, length, inputFile);
                memory[i] = strtol(instruction, NULL, MAX_BIN_BITS);
                //printf("\n %04X", memory[i]); // debugging, confirms the memory[] does have the data.
                fgets(instruction, length, inputFile); // processes the carriage return character.
                i++;
                }
        } else {
            printf("Error, starting adddress must be between %x and %x\n"
                    , ADDRESS_MIN, (ADDRESS_MIN + MEMORY_SIZE));
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
    FILE *theFile = openFileText(fileName);
    if(theFile != NULL) {
        loadProgramInstructions(openFileText(fileName));
    }
    displayCPU(&cpu, ADDRESS_MIN); // send the address of the object.
}
