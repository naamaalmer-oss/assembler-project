#include "assembler.h"
#include "parser.h"
#include "tables.h"
#include "errors.h"

/* ========================================= */
/* Macros to eliminate Magic Numbers         */
/* ========================================= */

/* Skips leading spaces and tabs in a string. Returns a pointer to the first non-whitespace character.*/
char *skip_whitespaces(char *str) {
    if (!str) {
        return NULL;
    }
    
    /* Continue advancing the pointer as long as we see a space or a tab */
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    
    return str;
}

/*
 * Extracts the first word from a given string into the 'word' buffer.
 * A word is defined as a sequence of characters up to a space, tab, or newline.
 * Returns a pointer to the string immediately following the extracted word.
 */
char *extract_word(char *word, char *str) {
    int i = 0;
    char *ptr = skip_whitespaces(str);

    /* If we reached the end of the string or an empty line */
    if (!ptr || *ptr == '\n' || *ptr == '\0') {
        word[0] = '\0';
        return ptr;
    }

    /* Copy characters into 'word' until we hit whitespace or end of string */
    while (*ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\0' && i < MAX_LABEL_LEN) {
        word[i] = *ptr;
        i++;
        ptr++;
    }
    
    /* Null-terminate the extracted word */
    word[i] = '\0';

    return ptr;
}

/* ========================================= */
/* Memory Calculation Functions for First Pass */
/* ========================================= */

/* Calculates data size and validates ranges and comma syntax */
int calculate_data_size(char *operands, int element_size, int line_number, boolean *is_error) {
    int count = 0;
    /* For numeric data (.db, .dh, .dw): Validate numbers and commas */
        int state = EXPECT_NUMBER; 
        char *curr = operands;
        char *endptr;
        long num;
        int i;
        int found_end;
    
    /* If element_size is 0, we treat it as .asciz string */
    if (element_size == 0) {
        i = 0;
        found_end = 0;
        
        /* Skip leading whitespace (including Windows \r) */
        while (operands[i] == ' ' || operands[i] == '\t' || operands[i] == '\r') i++;
        
        /* String must start with a double quote */
        if (operands[i] != '"') {
            print_error(ERR_INVALID_STRING, line_number);
            *is_error = TRUE;
            return 0;
        }
        i++;
        
        /* Count characters until the closing quote */
        while (operands[i] != '\0' && operands[i] != '\n' && operands[i] != '\r') {
            if (operands[i] == '"') {
                found_end = 1;
                count++; /* Add 1 byte for the null terminator ('\0') */
                i++;
                break;
            }
            count++;
            i++;
        }
        
        /* Missing closing quote */
        if (!found_end) {
            print_error(ERR_INVALID_STRING, line_number);
            *is_error = TRUE;
        }
        
        /* Check for extraneous text after string */
        while (operands[i] != '\0' && operands[i] != '\n' && operands[i] != '\r') {
            if (operands[i] != ' ' && operands[i] != '\t') {
                print_error(ERR_EXTRANEOUS_TEXT, line_number);
                *is_error = TRUE;
                break;
            }
            i++;
        }
    } else {       
        while (*curr != '\0' && *curr != '\n') {
            /* Skip spaces and Windows carriage returns */
            if (*curr == ' ' || *curr == '\t' || *curr == '\r') {
                curr++;
                continue;
            }
            
            if (state == EXPECT_NUMBER) {
                /* If we expect a number but see a comma -> syntax error */
                if (*curr == ',') {
                    print_error(ERR_INVALID_COMMA, line_number);
                    *is_error = TRUE;
                    return count * element_size;
                }
                
                /* Parse the number */
                num = strtol(curr, &endptr, DECIMAL_BASE);
                if (curr == endptr) {
                    /* It wasn't a valid number */
                    print_error(ERR_INVALID_OPERAND_TYPE, line_number);
                    *is_error = TRUE;
                    return count * element_size;
                }
                
                /* Check bounds based on directive size */
                if (element_size == SIZE_BYTE) {
                    if (num < MIN_8BIT_SIGNED || num > MAX_8BIT_SIGNED) {
                        print_error(ERR_DATA_OUT_OF_RANGE, line_number);
                        *is_error = TRUE;
                    }
                } else if (element_size == SIZE_HALF) {
                    if (num < MIN_16BIT_SIGNED || num > MAX_16BIT_SIGNED) {
                        print_error(ERR_DATA_OUT_OF_RANGE, line_number);
                        *is_error = TRUE;
                    }
                } else if (element_size == SIZE_WORD) {
                    if (num < MIN_32BIT_SIGNED || num > MAX_32BIT_SIGNED) {
                        print_error(ERR_DATA_OUT_OF_RANGE, line_number);
                        *is_error = TRUE;
                    }
                }
                
                count++;
                curr = endptr; /* Move pointer to after the number */
                state = EXPECT_COMMA; /* Now we expect a comma */
                
            } else {
                /* state == EXPECT_COMMA: Expecting a comma */
                if (*curr == ',') {
                    state = EXPECT_NUMBER; /* Found it, now expect next number */
                    curr++;
                } else {
                    /* Found something else instead of a comma */
                    print_error(ERR_MISSING_COMMA, line_number);
                    *is_error = TRUE;
                    return count * element_size;
                }
            }
        }
        
        /* Check end-of-line situations */
        if (state == EXPECT_NUMBER && count > 0) {
            /* We ended while expecting a number (trailing comma) */
            print_error(ERR_INVALID_COMMA, line_number);
            *is_error = TRUE;
        } else if (count == 0) {
            /* Nothing was provided after the directive */
            print_error(ERR_MISSING_OPERANDS, line_number);
            *is_error = TRUE;
        }
        
        count = count * element_size;
    }
    
    return count;
}

/* Calculates the number of memory bytes required for a code instruction */
int calculate_code_size(char *instruction_line) {
    return INSTRUCTION_SIZE; /* Always 4 bytes in our RISC architecture */
}

/* Extracts register number from operand string and validates range */
int parse_register(const char *operand) {
    int reg_num;
    int i;

    if (operand == NULL || operand[0] != '$') {
        return -1;
    }

    /* Check that all characters following '$' are digits */
    if (operand[1] == '\0') {
        return -1; /* Just "$" is invalid */
    }

    for (i = 1; operand[i] != '\0'; i++) {
        if (!isdigit((unsigned char)operand[i])) {
            return -1; /* Contains non-digit characters like "$3abc" */
        }
    }

    reg_num = atoi(operand + 1);

    /* Ensure the register number is within the valid range */
    if (reg_num < MIN_REGISTER || reg_num > MAX_REGISTER) {
        return -1;
    }

    return reg_num;
}

/* Converts immediate operand string to integer. */
int parse_number(const char *operand) {
    char *endptr;
    long num;

    if (operand == NULL || operand[0] == '\0') {
        return 0;
    }

    /* Use strtol to parse the integer safely */
    num = strtol(operand, &endptr, DECIMAL_BASE);

    /* If endptr points to the beginning, no digits were parsed at all */
    if (operand == endptr) {
        return 0; /* Or handle as invalid number error */
    }

    /* Skip trailing whitespace if any */
    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    /* If there are leftover characters (e.g., "123xyz"), it's an invalid number format */
    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
        return 0; /* Invalid trailing characters */
    }

    return (int)num;
}

/**
 * Validates the syntax of commas in the operands string.
 * It checks for common comma errors such as:
 * - Leading commas at the beginning of the operand list.
 * - Consecutives/multiple commas (e.g., ",,").
 * - Missing commas between operands (e.g., "operand1 operand2").
 * - Trailing commas at the end of the operand list.
 * 
 * State machine logic:
 * - state = 0: Expecting an operand (meaning a comma here is illegal).
 * - state = 1: Expecting a comma or end of string (meaning an operand here without a comma is illegal).
 */
int validate_commas(char *operands, int line_number) {
    int state = 0; 
    int i = 0;

    /* Skip leading whitespace */
    while (operands[i] == ' ' || operands[i] == '\t') {
        i++;
    }
    
    /* Empty operand string is valid */
    if (operands[i] == '\0' || operands[i] == '\n') {
        return 1;
    }

    /* A leading comma at the very beginning is illegal */
    if (operands[i] == ',') {
        print_error(ERR_INVALID_COMMA, line_number);
        return 0;
    }

    /* Iterate through the operands string to validate comma placement */
    while (operands[i] != '\0' && operands[i] != '\n') {
        /* Skip whitespace between tokens */
        if (operands[i] == ' ' || operands[i] == '\t') {
            i++;
            continue;
        }

        if (operands[i] == ',') {
            /* If state is 0, we expected an operand, so finding a comma means consecutive or unexpected commas */
            if (state == 0) {
                print_error(ERR_INVALID_COMMA, line_number);
                return 0;
            }
            state = 0; /* After a comma, we now expect the next operand */
        } else {
            /* If state is 1, we expected a comma before this new operand, meaning a comma is missing */
            if (state == 1) {
                print_error(ERR_MISSING_COMMA, line_number);
                return 0;
            }
            
            /* Consume the entire current operand token */
            while (operands[i] != ' ' && operands[i] != '\t' && operands[i] != ',' && operands[i] != '\0' && operands[i] != '\n') {
                i++;
            }
            state = 1; /* After an operand, we now expect a comma or the end of the line */
            continue; 
        }
        i++;
    }

    /* If state ends at 0, the string ended with a trailing comma, which is illegal */
    if (state == 0) {
        print_error(ERR_INVALID_COMMA, line_number);
        return 0;
    }

    return 1;
}

/**
 * Extracts a single operand token from the source string into the destination buffer.
 * It skips any leading whitespace, copies characters until a delimiter (space, tab, comma, 
 * newline, or null terminator) is reached, and then advances the source pointer past 
 * the separating comma and any following whitespace, preparing it for the next extraction.
 */
char *extract_operand(char *destination, char *source) {
    int i = 0;
    
    /* Skip any leading whitespace */
    while (*source == ' ' || *source == '\t') source++;
    
    /* If we reached the end of the string, return NULL */
    if (*source == '\0') return NULL;
    
    /* Copy characters belonging to the current operand token */
    while (*source != ',' && *source != ' ' && *source != '\t' && *source != '\0' && *source != '\n') {
        destination[i++] = *source++;
    }
    destination[i] = '\0';
    
    /* Skip whitespace following the operand */
    while (*source == ' ' || *source == '\t') source++;
    
    /* If a comma separates the operands, consume it */
    if (*source == ',') source++;
    
    return source;
}

/* 
 * Checks if there is any non-whitespace text remaining in the string.
 * Returns 1 if the remainder is clean, and 0 if extraneous text is found.
 */
int check_extraneous_text(char *str, int line_number) {
    if (str == NULL) {
        return 1;
    }
    
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    
    if (*str == '\0' || *str == '\n' || *str == '\r') {
        return 1;
    }
    
    print_error(ERR_EXTRANEOUS_TEXT, line_number);
    
    return 0;
}

/* 
 * Validates a label definition: checks syntax, reserved words, and colon spacing.
 * Returns TRUE if valid, FALSE if any error is found.
 */
boolean validate_label_definition(char *raw_word, char *next_ptr, int line_number) {
    int i = 0;
    int len = strlen(raw_word);
    char *temp = next_ptr;

    /* 1. Check for space before colon */
    if (temp != NULL) {
        while (*temp == ' ' || *temp == '\t') {
            temp++;
        }
        if (*temp == ':') {
            print_error(ERR_SPACE_BEFORE_COLON, line_number);
            return FALSE;
        }
    }

    /* Remove trailing colon for syntax validation if it's attached */
    if (len > 0 && raw_word[len - 1] == ':') {
        raw_word[len - 1] = '\0';
        len--;
    }

    /* 2. Check maximum length using the defined constant */
    if (len > MAX_LABEL_LEN) {
        print_error(ERR_INVALID_LABEL_NAME, line_number);
        return FALSE;
    }

    /* 3. Check syntax: must start with a letter, and contain only letters and digits */
    if (!isalpha(raw_word[0])) {
        print_error(ERR_INVALID_LABEL_NAME, line_number);
        return FALSE;
    }

    for (i = 0; i < len; i++) {
        if (!isalnum(raw_word[i])) {
            print_error(ERR_INVALID_LABEL_NAME, line_number);
            return FALSE;
        }
    }

    /* 4. Check if the label is a reserved word (instruction name) using get_instruction_info */
    if (get_instruction_info(raw_word) != NULL) {
        print_error(ERR_LABEL_ALREADY_DEFINED, line_number); 
        return FALSE;
    }

    /* Also check if it's a reserved directive name (WITHOUT THE DOT!) */
    if (strcmp(raw_word, "data") == 0 || strcmp(raw_word, "string") == 0 || 
        strcmp(raw_word, "db") == 0 || strcmp(raw_word, "dh") == 0 || 
        strcmp(raw_word, "dw") == 0 || strcmp(raw_word, "asciz") == 0 || 
        strcmp(raw_word, "entry") == 0 || strcmp(raw_word, "extern") == 0) {
        print_error(ERR_INVALID_LABEL_NAME, line_number);
        return FALSE;
    }
        
    return TRUE;
}
    
/**
 * Checks for syntax errors related to commas in an instruction's operands string.
 * It validates comma placement rules such as preventing:
 * - Leading commas at the very beginning of the string.
 * - Consecutive/multiple commas (e.g., "a,,b").
 * - Missing commas between operands (handled implicitly or via structural checks).
 * - Trailing commas at the end of the line.
 */
boolean check_comma_syntax(const char *str, int line_number) {
    int i = 0;
    boolean expect_operand = TRUE;
    boolean found_comma = FALSE;

    /* Skip any leading whitespace at the beginning of the string */
    while (str[i] == ' ' || str[i] == '\t') {
        i++;
    }

    /* A leading comma at the very beginning of the operands list is illegal */
    if (str[i] == ',') {
        print_error(ERR_INVALID_COMMA, line_number);
        return FALSE;
    }

    /* Iterate through the string character by character */
    while (str[i] != '\0' && str[i] != '\n') {
        if (str[i] == ',') {
            /* If we encounter a comma while still expecting an operand, 
               it means we found consecutive commas or an unexpected comma */
            if (expect_operand) {
                print_error(ERR_INVALID_COMMA, line_number);
                return FALSE;
            }
            expect_operand = TRUE; /* After a valid comma, we expect the next operand */
            found_comma = TRUE;    /* Mark that the current token ended with a comma */
        } else if (str[i] != ' ' && str[i] != '\t') {
            /* When encountering a non-whitespace character (part of an operand), 
               we are no longer expecting an operand, and we clear the comma flag */
            expect_operand = FALSE;
            found_comma = FALSE;
        }
        i++;
    }

    /* If the string ends with a comma (found_comma is still TRUE), 
       it is a trailing comma error, which is illegal */
    if (found_comma) {
        print_error(ERR_INVALID_COMMA, line_number);
        return FALSE;
    }

    return TRUE;
}
