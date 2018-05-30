; Stack test program.

.orig x3000 ; this is the start of the program.

	ADD R3 R3 #3
	ADD R4 R4 #4
	ADD R5 R5 #5

PUSH	ADD R0 R3 R6 ;push
	ADD R0 R4 R6 ;push
	ADD R0 R5 R6 ;push

POP	ADD R1 R0 R6 ;pop
	ADD R2 R0 R6 ;pop
	ADD R3 R0 R6 ;pop
	
	HALT
	.END ; this is the end of the program.