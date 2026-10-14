#ifndef TABLES_H
#define TABLES_H

#include "assembler.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================================= */
/* Macro Table Management                    */
/* ========================================= */

/* Adds a new macro to the end of the linked list */
boolean add_macro(MacroNode **head, const char *name, const char *content);

/* Searches for a macro by name in the linked list */
MacroNode *find_macro(MacroNode *head, const char *name);

/* Frees the entire macro table to prevent memory leaks */
void free_macro_table(MacroNode *head);

/* ========================================= */
/* Symbol Table Management                   */
/* ========================================= */

/* Searches for a symbol by name in the linked list */
SymbolNode *get_symbol(SymbolNode *head, const char *name);

/* Adds a new symbol to the end of the symbol linked list */
boolean add_symbol(SymbolNode **head, const char *name, int address, SymbolType type);

/* Frees the entire symbol table */
void free_symbol_table(SymbolNode *head);

/* Prints the symbol table in a formatted way for debugging */
void print_symbol_table(SymbolNode *head);

/* ========================================= */
/* Instruction Dictionary (Lookup Table)     */
/* ========================================= */

/* 
 * Searches for an instruction by its string name.
 * Returns a pointer to the InstructionInfo struct if found, NULL otherwise.
 */
InstructionInfo *get_instruction_info(const char *name);

/* ========================================= */
/* Label Validation & Reserved Words         */
/* ========================================= */

/* Checks if a given string is a reserved word (directive or instruction) */
boolean is_reserved_word(const char *name);

/* Validates that a label name complies with all structural and reserved-word rules */
boolean is_valid_label_name(const char *name);

#endif /* TABLES_H */
