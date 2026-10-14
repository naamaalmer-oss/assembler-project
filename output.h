#ifndef OUTPUT_H
#define OUTPUT_H

#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Creates the .ob (Object) file with the Code and Data images */
void create_ob_file(char *filename, MachineWord *code_head, DataByte *data_head, int icf, int dcf);

/* Creates the .ent (Entry) file if there are any entry symbols */
void create_ent_file(char *filename, SymbolNode *symbol_table);

/* Creates the .ext (Extern) file if there are any external usages */
void create_ext_file(char *filename, SymbolNode *symbol_table, MachineWord *code_head);

#endif /* OUTPUT_H */
