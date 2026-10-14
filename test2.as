; =========================================
; File: test2.as
; Tests: I-type instructions, Branches, Memory load/store, J-type
; =========================================

.entry LOOP
.extern PRINT

START:  addi $1, -100, $2
        subi $3, 50, $4
LOOP:   bgt $1, $2, ENDL
        lw $5, 10, $6
        sw $0, 20, $7
        call PRINT
        jmp LOOP
ENDL:   jmp $3
        hlt

ARR2:   .dh 1000, -2000, 32000
ARR3:   .dw 100000, -100000, 0
