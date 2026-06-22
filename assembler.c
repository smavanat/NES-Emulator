//This file contains an assembler for converting 6502 assembly into bytecode
//This is done using a simple parser to tokenise the given file and check the syntax,
//before looking up opcode values in a lookup table

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cpu.h"

#define num_instr 57

size_t err_line = 0;
size_t err_col = 0;
char err_char = 0;

//Lookup table to show how many parameters an instruction takes
uint8_t nparams[num_instr] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//Lookup table to map instruction and addressing mode to an opcode
//0 is used as a placeholder value for no match, so should always check that the instruction
//is not BRK to determine if it is a true or sentinel value
uint8_t instr_map[num_instr][13] = {
    //  ABS  | ABS_X | ABS_Y | ACC | IMM | IMP | IND | IND_X | IND_Y | REL |  ZP  | ZPX | ZPY

    // Load and store
    { 0xAD, 0xBD, 0xB9,    0, 0xA9,    0,    0, 0xA1, 0xB1,    0, 0xA5, 0xB5,    0 }, // LDA
    { 0xAE,    0, 0xBE,    0, 0xA2,    0,    0,    0,    0,    0, 0xA6,    0, 0xB6 }, // LDX
    { 0xAC, 0xBC,    0,    0, 0xA0,    0,    0,    0,    0,    0, 0xA4, 0xB4,    0 }, // LDY
    { 0x8D, 0x9D, 0x99,    0,    0,    0,    0, 0x81, 0x91,    0, 0x85, 0x95,    0 }, // STA
    { 0x8E,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x86,    0, 0x96 }, // STX
    { 0x8C,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x84, 0x94,    0 }, // STY

    // Transfer
    {    0,    0,    0,    0,    0, 0xAA,    0,    0,    0,    0,    0,    0,    0 }, // TAX
    {    0,    0,    0,    0,    0, 0x8A,    0,    0,    0,    0,    0,    0,    0 }, // TXA
    {    0,    0,    0,    0,    0, 0xA8,    0,    0,    0,    0,    0,    0,    0 }, // TAY
    {    0,    0,    0,    0,    0, 0x98,    0,    0,    0,    0,    0,    0,    0 }, // TYA

    // Arithmetic
    { 0x6D, 0x7D, 0x79,    0, 0x69,    0,    0, 0x61, 0x71,    0, 0x65, 0x75,    0 }, // ADC
    { 0xED, 0xFD, 0xF9,    0, 0xE9,    0,    0, 0xE1, 0xF1,    0, 0xE5, 0xF5,    0 }, // SBC
    { 0xEE, 0xFE,    0,    0,    0,    0,    0,    0,    0,    0, 0xE6, 0xF6,    0 }, // INC
    { 0xCE, 0xDE,    0,    0,    0,    0,    0,    0,    0,    0, 0xC6, 0xD6,    0 }, // DEC
    {    0,    0,    0,    0,    0, 0xE8,    0,    0,    0,    0,    0,    0,    0 }, // INX
    {    0,    0,    0,    0,    0, 0xCA,    0,    0,    0,    0,    0,    0,    0 }, // DEX
    {    0,    0,    0,    0,    0, 0xC8,    0,    0,    0,    0,    0,    0,    0 }, // INY
    {    0,    0,    0,    0,    0, 0x88,    0,    0,    0,    0,    0,    0,    0 }, // DEY

    // Shift
    { 0x0E, 0x1E,    0, 0x0A,    0,    0,    0,    0,    0,    0, 0x06, 0x16,    0 }, // ASL
    { 0x4E, 0x5E,    0, 0x4A,    0,    0,    0,    0,    0,    0, 0x46, 0x56,    0 }, // LSR
    { 0x2E, 0x3E,    0, 0x2A,    0,    0,    0,    0,    0,    0, 0x26, 0x36,    0 }, // ROL
    { 0x6E, 0x7E,    0, 0x6A,    0,    0,    0,    0,    0,    0, 0x66, 0x76,    0 }, // ROR

    // Bitwise
    { 0x2D, 0x3D, 0x39,    0, 0x29,    0,    0, 0x21, 0x31,    0, 0x25, 0x35,    0 }, // AND
    { 0x0D, 0x1D, 0x19,    0, 0x09,    0,    0, 0x01, 0x11,    0, 0x05, 0x15,    0 }, // ORA
    { 0x4D, 0x5D, 0x59,    0, 0x49,    0,    0, 0x41, 0x51,    0, 0x45, 0x55,    0 }, // XOR/EOR
    { 0x2C,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x24,    0,    0 }, // BIT

    // Compare
    { 0xCD, 0xDD, 0xD9,    0, 0xC9,    0,    0, 0xC1, 0xD1,    0, 0xC5, 0xD5,    0 }, // CMP
    { 0xEC,    0,    0,    0, 0xE0,    0,    0,    0,    0,    0, 0xE4,    0,    0 }, // CPX
    { 0xCC,    0,    0,    0, 0xC0,    0,    0,    0,    0,    0, 0xC4,    0,    0 }, // CPY

    // Branch
    {    0,    0,    0,    0,    0, 0x90,    0,    0,    0, 0x90,    0,    0,    0 }, // BCC
    {    0,    0,    0,    0,    0, 0xB0,    0,    0,    0, 0xB0,    0,    0,    0 }, // BCS
    {    0,    0,    0,    0,    0, 0xF0,    0,    0,    0, 0xF0,    0,    0,    0 }, // BEQ
    {    0,    0,    0,    0,    0, 0xD0,    0,    0,    0, 0xD0,    0,    0,    0 }, // BNE
    {    0,    0,    0,    0,    0, 0x10,    0,    0,    0, 0x10,    0,    0,    0 }, // BPL
    {    0,    0,    0,    0,    0, 0x30,    0,    0,    0, 0x30,    0,    0,    0 }, // BMI
    {    0,    0,    0,    0,    0, 0x50,    0,    0,    0, 0x50,    0,    0,    0 }, // BVC
    {    0,    0,    0,    0,    0, 0x70,    0,    0,    0, 0x70,    0,    0,    0 }, // BVS

    // Jump
    { 0x4C,    0,    0,    0,    0,    0, 0x6C,    0,    0,    0,    0,    0,    0 }, // JMP
    { 0x20,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // JSR
    {    0,    0,    0,    0,    0, 0x60,    0,    0,    0,    0,    0,    0,    0 }, // RTS
    {    0,    0,    0,    0,    0, 0x00,    0,    0,    0,    0,    0,    0,    0 }, // BRK
    {    0,    0,    0,    0,    0, 0x40,    0,    0,    0,    0,    0,    0,    0 }, // RTI

    // Stack
    {    0,    0,    0,    0,    0, 0x48,    0,    0,    0,    0,    0,    0,    0 }, // PHA
    {    0,    0,    0,    0,    0, 0x68,    0,    0,    0,    0,    0,    0,    0 }, // PLA
    {    0,    0,    0,    0,    0, 0x08,    0,    0,    0,    0,    0,    0,    0 }, // PHP
    {    0,    0,    0,    0,    0, 0x28,    0,    0,    0,    0,    0,    0,    0 }, // PLP
    {    0,    0,    0,    0,    0, 0x9A,    0,    0,    0,    0,    0,    0,    0 }, // TXS
    {    0,    0,    0,    0,    0, 0xBA,    0,    0,    0,    0,    0,    0,    0 }, // TSX

    // Flags
    {    0,    0,    0,    0,    0, 0x18,    0,    0,    0,    0,    0,    0,    0 }, // CLC
    {    0,    0,    0,    0,    0, 0x38,    0,    0,    0,    0,    0,    0,    0 }, // SEC
    {    0,    0,    0,    0,    0, 0x58,    0,    0,    0,    0,    0,    0,    0 }, // CLI
    {    0,    0,    0,    0,    0, 0x78,    0,    0,    0,    0,    0,    0,    0 }, // SEI
    {    0,    0,    0,    0,    0, 0xD8,    0,    0,    0,    0,    0,    0,    0 }, // CLD
    {    0,    0,    0,    0,    0, 0xF8,    0,    0,    0,    0,    0,    0,    0 }, // SED
    {    0,    0,    0,    0,    0, 0xB8,    0,    0,    0,    0,    0,    0,    0 }, // CLV

    {    0,    0,    0,    0,    0, 0xEA,    0,    0,    0,    0,    0,    0,    0 }, // NOP
    {    0,    0,    0,    0,    0, 0x12,    0,    0,    0,    0,    0,    0,    0 }, // STP
};

//A simple hash function for 4 byte strings.
//This is sufficient for instructions since all opcodes
//are 3 characters in length, so max 3 bytes
#define HASH_STR(str) ((uint32_t)(unsigned char)(str)[0] << 16) | ((uint32_t)(unsigned char)(str)[1] << 8) | ((uint32_t)(unsigned char)(str)[2])

//This hash function operates on 3 characters instead of 3 byte strings since apparently strings are not compile time constants but chars are.
#define HASH(a, b, c) ((uint32_t)(unsigned char)(a) << 16) | ((uint32_t)(unsigned char)(b) << 8) | ((uint32_t)(unsigned char)(c))

//Lookup table to convert (uppercase) opcode strings to integers
uint8_t lookup_instr(char *instr_str) {
    switch(HASH_STR(instr_str)) {
        case HASH('L','D','A'):
            return LDA;
        case HASH('L','D','X'):
            return LDX;
        case HASH('L','D','Y'):
            return LDY;
        case HASH('S','T','A'):
            return STA;
        case HASH('S','T','X'):
            return STX;
        case HASH('S','T','Y'):
            return STY;
        case HASH('T','A','X'):
            return TAX;
        case HASH('T','X','A'):
            return TXA;
        case HASH('T','A','Y'):
            return TAY;
        case HASH('T','Y','A'):
            return TYA;
        case HASH('A','D','C'):
            return ADC;
        case HASH('S','B','C'):
            return SBC;
        case HASH('I','N','C'):
            return INC;
        case HASH('D','E','C'):
            return DEC;
        case HASH('I','N','X'):
            return INX;
        case HASH('D','E','X'):
            return DEX;
        case HASH('I','N','Y'):
            return INY;
        case HASH('D','E','Y'):
            return DEY;
        case HASH('A','S','L'):
            return ASL;
        case HASH('L','S','R'):
            return LSR;
        case HASH('R','O','L'):
            return ROL;
        case HASH('R','O','R'):
            return ROR;
        case HASH('A','N','D'):
            return AND;
        case HASH('O','R','A'):
            return ORA;
        case HASH('X','O','R'):
            return XOR;
        case HASH('B','I','T'):
            return BIT;
        case HASH('C','M','P'):
            return CMP;
        case HASH('C','P','X'):
            return CPX;
        case HASH('C','P','Y'):
            return CPY;
        case HASH('B','C','C'):
            return BCC;
        case HASH('B','C','S'):
            return BCS;
        case HASH('B','E','Q'):
            return BEQ;
        case HASH('B','N','E'):
            return BNE;
        case HASH('B','P','L'):
            return BPL;
        case HASH('B','M','I'):
            return BMI;
        case HASH('B','V','C'):
            return BVC;
        case HASH('B','V','S'):
            return BVS;
        case HASH('J','M','P'):
            return JMP;
        case HASH('J','S','R'):
            return JSR;
        case HASH('R','T','S'):
            return RTS;
        case HASH('B','R','K'):
            return BRK;
        case HASH('R','T','I'):
            return RTI;
        case HASH('P','H','A'):
            return PHA;
        case HASH('P','L','A'):
            return PLA;
        case HASH('P','H','P'):
            return PHP;
        case HASH('P','L','P'):
            return PLP;
        case HASH('T','X','S'):
            return TXS;
        case HASH('T','S','X'):
            return TSX;
        case HASH('C','L','C'):
            return CLC;
        case HASH('S','E','C'):
            return SEC;
        case HASH('C','L','I'):
            return CLI;
        case HASH('S','E','I'):
            return SEI;
        case HASH('C','L','D'):
            return CLD;
        case HASH('S','E','D'):
            return SED;
        case HASH('C','L','V'):
            return CLV;
        case HASH('N','O','P'):
            return NOP;
        case HASH('S','T','P'):
            return STP;
        default:
            return 0xFF;
    }
}

//Writes text into an output file
void write_text(char *text, size_t len) {
    FILE *fptr = fopen("output.txt", "w");
    fwrite(text, sizeof(char), len, fptr);
    fclose(fptr);
}

//Reads the entirety of a file into the given buffer
int read_to_end(char const *path, char **buf, uint8_t add_null) {
    FILE *fp;
    size_t fsz;
    long offEnd;
    int rc;

    //Open the file
    fp = fopen(path, "rb");
    if(NULL == fp) {
        return -1;
    }

    //Seek to the end of the file
    rc = fseek(fp, 0L, SEEK_END);
    if(0 != rc) {
        return -1;
    }

    //Byte offset to the end of the file size
    if(0 > (offEnd = ftell(fp))) {
        return -1;
    }
    fsz = (size_t)offEnd;

    //Allocate a buffer to hold the whole file
    *buf = malloc(fsz + (int)add_null);
    if(NULL == *buf) {
        return -1;
    }

    //Rewind file pointer to the start of the file:
    rewind(fp);

    //Place the file into a buffer
    if(fsz != fread(*buf, 1, fsz, fp)) {
        free(buf);
        return -1;
    }

    //Close the file
    if(EOF == fclose(fp)) {
        free(*buf);
        return -1;
    }

    //Add null terminator
    if(add_null) {
        buf[fsz] = "\0";
    }

    return fsz;
}

typedef enum {
    assembler_opcode,
    assembler_opcode0,
    assembler_opcode1,
    assembler_binary,
    assembler_hex,
    assembler_imm,
    assembler_nl,
    assembler_dec,
    assembler_string,
    assembler_lparen,
    assembler_rparen,
    assembler_x,
    assembler_y,
    assembler_a,
    assembler_comma,
    assembler_none
} assembler_token_type;

typedef struct {
    char lexeme[32];
    size_t line;
    size_t column;
    size_t len;
    assembler_token_type type;
} assembler_token;

typedef struct {
    assembler_token *tokens;
    size_t len;
    size_t capacity;
    uint8_t open_bracket; //For determining if the current expression is inside parentheses. Do not need anything more complex as there should not be nested parentheses
} lexer;

//Returns a string equivalent for each token type
char *token_type_string(assembler_token_type type) {
    switch (type) {
        case assembler_opcode:
            return "OPCODE";
        case assembler_opcode0:
            return "OPCODE0";
        case assembler_opcode1:
            return "OPCODE1";
        case assembler_string:
            return "STRING";
        case assembler_binary:
            return "BINARY";
        case assembler_hex:
            return "HEX";
        case assembler_dec:
            return "DECIMAL";
        case assembler_imm:
            return "IMM";
        case assembler_nl:
            return "NEWLINE";
        case assembler_lparen:
            return "LPAREN";
        case assembler_rparen:
            return "RPAREN";
        case assembler_x:
            return "X";
        case assembler_y:
            return "Y";
        case assembler_a:
            return "A";
        case assembler_comma:
            return "COMMA";
        default:
            return "NONE";
    }
}

//Prints the value of a token
void print_token(assembler_token *tok) {
    printf("Lexeme: %s Line: %zu, Column: %zu, Size: %zu, Type: %s\n", tok->lexeme, tok->line, tok->column, tok->len, token_type_string(tok->type));
}

//Prints the contents of the lexer
void print_lexer(lexer *l) {
    for(size_t i = 0; i < l->len; i++) {
        printf("%s ", token_type_string(l->tokens[i].type));
    }
    printf("\n");
    for(size_t i = 0; i < l->len; i++) {
        printf("%s ", l->tokens[i].lexeme);
    }
}

//Adds a token to the end of the lexer
void append_token(lexer *l, assembler_token tok) {
    if(l->len >= l->capacity) {
        size_t new_cap = (l->capacity > 0) ? l->capacity * 2 : 1;
        assembler_token *temp = malloc(sizeof(assembler_token) * new_cap);
        memcpy(temp, l->tokens, sizeof(assembler_token) * l->capacity);
        free(l->tokens);
        l->tokens = temp;
        l->capacity = new_cap;
    }

    l->tokens[l->len++] = tok;
}

//Checks if a character is a letter
uint8_t is_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

//Checks if a character is a decimal number
uint8_t is_number(char c) {
    return (c >= '0' && c <= '9');
}

//Checks if a character is a hexadecimal number
uint8_t is_hex(char c) {
    return (is_number(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

//Checks if a character is a binary number
uint8_t is_binary(char c) {
    return (c == '0' || c == '1');
}

//Checks if a character is either a decimal number or a letter
uint8_t is_string(char c) {
    return is_number(c) && is_char(c);
}

//Consumes characters given a discriminator function until it reaches a character
//that is not accepted by the discriminator. If the character is a space, tab, or
//newline, we have a full token and can construct it, otherwise we have a mismatch
//between the expected and actual token type, which could lead to an error in the
//calling function. Should only be called as a helper for tokenise
uint8_t consume(char *buf, lexer *l, size_t *i, size_t *curr_col, size_t curr_line, uint8_t (*disc)(char c), assembler_token_type type) {
    char lex_buf[32]; //Except for maybe labels, no token string is going to be longer than 4 chars
    size_t start_col = *curr_col;
    size_t curr_len = 0;

    //Keep consuming and storing chars while they match the discriminator
    while(disc(buf[*i])) {
        lex_buf[curr_len] = buf[*i];
        (*i)++;
        (*curr_col)++;
        curr_len++;
    }

    //If the char after the label is not blank or a closed bracket, we may have an error
    if(buf[(*i)] != ' ' && buf[(*i)] != '\n' && buf[(*i)] != '\t' && buf[(*i)] != '/' && buf[(*i)] != ')' && buf[(*i)] != ',') {
        err_col = start_col+curr_len;
        err_line = curr_line;
        err_char = buf[(*i)];
        return 0;
    }

    //Otherwise build the token
    assembler_token tok = {
        .type = type,
        .column = start_col,
        .line = curr_line,
        .len = curr_len,
    };
    memcpy(&tok.lexeme, &lex_buf, curr_len);

    //If the given token is an opcode token we need to determine what
    //kind of opcode it is by looking it up in the lookup table
    if(type == assembler_opcode) {
        //Could a string for addressing
        if(curr_len == 1) {
            switch(toupper(lex_buf[0])) {
                //X register
                case 'X':
                    tok.type = assembler_x;
                    break;
                //Y register
                case 'Y':
                    tok.type = assembler_y;
                    break;
                //Accumulator
                case 'A':
                    tok.type = assembler_a;
                    break;
                //Otherwise just a regular string
                default:
                    tok.type = assembler_string;
                    break;
            }
        }
        //Otherwise need to look it up
        else {
            for(size_t j = 0; j < curr_len; j++) {
                lex_buf[j] = toupper(lex_buf[j]);
            }
            lex_buf[curr_len] = '\0';

            uint8_t mapping = lookup_instr(lex_buf);
            if(mapping != 0xFF) {
                tok.type = nparams[mapping] == 0 ? assembler_opcode0 : assembler_opcode1;
            }

            //If it does not match any of the opcode names then it must be a string name for a variable/label
            if(tok.type == assembler_opcode) tok.type = assembler_string;
        }
    }
    append_token(l, tok); //Add the token to the lexer

    return 1;
}

//Tokenises the given buffer.
//Produces the following error codes:
// 0 - All good
// 1 - unexpected char
// 2 - unsupported char
int tokenise(char *buf, size_t len, lexer *l) {
    size_t curr_line = 0;
    size_t curr_col = 0;
    assembler_token_type expected_type = assembler_none;

    size_t i = 0;
    while(i < len) {
        // Numbers and Alphabetical letters (upper and lower case)
        if (is_number(buf[i])) {
            //If this is a fresh token, assume it is a number
            if(expected_type == assembler_none) {
                if(consume(buf, l, &i, &curr_col, curr_line, &is_number, assembler_dec)) expected_type = assembler_none;
                continue;
            }
            else {
                //If it is not a fresh token, it must be a string
                if(!consume(buf, l, &i, &curr_col, curr_line, &is_string, assembler_string)) return 1;
                expected_type = assembler_none;
                continue;
            }
        }
        else if (is_char(buf[i])) {
            //If this is a fresh token, assume it is an opcode
            if(expected_type == assembler_none) {
                if(consume(buf, l, &i, &curr_col, curr_line, &is_char, assembler_opcode)) expected_type = assembler_none;
                continue;
            }
            else {
                //Otherwise it must be a string
                if(!consume(buf, l, &i, &curr_col, curr_line, &is_string, assembler_string)) return 1;
                expected_type = assembler_none;
                continue;
            }
        }
        else {
            switch(buf[i]) {
                //Skip spaces and tabs
                case ' ':    //Space
                case '\t':     //Tab
                    break;
                case '#':    //#
                    //# can only be used to denote immediate operators, cannot be used in a string/other token
                    if(expected_type != assembler_none) {
                        err_col = curr_col;
                        err_line = curr_line;
                        err_char = '#';
                        return 1;
                    }
                    else {
                       assembler_token tok = {
                            .type = assembler_imm,
                            .column = curr_col,
                            .line = curr_line,
                            .len = 1,
                            .lexeme = "#"
                        };
                        append_token(l, tok);

                        expected_type = assembler_none;
                    }
                    break;
                case '$':    //$
                    //$ can only be used to denote hexadecimal numbers, cannot be used in a string/other token
                    if(expected_type != assembler_none) {
                        err_col = curr_col;
                        err_line = curr_line;
                        err_char = '$';
                        return 1;
                    }
                    else {
                        i++;
                        curr_col++;
                        if(!consume(buf, l, &i, &curr_col, curr_line, &is_hex, assembler_hex)) return 1;
                        expected_type = assembler_none;
                        continue;
                    }
                    break;
                case '%':    //%
                    //% can only be used to denote binary numbers, cannot be used in a string/other token
                    if(expected_type != assembler_none) {
                        err_col = curr_col;
                        err_line = curr_line;
                        err_char = '%';
                        return 1;
                    }
                    else {
                        i++;
                        curr_col++;
                        if(!consume(buf, l, &i, &curr_col, curr_line, &is_binary, assembler_binary)) return 1;
                        expected_type = assembler_none;
                        continue;
                    }
                    break;
                case '\n':    //\n
                    //Create the newline token, and reset curr_col and expected_type, and increment the curr_line
                    append_token(l, (assembler_token){
                        .type = assembler_nl,
                        .column = curr_col,
                        .line = curr_line,
                        .len = 1,
                        .lexeme = "\n"
                    });

                    curr_col = 0;
                    curr_line++;
                    expected_type = assembler_none;
                    break;
                case '/':
                    if(i < len && buf[i+1] == '/') {
                        i++;
                        curr_col++;
                        while(i < len && buf[i] != '\n') {
                            i++;
                            curr_col++;
                        }
                        continue;
                    }
                    else {
                        err_col = curr_col;
                        err_line = curr_line;
                        err_char = '/';
                        return 1;
                    }
                    break;
                case '(':
                    if(expected_type != assembler_none) {
                        err_col = curr_col;
                        err_line = curr_line;
                        err_char = '(';
                        return 1;
                    }
                    else {
                       assembler_token tok = {
                            .type = assembler_lparen,
                            .column = curr_col,
                            .line = curr_line,
                            .len = 1,
                            .lexeme = "("
                        };
                        append_token(l, tok);
                        l->open_bracket = 1;

                        expected_type = assembler_none;
                    }
                    break;
                case ')':
                    if(!l->open_bracket) {
                        err_col = curr_col;
                        err_line = curr_line;
                        err_char = ')';
                        return 1;
                    }
                    else {
                       assembler_token tok = {
                            .type = assembler_rparen,
                            .column = curr_col,
                            .line = curr_line,
                            .len = 1,
                            .lexeme = ")"
                        };
                        append_token(l, tok);
                        l->open_bracket = 0;

                        expected_type = assembler_none;
                    }
                    break;
                case ',':
                    append_token(l, (assembler_token){
                        .type = assembler_comma,
                        .column = curr_col,
                        .line = curr_line,
                        .len = 1,
                        .lexeme = ","
                    });
                    expected_type = assembler_none;
                    break;
                //Usupported char
                default:
                    err_col = curr_col;
                    err_line = curr_line;
                    err_char = buf[i];
                    return 2;
            }
        }
        curr_col++;
        i++;
    }

    return 0;
}

//Parses a decimal number string into a usable value
uint32_t parse_dec(char *str, size_t len) {
    uint32_t ret = 0;

    for(size_t i = 0; i < len; i++) {
        ret *= 10;
        ret += str[i] - '0';
    }
    return ret;
}

//Parses a binary number string into a usable value
uint32_t parse_binary(char *str, size_t len) {
    uint32_t ret = 0;

    for(size_t i = 0; i < len; i++) {
        ret *= 2;
        ret += str[i] - '0';
    }
    return ret;
}

//Parses a hexadecimal number string into a usable value
uint32_t parse_hex(char *str, size_t len) {
    uint32_t ret = 0;

    for(size_t i = 0; i < len; i++) {
        ret *= 16;
        ret += (is_number(str[i])) ? str[i] - '0' : (toupper(str[i]) - 'A') + 10;
    }
    return ret;
}

uint32_t parse_number(char *str, size_t len, assembler_token_type type) {
    switch(type) {
        case assembler_dec:
            return parse_dec(str, len);
        case assembler_hex:
            return parse_hex(str, len);
        case assembler_binary:
            return parse_binary(str, len);
        default:
            return 65536;
    }
}

//Checks if a sequence of tokens parsed on a line matches a valid grammar production
//Current valid grammar productions:
//  (1) OPCODE0 NEWLINE
//  (2) OPCODE1 A NEWLINE
//  (3) OPCODE1 (DECIMAL|HEXADECIMAL|BINARY) NEWLINE
//  (6) OPCODE1 IMM (DECIMAL|HEXADECIMAL|BINARY) NEWLINE
//  (4) OPCODE1 (DECIMAL|HEXADECIMAL|BINARY) COMMA X NEWLINE
//  (5) OPCODE1 (DECIMAL|HEXADECIMAL|BINARY) COMMA Y NEWLINE
//  (7) OPCODE1 LPAREN (DECIMAL|HEXADECIMAL|BINARY) RPAREN NEWLINE
//  (8) OPCODE1 LPAREN (DECIMAL|HEXADECIMAL|BINARY) COMMA X RPAREN NEWLINE
//  (9) OPCODE1 LPAREN (DECIMAL|HEXADECIMAL|BINARY) RPAREN COMMA Y NEWLINE
//Returns 0 if no production matches
uint8_t match_production(assembler_token_type *tokens, size_t len) {
    #define token_is_number(token) ((token) == assembler_hex || (token) == assembler_binary || (token) == assembler_dec)

    switch (len) {
        case 1:
            if(tokens[0] == assembler_opcode0) return 1;
            break;
        case 2:
            if(tokens[0] == assembler_opcode1 && tokens[1] == assembler_a) return 2;
            if(tokens[0] == assembler_opcode1 && token_is_number(tokens[1])) return 3;
            break;
        case 3:
            if(tokens[0] == assembler_opcode1 && tokens[1] == assembler_imm && token_is_number(tokens[2])) return 4;
            break;
        case 4:
            if(tokens[0] == assembler_opcode1 && token_is_number(tokens[1]) && tokens[3] == assembler_x) return 5;
            if(tokens[0] == assembler_opcode1 && token_is_number(tokens[1]) && tokens[3] == assembler_y) return 6;
            if(tokens[0] == assembler_opcode1 && tokens[1] == assembler_lparen && token_is_number(tokens[2]) && tokens[3] == assembler_rparen) return 7;
            break;
        case 6:
            if(tokens[0] == assembler_opcode1 && tokens[1] == assembler_lparen && token_is_number(tokens[2])
            && tokens[3] == assembler_comma && tokens[4] == assembler_x && tokens[5] == assembler_rparen) return 8;
            if(tokens[0] == assembler_opcode1 && tokens[1] == assembler_lparen && token_is_number(tokens[2])
            && tokens[3] == assembler_rparen && tokens[4] == assembler_comma && tokens[5] == assembler_y) return 9;
            break;
    }
    return 0;
}

//Parses a series of tokens into a byte string that is the output of the assembler
//If everything is correct, it will return the length of the output buffer
//Otherwise it can return one of 4 error codes as negative values:
//  1: Invalid Production
//  2: Invalid opcode/address mode combination
//  3: Numerical value out of range
int parse(lexer *l, char *out) {
    size_t i = 0;
    int ch_ptr = 0;
    size_t buf_sz = 0;
    assembler_token_type tok_buf[8];
    uint32_t param = 0;

    while(i < l->len) {
        for(buf_sz = 0; buf_sz < 8; buf_sz++) {
            if(i + buf_sz >= l->len || l->tokens[i+buf_sz].type == assembler_nl) break;
            tok_buf[buf_sz] = l->tokens[i + buf_sz].type;
        }

        switch(match_production(tok_buf, buf_sz)) {
            //Invalid production
            case 0:
                return -1;
            case 1:
                out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][IMPLICIT];
                if(out[ch_ptr-1] == 0 && lookup_instr(l->tokens[i].lexeme) != BRK) return -2;
                break;
            case 2:
                out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][ACCUMULATOR];
                if(out[ch_ptr-1] == 0) return -2;
                break;
            case 3:
            case 5:
            case 6:
                param = parse_number(l->tokens[i+1].lexeme, l->tokens[i+1].len, l->tokens[i+1].type);
                if(param >= MEMORY_SIZE) return -3;
                if(param > 255) {
                    if(buf_sz == 2) out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][ABSOLUTE];
                    else out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][(l->tokens[i+2].type == assembler_x) ? ABSOLUTE_X : ABSOLUTE_Y];
                    if(out[ch_ptr-1] == 0) return -2;

                    //Need to split it into two bytes if too large
                    out[ch_ptr++] = (uint8_t)(param & 0xFF);
                    out[ch_ptr++] = (uint8_t)((param & 0xFF00) >> 8);
                }
                else {
                    if(buf_sz == 2) out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][ZERO_PAGE];
                    else out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][(l->tokens[i+2].type == assembler_x) ? ZERO_PAGE_X : ZERO_PAGE_Y];
                    if(out[ch_ptr-1] == 0) return -2;
                    out[ch_ptr++] = (uint8_t)(param & 0xFF);
                }
                break;
            case 4:
                param = parse_number(l->tokens[i+2].lexeme, l->tokens[i+2].len, l->tokens[i+2].type);
                if(param > 255) return -3;
                out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][IMMEDIATE];
                if(out[ch_ptr-1] == 0 && lookup_instr(l->tokens[i].lexeme)!= BRK) return -2;
                out[ch_ptr++] = (uint8_t)(param & 0xFF);
                break;
            case 7:
                if(lookup_instr(l->tokens[i].lexeme) != JMP) return -2;
                param = parse_number(l->tokens[i+2].lexeme, l->tokens[i+2].len, l->tokens[i+2].type);
                out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][INDIRECT];

                //Need to split it into two bytes if too large
                out[ch_ptr++] = (uint8_t)(param & 0xFF);
                out[ch_ptr++] = (uint8_t)((param & 0xFF00) >> 8);
                break;
            case 8:
            case 9:
                param = parse_number(l->tokens[i+2].lexeme, l->tokens[i+2].len, l->tokens[i+2].type);
                if(param > 255) return -3;
                out[ch_ptr++] = instr_map[lookup_instr(l->tokens[i].lexeme)][(l->tokens[i+4].type == assembler_x) ? INDIRECT_X : INDIRECT_Y];
                if(out[ch_ptr-1] == 0) return -2;
                out[ch_ptr++] = (uint8_t)(param & 0xFF);
        }

        i += buf_sz+1;
    }

    return ch_ptr;
}

int main(void) {
    FILE *fptr = fopen("input.txt", "r");
    char *buf;

    int len = read_to_end("input.txt", &buf, 1);
    fclose(fptr);

    lexer l = {0};

    int res = tokenise(buf, len, &l);
    printf("Result = %i\n", res);
    print_lexer(&l);
    if(res) {
        printf("Error line: %zu, Error col: %zu, Error char: %c\n", err_line, err_col, err_char);
    }
    char out_buf[l.len*2];
    int parsed_len = parse(&l, out_buf);
    printf("Parsed len: %i\n", parsed_len);
    if(parsed_len > 0) write_text(out_buf, (size_t)parsed_len);

    free(l.tokens);
    free(buf);

    return 0;
}
