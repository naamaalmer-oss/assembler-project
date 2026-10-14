#include "first_pass.h"
#include "parser.h"
#include "tables.h"
#include "errors.h"

/**
 * Executes the first pass of the two-pass assembler algorithm.
 * 
 * Responsibilities:
 * 1. Scans the expanded source file (.am) line by line.
 * 2. Identifies and validates all label definitions.
 * 3. Builds the Symbol Table, assigning appropriate addresses and types (CODE/DATA/EXTERNAL).
 * 4. Counts the required memory size for instructions (IC) and data (DC).
 * 5. Performs initial syntax checks (line length, valid names, extraneous text).
 * 6. Adjusts the addresses of all DATA symbols at the end so they follow the CODE segment.
 * 
 * @param filename The base name of the source file (without extension).
 * @param symbol_table A double pointer to the head of the Symbol Table linked list.
 * @param ic A pointer to the Instruction Counter (tracks code memory size).
 * @param dc A pointer to the Data Counter (tracks data memory size).
 * @return TRUE if the first pass completed with zero errors, FALSE otherwise.
 */
boolean execute_first_pass(char *filename, SymbolNode **symbol_table, int *ic, int *dc) {
    char line[MAX_LINE_LEN];
    char *am_filename;
    char word[MAX_LINE_LEN]; 
    char label_name[MAX_LABEL_LEN];
    char op1[MAX_LINE_LEN]; /* Buffer for operand extraction */
    char *ptr;
    FILE *file;
    boolean has_label = FALSE;
    boolean is_error = FALSE;
    int line_number = 0;
    SymbolNode *current_sym;
    SymbolNode *target; /* Used for symbol lookups */
    int c;
    InstructionInfo *info;
    
    /* Initialization of counters */
    current_sym = *symbol_table;
    *ic = START_ADDRESS; /* Memory starts at address 100 */
    *dc = 0;

    /* Build the .am filename */
    am_filename = malloc(strlen(filename) + EXT_LEN);
    if (!am_filename) {
        fprintf(stderr, "Error: Memory allocation failed for .am file.\n");
        return FALSE;
    }
    strcpy(am_filename, filename);
    strcat(am_filename, ".am");

    /* Open the .am file for reading */
    file = fopen(am_filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s for first pass.\n", am_filename);
        free(am_filename);
        return FALSE;
    }

    /* Read the file line by line */
    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        line_number++;
        has_label = FALSE;
        ptr = line;
        
        /* 1. Check for line length exceeding the allowed limit (80 chars) */
        if (strchr(line, '\n') == NULL && !feof(file)) {
            print_error(ERR_LINE_TOO_LONG, line_number);
            is_error = TRUE;
            
            /* Flush the remaining characters in the buffer to prevent spillover */
            while ((c = fgetc(file)) != '\n' && c != EOF);
            
            continue; /* Skip processing this invalid line */
        }
    
        /* Extract the first word of the line */
        ptr = extract_word(word, ptr);

        /* 2. Skip empty lines and comments */
        if (word[0] == '\0' || word[0] == ';') {
            continue;
        }

        /* 3. Check if there is a label (word ends with ':') */
        if (word[strlen(word) - 1] == ':') {
            has_label = TRUE;
            
            /* Copy the label name without the ':' character */
            strncpy(label_name, word, strlen(word) - 1);
            label_name[strlen(word) - 1] = '\0';
            
            /* Validate the label definition before processing it */
            if (!validate_label_definition(word, ptr, line_number)) {
                is_error = TRUE;
                continue; /* Skip to the next line if the label is invalid */
            }
            
            /* Read the next word to see what comes after the label */
            ptr = extract_word(word, ptr);
            
            /* If the line only contained a label and nothing else, move on */
            if (word[0] == '\0') {
                continue;
            }
        }

        /* 4. Handle data directives (.db, .dh, .dw, .asciz) */
        if (strcmp(word, ".db") == 0 || strcmp(word, ".dh") == 0 || 
            strcmp(word, ".dw") == 0 || strcmp(word, ".asciz") == 0) {
            
            if (has_label) {
                /* Validate label name structure */
                if (!is_valid_label_name(label_name)) {
                    print_error(ERR_INVALID_LABEL_NAME, line_number);
                    is_error = TRUE;
                } 
                /* Add to symbol table as DATA with the current DC */
                else if (!add_symbol(symbol_table, label_name, *dc, DATA)) {
                    print_error(ERR_LABEL_ALREADY_DEFINED, line_number);
                    is_error = TRUE;
                }
            }

            /* Determine the correct size to advance DC */
            if (strcmp(word, ".asciz") == 0) {
                *dc += calculate_data_size(ptr, 0, line_number, &is_error); 
            } else if (strcmp(word, ".db") == 0) {
                *dc += calculate_data_size(ptr, 1, line_number, &is_error);
            } else if (strcmp(word, ".dh") == 0) {
                *dc += calculate_data_size(ptr, 2, line_number, &is_error);
            } else if (strcmp(word, ".dw") == 0) {
                *dc += calculate_data_size(ptr, 4, line_number, &is_error);
            }
            
            continue;
        }
            
        /* 5. Handle .extern directive */
        else if (strcmp(word, ".extern") == 0) {
            ptr = extract_operand(op1, ptr);

            if (op1[0] == '\0') {
                print_error(ERR_MISSING_OPERANDS, line_number);
                is_error = TRUE;
                continue;
            }
            
            if (!is_valid_label_name(op1)) {
                print_error(ERR_INVALID_LABEL_NAME, line_number);
                is_error = TRUE;
                continue;
            }
            
            if (!check_extraneous_text(ptr, line_number)) {
                is_error = TRUE;
                continue;
            }

            target = get_symbol(*symbol_table, op1);
            
            if (target != NULL) {
                /* If the symbol exists but isn't external, it's a conflict */
                if (target->type != EXTERNAL) {
                    print_error(ERR_LABEL_ALREADY_DEFINED, line_number);
                    is_error = TRUE;
                }
            } else {
                /* Add the new external symbol with address 0 */
                if (!add_symbol(symbol_table, op1, 0, EXTERNAL)) {
                    is_error = TRUE;
                }
            }
            
            continue;
        }
            
        /* 6. Handle .entry directive */
        else if (strcmp(word, ".entry") == 0) {
            ptr = extract_operand(op1, ptr);

            if (op1[0] == '\0') {
                print_error(ERR_MISSING_OPERANDS, line_number);
                is_error = TRUE;
                continue;
            }
            
            if (!is_valid_label_name(op1)) {
                print_error(ERR_INVALID_LABEL_NAME, line_number);
                is_error = TRUE;
                continue;
            }
            
            if (!check_extraneous_text(ptr, line_number)) {
                is_error = TRUE;
                continue;
            }

            target = get_symbol(*symbol_table, op1);
            if (target != NULL && target->type == EXTERNAL) {
                print_error(ERR_ENTRY_EXTERN_CONFLICT, line_number);
                is_error = TRUE;
                continue;
            }

            /* If target exists, mark it as entry. If not, it will be handled in pass 2 */
            if (target != NULL) {
                target->is_entry = 1;
            }
            
            continue;
        }
            
        /* 7. It's a regular code instruction */
        else {
            if (has_label) {
                /* Validate the label name structure and check for reserved words */
                if (!is_valid_label_name(label_name)) {
                    print_error(ERR_INVALID_LABEL_NAME, line_number);
                    is_error = TRUE;
                } else {
                    /* Add to symbol table as CODE with the current IC */
                    if (!add_symbol(symbol_table, label_name, *ic, CODE)) {
                        print_error(ERR_LABEL_ALREADY_DEFINED, line_number);
                        is_error = TRUE;
                    }
                }
            }
            
            info = get_instruction_info(word); 
            if (info == NULL) {
                print_error(ERR_UNKNOWN_OPCODE, line_number);
                is_error = TRUE;
                continue;
            }
            
            /* Calculate instruction footprint (usually 4 bytes) and advance IC */
            *ic += calculate_code_size(ptr);
        }
    }

    /* Clean up */
    fclose(file);
    free(am_filename);

    /* Update all DATA symbols with the final IC value (so they sit after the code in memory) */
    current_sym = *symbol_table; /* Reset pointer to the head of the list */
    while (current_sym != NULL) {
        if (current_sym->type == DATA) {
            current_sym->address += *ic;
        }
        current_sym = current_sym->next;
    }

    return !is_error;
}
