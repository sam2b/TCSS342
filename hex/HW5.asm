

; Michael Josten
; 4/24/2018
; TCSS 372 Problem 5
; This program, using the lc3 ISA, will prompt the user to enter their name and then it will 
; encrypt the name by subtracting a constant value and then it will output the name to the console.

	; MAIN
		.ORIG x3000			; Start at location x3000
		LEA R1, STRING			; load address of string into R1
		LEA R0, PROMPT1			; Load address of prompt 1 into R0
		PUTS				; Print prompt1 to console
		LD R2, NEWLINE			; Load R2 with the value of a newline char
		NOT R2, R2			; convert R2 to negative
		ADD R2, R2, #1

GETNAME		GETC				; get first character from console. start of loop
		ADD R3, R0, R2			; compare input with newline 
		BRz BREAK			; if character = newline, break loop
		STR R0, R1, #0			; Store input character into string array
		ADD R1, R1, #1			; increment pointer to string array
		OUT				; echo input to console
		BRnzp GETNAME			; unconditional branch to start of loop for iteration
		
BREAK		OUT				; output newline character to console
		LEA R1, STRING			; load address of string into R1
		LEA R5, ENCRYPT			; Load R5 with address of encrypt subroutine
		JSRR R5				; jump to encrypt subroutine 

		LEA R0, PROMPT2			; load R0 with address of prompt 2
		PUTS				; print prompt 2 to console
		GETC				; stall program until user enters key
		LEA R0, STRING			; load R0 with address of string
		PUTS				; print encrypted string to console		

		HALT

		; Encryption Subroutine 
ENCRYPT		AND R4, R4, #0 			; Set R4 to zero	
		ADD R4, R4, #3			; set R4 to constant 3, to subtract from chars
		NOT R4, R4			; set R4 to negative 3
		ADD R4, R4, #1		
		
ENCLOOP		LDR R2, R1, #0			; load R2 with character from string
		
		BRz ENCBREAK			; if char = null terminator then break loop
		ADD R2, R2, R4			; subtract constant from character
		STR R2, R1, #0			; store character in string.
		ADD R1, R1, #1			; increment pointer
		BRnzp ENCLOOP			; unconditional branch to start of encrypt loop

ENCBREAK	LEA R0, STRING			; on success, load R0 with string address
		RET				; return from subroutine.
	
		; Variable Storage
NEWLINE		.FILL x0A				; ASCII value of newline
STRING		.BLKW #20				; username string
PROMPT1		.STRINGZ "Enter your name: "		; first prompt
PROMPT2		.STRINGZ "Press any key to continue..."	; prompt 2

		.END
