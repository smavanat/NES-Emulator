#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cpu.h"

// TODO: Make a simple parser to parse the assembly and write the equivalent bytes to the
// output file

#define stringify(op) #op

size_t err_line = 0;
size_t err_col = 0;
char err_char = 0;

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
    size_t ptr;
} lexer;

char *token_type_string(assembler_token_type type) {
    switch (type) {
        case assembler_opcode:
            return "OPCODE";
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

void print_token(assembler_token *tok) {
    printf("Lexeme: %s Line: %zu, Column: %zu, Size: %zu, Type: %s\n", tok->lexeme, tok->line, tok->column, tok->len, token_type_string(tok->type));
}

void print_lexer(lexer *l) {
    for(size_t i = 0; i < l->len; i++) {
        printf("%s ", token_type_string(l->tokens[i].type));
    }
    printf("\n");
    for(size_t i = 0; i < l->len; i++) {
        printf("%s ", l->tokens[i].lexeme);
    }
}

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

uint8_t is_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

uint8_t is_number(char c) {
    return (c >= '0' && c <= '9');
}

uint8_t is_hex(char c) {
    return (is_number(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

uint8_t is_string(char c) {
    return is_number(c) && is_char(c);
}

uint8_t is_binary(char c) {
    return (c == '0' || c == '1');
}

uint8_t consume(char *buf, lexer *l, size_t *i, size_t *curr_col, size_t curr_line, uint8_t (*disc)(char c), assembler_token_type type) {
    char lex_buf[32];
    size_t start_col = *curr_col;
    size_t curr_len = 0;
    while(disc(buf[*i])) {
        lex_buf[curr_len] = buf[*i];
        (*i)++;
        (*curr_col)++;
        curr_len++;
    }
    if(buf[(*i)] != ' ' && buf[(*i)] != '\n') {
        err_col = start_col+curr_len;
        err_line = curr_line;
        err_char = buf[(*i)];
        return 0;
    }

    assembler_token tok = {
        .type = type,
        .column = start_col,
        .line = curr_line,
        .len = curr_len,
    };
    memcpy(&tok.lexeme, &lex_buf, curr_len);
    append_token(l, tok);

    return 1;
}

// 0 - All good
// 1 - unexpected char
// 2 - unsupported char
int scan(char *buf, size_t len, lexer *l) {
    size_t curr_line = 0;
    size_t curr_col = 0;
    assembler_token_type expected_type = assembler_none;

    size_t i = 0;
    while(i < len) {
        // Numbers and Alphabetical letters (upper and lower case)
        if (is_number(buf[i])) {
            if(expected_type == assembler_none) {
                if(consume(buf, l, &i, &curr_col, curr_line, &is_number, assembler_dec)) expected_type = assembler_none;
                continue;
            }
            else {
                if(!consume(buf, l, &i, &curr_col, curr_line, &is_string, assembler_string)) return 1;
                expected_type = assembler_none;
                continue;
            }
        }
        else if (is_char(buf[i])) {
            if(expected_type == assembler_none) {
                if(consume(buf, l, &i, &curr_col, curr_line, &is_char, assembler_opcode)) expected_type = assembler_none;
                continue;
            }
            else {
                if(!consume(buf, l, &i, &curr_col, curr_line, &is_string, assembler_string)) return 1;
                expected_type = assembler_none;
                continue;
            }
        }
        else {
            switch(buf[i]) {
                case 32:    //Space
                case 8:     //Tab
                    break;
                case 35:    //#
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
                case 36:    //$
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
                case 37:    //%
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
                case 10:    //\n
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

// void read() {
//     FILE *fptr = fopen("input.txt", "r");
//     char *buf;
//
//     int len = read_to_end("input.txt", &buf, 1);
//     fclose(fptr);
//
//     write_text(buf, len);
// }

int main(void) {
    FILE *fptr = fopen("input.txt", "r");
    char *buf;

    int len = read_to_end("input.txt", &buf, 1);
    fclose(fptr);

    lexer l = {0};

    int res = scan(buf, len, &l);
    printf("Result = %i\n", res);
    print_lexer(&l);
    if(res) {
        printf("Error line: %zu, Error col: %zu, Error char: %c\n", err_line, err_col, err_char);
    }

    return 0;
}
