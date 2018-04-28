; Michael Josten
; This program will output a string to the console.


	.ORIG x3000 	;start at memory location x3000

	LEA R0, STRING	; load R0 with address of string.

	PUTS		; trap command to print string

	ADD R1, R2, R3
	HALT

STRING	.STRINGZ "Hello it is Michael!!!"

	.END
