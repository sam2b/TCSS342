	.orig x3000
	AND R2 R2 0
	AND R4 R4 0
	ADD R2 R2 #2
	ADD R4 R4 #4

;	ST  R2 DATA
	STI R4 DATA

;	LD  R2 DATA	; x300B location.
	LDI R2 DATA	; x300B location.

	HALT		; Stop this program now.

DATA	.FILL x300F

	.end		; End of program