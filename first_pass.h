#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "assembler.h"

/*
 * Executes the first pass on the .am file.
 * Builds the symbol table and calculates the required memory size.
 * Returns TRUE if the pass completed without syntax errors, FALSE otherwise.
 */
boolean execute_first_pass(char *filename, SymbolNode **symbol_table, int *ic, int *dc);

#endif /* FIRST_PASS_H */
