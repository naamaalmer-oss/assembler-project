; =========================================
; File: test3.as
; =========================================

.extern L1
.extern L2
.entry DATABLK

mcro MACA
    add $1, $2, $3
mcroend

mcro MACB
    sub $4, $5, $6
    la L1
mcroend

PROG:   MACA
        MACB
        bne $1, $4, SKIP
        jmp L2
        
SKIP:   addi $10, -15, $11
        call L1
        hlt

DATABLK: .db 1, 2, 3, 4
         .asciz "End of Tests"
