; Michael Josten
; TCSS 372 version 4.28c
; This program will use the GETC trap command, 
; check R0 after this program runs to see if you get the correct result.
; for example 
; 's' = x73
; 'F' = x46

	.ORIG x3000	; start at x3000
	GETC
	HALT
	.END