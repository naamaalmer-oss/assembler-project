# Macros for compiler and flags
CC = gcc
CFLAGS = -Wall -ansi -pedantic

# The main target (executable)
assembler: assembler.o pre_assembler.o tables.o parser.o first_pass.o second_pass.o output.o errors.o
	$(CC) $(CFLAGS) -o assembler assembler.o pre_assembler.o tables.o parser.o first_pass.o second_pass.o output.o errors.o

# Explicit dependencies and rules for each object file
assembler.o: assembler.c assembler.h pre_assembler.h first_pass.h second_pass.h
	$(CC) $(CFLAGS) -c assembler.c

pre_assembler.o: pre_assembler.c pre_assembler.h
	$(CC) $(CFLAGS) -c pre_assembler.c

tables.o: tables.c tables.h
	$(CC) $(CFLAGS) -c tables.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

first_pass.o: first_pass.c first_pass.h
	$(CC) $(CFLAGS) -c first_pass.c

second_pass.o: second_pass.c second_pass.h
	$(CC) $(CFLAGS) -c second_pass.c

output.o: output.c output.h
	$(CC) $(CFLAGS) -c output.c

errors.o: errors.c errors.h
	$(CC) $(CFLAGS) -c errors.c

# Clean rule to remove compiled files and generated output files
clean:
	rm -f *.o assembler *.am *.ob *.ent *.ext
