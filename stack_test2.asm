; Stack test2 program, nested subroutines.  WILL NOT WORK ON BOOK'S SIMULATOR.

.orig x3000 ; this is the start of the program.

	ADD R1 R1 #1
	ADD R2 R2 #2
	ADD R3 R3 #3
	JSR WORK     ; The real R0 is the result of the WORK subroutine.
	ADD R3 R3 R0 ; The result R0 from the WORK subroutine is added to this R3.
STOP	HALT
	
WORK	ADD R4 R1 R2
	ADD R0 R3 R4 ; Can access WORK R0 value via memory[stackPointer - 4]
	JSR TEST     ; The real R0 is the result of the TEST subroutine.
	ADD R4 R4 R0 ; The result R0 from the TEST subroutine is added to this R4.
	RET

TEST	ADD R1 R1 #-1
	ADD R2 R2 #-1
	ADD R3 R3 #-1
	ADD R0 R1 R2
	ADD R0 R0 R3
	RET
	
	.END ; this is the end of the program.