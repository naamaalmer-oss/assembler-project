/**
 * second_pass.c
 * 
 * Implements the second pass of the assembler.
 * This phase is responsible for:
 * 1. Completing the Symbol Table with .entry declarations.
 * 2. Encoding all instructions (R-type, I-type, J-type) into 32-bit machine words.
 * 3. Populating the code and data image linked lists.
 * 4. Generating the final output files (.ob, .ent, .ext) if no errors occurred.
 */

#include "second_pass.h"
#include "parser.h"
#include "output.h"
#include "errors.h"
#include "tables.h"

/**
 * Encodes an R-Type instruction into a 32-bit machine word.
 * 
 * @param opcode The instruction's opcode.
 * @param rs The source register.
 * @param rt The target register.
 * @param rd The destination register.
 * @param funct The function code.
 * @return The encoded 32-bit machine word.
 */
unsigned long encode_R_type(int opcode, int rs, int rt, int rd, int funct) {
    unsigned long word = 0;
    word |= (opcode & OPCODE_MASK) << OPCODE_SHIFT;
    word |= (rs     & REG_MASK)    << RS_SHIFT;
    word |= (rt     & REG_MASK)    << RT_SHIFT;
    word |= (rd     & REG_MASK)    << RD_SHIFT;
    word |= (funct  & OPCODE_MASK) << FUNCT_SHIFT; /* funct also uses 6 bits like opcode */
    return word;
}

/**
 * Encodes an I-Type instruction into a 32-bit machine word.
 * 
 * @param opcode The instruction's opcode.
 * @param rs The source register.
 * @param rt The target register.
 * @param immed The immediate value or relative address offset.
 * @return The encoded 32-bit machine word.
 */
unsigned long encode_I_type(int opcode, int rs, int rt, int immed) {
    unsigned long word = 0;
    word |= (opcode & OPCODE_MASK) << OPCODE_SHIFT;
    word |= (rs     & REG_MASK)    << RS_SHIFT;
    word |= (rt     & REG_MASK)    << RT_SHIFT;
    word |= (immed  & IMMED_MASK);
    return word;
}

/**
 * Encodes a J-Type instruction into a 32-bit machine word.
 * 
 * @param opcode The instruction's opcode.
 * @param reg A flag (1 or 0) indicating if the operand is a register.
 * @param address The absolute address or register number.
 * @return The encoded 32-bit machine word.
 */
unsigned long encode_J_type(int opcode, int reg, unsigned long address) {
    unsigned long word = 0;
    word |= (opcode  & OPCODE_MASK)  << OPCODE_SHIFT;
    word |= (reg     & REG_BIT_MASK) << REG_BIT_SHIFT;
    word |= (address & ADDRESS_MASK);
    return word;
}

/**
 * Adds a new machine word to the end of the code image linked list.
 * 
 * @param head A double pointer to the head of the MachineWord list.
 * @param address The memory address of the instruction.
 * @param word The 32-bit encoded machine word.
 */
void add_machine_word(MachineWord **head, int address, unsigned long word) {
    MachineWord *new_node;
    MachineWord *current;
    
    new_node = (MachineWord *)malloc(sizeof(MachineWord));
    if (!new_node) {
        return;
    }
    
    new_node->is_external = 0;
    new_node->external_name[0] = '\0';
    new_node->address = address;
    new_node->word = word;
    new_node->next = NULL;
    
    current = *head;
    
    if (*head == NULL) {
        *head = new_node;
    } else {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

/**
 * Frees the dynamically allocated memory for the code image linked list.
 * 
 * @param head A pointer to the head of the MachineWord list.
 */
void free_machine_words(MachineWord *head) {
    MachineWord *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

/**
 * Adds a new data byte to the end of the data image linked list.
 * 
 * @param head A double pointer to the head of the DataByte list.
 * @param address The memory address of the data byte.
 * @param byte The 8-bit data value to store.
 */
void add_data_byte(DataByte **head, int address, unsigned char byte) {
    DataByte *new_node = (DataByte *)malloc(sizeof(DataByte));
    DataByte *current = *head;
    
    if (!new_node) return;
    
    new_node->address = address;
    new_node->byte = byte;
    new_node->next = NULL;
    
    if (*head == NULL) {
        *head = new_node;
    } else {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

/**
 * Frees the dynamically allocated memory for the data image linked list.
 * 
 * @param head A pointer to the head of the DataByte list.
 */
void free_data_bytes(DataByte *head) {
    DataByte *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

/**
 * Executes the second pass of the assembler.
 * Processes the intermediate .am file, validates operand types and counts,
 * calculates relative offsets, updates entry symbols, encodes instructions, 
 * and delegates the creation of the final output files.
 * 
 * @param filename The original file name (without extension).
 * @param symbol_table The symbol table built during the first pass.
 * @param icf The final Instruction Counter value from the first pass.
 * @param dcf The final Data Counter value from the first pass.
 * @return TRUE if the pass completed without errors, FALSE otherwise.
 */
boolean execute_second_pass(char *filename, SymbolNode *symbol_table, int icf, int dcf) {
    char line[MAX_LINE_LEN];
    char word[MAX_LINE_LEN];
    char op1[MAX_LINE_LEN], op2[MAX_LINE_LEN], op3[MAX_LINE_LEN];
    char *am_filename;
    char *ptr;
    FILE *file;
    boolean is_error = FALSE;
    int line_number = 0;
    int ic = START_ADDRESS; 
    DataByte *data_image = NULL;
    int current_data_address = icf; /* Data memory starts immediately after the instruction memory */
    MachineWord *code_image = NULL;

    /* Prepare the .am file name and open it */
    am_filename = malloc(strlen(filename) + EXT_LEN);
    strcpy(am_filename, filename);
    strcat(am_filename, ".am");

    file = fopen(am_filename, "r");
    if (!file) {
        free(am_filename);
        return FALSE;
    }

    /* Iterate through each line of the .am file */
    while (fgets(line, MAX_LINE_LEN, file) != NULL) 
    {
        InstructionInfo *info;
        unsigned long machine_word = 0;
        SymbolNode *target = NULL;
        int rs = 0, rt = 0, rd = 0, immed = 0, reg_bit = 0;
        unsigned long address = 0;
        char *s;
        int is_byte = 0, is_half = 0, is_word = 0, val = 0;
        SymbolNode *entry_sym = NULL;
        MachineWord *last_word = NULL;
        int c;
        
        line_number++;
        ptr = line;
        
        /* Check if the line exceeds the maximum allowed length (80 characters) */
        if (strchr(line, '\n') == NULL && !feof(file)) 
        {
            print_error(ERR_LINE_TOO_LONG, line_number);
            is_error = TRUE;
            
            /* Flush the remaining characters in the buffer to prevent spillover */
            while ((c = fgetc(file)) != '\n' && c != EOF);
            continue; 
        }
        
        ptr = extract_word(word, ptr);

        /* Skip empty lines and comments */
        if (word[0] == '\0' || word[0] == ';') continue;
        
        /* Skip label definitions (already processed in the First Pass) */
        if (word[strlen(word) - 1] == ':') ptr = extract_word(word, ptr);
        
        /* Skip .extern directives (already processed in the First Pass) */
        if (strcmp(word, ".extern") == 0) continue;

        /* Process string directives */
        if (strcmp(word, ".string") == 0 || strcmp(word, ".asciz") == 0) {
            s = strchr(ptr, '"');
            if (s) {
                s++;
                while (*s != '"' && *s != '\0') {
                    add_data_byte(&data_image, current_data_address++, *s);
                    s++;
                }
                add_data_byte(&data_image, current_data_address++, '\0');
            }
            continue;
        }
        
        /* Process numerical data directives */
        if (strcmp(word, ".data") == 0 || strcmp(word, ".dw") == 0 || strcmp(word, ".db") == 0 || strcmp(word, ".dh") == 0) {
            is_byte = (strcmp(word, ".db") == 0);
            is_half = (strcmp(word, ".dh") == 0);
            is_word = (strcmp(word, ".data") == 0 || strcmp(word, ".dw") == 0);
            
          while (1) {
                ptr = extract_operand(op1, ptr);
                if (op1[0] != '\0') {
                    val = parse_number(op1);
                    if (is_byte) {
                        /* .db - 1 byte */
                        add_data_byte(&data_image, current_data_address++, val & BYTE_MASK);
                    } else if (is_half) {
                        /* .dh - 2 bytes */
                        add_data_byte(&data_image, current_data_address++, val & BYTE_MASK);
                        add_data_byte(&data_image, current_data_address++, (val >> SHIFT_8_BITS) & BYTE_MASK);
                    } else if (is_word) {
                        /* .dw - 4 bytes */
                        add_data_byte(&data_image, current_data_address++, val & BYTE_MASK);
                        add_data_byte(&data_image, current_data_address++, (val >> SHIFT_8_BITS) & BYTE_MASK);
                        add_data_byte(&data_image, current_data_address++, (val >> SHIFT_16_BITS) & BYTE_MASK);
                        add_data_byte(&data_image, current_data_address++, (val >> SHIFT_24_BITS) & BYTE_MASK);
                    }
                }
                if (ptr == NULL || *ptr == '\0' || *ptr == '\n' || *ptr == '\r') break;
            }
            continue;
        }

        /* Process .entry directives */
        if (strcmp(word, ".entry") == 0) {
            ptr = extract_operand(op1, ptr);
            if (op1[0] != '\0') {
                entry_sym = get_symbol(symbol_table, op1);
                if (entry_sym != NULL) {
                    entry_sym->is_entry = 1; /* Mark the symbol as an entry point */
                } else {
                    printf("Error: .entry label '%s' not found in line %d\n", op1, line_number);
                    is_error = TRUE;
                }
            }
            continue;
        }

        /* Parse and encode machine instructions */
        info = get_instruction_info(word); 
        if (info != NULL) {
            machine_word = 0;
            target = NULL;
            op1[0] = '\0'; op2[0] = '\0'; op3[0] = '\0';

            /* -------------- R-TYPE INSTRUCTIONS -------------- */
            if (info->type == R_TYPE) {
                if (!check_comma_syntax(ptr, line_number)) {
                    is_error = TRUE;
                    continue;
                }

                if (info->opcode == OP_MOVE) { 
                    ptr = extract_operand(op1, ptr);
                    ptr = extract_operand(op2, ptr);
                    if (op1[0] == '\0' || op2[0] == '\0') {
                        print_error(ERR_INVALID_OPERAND_COUNT, line_number);
                        is_error = TRUE;
                        continue;
                    }
                } else { 
                    ptr = extract_operand(op1, ptr);
                    ptr = extract_operand(op2, ptr);
                    ptr = extract_operand(op3, ptr);
                    if (op1[0] == '\0' || op2[0] == '\0' || op3[0] == '\0') {
                        print_error(ERR_INVALID_OPERAND_COUNT, line_number);
                        is_error = TRUE;
                        continue;
                    }
                }
                
                if (!check_extraneous_text(ptr, line_number)) {
                    is_error = TRUE;
                    continue;
                }

                if (op1[0] != '$') {
                    print_error(ERR_INVALID_OPERAND_TYPE, line_number);
                    is_error = TRUE;
                    continue;
                }
                rs = parse_register(op1);
                if (rs == -1) {
                    print_error(ERR_REGISTER_OUT_OF_RANGE, line_number);
                    is_error = TRUE;
                    continue;
                }
                
                if (info->opcode == OP_MOVE) {
                    if (op2[0] != '$') {
                        print_error(ERR_INVALID_OPERAND_TYPE, line_number);
                        is_error = TRUE;
                        continue;
                    }
                    rd = parse_register(op2);
                    rt = 0; 
                    
                    if (rd == -1) {
                        print_error(ERR_REGISTER_OUT_OF_RANGE, line_number);
                        is_error = TRUE;
                        continue;
                    }
                } else {
                    if (op2[0] != '$' || op3[0] != '$') {
                        print_error(ERR_INVALID_OPERAND_TYPE, line_number);
                        is_error = TRUE;
                        continue;
                    }
                    rt = parse_register(op2);
                    rd = parse_register(op3);
                    
                    if (rt == -1 || rd == -1) {
                        print_error(ERR_REGISTER_OUT_OF_RANGE, line_number);
                        is_error = TRUE;
                        continue;
                    }
                }

                machine_word = encode_R_type(info->opcode, rs, rt, rd, info->funct);

            /* -------------- I-TYPE INSTRUCTIONS -------------- */
            } else if (info->type == I_TYPE) {
                if (!check_comma_syntax(ptr, line_number)) {
                    is_error = TRUE;
                    continue;
                }

                ptr = extract_operand(op1, ptr);
                ptr = extract_operand(op2, ptr);
                ptr = extract_operand(op3, ptr);
                
                if (op1[0] == '\0' || op2[0] == '\0' || op3[0] == '\0') {
                    print_error(ERR_INVALID_OPERAND_COUNT, line_number);
                    is_error = TRUE;
                    continue;
                }
                
                if (!check_extraneous_text(ptr, line_number)) {
                    is_error = TRUE;
                    continue;
                }

                if (op1[0] != '$') {
                    print_error(ERR_INVALID_OPERAND_TYPE, line_number);
                    is_error = TRUE;
                    continue;
                }
                rs = parse_register(op1);
                if (rs == -1) {
                    print_error(ERR_REGISTER_OUT_OF_RANGE, line_number);
                    is_error = TRUE;
                    continue;
                }

                immed = 0;
                rt = 0;
                
                /* Conditional branch processing (bne, beq, blt, bgt) */
                if (info->opcode >= OP_BNE && info->opcode <= OP_BGT) {
                    if (op2[0] != '$') {
                        print_error(ERR_INVALID_OPERAND_TYPE, line_number);
                        is_error = TRUE;
                        continue;
                    }
                    rt = parse_register(op2);
                    if (rt == -1) {
                        print_error(ERR_REGISTER_OUT_OF_RANGE, line_number);
                        is_error = TRUE;
                        continue;
                    }

                    target = get_symbol(symbol_table, op3);
                    if (target) {
                        if (target->type == EXTERNAL) { 
                            print_error(ERR_BRANCH_TO_EXTERNAL, line_number);
                            is_error = TRUE;
                            continue;
                        }

                        /* Calculate relative offset */
                        immed = target->address - ic; 
                        
                        if (immed < MIN_16BIT_SIGNED || immed > MAX_16BIT_SIGNED) {
                            print_error(ERR_BRANCH_OFFSET_OUT_OF_RANGE, line_number);
                            is_error = TRUE;
                            continue;
                        }
                    } else {
                        print_error(ERR_UNKNOWN_LABEL, line_number);
                        is_error = TRUE;
                        continue;
                    }
                } else {
                    /* Regular arithmetic/load/store I-Type instructions */
                    immed = parse_number(op2);
                    
                    if (immed < MIN_16BIT_SIGNED || immed > MAX_16BIT_SIGNED) {
                        print_error(ERR_BRANCH_OFFSET_OUT_OF_RANGE, line_number); 
                        is_error = TRUE;
                        continue;
                    }
                    
                    if (op3[0] != '$') {
                        print_error(ERR_INVALID_OPERAND_TYPE, line_number);
                        is_error = TRUE;
                        continue;
                    }
                    rt = parse_register(op3);
                    if (rt == -1) {
                        print_error(ERR_REGISTER_OUT_OF_RANGE, line_number);
                        is_error = TRUE;
                        continue;
                    }
                }

                machine_word = encode_I_type(info->opcode, rs, rt, immed);

            /* -------------- J-TYPE INSTRUCTIONS -------------- */
            } else if (info->type == J_TYPE) {
                reg_bit = 0;
                address = 0;

                if (info->opcode != OP_HLT) {
                    if (!check_comma_syntax(ptr, line_number)) {
                        is_error = TRUE;
                        continue;
                    }

                    ptr = extract_operand(op1, ptr);
                    
                    if (op1[0] == '\0') {
                        print_error(ERR_INVALID_OPERAND_COUNT, line_number);
                        is_error = TRUE;
                        continue;
                    }
                }
                
                if (!check_extraneous_text(ptr, line_number)) {
                    is_error = TRUE;
                    continue;
                }

                if (info->opcode != OP_HLT) {
                    if (op1[0] == '$') 
                    {
                        if (info->opcode == OP_LA || info->opcode == OP_CALL) {
                            print_error(ERR_INVALID_J_JUMP, line_number);
                            is_error = TRUE;
                            continue;
                        } else {
                            reg_bit = 1;
                            address = parse_register(op1);
                            
                            if (address == -1) {
                                print_error(ERR_REGISTER_OUT_OF_RANGE, line_number);
                                is_error = TRUE;
                                continue;
                            }
                        }
                    } 
                    else 
                    {
                        reg_bit = 0;
                        target = get_symbol(symbol_table, op1);
                        if (target) 
                        {
                            address = target->address;
                        } 
                        else 
                        {
                            print_error(ERR_UNKNOWN_LABEL, line_number);
                            is_error = TRUE;
                            continue;
                        }
                    }
                }

                machine_word = encode_J_type(info->opcode, reg_bit, address);
            }
            
            /* Append the encoded word to the instruction linked list */
            add_machine_word(&code_image, ic, machine_word);
            
            /* If the target operand was an external label, mark it in the node */
            if (target != NULL && target->type == EXTERNAL) {
                last_word = code_image;
                
                while (last_word->next != NULL) {
                    last_word = last_word->next;
                }
                
                last_word->is_external = 1;
                strcpy(last_word->external_name, target->name);
            }
            
            ic += INSTRUCTION_SIZE; /* Increment Instruction Counter by 4 bytes */
        }
    }

    /* Clean up file resources */
    fclose(file);
    free(am_filename);
    
    /* If the second pass finished without errors, proceed to generate output files */
    if (!is_error) {
        printf("Second pass completed successfully!\n");
        create_ob_file(filename, code_image, data_image, icf, dcf);
        create_ent_file(filename, symbol_table);
        create_ext_file(filename, symbol_table, code_image);
    }

    /* Free the linked lists representing memory images */
    free_machine_words(code_image);
    free_data_bytes(data_image);
    
    return !is_error;
}
