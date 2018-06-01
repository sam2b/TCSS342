    .ORIG x3000
    ADD   R3 R3 #3
    ADD   R4 R4 #4
    ADD   R5 R5 #5
    PUSH  R3
    PUSH  R4
    PUSH  R5
    POP   R3
    POP   R2
    POP   R1
    POP   R1 ; illegal pop on an empty stack.
    HALT
    .END
    