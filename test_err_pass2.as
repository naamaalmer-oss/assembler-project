; Errors meant to be caught by the Second Pass
.entry startLabel

startLabel: add $1, $2, $3
    jmp missingLabel

.extern extLabel
    beq $1, $2, extLabel

    la $5
