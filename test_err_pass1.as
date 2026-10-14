; Errors meant to be caught by the First Pass

.entry duplicateLabel
.extern duplicateLabel

1badLabel: move $1, $2
move: move $1, $2

myLabel: add $1, $2, $3
myLabel: sub $1, $2, $3

add $1 $2, $3
sub $1,,$2, $3
hlt extra_garbage

move $1
move $1, $2, $3, $4

add 5, $2, $3
add $32, $2, $3

.db 300
.asciz "Missing closing quote
