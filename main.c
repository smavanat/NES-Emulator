#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

uint8_t stop = 0;

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


int read_to_end(char const *path, char **buf, uint8_t add_null) {
    FILE *fp;
    size_t fsz;
    long offEnd;
    int rc;

    //Open the file
    fp = fopen(path, "rb");
    if(NULL == fp) {
        fclose(fp);
        return -1;
    }

    //Seek to the end of the file
    rc = fseek(fp, 0L, SEEK_END);
    if(0 != rc) {
        fclose(fp);
        return -1;
    }

    //Byte offset to the end of the file size
    if(0 > (offEnd = ftell(fp))) {
        fclose(fp);
        return -1;
    }
    fsz = (size_t)offEnd;

    //Allocate a buffer to hold the whole file
    *buf = malloc(fsz + (int)add_null);
    if(NULL == *buf) {
        fclose(fp);
        return -1;
    }

    //Rewind file pointer to the start of the file:
    rewind(fp);

    //Place the file into a buffer
    if(fsz != fread(*buf, 1, fsz, fp)) {
        free(*buf);
        fclose(fp);
        return -1;
    }

    //Close the file
    if(EOF == fclose(fp)) {
        free(*buf);
        return -1;
    }

    //Add null terminator
    if(add_null) {
        (*buf)[fsz] = '\0';
    }

    return fsz;
}

//Returns the value of the byte at pc and increments pc
uint8_t fetch_byte(cpu *c) {
    return c->memory[c->pc++];
}

//Gets the value of the given cpu flag
uint8_t get_cpu_flag(cpu *c, cpu_flag fl) {
    if(c->proc_stat_reg & (1 << fl)) return 1;
    else return 0;
}

//Sets a cpu flag to a 1 if the given condition is true, other wise 0
void set_cpu_flag(cpu *c, cpu_flag fl, uint8_t cond) {
    if(cond) c->proc_stat_reg |= (uint8_t)(1 << fl);
    else c->proc_stat_reg &= (uint8_t)~(1 << fl);
}

//Branches on the argument address if the given expression is true
void branch(cpu *c, uint8_t expr) {
    int8_t offset = (int8_t)fetch_byte(c);
    if(expr) {
        c->pc += offset;
    }
}

//Pushes a value onto the stack and decrements sp
void push(cpu* c, uint8_t val) {
    c->memory[0x100 + c->sp] = val;
    c->sp--;
}

//Pops the top value off of the stack and increments sp
uint8_t pull(cpu *c) {
    c->sp++;
    return c->memory[0x100 + c->sp];
}

//Pushes the pc to the stack, in the order: high byte, low byte
void push_pc(cpu *c) {
    uint8_t high = (uint8_t)(c->pc >> 8);
    uint8_t low = (uint8_t)(c->pc);

    push(c, high);
    push(c, low);
}

//Pulls the value of the pc from the stack in the order low byte, high byte
void pull_pc(cpu *c) {
    uint8_t low = pull(c);
    uint8_t high = pull(c);

    c->pc = (high << 8) | low;
}

//Sets the pc to the little endian 2 byte value stored at the given address and its successor
void set_pc(cpu *c, uint16_t addr) {
    c->pc = (((uint16_t)c->memory[addr+1]) << 8) | c->memory[addr];
}

uint16_t get_addr(cpu *c) {
    uint8_t low = fetch_byte(c);
    uint16_t high = fetch_byte(c);

    return (high << 8) | low;
}

uint16_t get_indirect_add(cpu *c, uint8_t start) {
    return (((uint16_t)c->memory[start+1]) << 8) | c->memory[start];
}

void compare(cpu *c, uint8_t v1, uint8_t v2) {
    set_cpu_flag(c, CARRY, v1 >= v2);
    set_cpu_flag(c, ZERO, v1 == v2);
    set_cpu_flag(c, NEGATIVE, (v1 - v2) & 0x80);
}

//TODO: See if we should return the address or the value at the address
//      Could return a UNION type to allow for the fact that it could be an address or a value
uint16_t get_argument(cpu *c, address_mode mode, uint8_t get_val) {
    uint16_t addr = 0;
    switch(mode) {
        case ABSOLUTE:
            addr = get_addr(c);
            break;
        case ABSOLUTE_X:
            addr = get_addr(c) + c->x;
            break;
        case ABSOLUTE_Y:
            addr = get_addr(c) + c->y;
            break;
        case ACCUMULATOR:
            break;
        case IMMEDIATE:
            return fetch_byte(c);
        case IMPLICIT: //Only used by BRK
            break;
        case INDIRECT:
        {
            uint16_t ptr = get_addr(c);
            addr = c->memory[ptr] | (c->memory[ptr+1] << 8);
        }
            break;
        case INDIRECT_X:
            addr = (fetch_byte(c) + c ->x) % 256;
            addr = (((uint16_t)c->memory[(addr+1)%256]) << 8) | c->memory[addr];
            break;
        case INDIRECT_Y:
            addr = fetch_byte(c);
            addr = (((uint16_t)c->memory[(addr+1)%256]) << 8) | c->memory[addr];
            addr += c->y;
            break;
        case RELATIVE:
            // addr = fetch_byte(c);
            break;
        case ZERO_PAGE:
            addr = fetch_byte(c);
            break;
        case ZERO_PAGE_X:
            addr = (fetch_byte(c)+ c->x) % 256;
            break;
        case ZERO_PAGE_Y:
            addr = (fetch_byte(c) + c->y) % 256;
            break;
    }

    return get_val ? c->memory[addr] : addr;
}

void execute_instr(cpu *c, uint8_t op) {
    instruction instr = instructions[op];
    uint16_t param[4]; //Sticking with immediate mode addressing for all instructions for now
    // param[0] = get_argument(c, instr.addr_mode);

    switch(instr.instr) {
        //Loads value from memory address into accumulator
        case LDA:
            c->acc = get_argument(c, instr.addr_mode, 1);
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Loads value from memory address into x register
        case LDX:
            c->x = get_argument(c, instr.addr_mode, 1);
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Loads value from memory address into y register
        case LDY:
            c->y = get_argument(c, instr.addr_mode, 1);
            set_cpu_flag(c, ZERO, c->y == 0);
            set_cpu_flag(c, NEGATIVE, (c->y & 0x80));
            break;
        //Stores the value in the accumulator at the given memory address
        case STA:
            c->memory[get_argument(c, instr.addr_mode, 0)] = c->acc;
            break;
        //Stores the value in the x register at the given memory address
        case STX:
            c->memory[get_argument(c, instr.addr_mode, 0)] = c->x;
            break;
        //Stores the value in the y register at the given memory address
        case STY:
            c->memory[get_argument(c, instr.addr_mode, 0)] = c->y;
            break;
        //Transfers accumulator value into x register;
        case TAX:
            c->x = c->acc;
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Transfers x register value into accumulator;
        case TXA:
            c->acc = c->x;
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Transfers accumulator value into y register;
        case TAY:
            c->y = c->acc;
            set_cpu_flag(c, ZERO, c->y == 0);
            set_cpu_flag(c, NEGATIVE, (c->y & 0x80));
            break;
        //Transfers x register value into accumulator;
        case TYA:
            c->acc = c->y;
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Adds a memory value and the carry flag to the accumulator
        //Sets the carry flag if there is overflow
        //Sets the overflow flag if there is signed overflow (result has different sign from both memory value and accumulator)
        case ADC:
            param[0] = get_argument(c, instr.addr_mode, 1);
            param[1] = c->acc + param[0] + get_cpu_flag(c, CARRY);
            param[2] = (uint8_t)param[1];
            set_cpu_flag(c, CARRY, param[1] > 0xFF);
            set_cpu_flag(c, ZERO, param[2] == 0);
            set_cpu_flag(c, OVERFLOW, (c->acc ^ param[2]) & (param[2] ^ param[0]) & 0x80);
            set_cpu_flag(c, NEGATIVE,  param[2] & 0x80);
            c->acc = (uint8_t)param[1];
            break;
        //Subtracts a memory value and the carry flag to the accumulator
        //Sets the carry flag if there is overflow
        //Sets the overflow flag if there is signed overflow (result has different sign from both memory value and accumulator)
        case SBC:
            param[0] = get_argument(c, instr.addr_mode, 1);
            param[1] = c->acc - param[0] - !get_cpu_flag(c, CARRY);
            param[2] = (uint8_t)param[1];
            set_cpu_flag(c, CARRY, !(param[1] < 0xFF));
            set_cpu_flag(c, ZERO, param[2] == 0);
            set_cpu_flag(c, OVERFLOW, (c->acc ^ param[2]) & (param[2] ^ ~param[0]) & 0x80);
            set_cpu_flag(c, NEGATIVE,  param[2] & 0x80);
            c->acc = (uint8_t)param[1];
            break;
        //Increments a given memory value by 1
        case INC:
            param[0] = get_argument(c, instr.addr_mode, 0);
            c->memory[param[0]]++;
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, (c->memory[param[0]] & 0x80));
            break;
        //Decrements a given memory value by 1
        case DEC:
            param[0] = get_argument(c, instr.addr_mode, 0);
            c->memory[param[0]]--;
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, (c->memory[param[0]] & 0x80));
            break;
        //Increments the value in the x register by 1
        case INX:
            c->x++;
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Decrements the value in the x register by 1
        case DEX:
            c->x--;
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Increments the value in the y register by 1
        case INY:
            c->y++;
            set_cpu_flag(c, ZERO, c->y == 0);
            set_cpu_flag(c, NEGATIVE, (c->y & 0x80));
            break;
        //Decrements the value in the y register by 1
        case DEY:
            c->y--;
            set_cpu_flag(c, ZERO, c->y == 0);
            set_cpu_flag(c, NEGATIVE, (c->y & 0x80));
            break;
        //Shifts a memory value one bit to the left,
        //Setting the carry flag to the bit shifted out
        case ASL:
            if(instr.addr_mode == ACCUMULATOR) {
                set_cpu_flag(c, CARRY, (c->acc & 0x80));
                c->acc <<= 1;
                set_cpu_flag(c, ZERO, c->acc == 0);
                set_cpu_flag(c, NEGATIVE, c->acc & 0x80);
            }
            else {
                param[0] = get_argument(c, instr.addr_mode, 0);
                set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x80));
                c->memory[param[0]] <<= 1;
                set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
                set_cpu_flag(c, NEGATIVE, c->memory[param[0]] & 0x80);
            }
            break;
        //Shifts a memory value one bit to the right,
        //Setting the carry flag to the bit shifted out
        case LSR:
            if(instr.addr_mode == ACCUMULATOR) {
                set_cpu_flag(c, CARRY, (c->acc & 0x1));
                c->acc >>= 1;
                set_cpu_flag(c, ZERO, c->acc == 0);
                set_cpu_flag(c, NEGATIVE, 0);
            }
            else {
                param[0] = get_argument(c, instr.addr_mode, 0);
                set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x1));
                c->memory[param[0]] >>= 1;
                set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
                set_cpu_flag(c, NEGATIVE, 0);
            }
            break;
        //Rotates a memory value one bit to the left
        //by shifting one bit to the left and setting the MSB
        //to the bit shifted out
        //Sets the carry flag to the bit shifted out
        case ROL:
            if(instr.addr_mode == ACCUMULATOR) {
                uint8_t old_carry = get_cpu_flag(c, CARRY);
                set_cpu_flag(c, CARRY, (c->acc & 0x80));
                c->acc <<= 1;
                c->acc |= old_carry;
                set_cpu_flag(c, ZERO, c->acc == 0);
                set_cpu_flag(c, NEGATIVE, c->acc & 0x80);
            }
            else {
                param[0] = get_argument(c, instr.addr_mode, 0);
                uint8_t old_carry = get_cpu_flag(c, CARRY);
                set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x80));
                c->memory[param[0]] <<= 1;
                c->memory[param[0]] |= old_carry;
                set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
                set_cpu_flag(c, NEGATIVE, c->memory[param[0]] & 0x80);
            }
            break;
        //Rotates a memory value one bit to the right
        //by shifting one bit to the right and setting the LSB
        //to the bit shifted out
        //Sets the carry flag to the bit shifted out
        case ROR:
            if(instr.addr_mode == ACCUMULATOR) {
                uint8_t old_carry = get_cpu_flag(c, CARRY);
                set_cpu_flag(c, CARRY, (c->acc & 0x1));
                c->acc >>= 1;
                c->acc |= (old_carry << 7);
                set_cpu_flag(c, ZERO, c->acc == 0);
                set_cpu_flag(c, NEGATIVE, c->acc & 0x80);
            }
            else {
                param[0] = get_argument(c, instr.addr_mode, 0);
                uint8_t old_carry = get_cpu_flag(c, CARRY);
                set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x1));
                c->memory[param[0]] >>= 1;
                c->memory[param[0]] |= (old_carry << 7);
                set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
                set_cpu_flag(c, NEGATIVE, c->memory[param[0]] & 0x80);
            }
            break;
        //Ands the value in the accumulator and some memory value
        case AND:
            param[0] = get_argument(c, instr.addr_mode, 1);
            c->acc &= param[0];
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Ors the value in the accumulator and some memory value
        case ORA:
            param[0] = get_argument(c, instr.addr_mode, 1);
            c->acc |= param[0];
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Xors the value in the accumulator and some memory value
        case XOR:
            param[0] = get_argument(c, instr.addr_mode, 1);
            c->acc ^= param[0];
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Ands the accumulator and a memory value and then sets CPU flags
        //if any bits are set, specifically NEGATIVE if bit 7 is set
        //OVERFLOW if bit 6 is set, or ZERO if the result is 0
        //Does not modfiy the value in the accumulator.
        case BIT:
            param[0] = get_argument(c, instr.addr_mode, 1);
            set_cpu_flag(c, NEGATIVE, (param[0] & 0x80));
            set_cpu_flag(c, OVERFLOW, (param[0] & 0x40));
            param[0] &= c->acc;
            set_cpu_flag(c, ZERO, param[0] == 0);
            // set_cpu_flag(c, NEGATIVE, (param[0] & 0x80));
            // set_cpu_flag(c, OVERFLOW, (param[0] & 0x40));
            break;
        //Compares the accumulator to a memory value
        case CMP:
            compare(c, c->acc, get_argument(c, instr.addr_mode, 1));
            break;
        //Compares the x register to a memory value
        case CPX:
            compare(c, c->x, get_argument(c, instr.addr_mode, 1));
            break;
        //Compares the y register to a memory value
        case CPY:
            compare(c, c->y, get_argument(c, instr.addr_mode, 1));
            break;
        //Branches if the carry flag is not set
        case BCC:
            branch(c, !get_cpu_flag(c, CARRY));
            break;
        //Braches if the carry flag is set
        case BCS:
            branch(c, get_cpu_flag(c, CARRY));
            break;
        //Branches if the zero flag is set
        case BEQ:
            branch(c, get_cpu_flag(c, ZERO));
            break;
        //Branches if the zero flag is not set
        case BNE:
            branch(c, !get_cpu_flag(c, ZERO));
            break;
        //Branches if the negative flag is not set
        case BPL:
            branch(c, !get_cpu_flag(c, NEGATIVE));
            break;
        //Branches if the negative flag is set
        case BMI:
            branch(c, get_cpu_flag(c, NEGATIVE));
            break;
        //Branches if the overflow flag is not set
        case BVC:
            branch(c, !get_cpu_flag(c, OVERFLOW));
            break;
        //Branches if the overflow flag is set
        case BVS:
            branch(c, get_cpu_flag(c, OVERFLOW));
            break;
        //Sets the pc to the memory address specified
        case JMP:
            c->pc = get_argument(c, instr.addr_mode, 0);
            // set_pc(c, param[0]);
            break;
        //Pushes the pc onto the stack and jumps to the subroutine
        //at the given memory address
        case JSR:
            push_pc(c);
            c->pc = get_argument(c, instr.addr_mode, 0);
            break;
        //Returns from a subroutine by pulling the pc from the stack
        case RTS:
            pull_pc(c);
            break;
        //Pushes the high and low bytes of the pc to the stack (separately);
        //Pushes the processor status register with the Break flag set to true to the stack
        //Disables interrupts
        //Loads the pc low byte from 0xFFFE and the high byte from 0xFFFF
        case BRK:
            if(instr.addr_mode == IMPLICIT) fetch_byte(c); //Need to fetch dummy opcode since BRK behaves as a 2 byte instruction even though it is only 1 byte
            push_pc(c);
            push(c, (c->proc_stat_reg | (1 << 4)));
            set_cpu_flag(c, INTERRUPT_DISABLE, 1);
            set_pc(c, 0xFFFE);
            break;
        //Returns from an interrupt by pulling the flags from
        //the stack and then pulling the old pc from the stack
        case RTI:
            c->proc_stat_reg = pull(c);
            pull_pc(c);
            break;
        //Pushes the accumulator to the stack
        case PHA:
            push(c, c->acc);
            break;
        //Pulls the accumulator from the stack
        case PLA:
            c->acc = pull(c);
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Pushes the status register to the stack
        case PHP:
            push(c, c->proc_stat_reg | 0x30);
            break;
        //Pulls the status register from the stack
        case PLP:
            c->proc_stat_reg = pull(c);
            break;
        //Sets the stack pointer to the value of the x register
        case TXS:
            c->sp = c->x;
            break;
        //Sets the x register to the value of the stack pointer
        case TSX:
            c->x = c->sp;
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Clears the carry flag
        case CLC:
            set_cpu_flag(c, CARRY, 0);
            break;
        //Sets the carry flag
        case SEC:
            set_cpu_flag(c, CARRY, 1);
            break;
        //Clears the interrupt disable flag
        case CLI:
            set_cpu_flag(c, INTERRUPT_DISABLE, 0);
            break;
        //Sets the interrupt disable flag
        case SEI:
            set_cpu_flag(c, INTERRUPT_DISABLE, 1);
            break;
        //Clears the decimal flag
        case CLD:
            set_cpu_flag(c, DECIMAL, 0);
            break;
        //Sets the decimal flag
        case SED:
            set_cpu_flag(c, DECIMAL, 1);
            break;
        //Clears the overflow flag
        case CLV:
            set_cpu_flag(c, OVERFLOW, 0);
            break;
        //Do nothing. Its in the name
        case NOP:
            break;
        //Stop the program early
        case STP:
            stop = 1;
            break;
        default:
            printf("Opcode not supported\n");
            break;
    }
}

//Prints the state of the cpu, meaning the contents of its registers, the value of sp and pc, and the state of all flags
void print_cpu_state(cpu *c) {
    printf("Accumulator: %hhu \nX Register: %hhu \nY Register: %hhu \nStack Pointer: %hhu\nPC: %hu\n", c->acc, c->x, c->y, c->sp, c->pc);
    printf("Status Register:\n Carry: %hhu\n Zero: %hhu\n Interrupt Disable: %hhu\n Decimal: %hhu\n Break: %hhu\n Overflow: %hhu\n Negative: %hhu\n",
           get_cpu_flag(c, CARRY), get_cpu_flag(c, ZERO), get_cpu_flag(c, INTERRUPT_DISABLE), get_cpu_flag(c, DECIMAL),
           get_cpu_flag(c, BREAK), get_cpu_flag(c, OVERFLOW), get_cpu_flag(c, NEGATIVE));
}

//Prints the current contents of the stack in a numbered list with the most recently added value at the top
void print_stack(cpu *c) {
    printf("Addr    Val\n");
    for(int i = c->sp+1; i < 256; i++) {
        printf(" %03X     %02X\n", 0x100 + i, c->memory[0x100 + i]);
    }
}

//Prints the entire contents of a given page in memory in byte form in a nice square block
void print_page(cpu *c, uint8_t page_num) {
    uint16_t start_addr = page_num * 256;

    for(int i = 0; i < 256; i++) {
        if(i > 0 && i % 16 == 0) printf("\n");
        printf("%02X ", c->memory[start_addr+i]);
    }
    printf("\n");
}

int main(void) {
    cpu c = {0};
    c.sp = 0xFF;
    // c.memory[0xFFFE] = 25;
    // c.memory[25] = RTS;

    char *buf;
    int sz = read_to_end("nestest.nes", &buf, 0);
    if(sz < 0) {
        fprintf(stderr, "Error when opening a file\n");
        return 0;
    }
    memcpy(&c.memory[0x8000], buf, sz);
    c.pc = 0x8000;

    // for(int i = 0; i < sz; i++) {
    //     printf("%i:   %02X\n", i, c.memory[i]);
    // }

    while(stop == 0) {
        uint8_t instr = fetch_byte(&c);
        execute_instr(&c, instr);
        printf("0x02: %02X, 0x03: %02X\n", c.memory[0x02], c.memory[0x03]);
    }
    print_cpu_state(&c);
    print_stack(&c);
    print_page(&c, 0);

    free(buf);

    return 0;
}
