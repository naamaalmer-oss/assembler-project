#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "assembler.h"
#include "tables.h"

/*
 * Executes the second pass on the .am file.
 * Encodes code instructions into 32-bit machine code words,
 * resolves label target addresses using the symbol table,
 * and updates entry symbols.
 */
boolean execute_second_pass(char *filename, SymbolNode *symbol_table, int icf, int dcf);

/* Encodes an R-Type instruction into a 32-bit machine word */
unsigned long encode_R_type(int opcode, int rs, int rt, int rd, int funct);

/* Encodes an I-Type instruction into a 32-bit machine word */
unsigned long encode_I_type(int opcode, int rs, int rt, int immed);

/* Encodes a J-Type instruction into a 32-bit machine word */
unsigned long encode_J_type(int opcode, int reg, unsigned long address);

/* Adds a new machine word to the code image linked list */
void add_machine_word(MachineWord **head, int address, unsigned long word);

/* Frees the machine word linked list */
void free_machine_words(MachineWord *head);

void add_data_byte(DataByte **head, int address, unsigned char byte);

void free_data_bytes(DataByte *head);

#endif /* SECOND_PASS_H */
