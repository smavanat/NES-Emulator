//This file contains an assembler for converting 6502 assembly into bytecode
//This is done using a simple parser to tokenise the given file and check the syntax,
//before looking up opcode values in a lookup table
//TODO: Add support for comments

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cpu.h"

#define num_instr 25

size_t err_line = 0;
size_t err_col = 0;
char err_char = 0;

//TODO: Make this into a hashmap
char *instr[num_instr] = {"LDA", "LDX", "LDY", "STA", "STX", "STY", "TAX", "TXA", "TAY", "TYA", "ADC", "SBC", "INC", "DEC", "INX", "DEX", "INY", "DEY", "AND", "ORA", "XOR", "BIT", "BNE", "BRK", "JMP"};
uint8_t nparams[num_instr] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1};

void write_text(char *text, size_t len) {
    FILE *fptr = fopen("output.txt", "w");
    fwrite(text, sizeof(char), len, fptr);
    fclose(fptr);
}

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

    //If the char after the label is not blank, we may have an error
    if(buf[(*i)] != ' ' && buf[(*i)] != '\n' && buf[(*i)] != '\t' && buf[(*i)] != '/') {
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
        for(size_t j = 0; j < curr_len; j++) {
            lex_buf[j] = toupper(lex_buf[j]);
        }
        lex_buf[curr_len] = '\0';

        for(size_t j = 0; j < num_instr; j++) {
            if(!strcmp(lex_buf, instr[j])) {
                tok.type = nparams[j] == 0 ? assembler_opcode0 : assembler_opcode1;
                break;
            }
        }

        //If it does not match any of the opcode names then it must be a string name for a variable/label
        if(tok.type == assembler_opcode) tok.type = assembler_string;
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

//Parses a series of tokens into a byte string that is the output of the assembler
//Currently only three valid grammar productions:
//      OPCODE0 NEWLINE
//      OPCODE1 (DECIMAL|HEXADECIMAL|BINARY) NEWLINE
//      OPCODE1 IMM (DECIMAL|HEXADECIMAL|BINARY) NEWLINE
//
//If everything is correct, it will return the length of the output buffer
//Otherwise it can return one of 4 error codes as negative values:
//  1: Nonexistent opcode
//  2: Invalid opcode parameter
//  3: Parameter is too large (> 65535)
//  4: Unexpected token
int parse(lexer *l, char *out) {
    assembler_token_type previous_type = assembler_none; //Intially assume no previous token

    size_t i = 0;
    int ch_ptr = 0; //Pointer into the output array
    uint32_t res = 0; //Stores any intermediate values computed in assembly

    //Loop compares current a previous tokens to see if they match any productions
    while(i < l->len) {
        assembler_token tok = l->tokens[i];
        switch(previous_type) {
            //If at the start of a line, first token must be an opcode
            case assembler_none:
            case assembler_nl:
                if(tok.type == assembler_opcode0 || tok.type == assembler_opcode1) {
                    //Lookup the opcode binary encoding
                    for(size_t j = 0; j < num_instr; j++) {
                        if(!strcmp(tok.lexeme, instr[j])) {
                            out[ch_ptr++] = j;
                            break;
                        }
                    }
                }
                //If the opcode somehow doesn't exist
                else if(tok.type != assembler_nl) {
                    return -1;
                }
            break;
            //Assembling the argument for an opcode
            case assembler_imm:
            case assembler_opcode1:
                switch(tok.type) {
                    case assembler_dec:
                        res = parse_dec(tok.lexeme, tok.len);
                        break;
                    case assembler_hex:
                        res = parse_hex(tok.lexeme, tok.len);
                        break;
                    case assembler_binary:
                        res = parse_binary(tok.lexeme, tok.len);
                        break;
                    case assembler_imm:
                        if(previous_type == assembler_imm) return -2;
                        break;
                    default:
                        //Invalid operand
                        return -2;
                }
                if(tok.type != assembler_imm) {
                    //If we have too large of a memory value
                    if(res > MEMORY_SIZE - 1) return -3;
                    else if(res > 255) {
                        //Need to split it into two bytes if too large
                        out[ch_ptr++] = (uint8_t)(res & 0xFF);
                        out[ch_ptr++] = (uint8_t)((res & 0xFF00) >> 8);
                    }
                    else {
                        out[ch_ptr++] = (uint8_t)(res & 0xFF);
                    }
                }
                break;
            default:
                //Unexpected token
                if(tok.type != assembler_nl) return -4;
        }
        previous_type = tok.type;
        i++;
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
