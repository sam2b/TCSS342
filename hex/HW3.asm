; Sample program copied from the PDF of problem 3.
	.ORIG x3000
	AND R0, R0, #0
	AND R7, R7, #0
	ADD R7, R7, #2
START 	LD R1, DATA1
	LD R2, DATA2
	ADD R3, R1, R2
	AND R4, R1, R2
	NOT R5, R1
	BRn HERE1
	AND R1, R1, #0
	BRz HERE2
	BRnzp GETOUT
HERE1 	ADD R0, R0, #-1
	ADD R6, R0, R7
	BRz GETOUT
HERE2 	ST R5, RESULT
GETOUT 	HALT
	.BLKW #10
DATA1 	.FILL #5
DATA2 	.FILL #25
RESULT 	.BLKW #1
	.END