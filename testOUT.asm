; Michael Josten
; TCSS 372 version 4.28c
; This program will test the trap OUT instruction by 
; putting an arbitrary character into R0 and then calling the OUT instruction.
; for example, we will use 'P' = x50.

	.ORIG x3000

	LD R0, CHAR
	OUT

	HALT

CHAR	.FILL x50  ; 'P'

	.END