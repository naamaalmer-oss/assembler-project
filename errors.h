#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    /* 1. Syntax, Commas and Spaces */
    ERR_INVALID_COMMA,          /* Covers illegal commas, multiple commas, leading/trailing commas */
    ERR_MISSING_COMMA,
    ERR_EXTRANEOUS_TEXT,
    
    /* 2. Labels */
    ERR_LABEL_ALREADY_DEFINED,  /* Replaced ERR_DUPLICATE_LABEL */
    ERR_INVALID_LABEL_NAME,     /* Replaced ERR_INVALID_LABEL_SYNTAX */
    ERR_SPACE_BEFORE_COLON,
    ERR_UNKNOWN_LABEL,
    ERR_ENTRY_EXTERN_CONFLICT,  /* Added: conflict between entry and extern */
    
    /* 3. Instructions and Operands */
    ERR_UNKNOWN_OPCODE,
    ERR_MISSING_OPERANDS,       /* Added: missing operand */
    ERR_INVALID_OPERAND_COUNT,
    ERR_INVALID_OPERAND_TYPE,
    ERR_REGISTER_OUT_OF_RANGE,
    ERR_INVALID_J_JUMP,
    ERR_BRANCH_TO_EXTERNAL,
    ERR_BRANCH_OFFSET_OUT_OF_RANGE,
    
    /* 4. Directives and Structure */
    ERR_LINE_TOO_LONG,
    ERR_DATA_OUT_OF_RANGE,
    ERR_INVALID_STRING,
    
    /* 5. Macros (Pre-Assembler) */
    ERR_MACRO_RESERVED_WORD,
    ERR_MACRO_LABEL_CONFLICT,
    ERR_MACRO_EXTRANEOUS_TEXT
} ErrorCode;

void print_error(ErrorCode code, int line_number);

#endif /* ERRORS_H */
