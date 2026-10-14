; =========================================
; File: test1.as
; Tests: Macros with labels, R-type instructions, and directives
; =========================================

.entry START
.extern EXTF

mcro SETUP
    add $1, $2, $3
    sub $4, $5, $6
    move $7, $8
mcroend

START:  SETUP
        and $9, $10, $11
        nor $12, $13, $14
        jmp EXTF
        hlt

STR1:   .asciz "Testing 123"
ARR1:   .db 15, -15, 127
