#ifndef PRE_ASSEMBLER_H
#define PRE_ASSEMBLER_H

#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Runs the pre-assembler phase.
 * Reads the input file, expands all macros, and generates a .am file.
 * Returns TRUE if successful, FALSE if memory allocation failed or errors occurred.
 */
boolean expand_macros(FILE *input_file, char *filename);

#endif /* PRE_ASSEMBLER_H */
