#include <stdio.h>
#include "errors.h"

void print_error(ErrorCode code, int line_number) {
    /* Common prefix for all error messages */
    printf("Error in line %d: ", line_number);
    
    switch (code) {
        /* 1. Syntax, Commas and Spaces */
        case ERR_INVALID_COMMA:
            printf("Invalid comma syntax (e.g., multiple consecutive commas, leading comma, or trailing comma).\n");
            break;
        case ERR_MISSING_COMMA:
            printf("Missing comma between operands.\n");
            break;
        case ERR_EXTRANEOUS_TEXT:
            printf("Extraneous text after the end of a valid command.\n");
            break;

        /* 2. Labels */
        case ERR_LABEL_ALREADY_DEFINED:
            printf("Duplicate label. This label has already been defined.\n");
            break;
        case ERR_INVALID_LABEL_NAME:
            printf("Invalid label name. Must start with a letter, contain only alphanumeric characters (max 31), and cannot be a reserved word.\n");
            break;
        case ERR_SPACE_BEFORE_COLON:
            printf("Spaces are not allowed between the label name and the colon (:).\n");
            break;
        case ERR_UNKNOWN_LABEL:
            printf("Label is used as an operand but is not defined in the symbol table.\n");
            break;
        case ERR_ENTRY_EXTERN_CONFLICT:
            printf("Conflict: Cannot define a symbol as both ENTRY and EXTERN.\n");
            break;

        /* 3. Instructions and Operands */
        case ERR_UNKNOWN_OPCODE:
            printf("Unknown instruction or directive name.\n");
            break;
        case ERR_MISSING_OPERANDS:
            printf("Missing required operand(s) for this instruction or directive.\n");
            break;
        case ERR_INVALID_OPERAND_COUNT:
            printf("Incorrect number of operands for this instruction.\n");
            break;
        case ERR_INVALID_OPERAND_TYPE:
            printf("Invalid operand type for this instruction (e.g., expected register but got label).\n");
            break;
        case ERR_REGISTER_OUT_OF_RANGE:
            printf("Register number is out of range. Must be between $0 and $31.\n");
            break;
        case ERR_INVALID_J_JUMP:
            printf("Instructions 'la' and 'call' cannot take a register as an operand.\n");
            break;
        case ERR_BRANCH_TO_EXTERNAL:
            printf("Conditional branch instructions cannot use an external label as a target.\n");
            break;
        case ERR_BRANCH_OFFSET_OUT_OF_RANGE:
            printf("Branch offset exceeds the 16-bit signed integer limit.\n");
            break;

        /* 4. Directives and Structure */
        case ERR_LINE_TOO_LONG:
            printf("Line exceeds the maximum allowed length of 80 characters.\n");
            break;
        case ERR_DATA_OUT_OF_RANGE:
            printf("Data value is out of bounds for the specified directive size.\n");
            break;
        case ERR_INVALID_STRING:
            printf("Invalid string format for .asciz directive (missing quotes or non-printable ASCII).\n");
            break;

        /* 5. Macros (Pre-Assembler) */
        case ERR_MACRO_RESERVED_WORD:
            printf("Macro name cannot be a reserved instruction or directive name.\n");
            break;
        case ERR_MACRO_LABEL_CONFLICT:
            printf("Macro name conflicts with an existing label definition.\n");
            break;
        case ERR_MACRO_EXTRANEOUS_TEXT:
            printf("Extraneous text found after 'mcro' or 'mcroend' definition.\n");
            break;

        default:
            printf("Unknown syntax error.\n");
            break;
    }
}
