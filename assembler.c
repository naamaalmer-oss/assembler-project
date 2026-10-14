/**
 * assembler.c
 * 
 * Main entry point for the Two-Pass Assembler project.
 * This program reads assembly source files (.as), expands macros,
 * and processes them through a two-pass assembly algorithm to generate
 * machine code and output files (.ob, .ent, .ext).
 */
 
#include "assembler.h"
#include "pre_assembler.h"
#include "first_pass.h"
#include "tables.h"
#include "second_pass.h"

/* Function prototype for processing a single file */
void process_file(char *filename);

/**
 * Main entry point of the assembler.
 * Iterates over all file names provided as command-line arguments 
 * and processes them one by one.
 * 
 * @param argc The number of command-line arguments.
 * @param argv Array of command-line argument strings (file names without extension).
 * @return 0 on successful execution, 1 if no files were provided.
 */
int main(int argc, char *argv[]) {
    int i;

    /* Check if at least one file name was provided (excluding the program name) */
    if (argc < MIN_ARGS) {
        fprintf(stderr, "Error: No input files provided.\n");
        fprintf(stderr, "Usage: %s <file1> <file2> ...\n", argv[0]);
        return 1;
    }

    /* Iterate through each file provided in the command line */
    for (i = 1; i < argc; i++) {
        process_file(argv[i]);
    }

    return 0;
}

/**
 * Processes a single assembly source file.
 * Manages the complete pipeline: Pre-Assembler -> First Pass -> Second Pass.
 * Dynamically allocates structures, tracks errors, and safely cleans up memory.
 * 
 * @param filename The base name of the file (without the .as extension).
 */
void process_file(char *filename) {
    char *full_filename;
    FILE *file;
    SymbolNode *symbol_table = NULL;
    int ic = 0; /* Instruction Counter */
    int dc = 0; /* Data Counter */

    /* Allocate memory for filename + ".as" + null terminator */
    full_filename = malloc(strlen(filename) + EXT_LEN);
    if (!full_filename) {
        fprintf(stderr, "Error: Memory allocation failed for file %s.\n", filename);
        return;
    }

    /* Build the full file name by appending the ".as" extension */
    strcpy(full_filename, filename);
    strcat(full_filename, ".as");

    /* Attempt to open the source file for reading */
    file = fopen(full_filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", full_filename);
        free(full_filename);
        return;
    }

    printf("Successfully opened %s for processing.\n", full_filename);

    /* ---------------------------------------------------------
     * Phase 1: Pre-Assembler (Macro Expansion)
     * --------------------------------------------------------- */
    if (expand_macros(file, filename)) {
        printf("Pre-assembler finished successfully. Created %s.am\n", filename);
        
        printf("Starting first pass on %s.am...\n", filename);
        
        /* ---------------------------------------------------------
         * Phase 2: First Pass (Syntax check, Symbol Table, IC/DC tracking)
         * --------------------------------------------------------- */
        if (execute_first_pass(filename, &symbol_table, &ic, &dc)) {
            printf("First pass completed successfully!\n");
            
            printf("Starting second pass on %s.am...\n", filename);
            
            /* ---------------------------------------------------------
             * Phase 3: Second Pass (Machine code encoding & Output files)
             * --------------------------------------------------------- */
            if (!execute_second_pass(filename, symbol_table, ic, dc)) {
                fprintf(stderr, "Second pass failed with errors for %s.\n", filename);
            }
            
        } else {
            fprintf(stderr, "First pass failed with errors for %s.\n", filename);
        }

        /* Free the dynamically allocated symbol table after processing */
        free_symbol_table(symbol_table);

    } else {
        fprintf(stderr, "Pre-assembler failed for %s.\n", filename);
    }

    /* Clean up allocated resources and close file pointers */
    fclose(file);
    free(full_filename);
}
