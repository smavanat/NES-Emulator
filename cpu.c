#include "cpu.h"
#include "memory.h"
#include <SDL3/SDL_events.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
//TODO: Fix INDIRECT_Y addressing. Maybe fine????
//      Need to figure out what AHX, TAS, LAS and STP do

//Since every NES instruction has a disassemly value between 0x00 and 0xFF, can just make a 256 value array storing all the  opcodes,
//addressing mode, and cycle count for every instruction as a very quick way of getting an instruction from its machine code value
instruction instructions[256] = {
    (instruction){BRK, IMMEDIATE, 7},       // 0x00
    (instruction){ORA, INDIRECT_X, 6},      // 0x01
    (instruction){STP, IMPLICIT, 2},        // 0x02
    (instruction){SLO, INDIRECT_X, 8},      // 0x03 - unofficial
    (instruction){NOP, ZERO_PAGE, 3},       // 0x04 - unofficial
    (instruction){ORA, ZERO_PAGE, 3},       // 0x05
    (instruction){ASL, ZERO_PAGE, 5},       // 0x06
    (instruction){SLO, ZERO_PAGE, 5},       // 0x07 - unofficial
    (instruction){PHP, IMPLICIT, 3},        // 0x08
    (instruction){ORA, IMMEDIATE, 2},       // 0x09
    (instruction){ASL, ACCUMULATOR, 2},     // 0x0a
    (instruction){ANC, IMMEDIATE, 2},       // 0x0b - unofficial
    (instruction){NOP, ABSOLUTE, 4},        // 0x0c - unofficial
    (instruction){ORA, ABSOLUTE, 4},        // 0x0d
    (instruction){ASL, ABSOLUTE, 6},        // 0x0e
    (instruction){SLO, ABSOLUTE, 6},        // 0x0f - unofficial

    (instruction){BPL, RELATIVE, 2},        // 0x10
    (instruction){ORA, INDIRECT_Y, 5},      // 0x11
    (instruction){STP, IMPLICIT, 2},        // 0x12
    (instruction){SLO, INDIRECT_Y, 8},      // 0x13 - unofficial
    (instruction){NOP, ZERO_PAGE_X, 4},     // 0x14 - unofficial
    (instruction){ORA, ZERO_PAGE_X, 4},     // 0x15
    (instruction){ASL, ZERO_PAGE_X, 6},     // 0x16
    (instruction){SLO, ZERO_PAGE_X, 6},     // 0x17 - unofficial
    (instruction){CLC, IMPLICIT, 2},        // 0x18
    (instruction){ORA, ABSOLUTE_Y, 4},      // 0x19
    (instruction){NOP, IMPLICIT, 2},        // 0x1a - unofficial
    (instruction){SLO, ABSOLUTE_X, 7},      // 0x1b - unofficial
    (instruction){NOP, ABSOLUTE_X, 4},      // 0x1c - unofficial
    (instruction){ORA, ABSOLUTE_X, 4},      // 0x1d
    (instruction){ASL, ABSOLUTE_X, 7},      // 0x1e
    (instruction){SLO, ABSOLUTE_X, 7},      // 0x1f - unofficial

    (instruction){JSR, ABSOLUTE, 6},        // 0x20
    (instruction){AND, INDIRECT_X, 6},      // 0x21
    (instruction){STP, IMPLICIT, 2},        // 0x22
    (instruction){RLA, INDIRECT_X, 8},      // 0x23 - unofficial
    (instruction){BIT, ZERO_PAGE, 3},       // 0x24
    (instruction){AND, ZERO_PAGE, 3},       // 0x25
    (instruction){ROL, ZERO_PAGE, 5},       // 0x26
    (instruction){RLA, ZERO_PAGE, 5},       // 0x27 - unofficial
    (instruction){PLP, IMPLICIT, 4},        // 0x28
    (instruction){AND, IMMEDIATE, 2},       // 0x29
    (instruction){ROL, ACCUMULATOR, 2},     // 0x2a
    (instruction){ANC, IMPLICIT, 2},        // 0x2b
    (instruction){BIT, ABSOLUTE, 4},        // 0x2c
    (instruction){AND, ABSOLUTE, 4},        // 0x2d
    (instruction){ROL, ABSOLUTE, 6},        // 0x2e
    (instruction){RLA, ABSOLUTE, 6},        // 0x2f - unofficial

    (instruction){BMI, RELATIVE, 2},        // 0x30
    (instruction){AND, INDIRECT_Y, 5},      // 0x31
    (instruction){STP, IMPLICIT, 2},        // 0x32
    (instruction){RLA, INDIRECT_Y, 8},      // 0x33 - unofficial
    (instruction){NOP, ZERO_PAGE_X, 4},     // 0x34 - unofficial
    (instruction){AND, ZERO_PAGE_X, 4},     // 0x35
    (instruction){ROL, ZERO_PAGE_X, 6},     // 0x36
    (instruction){RLA, ZERO_PAGE_X, 6},     // 0x37 - unofficial
    (instruction){SEC, IMPLICIT, 2},        // 0x38
    (instruction){AND, ABSOLUTE_Y, 4},      // 0x39
    (instruction){NOP, IMPLICIT, 2},        // 0x3a - unofficial
    (instruction){RLA, ABSOLUTE_Y, 7},      // 0x3b - unofficial
    (instruction){NOP, ABSOLUTE_X, 4},      // 0x3c - unofficial
    (instruction){AND, ABSOLUTE_X, 4},      // 0x3d
    (instruction){ROL, ABSOLUTE_X, 7},      // 0x3e
    (instruction){RLA, ABSOLUTE_X, 7},      // 0x3f - unofficial

    (instruction){RTI, IMPLICIT, 6},        // 0x40
    (instruction){XOR, INDIRECT_X, 6},      // 0x41
    (instruction){STP, IMPLICIT, 2},        // 0x42
    (instruction){SRE, INDIRECT_X, 8},      // 0x43 - unofficial
    (instruction){NOP, ZERO_PAGE, 3},       // 0x44 - unofficial
    (instruction){XOR, ZERO_PAGE, 3},       // 0x45
    (instruction){LSR, ZERO_PAGE, 5},       // 0x46
    (instruction){SRE, ZERO_PAGE, 5},       // 0x47 - unofficial
    (instruction){PHA, IMPLICIT, 3},        // 0x48
    (instruction){XOR, IMMEDIATE, 2},       // 0x49
    (instruction){LSR, ACCUMULATOR, 2},     // 0x4a
    (instruction){ALR, IMMEDIATE, 2},       // 0x4b - unofficial
    (instruction){JMP, ABSOLUTE, 3},        // 0x4c
    (instruction){XOR, ABSOLUTE, 4},        // 0x4d
    (instruction){LSR, ABSOLUTE, 6},        // 0x4e
    (instruction){SRE, ABSOLUTE, 6},        // 0x4f - unofficial

    (instruction){BVC, RELATIVE, 2},        // 0x50
    (instruction){XOR, INDIRECT_Y, 5},      // 0x51
    (instruction){STP, IMPLICIT, 2},        // 0x52
    (instruction){SRE, INDIRECT_Y, 8},      // 0x53 - unofficial
    (instruction){NOP, ZERO_PAGE_X, 4},     // 0x54 - unofficial
    (instruction){XOR, ZERO_PAGE_X, 4},     // 0x55
    (instruction){LSR, ZERO_PAGE_X, 6},     // 0x56
    (instruction){SRE, ZERO_PAGE_X, 6},     // 0x57 - unofficial
    (instruction){CLI, IMPLICIT, 2},        // 0x58
    (instruction){XOR, ABSOLUTE_Y, 4},      // 0x59
    (instruction){NOP, IMPLICIT, 2},        // 0x5a - unofficial
    (instruction){SRE, ABSOLUTE_Y, 7},      // 0x5b - unofficial
    (instruction){NOP, ABSOLUTE_X, 4},      // 0x5c - unofficial
    (instruction){XOR, ABSOLUTE_X, 4},      // 0x5d
    (instruction){LSR, ABSOLUTE_X, 7},      // 0x5e
    (instruction){SRE, ABSOLUTE_X, 7},      // 0x5f - unofficial

    (instruction){RTS, IMPLICIT, 6},        // 0x60
    (instruction){ADC, INDIRECT_X, 6},      // 0x61
    (instruction){STP, IMPLICIT, 2},        // 0x62
    (instruction){RRA, INDIRECT_X, 8},      // 0x63 - unofficial
    (instruction){NOP, ZERO_PAGE, 3},       // 0x64 - unofficial
    (instruction){ADC, ZERO_PAGE, 3},       // 0x65
    (instruction){ROR, ZERO_PAGE, 5},       // 0x66
    (instruction){RRA, ZERO_PAGE, 5},       // 0x67 - unofficial
    (instruction){PLA, IMPLICIT, 4},        // 0x68
    (instruction){ADC, IMMEDIATE, 2},       // 0x69
    (instruction){ROR, ACCUMULATOR, 2},     // 0x6a
    (instruction){ARR, IMMEDIATE, 2},       // 0x6b - unofficial
    (instruction){JMP, INDIRECT, 5},        // 0x6c
    (instruction){ADC, ABSOLUTE, 4},        // 0x6d
    (instruction){ROR, ABSOLUTE, 6},        // 0x6e
    (instruction){RRA, ABSOLUTE, 6},        // 0x6f - unofficial

    (instruction){BVS, RELATIVE, 2},        // 0x70
    (instruction){ADC, INDIRECT_Y, 5},      // 0x71
    (instruction){STP, IMPLICIT, 2},        // 0x72
    (instruction){RRA, INDIRECT_Y, 8},      // 0x73 - unofficial
    (instruction){NOP, ZERO_PAGE_X, 4},     // 0x74 - unofficial
    (instruction){ADC, ZERO_PAGE_X, 4},     // 0x75
    (instruction){ROR, ZERO_PAGE_X, 6},     // 0x76
    (instruction){RRA, ZERO_PAGE_X, 6},     // 0x77 - unofficial
    (instruction){SEI, IMPLICIT, 2},        // 0x78
    (instruction){ADC, ABSOLUTE_Y, 4},      // 0x79
    (instruction){NOP, IMPLICIT, 2},        // 0x7a - unofficial
    (instruction){RRA, ABSOLUTE_Y, 7},      // 0x7b - unofficial
    (instruction){NOP, ABSOLUTE_X, 4},      // 0x7c - unofficial
    (instruction){ADC, ABSOLUTE_X, 4},      // 0x7d
    (instruction){ROR, ABSOLUTE_X, 7},      // 0x7e
    (instruction){RRA, ABSOLUTE_X, 7},      // 0x7f - unofficial

    (instruction){NOP, IMMEDIATE, 2},       // 0x80 - unofficial
    (instruction){STA, INDIRECT_X, 6},      // 0x81
    (instruction){NOP, IMMEDIATE, 2},       // 0x82 - unofficial
    (instruction){SAX, INDIRECT_X, 6},      // 0x83 - unofficial
    (instruction){STY, ZERO_PAGE, 3},       // 0x84
    (instruction){STA, ZERO_PAGE, 3},       // 0x85
    (instruction){STX, ZERO_PAGE, 3},       // 0x86
    (instruction){SAX, ZERO_PAGE, 3},       // 0x87 - unofficial
    (instruction){DEY, IMPLICIT, 2},        // 0x88
    (instruction){NOP, IMMEDIATE, 2},       // 0x89 - unofficial
    (instruction){TXA, IMPLICIT, 2},        // 0x8a
    (instruction){XAA, IMPLICIT, 2},        // 0x8b
    (instruction){STY, ABSOLUTE, 4},        // 0x8c
    (instruction){STA, ABSOLUTE, 4},        // 0x8d
    (instruction){STX, ABSOLUTE, 4},        // 0x8e
    (instruction){SAX, ABSOLUTE, 4},        // 0x8f - unofficial

    (instruction){BCC, RELATIVE, 2},        // 0x90
    (instruction){STA, INDIRECT_Y, 6},      // 0x91
    (instruction){STP, IMPLICIT, 2},        // 0x92
    (instruction){AHX, IMPLICIT, 6},        // 0x93
    (instruction){STY, ZERO_PAGE_X, 4},     // 0x94
    (instruction){STA, ZERO_PAGE_X, 4},     // 0x95
    (instruction){STX, ZERO_PAGE_Y, 4},     // 0x96
    (instruction){SAX, ZERO_PAGE_Y, 4},     // 0x97 - unofficial
    (instruction){TYA, IMPLICIT, 2},        // 0x98
    (instruction){STA, ABSOLUTE_Y, 5},      // 0x99
    (instruction){TXS, IMPLICIT, 2},        // 0x9a
    (instruction){TAS, IMPLICIT, 5},        // 0x9b
    (instruction){SHY, IMPLICIT, 5},        // 0x9c
    (instruction){STA, ABSOLUTE_X, 5},      // 0x9d
    (instruction){SHX, IMPLICIT, 5},        // 0x9e
    (instruction){AHX, IMPLICIT, 5},        // 0x9f

    (instruction){LDY, IMMEDIATE, 2},       // 0xa0
    (instruction){LDA, INDIRECT_X, 6},      // 0xa1
    (instruction){LDX, IMMEDIATE, 2},       // 0xa2
    (instruction){LAX, INDIRECT_X, 6},      // 0xa3 - unofficial
    (instruction){LDY, ZERO_PAGE, 3},       // 0xa4
    (instruction){LDA, ZERO_PAGE, 3},       // 0xa5
    (instruction){LDX, ZERO_PAGE, 3},       // 0xa6
    (instruction){LAX, ZERO_PAGE, 3},       // 0xa7 - unofficial
    (instruction){TAY, IMPLICIT, 2},        // 0xa8
    (instruction){LDA, IMMEDIATE, 2},       // 0xa9
    (instruction){TAX, IMPLICIT, 2},        // 0xaa
    (instruction){LAX, IMPLICIT, 2},        // 0xab
    (instruction){LDY, ABSOLUTE, 4},        // 0xac
    (instruction){LDA, ABSOLUTE, 4},        // 0xad
    (instruction){LDX, ABSOLUTE, 4},        // 0xae
    (instruction){LAX, ABSOLUTE, 4},        // 0xaf - unofficial

    (instruction){BCS, RELATIVE, 2},        // 0xb0
    (instruction){LDA, INDIRECT_Y, 5},      // 0xb1
    (instruction){STP, IMPLICIT, 2},        // 0xb2
    (instruction){LAX, INDIRECT_Y, 5},      // 0xb3 - unofficial
    (instruction){LDY, ZERO_PAGE_X, 4},     // 0xb4
    (instruction){LDA, ZERO_PAGE_X, 4},     // 0xb5
    (instruction){LDX, ZERO_PAGE_Y, 4},     // 0xb6
    (instruction){LAX, ZERO_PAGE_Y, 4},     // 0xb7 - unofficial
    (instruction){CLV, IMPLICIT, 2},        // 0xb8
    (instruction){LDA, ABSOLUTE_Y, 4},      // 0xb9
    (instruction){TSX, IMPLICIT, 2},        // 0xba
    (instruction){LAS, IMPLICIT, 4},        // 0xbb
    (instruction){LDY, ABSOLUTE_X, 4},      // 0xbc
    (instruction){LDA, ABSOLUTE_X, 4},      // 0xbd
    (instruction){LDX, ABSOLUTE_Y, 4},      // 0xbe
    (instruction){LAX, ABSOLUTE_Y, 4},      // 0xbf - unofficial

    (instruction){CPY, IMMEDIATE, 2},       // 0xc0
    (instruction){CMP, INDIRECT_X, 6},      // 0xc1
    (instruction){NOP, IMMEDIATE, 2},       // 0xc2 - unofficial
    (instruction){DCP, INDIRECT_X, 8},      // 0xc3 - unofficial
    (instruction){CPY, ZERO_PAGE, 3},       // 0xc4
    (instruction){CMP, ZERO_PAGE, 3},       // 0xc5
    (instruction){DEC, ZERO_PAGE, 5},       // 0xc6
    (instruction){DCP, ZERO_PAGE, 5},       // 0xc7 - unofficial
    (instruction){INY, IMPLICIT, 2},        // 0xc8
    (instruction){CMP, IMMEDIATE, 2},       // 0xc9
    (instruction){DEX, IMPLICIT, 2},        // 0xca
    (instruction){AXS, IMMEDIATE, 2},       // 0xcb - unofficial
    (instruction){CPY, ABSOLUTE, 4},        // 0xcc
    (instruction){CMP, ABSOLUTE, 4},        // 0xcd
    (instruction){DEC, ABSOLUTE, 6},        // 0xce
    (instruction){DCP, ABSOLUTE, 6},        // 0xcf - unofficial

    (instruction){BNE, RELATIVE, 2},        // 0xd0
    (instruction){CMP, INDIRECT_Y, 5},      // 0xd1
    (instruction){STP, IMPLICIT, 2},        // 0xd2
    (instruction){DCP, INDIRECT_Y, 8},      // 0xd3 - unofficial
    (instruction){NOP, ZERO_PAGE_X, 4},     // 0xd4 - unofficial
    (instruction){CMP, ZERO_PAGE_X, 4},     // 0xd5
    (instruction){DEC, ZERO_PAGE_X, 6},     // 0xd6
    (instruction){DCP, ZERO_PAGE_X, 6},     // 0xd7 - unofficial
    (instruction){CLD, IMPLICIT, 2},        // 0xd8
    (instruction){CMP, ABSOLUTE_Y, 4},      // 0xd9
    (instruction){NOP, IMPLICIT, 2},        // 0xda - unofficial
    (instruction){DCP, ABSOLUTE_Y, 7},      // 0xdb - unofficial
    (instruction){NOP, ABSOLUTE_X, 4},      // 0xdc - unofficial
    (instruction){CMP, ABSOLUTE_X, 4},      // 0xdd
    (instruction){DEC, ABSOLUTE_X, 7},      // 0xde
    (instruction){DCP, ABSOLUTE_X, 7},      // 0xdf - unofficial

    (instruction){CPX, IMMEDIATE, 2},       // 0xe0
    (instruction){SBC, INDIRECT_X, 6},      // 0xe1
    (instruction){NOP, IMMEDIATE, 2},       // 0xe2 - unofficial
    (instruction){ISC, INDIRECT_X, 8},      // 0xe3 - unofficial
    (instruction){CPX, ZERO_PAGE, 3},       // 0xe4
    (instruction){SBC, ZERO_PAGE, 3},       // 0xe5
    (instruction){INC, ZERO_PAGE, 5},       // 0xe6
    (instruction){ISC, ZERO_PAGE, 5},       // 0xe7 - unofficial
    (instruction){INX, IMPLICIT, 2},        // 0xe8
    (instruction){SBC, IMMEDIATE, 2},       // 0xe9
    (instruction){NOP, IMPLICIT, 2},        // 0xea
    (instruction){SBC, IMMEDIATE, 2},       // 0xeb - unofficial
    (instruction){CPX, ABSOLUTE, 4},        // 0xec
    (instruction){SBC, ABSOLUTE, 4},        // 0xed
    (instruction){INC, ABSOLUTE, 6},        // 0xee
    (instruction){ISC, ABSOLUTE, 6},        // 0xef - unofficial

    (instruction){BEQ, RELATIVE, 2},        // 0xf0
    (instruction){SBC, INDIRECT_Y, 5},      // 0xf1
    (instruction){STP, IMPLICIT, 2},        // 0xf2
    (instruction){ISC, INDIRECT_Y, 8},      // 0xf3 - unofficial
    (instruction){NOP, ZERO_PAGE_X, 4},     // 0xf4 - unofficial
    (instruction){SBC, ZERO_PAGE_X, 4},     // 0xf5
    (instruction){INC, ZERO_PAGE_X, 6},     // 0xf6
    (instruction){ISC, ZERO_PAGE_X, 6},     // 0xf7 - unofficial
    (instruction){SED, IMPLICIT, 2},        // 0xf8
    (instruction){SBC, ABSOLUTE_Y, 4},      // 0xf9
    (instruction){NOP, IMPLICIT, 2},        // 0xfa - unofficial
    (instruction){ISC, ABSOLUTE_Y, 7},      // 0xfb - unofficial
    (instruction){NOP, ABSOLUTE_X, 4},      // 0xfc - unofficial
    (instruction){SBC, ABSOLUTE_X, 4},      // 0xfd
    (instruction){INC, ABSOLUTE_X, 7},      // 0xfe
    (instruction){ISC, ABSOLUTE_X, 7}       // 0xff - unofficial
};

void set_cpu_flag(cpu *c, int expr, status_flag flag) {
    if(expr)
        c->processor_status_register |= (uint8_t)(expr << flag);
    else
        c->processor_status_register &= ~(uint8_t)(expr << flag);
}

uint8_t get_cpu_flag(cpu *c, status_flag flag) {
    if (c->processor_status_register & (uint8_t)(1 << flag))
        return 1;
    return 0;
}

uint16_t add_wrapping(uint16_t a, uint16_t b) {
    int res = a + b;
    return res % 0xffff;
}

int8_t check_boundary_crossed(uint16_t a, uint16_t b) {
    uint16_t page_one = a & 0xff00;
    uint16_t page_two = b & 0xff00;
    return page_one != page_two;
}

void branch(cpu *c, uint16_t data, int expr) {
    if(expr)
        c->pc += 2 + c->memory.memory[data];
}

uint16_t get_address(cpu *c, address_mode a, int *num_cycles) {
    uint16_t data = 0;
    switch(a) {
        case ABSOLUTE:
            data = c->memory.memory[c->pc + 1];
            data = data << 8;
            data |= c->memory.memory[c->pc];
            c->pc += 3;
            break;
        case ABSOLUTE_X:
            data = c->memory.memory[c->pc + 1];
            data = data << 8;
            data |= c->memory.memory[c->pc];
            if(check_boundary_crossed(data, add_wrapping(data, c->x)))
                *num_cycles += 1;
            data += c->x;
            c->pc += 3;
            break;
        case ABSOLUTE_Y:
            data = c->memory.memory[c->pc + 1];
            data = data << 8;
            data |= c->memory.memory[c->pc];
            if(check_boundary_crossed(data, add_wrapping(data, c->y)))
                *num_cycles += 1;
            data += c->y;
            c->pc += 3;
            break;
        case ACCUMULATOR:
            data = c->accumulator;
            c->pc ++;
            break;
        case RELATIVE:
            data = c->pc + (int8_t)c->memory.memory[c->pc+1];
            c->pc += 2;
            break;
        case INDIRECT:
            data = c->memory.memory[c->pc+1];
            data = data << 8;
            data |= c->memory.memory[c->pc];
            c->pc += 2;

            data = (c->memory.memory[(data & 0xFF00) | ((data + 1) & 0x00FF)] << 8) | c->memory.memory[data];
            break;
        case INDIRECT_X:
            data = (c->memory.memory[(((c->memory.memory[c->pc+1] + c->x) & 0xFF) + 1) & 0xFF] << 8) | c->memory.memory[((c->memory.memory[c->pc+1] + c->x) & 0xFF)];
            c->pc += 2;
            break;
        case INDIRECT_Y:
            data = (c->memory.memory[(((c->memory.memory[c->pc+1]) & 0xFF) + 1) & 0xFF] << 8) | c->memory.memory[((c->memory.memory[c->pc+1]) & 0xFF)] + c->y;
            if(check_boundary_crossed(data, add_wrapping(data, c->y)))
                *num_cycles += 1;
            c->pc += 2;
            break;
        case IMMEDIATE:
            data = c->memory.memory[c->pc+1];
            c->pc += 2;
            break;
        case IMPLICIT:
            c->pc++;
            break;
        case ZERO_PAGE:
            data = c->memory.memory[c->pc+1];
            c->pc +=2;
            break;
        case ZERO_PAGE_X:
            data = (c->memory.memory[c->pc++] + c->x) & 0xff;
            c->pc +=2;
            break;
        case ZERO_PAGE_Y:
            data = (c->memory.memory[c->pc++] + c->y) & 0xff;
            c->pc +=2;
            break;
    }
    return data;
}

void expand_tilde(const char *input, char *output, size_t output_size) {
    if (input[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            snprintf(output, output_size, "%s%s", home, input + 1);
        } else {
            // If HOME is not set, fall back to raw input
            strncpy(output, input, output_size);
            output[output_size - 1] = '\0'; // Ensure null-termination
        }
    } else {
        strncpy(output, input, output_size);
        output[output_size - 1] = '\0';
    }
}

int load_rom(uint8_t memory[MEMORY_SIZE]) {
    char rawpath[260];
    char filepath[260];
    FILE* rom = NULL;

    while (rom == NULL) {
        printf("Enter the file path: ");
        if (fgets(rawpath, sizeof(rawpath), stdin) != NULL) {
            // Remove newline character
            size_t len = strlen(rawpath);
            if (len > 0 && rawpath[len - 1] == '\n') {
                rawpath[len - 1] = '\0';
            }

            expand_tilde(rawpath, filepath, sizeof(filepath));
            printf("%s\n", filepath);

            rom = fopen(filepath, "r");
            if (rom == NULL) {
                perror("Error opening file");
            }
        } else {
            printf("Failed to read input.\n");
            // Clear stdin in case of error
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
    }

    //Seek to end to get size:
    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    if(rom_size > (MEMORY_SIZE - ROM_START)) {
        fprintf(stderr, "ROM too large to fit in memory.memory");
        fclose(rom);
        return 0;
    }

    fread(&memory[0x8000], sizeof(uint8_t), rom_size, rom);
    fclose(rom);
    return rom_size;
}

void execute(cpu *c, uint8_t instr, int *num_cycles) {
    instruction i = instructions[instr];
    *num_cycles = i.num_cycles;
    uint16_t data = get_address(c, i.addr, num_cycles);
    uint8_t opval = 0; //Temp variable to hold intermediate values;

    switch(instr) {
        //Load/Store operations:
        case LDA:
            c->accumulator = c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case LDX:
            c->x = c->memory.memory[data];
            set_cpu_flag(c, c->x == 0, ZERO);
            set_cpu_flag(c, (c->x & 0x80) != 0, NEGATIVE);
            break;
        case LDY:
            c->y = c->memory.memory[data];
            set_cpu_flag(c, c->y == 0, ZERO);
            set_cpu_flag(c, (c->y & 0x80) != 0, NEGATIVE);
            break;
        case STA:
            c->memory.memory[data] = c->accumulator;
            break;
        case STX:
            c->memory.memory[data] = c->x;
            break;
        case STY:
            c->memory.memory[data] = c->y;
            break;

        //Register transfers
        case TAX:
            c->x = c->accumulator;
            set_cpu_flag(c, c->x == 0, ZERO);
            set_cpu_flag(c, (c->x & 0x80) != 0, NEGATIVE);
            break;
        case TAY:
            c->y = c->accumulator;
            set_cpu_flag(c, c->y == 0, ZERO);
            set_cpu_flag(c, (c->y & 0x80) != 0, NEGATIVE);
            break;
        case TXA:
            c->accumulator = c->x;
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case TYA:
            c->accumulator = c->y;
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;

        //Stack operations:
        case TSX:
            c->x = c->stack_pointer;
            set_cpu_flag(c, c->x == 0, ZERO);
            set_cpu_flag(c, (c->x & 0x80) != 0, NEGATIVE);
            break;
        case TXS:
            c->stack_pointer = c->x;
            break;
        case PHA:
            c->memory.memory[0x100 + c->stack_pointer] = c->accumulator;
            c->stack_pointer--;
            break;
        case PHP:
            c->memory.memory[0x100 + c->stack_pointer] = c->processor_status_register;
            c->stack_pointer--;
            break;
        case PLA:
            c->stack_pointer++;
            c->accumulator = c->memory.memory[0x100 + c->stack_pointer];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case PLP:
            c->stack_pointer++;
            c->processor_status_register = c->memory.memory[0x100 + c->stack_pointer];
            break;

        //Logical:
        case AND:
            c->accumulator &= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case XOR:
            c->accumulator ^= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case ORA:
            c->accumulator |= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case BIT:
            set_cpu_flag(c, (c->accumulator & c->memory.memory[data]) != 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x40) != 0, OVERFLOW);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;

        //Arithmetic:
        case ADC:
            opval = (c->accumulator + c->memory.memory[data] + (c->processor_status_register & (1 << CARRY)));
            c->accumulator += c->memory.memory[data] + (c->processor_status_register & (1 << CARRY));
            set_cpu_flag(c, opval > 0xFF, CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, ((!(c->accumulator ^ c->memory.memory[data]) & (c->accumulator ^ opval)) & 0x80) != 0, OVERFLOW);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            break;
        case SBC:
            opval = (c->accumulator - c->memory.memory[data] - !get_cpu_flag(c, CARRY));
            set_cpu_flag(c, ~(opval < 0x00), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, ((!(c->accumulator ^ c->memory.memory[data]) & (c->accumulator ^ opval)) & 0x80) != 0, OVERFLOW);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);

            c->accumulator = opval;
            break;

        //Increments and decrements:
        case INC:
            c->memory.memory[data]++;
            set_cpu_flag(c, c->memory.memory[data] == 0, ZERO);
            set_cpu_flag(c, (c->memory.memory[data] & 0x80) != 0, NEGATIVE);
            break;
        case INX:
            c->x++;
            set_cpu_flag(c, c->x == 0, ZERO);
            set_cpu_flag(c, (c->x & 0x80) != 0, NEGATIVE);
            break;
        case INY:
            c->y++;
            set_cpu_flag(c, c->y == 0, ZERO);
            set_cpu_flag(c, (c->y & 0x80) != 0, NEGATIVE);
            break;
        case DEC:
            c->memory.memory[data]--;
            set_cpu_flag(c, c->memory.memory[data] == 0, ZERO);
            set_cpu_flag(c, (c->memory.memory[data] & 0x80) != 0, NEGATIVE);
            break;
        case DEX:
            c->x--;
            set_cpu_flag(c, c->x == 0, ZERO);
            set_cpu_flag(c, (c->x & 0x80) != 0, NEGATIVE);
            break;
        case DEY:
            c->y--;
            set_cpu_flag(c, c->y == 0, ZERO);
            set_cpu_flag(c, (c->y & 0x80) != 0, NEGATIVE);
            break;

        //Comparisons:
        case CMP:
            opval = c->accumulator - c->memory.memory[data];
            set_cpu_flag(c, (opval < 0), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            break;
        case CPX:
            opval = c->x - c->memory.memory[data];
            set_cpu_flag(c, (opval < 0), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            break;
        case CPY:
            opval = c->y - c->memory.memory[data];
            set_cpu_flag(c, (opval < 0), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            break;

        //Shifts:
        case ASL:
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator << 1;
            else
                opval = c->memory.memory[data] << 1;
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;
            break;
        case LSR:
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator >> 1;
            else
                opval = c->memory.memory[data] >> 1;
            set_cpu_flag(c, (c->accumulator & 0x01), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;
            break;
        //Need to finish off these two below:
        case ROL:
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator << 1;
            else
                opval = c->memory.memory[data] << 1;

            //Need to make the lsb bit the carry bit
            if(get_cpu_flag(c, CARRY))
                opval |= (uint8_t)1;
            else
                opval &= ~(uint8_t)1;
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;
            break;
        case ROR:
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator >> 1;
            else
                opval = c->memory.memory[data] >> 1;

            //Need to make the msb the carry bit
            if(get_cpu_flag(c, CARRY))
                opval |= ((uint8_t)1 << 7);
            else
                opval &= ~((uint8_t)1 << 7);
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;
            break;

        //Jumps and Calls:
        case JMP:
            c->pc = c->memory.memory[data];
            break;
        case JSR:
            c->memory.memory[0x100 + c->stack_pointer] = c->pc+2;
            c->stack_pointer--;
            c->pc = c->memory.memory[data];
            break;
        case RTS:
            c->pc = c->memory.memory[0x100 + c->stack_pointer];
            c->stack_pointer++;
            c->pc++;
            break;

        //Branches:
        case BCC:
            branch(c, data, !get_cpu_flag(c, CARRY));
            break;
        case BCS:
            branch(c, data, get_cpu_flag(c, CARRY));
            break;
        case BEQ:
            branch(c, data, get_cpu_flag(c, ZERO));
            break;
        case BMI:
            branch(c, data, get_cpu_flag(c, NEGATIVE));
            break;
        case BNE:
            branch(c, data, !get_cpu_flag(c, ZERO));
            break;
        case BPL:
            branch(c, data, !get_cpu_flag(c, NEGATIVE));
            break;
        case BVC:
            branch(c, data, !get_cpu_flag(c, OVERFLOW));
            break;
        case BVS:
            branch(c, data, get_cpu_flag(c, OVERFLOW));
            break;

        //Status Flag Changes:
        case CLC:
            set_cpu_flag(c, 0, CARRY);
            break;
        case CLD:
            set_cpu_flag(c, 0, DECIMAL);
            break;
        case CLI:
            set_cpu_flag(c, 0, INTERRPUT_DISABLE);
            break;
        case CLV:
            set_cpu_flag(c, 0, OVERFLOW);
            break;
        case SEC:
            set_cpu_flag(c, 1, CARRY);
            break;
        case SED:
            set_cpu_flag(c, 1, DECIMAL);
            break;
        case SEI:
            set_cpu_flag(c, 1, INTERRPUT_DISABLE);
            break;

        //System functions:
        case BRK:
            c->memory.memory[0x100 + c->stack_pointer] = c->pc+2;
            c->stack_pointer--;
            c->memory.memory[0x100 + c->stack_pointer] = c->processor_status_register;
            c->stack_pointer--;
            c->pc = 0xFFFE;
            set_cpu_flag(c, 1, INTERRPUT_DISABLE);
            break;
        case NOP:
            break;
        case RTI:
            c->processor_status_register = c->memory.memory[0x100 + c->stack_pointer];
            c->stack_pointer++;
            c->pc = c->memory.memory[0x100 + c->stack_pointer];
            c->stack_pointer++;
            break;

        //Unofficial opcodes:
        case SLO: //Shortcut for ASL then ORA
            //ASL
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator << 1;
            else
                opval = c->memory.memory[data] << 1;
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;
            //ORA
            c->accumulator |= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case ANC:
            c->accumulator &= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            break;
        case RLA: //ROL then AND
            //ROL
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator << 1;
            else
                opval = c->memory.memory[data] << 1;

            //Need to make the lsb bit the carry bit
            if(get_cpu_flag(c, CARRY))
                opval |= (uint8_t)1;
            else
                opval &= ~(uint8_t)1;
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;
            //AND
            c->accumulator &= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case SRE: //Shortcut for LSR then XOR
            //LSR
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator >> 1;
            else
                opval = c->memory.memory[data] >> 1;
            set_cpu_flag(c, (c->accumulator & 0x01), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;
            //XOR
            c->accumulator ^= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);
            break;
        case ALR: //And then left shift
            //And
            c->accumulator &= c->memory.memory[data];
            set_cpu_flag(c, c->accumulator == 0, ZERO);
            set_cpu_flag(c, (c->accumulator & 0x80) != 0, NEGATIVE);

            //Left shift
            opval = c->accumulator >> 1;
            set_cpu_flag(c, (c->accumulator & 0x01), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, 0, NEGATIVE);
            c->accumulator = opval;
            break;
        case ARR: //AND #i then ROR acc
            c->accumulator &= c->memory.memory[data];

            opval = c->accumulator >> 1;

            //Need to make the msb the carry bit
            if(get_cpu_flag(c, CARRY))
                opval |= ((uint8_t)1 << 8);
            else
                opval &= ~((uint8_t)1 << 8);
            set_cpu_flag(c, (c->accumulator & 0x40), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            set_cpu_flag(c, (opval & ((uint8_t) 1 << 6)) ^ (opval & ((uint8_t) 1 << 5)), OVERFLOW);
            c->accumulator = opval;
            break;
        case RRA: //ROR then ADC
            //ROR:
            if(i.addr == ACCUMULATOR)
                opval = c->accumulator >> 1;
            else
                opval = c->memory.memory[data] >> 1;

            //Need to make the msb the carry bit
            if(get_cpu_flag(c, CARRY))
                opval |= ((uint8_t)1 << 7);
            else
                opval &= ~((uint8_t)1 << 7);
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            if(i.addr == ACCUMULATOR)
                c->accumulator = opval;
            else
                c->memory.memory[data] = opval;

            //ADC:
            opval = (c->accumulator + c->memory.memory[data] + get_cpu_flag(c, CARRY));
            c->accumulator += c->memory.memory[data] + (c->processor_status_register & (1 << CARRY));
            set_cpu_flag(c, opval > 0xFF, CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, ((!(c->accumulator ^ c->memory.memory[data]) & (c->accumulator ^ opval)) & 0x80) != 0, OVERFLOW);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            break;
        case SAX:
            c->memory.memory[data] = c->accumulator & c->x;
            break;
        case XAA:
            fprintf(stderr, "XAA is not a supported opcode\n");
            break;
        case AHX:
            fprintf(stderr, "AHX is not a supported opcode\n");
            break;
        case TAS:
            fprintf(stderr, "TAS is not a supported opcode\n");
            break;
        case LAX: //Shortcut for LDA then TAX
            //LDA
            c->accumulator = c->memory.memory[data];
            //TAX
            c->x = c->accumulator;
            set_cpu_flag(c, c->x == 0, ZERO);
            set_cpu_flag(c, (c->x & 0x80) != 0, NEGATIVE);
            break;
        case LAS:
            fprintf(stderr, "LAS is not a supported opcode\n");
            break;
        case DCP: //Shortcut for DEC then CMP
            //DEC
            c->memory.memory[data]--;

            //CMP
            opval = c->accumulator - c->memory.memory[data];
            set_cpu_flag(c, (opval < 0), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);
            break;
        case AXS:
            c->x = (c->x & c->accumulator) - c->memory.memory[data];
            set_cpu_flag(c, (c->accumulator & 0x80), CARRY);
            set_cpu_flag(c, c->x == 0, ZERO);
            set_cpu_flag(c, (c->x & 0x80) != 0, NEGATIVE);
            break;
        case ISC: //Shortcut for INC then SBC
            //INC
            c->memory.memory[data]++;
            //SBC
            opval = (c->accumulator - c->memory.memory[data] - !get_cpu_flag(c, CARRY));
            set_cpu_flag(c, ~(opval < 0x00), CARRY);
            set_cpu_flag(c, opval == 0, ZERO);
            set_cpu_flag(c, ((!(c->accumulator ^ c->memory.memory[data]) & (c->accumulator ^ opval)) & 0x80) != 0, OVERFLOW);
            set_cpu_flag(c, (opval & 0x80) != 0, NEGATIVE);

            c->accumulator = opval;
            break;
        case STP:
            // fprintf(stderr, "STP is not a supported opcode\n");
            printf("Stopping Program\n");
            SDL_PushEvent(&(SDL_Event){SDL_EVENT_QUIT});
            break;
        case SHX:
            c->memory.memory[data] = ((uint8_t)((data & 0xff00) >> 8) + 1) & c->x;
            break;
        case SHY:
            c->memory.memory[data] = ((uint8_t)((data & 0xff00) >> 8) + 1) & c->y;
            break;
    }
}

int cpu_init(cpu *c) {
    c->accumulator = 0;
    c->x = 0;
    c->y = 0;
    c->processor_status_register = 0;
    c->stack_pointer = 0;
    c->pc = 0xc000;
    c->processor_status_register = 0x34;
    return 1;
}

int cpu_load(cpu *c) {
    load_rom((uint8_t *)c->memory.memory);
    return 1;
}

int on_NMI(cpu *c, ppu *p) {
    c->memory.memory[0x100 + c->stack_pointer] = (uint8_t)(c->pc >> 8);
    c->stack_pointer--;
    c->memory.memory[0x100 + c->stack_pointer] = (uint8_t)(c->pc >> 0);
    c->stack_pointer--;
    c->memory.memory[0x100 + c->stack_pointer] = c->processor_status_register;
    c->stack_pointer--;
    set_cpu_flag(c, 1, INTERRPUT_DISABLE);

    uint16_t new_pc ;

    return 8;
}

// int main(int argc, char **argv) {
//     cpu c;
//     if(!cpu_init(&c)) {
//         return 1;
//     }
//     else {
//         if(!cpu_load(&c)) {
//             return 1;
//         }
//         else {
//             int cpu_cycle_count = 0;
//             int cycle_count = 0;
//             while(c.pc != 0x900) {
//                 if(cycle_count % 3 == 0) {
//                     if(cpu_cycle_count == 0) {
//                         execute(&c, c.memory.memory[c.pc], &cpu_cycle_count);
//                     }
//                     cpu_cycle_count--;
//                 }
//                 cycle_count++;
//             }
//         }
//         printf("Results: %u, %u\n", c.memory.memory[0x02], c.memory.memory[0x03]);
//     }
//     return 0;
// }
