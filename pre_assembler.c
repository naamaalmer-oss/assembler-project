/**
 * pre_assembler.c
 * 
 * Handles the first phase of the assembly process: Macro Expansion.
 * Scans the source .as file for macro definitions (using 'mcro' and 'mcroend'),
 * stores them in a linked list, and expands them wherever they are called.
 * Validates macro names against reserved words and checks for syntax errors 
 * in macro declarations. Generates a clean .am file for the next phases.
 */
#include "assembler.h"
#include "pre_assembler.h"
#include "parser.h"
#include "tables.h"
#include "errors.h"

/**
 * The main function of the pre-assembler phase.
 * It reads the .as file, expands macros, and creates a .am file.
 * 
 * @param input_file A pointer to the open .as source file.
 * @param filename The base name of the file (without extension).
 * @return TRUE if macro expansion succeeded without errors, FALSE otherwise.
 */
boolean expand_macros(FILE *input_file, char *filename) {
    char line[MAX_LINE_LEN];
    char word[MAX_LABEL_LEN + 1];
    char *ptr;    
    char *am_filename;
    FILE *output_file;
    boolean is_error = FALSE;
    int line_number = 0; /* Added line counter for accurate error reporting */
    int c;
    int i;
    
    /* Variables for macro handling state */
    boolean inside_macro = FALSE;
    char current_macro_name[MAX_LABEL_LEN + 1];
    char macro_buffer[MAX_MACRO_LEN]; /* A buffer to store macro content temporarily */
    
    /* The head of our macro linked list */
    MacroNode *macro_table = NULL;
    MacroNode *found_macro = NULL;
    
    /* 1. Create the .am filename and open it for writing */
    am_filename = malloc(strlen(filename) + EXT_LEN);
    if (!am_filename) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return FALSE;
    }
    strcpy(am_filename, filename);
    strcat(am_filename, ".am");
    
    output_file = fopen(am_filename, "w");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot create output file %s\n", am_filename);
        free(am_filename);
        return FALSE;
    }

    /* 2. Read the file line by line */
    while (fgets(line, MAX_LINE_LEN, input_file) != NULL) {
        line_number++; /* Increment line number for error tracking */
      
        /* Check for line length exceeding the allowed limit (80 chars) */
        if (strchr(line, '\n') == NULL && !feof(input_file)) {
            print_error(ERR_LINE_TOO_LONG, line_number);
            is_error = TRUE; /* Mark error so the program won't proceed to Pass 1 */
            
            /* Flush the remaining characters in the buffer to prevent spillover */
            while ((c = fgetc(input_file)) != '\n' && c != EOF);
            
            continue; /* Skip processing this invalid line */
        }
        
        /* Extract the first word from the line to check what we are dealing with */
        ptr = extract_word(word, line);
        
        /* If the line is empty or just a comment, write it as is and continue */
        if (word[0] == '\0' || word[0] == ';') {
            if (!inside_macro) {
                fputs(line, output_file);
            } else {
                strcat(macro_buffer, line);
            }
            continue;
        }
        
        if (inside_macro) {
            /* Check if we reached the end of the macro definition */
            if (strcmp(word, "endmcro") == 0 || strcmp(word, "mcroend") == 0) {
                inside_macro = FALSE;
                
                /* Check for extraneous text after 'endmcro' */
                if (!check_extraneous_text(ptr, line_number)) {
                    is_error = TRUE;
                }
                
                /* Save the macro to our linked list */
                if (!add_macro(&macro_table, current_macro_name, macro_buffer)) {
                    is_error = TRUE;
                }
            } else {
                /* We are still inside a macro, append the whole line to the buffer */
                strcat(macro_buffer, line);
            }
        } else {
            /* Check if the first word is "mcro" (start of a new macro) */
            if (strcmp(word, "mcro") == 0) {
                inside_macro = TRUE;
                
                /* Get the macro name and advance ptr */
                ptr = extract_word(current_macro_name, ptr); 
                
                /* Check if a macro name was actually provided */
                if (current_macro_name[0] == '\0') {
                    print_error(ERR_MISSING_OPERANDS, line_number);
                    is_error = TRUE;
                } else {
                    /* 1. Validate basic macro name format: must start with a letter and contain only alphanumeric chars */
                    if (!isalpha(current_macro_name[0])) {
                        print_error(ERR_INVALID_LABEL_NAME, line_number);
                        is_error = TRUE;
                    } else {
                        for (i = 0; current_macro_name[i] != '\0'; i++) {
                            if (!isalnum(current_macro_name[i])) {
                                print_error(ERR_INVALID_LABEL_NAME, line_number);
                                is_error = TRUE;
                                break;
                            }
                        }
                    }

                    /* 2. Check if the macro name is a reserved word (instruction, register, or directive) */
                    if (is_reserved_word(current_macro_name)) {
                        print_error(ERR_MACRO_RESERVED_WORD, line_number);
                        is_error = TRUE;
                    }
                    
                    /* 3. Check for extraneous text after the macro name */
                    if (!check_extraneous_text(ptr, line_number)) {
                        is_error = TRUE;
                    }
                    
                    /* 4. Check if a macro with this name already exists in the table */
                    if (find_macro(macro_table, current_macro_name) != NULL) {
                        print_error(ERR_LABEL_ALREADY_DEFINED, line_number);
                        is_error = TRUE;
                    }
                }
                
                macro_buffer[0] = '\0'; /* Clear the buffer for the new macro */
                
            } else {
                /* הגדרת משתנים בתחילת הבלוק כדי לעמוד בתקן C89 */
                char second_word[MAX_LABEL_LEN + 1];
                char *next_ptr;
                int word_len;

                /* 1. בדיקה: האם המילה הראשונה בשורה היא קריאה למאקרו? */
                found_macro = find_macro(macro_table, word);
                if (found_macro != NULL) {
                    /* זהו מאקרו! נדפיס את התוכן שלו במקום את השורה */
                    fputs(found_macro->content, output_file);
                    
                    /* נבדוק שאין טקסט מיותר אחרי הקריאה למאקרו */
                    if (!check_extraneous_text(ptr, line_number)) {
                        is_error = TRUE;
                    }
                } else {
                    /* 2. בדיקה: האם המילה הראשונה היא תווית, והמילה השנייה היא מאקרו? */
                    word_len = strlen(word);
                    if (word_len > 0 && word[word_len - 1] == ':') {
                        next_ptr = extract_word(second_word, ptr);
                        found_macro = find_macro(macro_table, second_word);
                        
                        if (found_macro != NULL) {
                            /* מצאנו תווית ואחריה קריאה למאקרו! */
                            /* נדפיס את התווית, נוסיף רווח, ואז נדפיס את תוכן המאקרו */
                            fprintf(output_file, "%s ", word);
                            fputs(found_macro->content, output_file);
                            
                            /* נבדוק שאין טקסט מיותר אחרי הקריאה */
                            if (!check_extraneous_text(next_ptr, line_number)) {
                                is_error = TRUE;
                            }
                            continue; /* סיימנו לטפל בשורה, עוברים לבאה */
                        }
                    }
                    
                    /* 3. שורת קוד רגילה לחלוטין - פשוט מעתיקים אותה לקובץ ה-.am */
                    fputs(line, output_file);
                }
            }
        }
    }

    /* 3. Clean up */
    free_macro_table(macro_table); /* We don't need the macros after this phase */
    fclose(output_file);
    free(am_filename);
    
    return !is_error;
}
