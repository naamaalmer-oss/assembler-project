#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* Important constants*/
#define MAX_LABEL_LEN 31  /* Maximum length of a label */
#define START_ADDRESS 100 /* Starting memory address for the program */
#define MAX_LINE_LEN 82   /* Maximum line length (including newline character and string terminator) */
#define MIN_ARGS 2         /* Minimum number of arguments required in command line */
#define EXT_LEN 4          /* Length of file extension string (e.g., ".as") including null terminator */
#define MAX_MACRO_LEN 4096 /* Maximum allowed length for a macro's content */
#define INSTRUCTION_SIZE 4 /* Size in bytes for every instruction in our RISC architecture */
#define MAX_32BIT_SIGNED 2147483647
#define MIN_32BIT_SIGNED (-2147483647 - 1)
#define MIN_16BIT_SIGNED -32768
#define MAX_16BIT_SIGNED 32767
#define MIN_8BIT_SIGNED -128
#define MAX_8BIT_SIGNED 127

/* =========================================================
 * Bitwise Shifts and Masks for Instruction Encoding 
 * ========================================================= */

/* Shift amounts for different fields in the 32-bit instruction */
#define OPCODE_SHIFT    26
#define REG_BIT_SHIFT   25
#define RS_SHIFT        21
#define RT_SHIFT        16
#define RD_SHIFT        11
#define FUNCT_SHIFT     6

/* Masks to ensure values fit within their allocated bits */
#define OPCODE_MASK     0x3F       /* 6 bits mask (for opcode and funct) */
#define REG_MASK        0x1F       /* 5 bits mask (for registers rs, rt, rd) */
#define REG_BIT_MASK    0x1        /* 1 bit mask  (for J-Type register flag) */
#define IMMED_MASK      0xFFFF     /* 16 bits mask (for I-Type immediate) */
#define ADDRESS_MASK    0x1FFFFFF  /* 25 bits mask (for J-Type address) */

/* =========================================================
 * Masks and Shifts for Data Directives Extraction
 * ========================================================= */

#define BYTE_MASK       0xFF    /* Mask to extract exactly 8 bits (1 byte) */
#define SHIFT_8_BITS    8       /* Shift to reach the 2nd byte */
#define SHIFT_16_BITS   16      /* Shift to reach the 3rd byte */
#define SHIFT_24_BITS   24      /* Shift to reach the 4th byte */

/* =========================================================
 * Macros for Output Generation
 * ========================================================= */
#define OB_EXT_LEN      4       /* Length of ".ob" + null terminator */
#define ENT_EXT_LEN     5       /* Length of ".ent" / ".ext" + null terminator */
#define BYTES_PER_LINE  4       /* Number of data bytes to print per line */

#define DECIMAL_BASE    10
#define MIN_REGISTER    0
#define MAX_REGISTER    31

#define SIZE_BYTE       1
#define SIZE_HALF       2
#define SIZE_WORD       4

#define EXPECT_NUMBER   0
#define EXPECT_COMMA    1

/* Boolean definition*/
typedef enum { FALSE = 0, TRUE = 1 } boolean;

/* Symbol properties for the symbol table (code, data, external, or entry) */
typedef enum {
    CODE,
    DATA,
    EXTERNAL,
    ENTRY
} SymbolType;

/* Structure for a node in the Symbol Table (implemented as a linked list) */
typedef struct SymbolNode {
    char name[MAX_LABEL_LEN + 1];
    int address;
    SymbolType type;
    int is_entry;
    struct SymbolNode *next;
} SymbolNode;

/* Structure for a node in the Macro Table (implemented as a linked list) */
typedef struct MacroNode {
    char name[MAX_LABEL_LEN + 1];
    char *content;
    struct MacroNode *next;
} MacroNode;

/* Represents the type of instruction in our RISC architecture */
typedef enum {
    R_TYPE,
    I_TYPE,
    J_TYPE
} InstructionType;

/* Structure to hold the hardcoded information about each machine instruction */
typedef struct {
    char *name;             /* Instruction name (e.g., "add", "jmp") */
    InstructionType type;   /* R_TYPE, I_TYPE, or J_TYPE */
    int opcode;             /* Opcode value */
    int funct;              /* Funct value (0 if not applicable) */
} InstructionInfo;

/* ========================================= */
/* Hardware Opcodes and Functs Definitions   */
/* ========================================= */

/* Opcodes for the RISC architecture */
typedef enum {
    OP_ALU       = 0,  /* add, sub, and, or, nor */
    OP_MOVE      = 1,  /* move, mvhi, mvlo */
    OP_ADDI      = 10, OP_SUBI    = 11, OP_ANDI    = 12, OP_ORI     = 13, OP_NORI    = 14,
    OP_BNE       = 15, OP_BEQ     = 16, OP_BLT     = 17, OP_BGT     = 18,
    OP_LB        = 19, OP_SB      = 20, OP_LW      = 21, OP_SW      = 22, OP_LH      = 23, OP_SH      = 24,
    OP_JMP       = 30, OP_LA      = 31, OP_CALL    = 32,
    OP_HLT       = 63
} Opcode;

/* Funct codes for R-Type instructions */
typedef enum {
    F_NONE       = 0,  /* For instructions without a funct code */
    F_ADD_MOVE   = 1,  /* add, move */
    F_SUB_MVHI   = 2,  /* sub, mvhi */
    F_AND_MVLO   = 3,  /* and, mvlo */
    F_OR         = 4,  /* or */
    F_NOR        = 5   /* nor */
} Funct;

/* Represents a single encoded 32-bit machine word in memory */
typedef struct MachineWord {
    int address;             /* Memory address (e.g., 100, 104...) */
    unsigned long word;      /* 32-bit machine code value */
    int is_external;
    char external_name[MAX_LABEL_LEN + 1];
    struct MachineWord *next;
} MachineWord;

/* Represents a single byte of data in memory */
typedef struct DataByte {
    int address;
    unsigned char byte;
    struct DataByte *next;
} DataByte;

#endif /* ASSEMBLER_H */
