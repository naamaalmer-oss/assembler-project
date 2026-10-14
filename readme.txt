========================================================================
Project: Two-Pass Assembler (Maman 14 - System Programming)
========================================================================

1. PROJECT OVERVIEW
-------------------
This project implements a custom two-pass assembler for a simulated 32-bit architecture. 
The assembler receives assembly source code files (.as), processes macros (Pre-Assembler), 
analyzes the syntax, manages symbol and data tables (First Pass), and encodes the 
instructions into hexadecimal machine code (Second Pass). 
Finally, it produces the required output files: Object file (.ob), Entries file (.ent), 
and Externals file (.ext).

2. PHASE-BY-PHASE ERROR HANDLING (THE ASSEMBLY PIPELINE)
--------------------------------------------------------
To ensure robust error detection without cascading failures, the assembler analyzes 
the source code in three distinct phases. Each phase is responsible for detecting 
specific types of errors based on the information available to it.

[ PHASE A: The Pre-Assembler ]
The Pre-Assembler is responsible for macro expansion and basic file integrity[cite: 1]. 
It processes the source file blindly regarding assembly logic, focusing only on macros and line limits.
Errors caught exclusively in this phase:
- Line Length Exceeded: Lines exceeding the maximum allowed length of 80 characters[cite: 1].
- Invalid Macro Names: Macros named using reserved keywords (instructions, directives, or registers)[cite: 1].
- Invalid Macro Syntax: Extraneous text after a 'mcro' definition or 'mcroend' closure[cite: 1].
* Note: If errors are found here, the assembler aborts processing for this file, and the First Pass is never executed[cite: 1].

[ PHASE B: The First Pass ]
The First Pass analyzes the syntax of individual commands, builds the Symbol Table, 
and calculates the Data Counter (DC) and Instruction Counter (IC)[cite: 1].
Errors caught exclusively in this phase:
- Unknown Instructions: Unrecognized opcodes or directives (e.g., typos in instruction names)[cite: 1].
- Duplicate Labels: A label defined more than once in the same file[cite: 1].
- Invalid Operands & Syntax: Missing commas, multiple consecutive commas, or extraneous text after a valid command[cite: 1].
- Register Out of Bounds: Usage of non-existent registers (valid range is $0 to $31).
- Data Out of Range: Values in data directives (.db, .dh, .dw) that exceed their permitted bit-width limits (e.g., 8-bit signed overflow)[cite: 1].

[ PHASE C: The Second Pass ]
The Second Pass is responsible for finalizing the machine code by resolving label addresses 
using the fully populated Symbol Table[cite: 1]. 
Errors caught exclusively in this phase:
- Undefined Labels: A label is used as an operand (e.g., in a jump or branch) but was never defined in the file or declared as .extern[cite: 1].
- Illegal Branch Targets: Conditional branch instructions (bne, beq, blt, bgt) attempting to target an external (.extern) label, which is strictly forbidden by the architecture[cite: 1].

3. KEY ASSUMPTIONS & DESIGN DECISIONS
-------------------------------------
* Dynamic Memory Allocation (Linked Lists): Although static arrays could be used 
  for the data and instruction images, I chose to implement them using 
  dynamically allocated Linked Lists. This decision was made to ensure a highly efficient 
  data structure that optimizes memory usage, allocating exactly what is needed.
* Meaningless Labels: If a label is defined at the beginning of a line containing a 
  .entry or .extern directive, the assembler validates the label's name for syntactic 
  correctness, but deliberately ignores it and does NOT add it to the Symbol Table[cite: 1].
* Phase Isolation on Errors: As described in Section 2, phase isolation prevents 
  "cascading phantom errors" that would occur from parsing a corrupted file in subsequent passes.
* Strict Extraneous Text Checking: The parser rigorously checks for garbage text at the end 
  of lines. Valid trailing whitespace/tabs are ignored, but any extraneous characters 
  after the command is successfully parsed will result in a syntax error[cite: 1].

4. MAIN MODULES & FUNCTIONS
---------------------------
[ assembler.c ]
- main(): The entry point. Iterates over the provided command-line arguments (filenames) 
  and orchestrates the assembly process for each file.
- process_file(): Handles the full pipeline (Pre-Assembler -> First Pass -> Second Pass) 
  and creates output files if no errors were found.

[ pre_assembler.c ]
- execute_pre_assembler(): Reads the original .as file, isolates macro definitions, stores 
  them, and expands them into a clean .am file.

[ first_pass.c ]
- execute_first_pass(): Scans the .am file, validates initial syntax, counts the Instruction 
  Counter (IC) and Data Counter (DC), identifies data/string directives, and populates the 
  Symbol Table.

[ second_pass.c ]
- execute_second_pass(): Performs the second read. Resolves unknown label addresses, marks 
  .entry symbols, and encodes the instructions into 32-bit machine words.

[ parser.c ]
- extract_word() / extract_operand(): Helper functions to safely parse and extract tokens 
  from a line of text.
- is_valid_label_name(): Validates that a label meets the alphanumeric length and reserved 
  word constraints.
- check_extraneous_text(): Ensures no illegal characters exist after a command is completed.

[ tables.c ]
- add_symbol(): Adds a new symbol (label) to the dynamically allocated Symbol Table.
- get_symbol(): Searches for an existing symbol by its name.

[ output.c ]
- add_machine_word(): Manages the Linked List structure that holds the encoded machine words.
- create_ob_file() / create_ent_file() / create_ext_file(): Iterates over the internal data 
  structures and generates the correctly formatted output files in hexadecimal representation.

[ errors.c ]
- print_error(): A centralized error management function that outputs descriptive error 
  messages alongside the corresponding line number, ensuring the assembler doesn't crash 
  but gracefully flags all user mistakes.

========================================================================
