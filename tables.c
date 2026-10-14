#include "tables.h"

/* ========================================= */
/* Macro Table Management                    */
/* ========================================= */

/* Adds a new macro to the end of the linked list */
boolean add_macro(MacroNode **head, const char *name, const char *content) {
    MacroNode *new_node;
    MacroNode *current;

    /* Allocate memory for the new node */
    new_node = (MacroNode *)malloc(sizeof(MacroNode));
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation failed for macro node.\n");
        return FALSE;
    }

    /* Copy the macro name (safely) */
    strncpy(new_node->name, name, MAX_LABEL_LEN);
    new_node->name[MAX_LABEL_LEN] = '\0'; /* Ensure null-termination */

    /* Allocate memory for the content string and copy it */
    new_node->content = (char *)malloc(strlen(content) + 1);
    if (!new_node->content) {
        fprintf(stderr, "Error: Memory allocation failed for macro content.\n");
        free(new_node);
        return FALSE;
    }
    strcpy(new_node->content, content);

    new_node->next = NULL;

    /* If the list is empty, make the new node the head */
    if (*head == NULL) {
        *head = new_node;
    } else {
        /* Otherwise, traverse to the last node and append the new node */
        current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    return TRUE;
}

/* Searches for a macro by name in the linked list */
MacroNode *find_macro(MacroNode *head, const char *name) {
    MacroNode *current = head;

    /* Traverse the list until we find the name or reach the end */
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current; /* Macro found! */
        }
        current = current->next;
    }

    return NULL; /* Macro not found */
}

/* Frees the entire macro table to prevent memory leaks */
void free_macro_table(MacroNode *head) {
    MacroNode *temp;

    while (head != NULL) {
        temp = head;
        head = head->next;
        /* Must free the inner dynamically allocated content first */
        free(temp->content);
        /* Then free the node itself */
        free(temp);
    }
}
    
/* ========================================= */
/* Symbol Table Management                   */
/* ========================================= */

/* Searches for a symbol by name in the linked list */
SymbolNode *get_symbol(SymbolNode *head, const char *name) {
    SymbolNode *current = head;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Adds a new symbol to the end of the symbol linked list */
boolean add_symbol(SymbolNode **head, const char *name, int address, SymbolType type) {
    SymbolNode *new_node;
    SymbolNode *current;
    SymbolNode *existing_symbol;

    /* Check if symbol already exists to prevent duplicates using the unified get_symbol */
    existing_symbol = get_symbol(*head, name);
    if (existing_symbol != NULL) {
        /* Allow duplicate ONLY if both are EXTERNAL definitions */
        if (type == EXTERNAL && existing_symbol->type == EXTERNAL) {
            return TRUE; /* Just ignore the duplicate and continue */
        }
        
        /* Any other collision (e.g., local and extern, or double local) is an error.
           We return FALSE and let the caller print the error with the line number. */
        return FALSE;
    }

    new_node = (SymbolNode *)malloc(sizeof(SymbolNode));
    if (!new_node) {
        printf("Error: Memory allocation failed for symbol node.\n");
        return FALSE; /* Here a direct print is okay because it's a fatal system error */
    }

    strncpy(new_node->name, name, MAX_LABEL_LEN);
    new_node->name[MAX_LABEL_LEN] = '\0';
    new_node->address = address;
    new_node->type = type;
    new_node->is_entry = 0;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    return TRUE;
}

/* Frees the entire symbol table */
void free_symbol_table(SymbolNode *head) {
    SymbolNode *temp;

    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp); /* Unlike macros, there is no inner dynamically allocated string to free */
    }
}


/* ========================================= */
/* Instruction Dictionary (Lookup Table)     */
/* ========================================= */

/* Static array holding the hardcoded information for all 27 RISC instructions */
static const InstructionInfo instructions_table[] = {
    {"add",  R_TYPE, OP_ALU,  F_ADD_MOVE},
    {"sub",  R_TYPE, OP_ALU,  F_SUB_MVHI},
    {"and",  R_TYPE, OP_ALU,  F_AND_MVLO},
    {"or",   R_TYPE, OP_ALU,  F_OR},
    {"nor",  R_TYPE, OP_ALU,  F_NOR},
    {"move", R_TYPE, OP_MOVE, F_ADD_MOVE},
    {"mvhi", R_TYPE, OP_MOVE, F_SUB_MVHI},
    {"mvlo", R_TYPE, OP_MOVE, F_AND_MVLO},
    
    {"addi", I_TYPE, OP_ADDI, F_NONE},
    {"subi", I_TYPE, OP_SUBI, F_NONE},
    {"andi", I_TYPE, OP_ANDI, F_NONE},
    {"ori",  I_TYPE, OP_ORI,  F_NONE},
    {"nori", I_TYPE, OP_NORI, F_NONE},
    {"bne",  I_TYPE, OP_BNE,  F_NONE},
    {"beq",  I_TYPE, OP_BEQ,  F_NONE},
    {"blt",  I_TYPE, OP_BLT,  F_NONE},
    {"bgt",  I_TYPE, OP_BGT,  F_NONE},
    {"lb",   I_TYPE, OP_LB,   F_NONE},
    {"sb",   I_TYPE, OP_SB,   F_NONE},
    {"lw",   I_TYPE, OP_LW,   F_NONE},
    {"sw",   I_TYPE, OP_SW,   F_NONE},
    {"lh",   I_TYPE, OP_LH,   F_NONE},
    {"sh",   I_TYPE, OP_SH,   F_NONE},
    
    {"jmp",  J_TYPE, OP_JMP,  F_NONE},
    {"la",   J_TYPE, OP_LA,   F_NONE},
    {"call", J_TYPE, OP_CALL, F_NONE},
    {"hlt",  J_TYPE, OP_HLT,  F_NONE}
};

/* Calculates the number of instructions in the table automatically */
#define NUM_INSTRUCTIONS (sizeof(instructions_table) / sizeof(instructions_table[0]))

/* 
 * Searches for an instruction by its string name.
 * Returns a pointer to the InstructionInfo struct if found, NULL otherwise.
 */
InstructionInfo *get_instruction_info(const char *name) {
    int i;
    for (i = 0; i < NUM_INSTRUCTIONS; i++) {
        if (strcmp(instructions_table[i].name, name) == 0) {
            return (InstructionInfo *)&instructions_table[i];
        }
    }
    return NULL; /* Instruction not found (invalid opcode) */
}

/* ========================================= */
/* Label Validation & Reserved Words         */
/* ========================================= */

/* Checks if a given string is a reserved word (directive or instruction) */
boolean is_reserved_word(const char *name) {
    /* 1. Check if the word is an instruction opcode */
    if (get_instruction_info(name) != NULL) {
        return TRUE;
    }

    /* 2. Check if the word is a system directive */
    if (strcmp(name, ".db") == 0 || strcmp(name, ".dh") == 0 ||
        strcmp(name, ".dw") == 0 || strcmp(name, ".asciz") == 0 ||
        strcmp(name, ".entry") == 0 || strcmp(name, ".extern") == 0 ||
        strcmp(name, "mcro") == 0 || strcmp(name, "endmcro") == 0) {
        return TRUE;
    }

    /* Not a reserved word */
    return FALSE;
}

/* Validates that a label name complies with all structural and reserved-word rules */
boolean is_valid_label_name(const char *name) {
    int i = 0;
    
    /* Check if the label is empty */
    if (name == NULL || name[0] == '\0') {
        return FALSE;
    }

    /* Rule 1: Must start with an alphabetic letter (a-z, A-Z) */
    if (!isalpha(name[0])) {
        return FALSE;
    }

    /* Rule 2: Length must not exceed MAX_LABEL_LEN (31 characters) */
    if (strlen(name) > MAX_LABEL_LEN) {
        return FALSE;
    }

    /* Rule 3: Must contain only alphanumeric characters (letters and digits) */
    while (name[i] != '\0') {
        if (!isalnum(name[i])) {
            return FALSE;
        }
        i++;
    }

    /* Rule 4: Cannot be a reserved word (instructions or directives) */
    if (is_reserved_word(name)) {
        return FALSE;
    }

    return TRUE;
}
