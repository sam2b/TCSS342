	.orig x3000
	AND R2 R2 0
	ADD R2 R2 #3

;	ST  R2 DATA
	STI R2 DATA


;	LD  R2 #10	; x300B location.
;	LDI R2 #10	; x300B location.


	HALT		; Stop this program now.

DATA	.FILL x3010

	.end		; End of program