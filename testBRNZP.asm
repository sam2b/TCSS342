; Michael Josten
; 4/30/2018
; v4.30a
; This test code will test the BRnzp logic to see if the simulator actually
; branches

	.ORIG x3000

LOOP	.BLKW 8		; 8 empty no operations

	BRnzp LOOP	; Loop back 9 spaces. PC = x3009 need to go to PC = x3000
			; offset should be xFFF7 = -9

	HALT

	.END