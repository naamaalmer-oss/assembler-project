/**
 * output.c
 * 
 * Handles the generation of the final output files for the assembler.
 * Responsibilities:
 * 1. Object File (.ob): Generates the hexadecimal representation of the machine code and data.
 * 2. Entries File (.ent): Lists all symbols declared as entry points and their addresses.
 * 3. Externals File (.ext): Lists all external symbol references and the addresses where they are used.
 */

#include "output.h"

/**
 * Creates the Object (.ob) file containing the translated machine code and data.
 * The file starts with a header indicating the sizes of the instruction and data images.
 * Following the header, the instructions are printed in hexadecimal format,
 * followed by the data bytes printed in chunks.
 * 
 * @param filename The base name of the source file (without extension).
 * @param code_head A pointer to the head of the MachineWord linked list.
 * @param data_head A pointer to the head of the DataByte linked list.
 * @param icf The final Instruction Counter (IC) value.
 * @param dcf The final Data Counter (DC) value.
 */
void create_ob_file(char *filename, MachineWord *code_head, DataByte *data_head, int icf, int dcf) {
    char *ob_filename;
    FILE *file;
    MachineWord *current = code_head;

    /* Allocate memory for the filename and append the ".ob" extension */
    ob_filename = malloc(strlen(filename) + OB_EXT_LEN);
    if (!ob_filename) {
        fprintf(stderr, "Error: Memory allocation failed for .ob filename.\n");
        return;
    }
    strcpy(ob_filename, filename);
    strcat(ob_filename, ".ob");

    /* Open the file for writing */
    file = fopen(ob_filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create file %s\n", ob_filename);
        free(ob_filename);
        return;
    }

    /* Print Header: [Instruction Segment Size] [Data Segment Size] */
    fprintf(file, "%d %d\n", (icf - START_ADDRESS), dcf);

    /* Print Code Image */
    while (current != NULL) {
        /* Extract individual bytes from the 32-bit machine word */
        unsigned char b1 = (current->word) & BYTE_MASK;
        unsigned char b2 = (current->word >> SHIFT_8_BITS) & BYTE_MASK;
        unsigned char b3 = (current->word >> SHIFT_16_BITS) & BYTE_MASK;
        unsigned char b4 = (current->word >> SHIFT_24_BITS) & BYTE_MASK;
        
        /* Print address and the 4 bytes in hexadecimal format */
        fprintf(file, "%04d %02X %02X %02X %02X\n", current->address, b1, b2, b3, b4);
        current = current->next;
    }

    /* Print Data Image in chunks of up to BYTES_PER_LINE (4) bytes per line */
    if (data_head != NULL) {
        DataByte *curr_data = data_head;
        int byte_count = 0;
        int line_address = icf; /* Data starts exactly after the instructions segment */

        while (curr_data != NULL) {
            /* Print the starting address for a new line of data */
            if (byte_count == 0) {
                fprintf(file, "%04d", line_address);
            }
            
            /* Print the data byte in hexadecimal format */
            fprintf(file, " %02X", curr_data->byte);
            
            byte_count++;
            line_address++;
            curr_data = curr_data->next;

            /* Start a new line after printing BYTES_PER_LINE bytes */
            if (byte_count == BYTES_PER_LINE) {
                fprintf(file, "\n");
                byte_count = 0;
            }
        }
        
        /* Ensure the last line is properly terminated with a newline */
        if (byte_count > 0) {
            fprintf(file, "\n");
        }
    }

    fclose(file);
    printf("Successfully created %s\n", ob_filename);
    free(ob_filename);
}

/**
 * Creates the Entries (.ent) file if there are any entry points defined in the code.
 * This file lists all symbols exported for use by other assembly files, 
 * alongside their assigned memory addresses.
 * 
 * @param filename The base name of the source file (without extension).
 * @param symbol_table A pointer to the head of the Symbol Table linked list.
 */
void create_ent_file(char *filename, SymbolNode *symbol_table) {
    FILE *file = NULL;
    char *ent_filename = NULL;
    SymbolNode *current = symbol_table;
    int file_created = 0;

    while (current != NULL) {
        /* Check if the current symbol is flagged as an entry point */
        if (current->is_entry) {
            /* If this is the first entry symbol we found, create the file */
            if (!file_created) {
                ent_filename = malloc(strlen(filename) + ENT_EXT_LEN); 
                if (!ent_filename) {
                    fprintf(stderr, "Error: Memory allocation failed for .ent filename.\n");
                    return;
                }
                strcpy(ent_filename, filename);
                strcat(ent_filename, ".ent");
                
                file = fopen(ent_filename, "w");
                if (!file) {
                    fprintf(stderr, "Error: Cannot create file %s\n", ent_filename);
                    free(ent_filename);
                    return;
                }
                file_created = 1;
            }
            
            /* Print the entry symbol and its zero-padded address */
            fprintf(file, "%s %04d\n", current->name, current->address);
        }
        current = current->next;
    }

    /* Clean up and print success message if the file was created */
    if (file_created) {
        fclose(file);
        printf("Successfully created %s\n", ent_filename);
        free(ent_filename);
    }
}

/**
 * Creates the Externals (.ext) file if there are any external symbols referenced in the code.
 * This file lists all external symbol names alongside the memory addresses 
 * where they were used (where their addresses need to be resolved by the linker).
 * 
 * @param filename The base name of the source file (without extension).
 * @param symbol_table A pointer to the head of the Symbol Table linked list (unused here but standard signature).
 * @param code_head A pointer to the head of the MachineWord linked list.
 */
void create_ext_file(char *filename, SymbolNode *symbol_table, MachineWord *code_head) {
    FILE *file = NULL;
    char *ext_filename = NULL;
    MachineWord *current_word = code_head;
    int file_created = 0;

    while (current_word != NULL) {
        /* Check if this machine word is associated with an external symbol reference */
        if (current_word->is_external && current_word->external_name[0] != '\0') {
            /* If this is the first external reference we found, create the file */
            if (!file_created) {
                ext_filename = malloc(strlen(filename) + ENT_EXT_LEN); 
                if (!ext_filename) {
                    fprintf(stderr, "Error: Memory allocation failed for .ext filename.\n");
                    return;
                }
                strcpy(ext_filename, filename);
                strcat(ext_filename, ".ext");
                
                file = fopen(ext_filename, "w");
                if (!file) {
                    fprintf(stderr, "Error: Cannot create file %s\n", ext_filename);
                    free(ext_filename);
                    return;
                }
                file_created = 1;
            }
            
            /* Print the external symbol name and the address where it was used */
            fprintf(file, "%s %04d\n", current_word->external_name, current_word->address);
        }
        current_word = current_word->next;
    }

    /* Clean up and print success message if the file was created */
    if (file_created) {
        fclose(file);
        printf("Successfully created %s\n", ext_filename);
        free(ext_filename);
    }
}
