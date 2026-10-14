#ifndef PARSER_H
#define PARSER_H

/* Skips leading spaces and tabs in a string */
char *skip_whitespaces(char *str);

/* Extracts the first word from a given string into the 'word' buffer */
char *extract_word(char *word, char *str);

/* Calculates the number of memory words required for a data directive (.data or .string) */
int calculate_data_size(char *operands, int element_size, int line_number, boolean *is_error);

/* Calculates the number of memory words required for a code instruction */
int calculate_code_size(char *instruction_line);

/* Extracts register number from operand string (e.g., "$3" -> 3). Returns -1 if invalid. */
int parse_register(const char *operand);

/* Converts immediate operand string to integer. */
int parse_number(const char *operand);

/* Validates the commas in the operands string */
int validate_commas(char *operands, int line_number);

/* Extracts the next operand separated by a comma. Returns pointer to the rest of the string. */
char *extract_operand(char *destination, char *source);

/* Checks if there is any extraneous text left at the end of the line */
int check_extraneous_text(char *str, int line_number);

/* Validates a label definition: checks length restrictions, starting character, alphanumeric composition, and ensures the name is not a reserved keyword.*/
boolean validate_label_definition(char *raw_word, char *next_ptr, int line_number);

/* Checks for syntax errors related to commas in an instruction's operands string. */
boolean check_comma_syntax(const char *str, int line_number);

#endif /* PARSER_H */
