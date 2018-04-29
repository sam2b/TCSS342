; Michael Josten
; TCSS 372 version 4.28c
; This program will test the trap OUT instruction by 
; putting an arbitrary character into R0 and then calling the OUT instruction.
; for example, we will use 'P' = x50.
; 'L' = x4C
; 'O' = x4F
; 'X' = x58

	.ORIG x3000

	LD R0, CHAR1
	OUT
	LD R0, CHAR2
	OUT
	LD R0, CHAR3
	OUT
	LD R0, CHAR4
	OUT

	HALT

CHAR1	.FILL x50  ; 'P'
CHAR2	.FILL x4C  ; 'L'
CHAR3	.FILL x4F  ; 'O'
CHAR4 	.FILL x58  ; 'X'

	.END