#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "bus.h"
#include "cpu.h"

//Since every NES instruction has a disassemly value between 0x00 and 0xFF, can just make a 256 value array storing all the  opcodes,
//addressing mode, and cycle count for every instruction as a very quick way of getting an instruction from its machine code value
instruction instructions[256] = {
    (instruction){CPU_OPCODE_BRK, CPU_ADDR_IMMEDIATE, 7},       // 0x00
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_INDIRECT_X, 6},      // 0x01
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x02
    (instruction){CPU_OPCODE_SLO, CPU_ADDR_INDIRECT_X, 8},      // 0x03 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE, 3},       // 0x04 - unofficial
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_ZERO_PAGE, 3},       // 0x05
    (instruction){CPU_OPCODE_ASL, CPU_ADDR_ZERO_PAGE, 5},       // 0x06
    (instruction){CPU_OPCODE_SLO, CPU_ADDR_ZERO_PAGE, 5},       // 0x07 - unofficial
    (instruction){CPU_OPCODE_PHP, CPU_ADDR_IMPLICIT, 3},        // 0x08
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_IMMEDIATE, 2},       // 0x09
    (instruction){CPU_OPCODE_ASL, CPU_ADDR_ACCUMULATOR, 2},     // 0x0a
    (instruction){CPU_OPCODE_ANC, CPU_ADDR_IMMEDIATE, 2},       // 0x0b - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ABSOLUTE, 4},        // 0x0c - unofficial
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_ABSOLUTE, 4},        // 0x0d
    (instruction){CPU_OPCODE_ASL, CPU_ADDR_ABSOLUTE, 6},        // 0x0e
    (instruction){CPU_OPCODE_SLO, CPU_ADDR_ABSOLUTE, 6},        // 0x0f - unofficial

    (instruction){CPU_OPCODE_BPL, CPU_ADDR_RELATIVE, 2},        // 0x10
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_INDIRECT_Y, 5},      // 0x11
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x12
    (instruction){CPU_OPCODE_SLO, CPU_ADDR_INDIRECT_Y, 8},      // 0x13 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x14 - unofficial
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x15
    (instruction){CPU_OPCODE_ASL, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x16
    (instruction){CPU_OPCODE_SLO, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x17 - unofficial
    (instruction){CPU_OPCODE_CLC, CPU_ADDR_IMPLICIT, 2},        // 0x18
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_ABSOLUTE_Y, 4},      // 0x19
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMPLICIT, 2},        // 0x1a - unofficial
    (instruction){CPU_OPCODE_SLO, CPU_ADDR_ABSOLUTE_X, 7},      // 0x1b - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ABSOLUTE_X, 4},      // 0x1c - unofficial
    (instruction){CPU_OPCODE_ORA, CPU_ADDR_ABSOLUTE_X, 4},      // 0x1d
    (instruction){CPU_OPCODE_ASL, CPU_ADDR_ABSOLUTE_X, 7},      // 0x1e
    (instruction){CPU_OPCODE_SLO, CPU_ADDR_ABSOLUTE_X, 7},      // 0x1f - unofficial

    (instruction){CPU_OPCODE_JSR, CPU_ADDR_ABSOLUTE, 6},        // 0x20
    (instruction){CPU_OPCODE_AND, CPU_ADDR_INDIRECT_X, 6},      // 0x21
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x22
    (instruction){CPU_OPCODE_RLA, CPU_ADDR_INDIRECT_X, 8},      // 0x23 - unofficial
    (instruction){CPU_OPCODE_BIT, CPU_ADDR_ZERO_PAGE, 3},       // 0x24
    (instruction){CPU_OPCODE_AND, CPU_ADDR_ZERO_PAGE, 3},       // 0x25
    (instruction){CPU_OPCODE_ROL, CPU_ADDR_ZERO_PAGE, 5},       // 0x26
    (instruction){CPU_OPCODE_RLA, CPU_ADDR_ZERO_PAGE, 5},       // 0x27 - unofficial
    (instruction){CPU_OPCODE_PLP, CPU_ADDR_IMPLICIT, 4},        // 0x28
    (instruction){CPU_OPCODE_AND, CPU_ADDR_IMMEDIATE, 2},       // 0x29
    (instruction){CPU_OPCODE_ROL, CPU_ADDR_ACCUMULATOR, 2},     // 0x2a
    (instruction){CPU_OPCODE_ANC, CPU_ADDR_IMPLICIT, 2},        // 0x2b
    (instruction){CPU_OPCODE_BIT, CPU_ADDR_ABSOLUTE, 4},        // 0x2c
    (instruction){CPU_OPCODE_AND, CPU_ADDR_ABSOLUTE, 4},        // 0x2d
    (instruction){CPU_OPCODE_ROL, CPU_ADDR_ABSOLUTE, 6},        // 0x2e
    (instruction){CPU_OPCODE_RLA, CPU_ADDR_ABSOLUTE, 6},        // 0x2f - unofficial

    (instruction){CPU_OPCODE_BMI, CPU_ADDR_RELATIVE, 2},        // 0x30
    (instruction){CPU_OPCODE_AND, CPU_ADDR_INDIRECT_Y, 5},      // 0x31
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x32
    (instruction){CPU_OPCODE_RLA, CPU_ADDR_INDIRECT_Y, 8},      // 0x33 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x34 - unofficial
    (instruction){CPU_OPCODE_AND, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x35
    (instruction){CPU_OPCODE_ROL, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x36
    (instruction){CPU_OPCODE_RLA, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x37 - unofficial
    (instruction){CPU_OPCODE_SEC, CPU_ADDR_IMPLICIT, 2},        // 0x38
    (instruction){CPU_OPCODE_AND, CPU_ADDR_ABSOLUTE_Y, 4},      // 0x39
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMPLICIT, 2},        // 0x3a - unofficial
    (instruction){CPU_OPCODE_RLA, CPU_ADDR_ABSOLUTE_Y, 7},      // 0x3b - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ABSOLUTE_X, 4},      // 0x3c - unofficial
    (instruction){CPU_OPCODE_AND, CPU_ADDR_ABSOLUTE_X, 4},      // 0x3d
    (instruction){CPU_OPCODE_ROL, CPU_ADDR_ABSOLUTE_X, 7},      // 0x3e
    (instruction){CPU_OPCODE_RLA, CPU_ADDR_ABSOLUTE_X, 7},      // 0x3f - unofficial

    (instruction){CPU_OPCODE_RTI, CPU_ADDR_IMPLICIT, 6},        // 0x40
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_INDIRECT_X, 6},      // 0x41
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x42
    (instruction){CPU_OPCODE_SRE, CPU_ADDR_INDIRECT_X, 8},      // 0x43 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE, 3},       // 0x44 - unofficial
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_ZERO_PAGE, 3},       // 0x45
    (instruction){CPU_OPCODE_LSR, CPU_ADDR_ZERO_PAGE, 5},       // 0x46
    (instruction){CPU_OPCODE_SRE, CPU_ADDR_ZERO_PAGE, 5},       // 0x47 - unofficial
    (instruction){CPU_OPCODE_PHA, CPU_ADDR_IMPLICIT, 3},        // 0x48
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_IMMEDIATE, 2},       // 0x49
    (instruction){CPU_OPCODE_LSR, CPU_ADDR_ACCUMULATOR, 2},     // 0x4a
    (instruction){CPU_OPCODE_ALR, CPU_ADDR_IMMEDIATE, 2},       // 0x4b - unofficial
    (instruction){CPU_OPCODE_JMP, CPU_ADDR_ABSOLUTE, 3},        // 0x4c
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_ABSOLUTE, 4},        // 0x4d
    (instruction){CPU_OPCODE_LSR, CPU_ADDR_ABSOLUTE, 6},        // 0x4e
    (instruction){CPU_OPCODE_SRE, CPU_ADDR_ABSOLUTE, 6},        // 0x4f - unofficial

    (instruction){CPU_OPCODE_BVC, CPU_ADDR_RELATIVE, 2},        // 0x50
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_INDIRECT_Y, 5},      // 0x51
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x52
    (instruction){CPU_OPCODE_SRE, CPU_ADDR_INDIRECT_Y, 8},      // 0x53 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x54 - unofficial
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x55
    (instruction){CPU_OPCODE_LSR, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x56
    (instruction){CPU_OPCODE_SRE, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x57 - unofficial
    (instruction){CPU_OPCODE_CLI, CPU_ADDR_IMPLICIT, 2},        // 0x58
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_ABSOLUTE_Y, 4},      // 0x59
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMPLICIT, 2},        // 0x5a - unofficial
    (instruction){CPU_OPCODE_SRE, CPU_ADDR_ABSOLUTE_Y, 7},      // 0x5b - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ABSOLUTE_X, 4},      // 0x5c - unofficial
    (instruction){CPU_OPCODE_XOR, CPU_ADDR_ABSOLUTE_X, 4},      // 0x5d
    (instruction){CPU_OPCODE_LSR, CPU_ADDR_ABSOLUTE_X, 7},      // 0x5e
    (instruction){CPU_OPCODE_SRE, CPU_ADDR_ABSOLUTE_X, 7},      // 0x5f - unofficial

    (instruction){CPU_OPCODE_RTS, CPU_ADDR_IMPLICIT, 6},        // 0x60
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_INDIRECT_X, 6},      // 0x61
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x62
    (instruction){CPU_OPCODE_RRA, CPU_ADDR_INDIRECT_X, 8},      // 0x63 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE, 3},       // 0x64 - unofficial
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_ZERO_PAGE, 3},       // 0x65
    (instruction){CPU_OPCODE_ROR, CPU_ADDR_ZERO_PAGE, 5},       // 0x66
    (instruction){CPU_OPCODE_RRA, CPU_ADDR_ZERO_PAGE, 5},       // 0x67 - unofficial
    (instruction){CPU_OPCODE_PLA, CPU_ADDR_IMPLICIT, 4},        // 0x68
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_IMMEDIATE, 2},       // 0x69
    (instruction){CPU_OPCODE_ROR, CPU_ADDR_ACCUMULATOR, 2},     // 0x6a
    (instruction){CPU_OPCODE_ARR, CPU_ADDR_IMMEDIATE, 2},       // 0x6b - unofficial
    (instruction){CPU_OPCODE_JMP, CPU_ADDR_INDIRECT, 5},        // 0x6c
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_ABSOLUTE, 4},        // 0x6d
    (instruction){CPU_OPCODE_ROR, CPU_ADDR_ABSOLUTE, 6},        // 0x6e
    (instruction){CPU_OPCODE_RRA, CPU_ADDR_ABSOLUTE, 6},        // 0x6f - unofficial

    (instruction){CPU_OPCODE_BVS, CPU_ADDR_RELATIVE, 2},        // 0x70
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_INDIRECT_Y, 5},      // 0x71
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x72
    (instruction){CPU_OPCODE_RRA, CPU_ADDR_INDIRECT_Y, 8},      // 0x73 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x74 - unofficial
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x75
    (instruction){CPU_OPCODE_ROR, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x76
    (instruction){CPU_OPCODE_RRA, CPU_ADDR_ZERO_PAGE_X, 6},     // 0x77 - unofficial
    (instruction){CPU_OPCODE_SEI, CPU_ADDR_IMPLICIT, 2},        // 0x78
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_ABSOLUTE_Y, 4},      // 0x79
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMPLICIT, 2},        // 0x7a - unofficial
    (instruction){CPU_OPCODE_RRA, CPU_ADDR_ABSOLUTE_Y, 7},      // 0x7b - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ABSOLUTE_X, 4},      // 0x7c - unofficial
    (instruction){CPU_OPCODE_ADC, CPU_ADDR_ABSOLUTE_X, 4},      // 0x7d
    (instruction){CPU_OPCODE_ROR, CPU_ADDR_ABSOLUTE_X, 7},      // 0x7e
    (instruction){CPU_OPCODE_RRA, CPU_ADDR_ABSOLUTE_X, 7},      // 0x7f - unofficial

    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMMEDIATE, 2},       // 0x80 - unofficial
    (instruction){CPU_OPCODE_STA, CPU_ADDR_INDIRECT_X, 6},      // 0x81
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMMEDIATE, 2},       // 0x82 - unofficial
    (instruction){CPU_OPCODE_SAX, CPU_ADDR_INDIRECT_X, 6},      // 0x83 - unofficial
    (instruction){CPU_OPCODE_STY, CPU_ADDR_ZERO_PAGE, 3},       // 0x84
    (instruction){CPU_OPCODE_STA, CPU_ADDR_ZERO_PAGE, 3},       // 0x85
    (instruction){CPU_OPCODE_STX, CPU_ADDR_ZERO_PAGE, 3},       // 0x86
    (instruction){CPU_OPCODE_SAX, CPU_ADDR_ZERO_PAGE, 3},       // 0x87 - unofficial
    (instruction){CPU_OPCODE_DEY, CPU_ADDR_IMPLICIT, 2},        // 0x88
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMMEDIATE, 2},       // 0x89 - unofficial
    (instruction){CPU_OPCODE_TXA, CPU_ADDR_IMPLICIT, 2},        // 0x8a
    (instruction){CPU_OPCODE_XAA, CPU_ADDR_IMPLICIT, 2},        // 0x8b
    (instruction){CPU_OPCODE_STY, CPU_ADDR_ABSOLUTE, 4},        // 0x8c
    (instruction){CPU_OPCODE_STA, CPU_ADDR_ABSOLUTE, 4},        // 0x8d
    (instruction){CPU_OPCODE_STX, CPU_ADDR_ABSOLUTE, 4},        // 0x8e
    (instruction){CPU_OPCODE_SAX, CPU_ADDR_ABSOLUTE, 4},        // 0x8f - unofficial

    (instruction){CPU_OPCODE_BCC, CPU_ADDR_RELATIVE, 2},        // 0x90
    (instruction){CPU_OPCODE_STA, CPU_ADDR_INDIRECT_Y, 6},      // 0x91
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0x92
    (instruction){CPU_OPCODE_AHX, CPU_ADDR_IMPLICIT, 6},        // 0x93
    (instruction){CPU_OPCODE_STY, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x94
    (instruction){CPU_OPCODE_STA, CPU_ADDR_ZERO_PAGE_X, 4},     // 0x95
    (instruction){CPU_OPCODE_STX, CPU_ADDR_ZERO_PAGE_Y, 4},     // 0x96
    (instruction){CPU_OPCODE_SAX, CPU_ADDR_ZERO_PAGE_Y, 4},     // 0x97 - unofficial
    (instruction){CPU_OPCODE_TYA, CPU_ADDR_IMPLICIT, 2},        // 0x98
    (instruction){CPU_OPCODE_STA, CPU_ADDR_ABSOLUTE_Y, 5},      // 0x99
    (instruction){CPU_OPCODE_TXS, CPU_ADDR_IMPLICIT, 2},        // 0x9a
    (instruction){CPU_OPCODE_TAS, CPU_ADDR_IMPLICIT, 5},        // 0x9b
    (instruction){CPU_OPCODE_SHY, CPU_ADDR_IMPLICIT, 5},        // 0x9c
    (instruction){CPU_OPCODE_STA, CPU_ADDR_ABSOLUTE_X, 5},      // 0x9d
    (instruction){CPU_OPCODE_SHX, CPU_ADDR_IMPLICIT, 5},        // 0x9e
    (instruction){CPU_OPCODE_AHX, CPU_ADDR_IMPLICIT, 5},        // 0x9f

    (instruction){CPU_OPCODE_LDY, CPU_ADDR_IMMEDIATE, 2},       // 0xa0
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_INDIRECT_X, 6},      // 0xa1
    (instruction){CPU_OPCODE_LDX, CPU_ADDR_IMMEDIATE, 2},       // 0xa2
    (instruction){CPU_OPCODE_LAX, CPU_ADDR_INDIRECT_X, 6},      // 0xa3 - unofficial
    (instruction){CPU_OPCODE_LDY, CPU_ADDR_ZERO_PAGE, 3},       // 0xa4
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_ZERO_PAGE, 3},       // 0xa5
    (instruction){CPU_OPCODE_LDX, CPU_ADDR_ZERO_PAGE, 3},       // 0xa6
    (instruction){CPU_OPCODE_LAX, CPU_ADDR_ZERO_PAGE, 3},       // 0xa7 - unofficial
    (instruction){CPU_OPCODE_TAY, CPU_ADDR_IMPLICIT, 2},        // 0xa8
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_IMMEDIATE, 2},       // 0xa9
    (instruction){CPU_OPCODE_TAX, CPU_ADDR_IMPLICIT, 2},        // 0xaa
    (instruction){CPU_OPCODE_LAX, CPU_ADDR_IMPLICIT, 2},        // 0xab
    (instruction){CPU_OPCODE_LDY, CPU_ADDR_ABSOLUTE, 4},        // 0xac
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_ABSOLUTE, 4},        // 0xad
    (instruction){CPU_OPCODE_LDX, CPU_ADDR_ABSOLUTE, 4},        // 0xae
    (instruction){CPU_OPCODE_LAX, CPU_ADDR_ABSOLUTE, 4},        // 0xaf - unofficial

    (instruction){CPU_OPCODE_BCS, CPU_ADDR_RELATIVE, 2},        // 0xb0
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_INDIRECT_Y, 5},      // 0xb1
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0xb2
    (instruction){CPU_OPCODE_LAX, CPU_ADDR_INDIRECT_Y, 5},      // 0xb3 - unofficial
    (instruction){CPU_OPCODE_LDY, CPU_ADDR_ZERO_PAGE_X, 4},     // 0xb4
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_ZERO_PAGE_X, 4},     // 0xb5
    (instruction){CPU_OPCODE_LDX, CPU_ADDR_ZERO_PAGE_Y, 4},     // 0xb6
    (instruction){CPU_OPCODE_LAX, CPU_ADDR_ZERO_PAGE_Y, 4},     // 0xb7 - unofficial
    (instruction){CPU_OPCODE_CLV, CPU_ADDR_IMPLICIT, 2},        // 0xb8
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_ABSOLUTE_Y, 4},      // 0xb9
    (instruction){CPU_OPCODE_TSX, CPU_ADDR_IMPLICIT, 2},        // 0xba
    (instruction){CPU_OPCODE_LAS, CPU_ADDR_IMPLICIT, 4},        // 0xbb
    (instruction){CPU_OPCODE_LDY, CPU_ADDR_ABSOLUTE_X, 4},      // 0xbc
    (instruction){CPU_OPCODE_LDA, CPU_ADDR_ABSOLUTE_X, 4},      // 0xbd
    (instruction){CPU_OPCODE_LDX, CPU_ADDR_ABSOLUTE_Y, 4},      // 0xbe
    (instruction){CPU_OPCODE_LAX, CPU_ADDR_ABSOLUTE_Y, 4},      // 0xbf - unofficial

    (instruction){CPU_OPCODE_CPY, CPU_ADDR_IMMEDIATE, 2},       // 0xc0
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_INDIRECT_X, 6},      // 0xc1
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMMEDIATE, 2},       // 0xc2 - unofficial
    (instruction){CPU_OPCODE_DCP, CPU_ADDR_INDIRECT_X, 8},      // 0xc3 - unofficial
    (instruction){CPU_OPCODE_CPY, CPU_ADDR_ZERO_PAGE, 3},       // 0xc4
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_ZERO_PAGE, 3},       // 0xc5
    (instruction){CPU_OPCODE_DEC, CPU_ADDR_ZERO_PAGE, 5},       // 0xc6
    (instruction){CPU_OPCODE_DCP, CPU_ADDR_ZERO_PAGE, 5},       // 0xc7 - unofficial
    (instruction){CPU_OPCODE_INY, CPU_ADDR_IMPLICIT, 2},        // 0xc8
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_IMMEDIATE, 2},       // 0xc9
    (instruction){CPU_OPCODE_DEX, CPU_ADDR_IMPLICIT, 2},        // 0xca
    (instruction){CPU_OPCODE_AXS, CPU_ADDR_IMMEDIATE, 2},       // 0xcb - unofficial
    (instruction){CPU_OPCODE_CPY, CPU_ADDR_ABSOLUTE, 4},        // 0xcc
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_ABSOLUTE, 4},        // 0xcd
    (instruction){CPU_OPCODE_DEC, CPU_ADDR_ABSOLUTE, 6},        // 0xce
    (instruction){CPU_OPCODE_DCP, CPU_ADDR_ABSOLUTE, 6},        // 0xcf - unofficial

    (instruction){CPU_OPCODE_BNE, CPU_ADDR_RELATIVE, 2},        // 0xd0
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_INDIRECT_Y, 5},      // 0xd1
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0xd2
    (instruction){CPU_OPCODE_DCP, CPU_ADDR_INDIRECT_Y, 8},      // 0xd3 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE_X, 4},     // 0xd4 - unofficial
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_ZERO_PAGE_X, 4},     // 0xd5
    (instruction){CPU_OPCODE_DEC, CPU_ADDR_ZERO_PAGE_X, 6},     // 0xd6
    (instruction){CPU_OPCODE_DCP, CPU_ADDR_ZERO_PAGE_X, 6},     // 0xd7 - unofficial
    (instruction){CPU_OPCODE_CLD, CPU_ADDR_IMPLICIT, 2},        // 0xd8
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_ABSOLUTE_Y, 4},      // 0xd9
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMPLICIT, 2},        // 0xda - unofficial
    (instruction){CPU_OPCODE_DCP, CPU_ADDR_ABSOLUTE_Y, 7},      // 0xdb - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ABSOLUTE_X, 4},      // 0xdc - unofficial
    (instruction){CPU_OPCODE_CMP, CPU_ADDR_ABSOLUTE_X, 4},      // 0xdd
    (instruction){CPU_OPCODE_DEC, CPU_ADDR_ABSOLUTE_X, 7},      // 0xde
    (instruction){CPU_OPCODE_DCP, CPU_ADDR_ABSOLUTE_X, 7},      // 0xdf - unofficial

    (instruction){CPU_OPCODE_CPX, CPU_ADDR_IMMEDIATE, 2},       // 0xe0
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_INDIRECT_X, 6},      // 0xe1
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMMEDIATE, 2},       // 0xe2 - unofficial
    (instruction){CPU_OPCODE_ISC, CPU_ADDR_INDIRECT_X, 8},      // 0xe3 - unofficial
    (instruction){CPU_OPCODE_CPX, CPU_ADDR_ZERO_PAGE, 3},       // 0xe4
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_ZERO_PAGE, 3},       // 0xe5
    (instruction){CPU_OPCODE_INC, CPU_ADDR_ZERO_PAGE, 5},       // 0xe6
    (instruction){CPU_OPCODE_ISC, CPU_ADDR_ZERO_PAGE, 5},       // 0xe7 - unofficial
    (instruction){CPU_OPCODE_INX, CPU_ADDR_IMPLICIT, 2},        // 0xe8
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_IMMEDIATE, 2},       // 0xe9
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMPLICIT, 2},        // 0xea
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_IMMEDIATE, 2},       // 0xeb - unofficial
    (instruction){CPU_OPCODE_CPX, CPU_ADDR_ABSOLUTE, 4},        // 0xec
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_ABSOLUTE, 4},        // 0xed
    (instruction){CPU_OPCODE_INC, CPU_ADDR_ABSOLUTE, 6},        // 0xee
    (instruction){CPU_OPCODE_ISC, CPU_ADDR_ABSOLUTE, 6},        // 0xef - unofficial

    (instruction){CPU_OPCODE_BEQ, CPU_ADDR_RELATIVE, 2},        // 0xf0
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_INDIRECT_Y, 5},      // 0xf1
    (instruction){CPU_OPCODE_STP, CPU_ADDR_IMPLICIT, 2},        // 0xf2
    (instruction){CPU_OPCODE_ISC, CPU_ADDR_INDIRECT_Y, 8},      // 0xf3 - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ZERO_PAGE_X, 4},     // 0xf4 - unofficial
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_ZERO_PAGE_X, 4},     // 0xf5
    (instruction){CPU_OPCODE_INC, CPU_ADDR_ZERO_PAGE_X, 6},     // 0xf6
    (instruction){CPU_OPCODE_ISC, CPU_ADDR_ZERO_PAGE_X, 6},     // 0xf7 - unofficial
    (instruction){CPU_OPCODE_SED, CPU_ADDR_IMPLICIT, 2},        // 0xf8
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_ABSOLUTE_Y, 4},      // 0xf9
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_IMPLICIT, 2},        // 0xfa - unofficial
    (instruction){CPU_OPCODE_ISC, CPU_ADDR_ABSOLUTE_Y, 7},      // 0xfb - unofficial
    (instruction){CPU_OPCODE_NOP, CPU_ADDR_ABSOLUTE_X, 4},      // 0xfc - unofficial
    (instruction){CPU_OPCODE_SBC, CPU_ADDR_ABSOLUTE_X, 4},      // 0xfd
    (instruction){CPU_OPCODE_INC, CPU_ADDR_ABSOLUTE_X, 7},      // 0xfe
    (instruction){CPU_OPCODE_ISC, CPU_ADDR_ABSOLUTE_X, 7}       // 0xff - unofficial
};

//Returns the value of the byte at pc and increments pc
uint8_t fetch_byte(cpu *c) {
    return mem_read(c->b, c->pc++);
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
void branch(cpu *c, uint8_t expr, size_t *cycles) {
    int8_t offset = (int8_t)fetch_byte(c);
    if(expr) {
        (*cycles)++;
        if((c->pc & 0xFF00) != ((c->pc + offset) & 0xFF00)) (*cycles)++;
        c->pc += offset;
    }
}

//Pushes a value onto the stack and decrements sp
void push(cpu* c, uint8_t val) {
    mem_write(c->b, 0x100 + c->sp, val);
    c->sp--;
}

//Pops the top value off of the stack and increments sp
uint8_t pull(cpu *c) {
    c->sp++;
    return mem_read(c->b, 0x100 + c->sp);
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
    uint8_t lo = mem_read(c->b, addr);
    uint8_t hi = mem_read(c->b, addr+1);

    c->pc = (((uint16_t)hi) << 8) | lo;
}

//Gets the two byte address stored at the next two bytes after pc
uint16_t get_addr(cpu *c) {
    uint8_t low = fetch_byte(c);
    uint16_t high = fetch_byte(c);

    return (high << 8) | low;
}

//Runs an interrupt IRQ
void interrupt_irq(cpu *c) {
    if(get_cpu_flag(c, CPU_INTERRUPT_DISABLE)) return;
    push_pc(c);
    push(c, (c->proc_stat_reg & ~(1 << 4) | (1 << 5)));
    set_cpu_flag(c, CPU_INTERRUPT_DISABLE, 1);

    set_pc(c, 0xFFFE);
    c->b->p->rom->irq_pending = 0; //Need to reset it to prevent repeat calls next frame
}

//Runs an interrupt NMI
void interrupt_nmi(cpu *c) {
    push_pc(c);
    push(c, (c->proc_stat_reg & ~(1 << 4) | (1 << 5)));
    set_cpu_flag(c, CPU_INTERRUPT_DISABLE, 1);

    set_pc(c, 0xFFFA);
    c->b->p->nmi_triggered = 0; //Need to reset it to prevent repeat calls next frame
}

//Sets the relevant flags that would occur from comparing two values
//Helper for comparison functions such as CMP, CPY, CPX
void compare(cpu *c, uint8_t v1, uint8_t v2) {
    set_cpu_flag(c, CPU_CARRY, v1 >= v2);
    set_cpu_flag(c, CPU_ZERO, v1 == v2);
    set_cpu_flag(c, CPU_NEGATIVE, (v1 - v2) & 0x80);
}

//Gets the argument for an instruction.
//get_val determines whether the returned paramter should be a memory
//address or a value
uint16_t get_argument(cpu *c, address_mode mode, uint8_t get_val, size_t *cycles) {
    uint16_t addr = 0;
    switch(mode) {
        case CPU_ADDR_ABSOLUTE:
            addr = get_addr(c);
            break;
        case CPU_ADDR_ABSOLUTE_X:
            addr = get_addr(c);
            if((*cycles) == 4) {
                if((addr & 0xFF00) != ((addr + c->x) & 0xFF00)) (*cycles)++;
            }
            addr += c->x;
            break;
        case CPU_ADDR_ABSOLUTE_Y:
            addr = get_addr(c);
            if((*cycles) == 4) {
                if((addr & 0xFF00) != ((addr + c->y) & 0xFF00)) (*cycles)++;
            }
            addr += c->y;
            break;
        case CPU_ADDR_ACCUMULATOR: //Should not do anything and be explictly handled by the case in question
            break;
        case CPU_ADDR_IMMEDIATE:
            return fetch_byte(c);
        case CPU_ADDR_IMPLICIT: //Only used by BRK
            break;
        case CPU_ADDR_INDIRECT: {
            uint16_t ptr = get_addr(c);
            uint8_t lo = mem_read(c->b, ptr);
            uint8_t hi = mem_read(c->b, (ptr & 0xFF00) | ((ptr + 1) & 0x00FF)); //wrap within page
            addr = (((uint16_t)hi) << 8) | lo;
        }
            break;
        case CPU_ADDR_INDIRECT_X:
            addr = (fetch_byte(c) + c ->x) % 256;
            addr = (((uint16_t)mem_read(c->b, (addr+1)%256)) << 8) | mem_read(c->b, addr);
            break;
        case CPU_ADDR_INDIRECT_Y:
            addr = fetch_byte(c);
            addr = (((uint16_t)mem_read(c->b, (addr+1)%256)) << 8) | mem_read(c->b, addr);
            if((*cycles) == 5) {
                if((addr & 0xFF00) != ((addr + c->y) & 0xFF00)) (*cycles)++;
            }
            addr += c->y;
            break;
        case CPU_ADDR_RELATIVE: //Don't bother as this should be handled by the branch function defined earlier
            break;
        case CPU_ADDR_ZERO_PAGE:
            addr = fetch_byte(c);
            break;
        case CPU_ADDR_ZERO_PAGE_X:
            addr = (fetch_byte(c)+ c->x) % 256;
            break;
        case CPU_ADDR_ZERO_PAGE_Y:
            addr = (fetch_byte(c) + c->y) % 256;
            break;
    }


    return get_val ? mem_read(c->b, addr) : addr;
}

size_t execute_instr(cpu *c) {
    uint8_t op = fetch_byte(c);
    instruction instr = instructions[op];
    size_t cycles = instr.num_cycles;

    switch(instr.instr) {
        //Loads value from memory address into accumulator
        case CPU_OPCODE_LDA:
            c->acc = get_argument(c, instr.addr_mode, 1, &cycles);
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
            break;
        //Loads value from memory address into x register
        case CPU_OPCODE_LDX:
            c->x = get_argument(c, instr.addr_mode, 1, &cycles);
            set_cpu_flag(c, CPU_ZERO, c->x == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->x & 0x80));
            break;
        //Loads value from memory address into y register
        case CPU_OPCODE_LDY:
            c->y = get_argument(c, instr.addr_mode, 1, &cycles);
            set_cpu_flag(c, CPU_ZERO, c->y == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->y & 0x80));
            break;
        //Stores the value in the accumulator at the given memory address
        case CPU_OPCODE_STA:
            mem_write(c->b, get_argument(c, instr.addr_mode, 0, &cycles), c->acc);
            break;
        //Stores the value in the x register at the given memory address
        case CPU_OPCODE_STX:
            // c->memory[get_argument(c, instr.addr_mode, 0)] = c->x;
            mem_write(c->b, get_argument(c, instr.addr_mode, 0, &cycles), c->x);
            break;
        //Stores the value in the y register at the given memory address
        case CPU_OPCODE_STY:
            // c->memory[get_argument(c, instr.addr_mode, 0)] = c->y;
            mem_write(c->b, get_argument(c, instr.addr_mode, 0, &cycles), c->y);
            break;
        //Transfers accumulator value into x register;
        case CPU_OPCODE_TAX:
            c->x = c->acc;
            set_cpu_flag(c, CPU_ZERO, c->x == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->x & 0x80));
            break;
        //Transfers x register value into accumulator;
        case CPU_OPCODE_TXA:
            c->acc = c->x;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
            break;
        //Transfers accumulator value into y register;
        case CPU_OPCODE_TAY:
            c->y = c->acc;
            set_cpu_flag(c, CPU_ZERO, c->y == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->y & 0x80));
            break;
        //Transfers x register value into accumulator;
        case CPU_OPCODE_TYA:
            c->acc = c->y;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
            break;
        //Adds a memory value and the carry flag to the accumulator
        //Sets the carry flag if there is overflow
        //Sets the overflow flag if there is signed overflow (result has different sign from both memory value and accumulator)
        case CPU_OPCODE_ADC: {
            uint16_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            uint16_t sum = c->acc + param + get_cpu_flag(c, CPU_CARRY);
            uint8_t trunc_sum = (uint8_t)sum;
            set_cpu_flag(c, CPU_CARRY, sum > 0xFF);
            set_cpu_flag(c, CPU_ZERO, trunc_sum == 0);
            set_cpu_flag(c, CPU_OVERFLOW, (c->acc ^ trunc_sum) & (trunc_sum ^ param) & 0x80);
            set_cpu_flag(c, CPU_NEGATIVE,  trunc_sum & 0x80);
            c->acc = trunc_sum;
        }
        break;
        //Subtracts a memory value and the carry flag to the accumulator
        //Sets the carry flag if there is overflow
        //Sets the overflow flag if there is signed overflow (result has different sign from both memory value and accumulator)
        case CPU_OPCODE_SBC: {
            uint16_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            uint16_t sum = c->acc - param - !get_cpu_flag(c, CPU_CARRY);
            uint8_t trunc_sum = (uint8_t)sum;
            set_cpu_flag(c, CPU_CARRY, !(sum > 0xFF));
            set_cpu_flag(c, CPU_ZERO, trunc_sum == 0);
            set_cpu_flag(c, CPU_OVERFLOW, (c->acc ^ trunc_sum) & (trunc_sum ^ ~param) & 0x80);
            set_cpu_flag(c, CPU_NEGATIVE,  trunc_sum & 0x80);
            c->acc = trunc_sum;
        }
        break;
        //Increments a given memory value by 1
        case CPU_OPCODE_INC: {
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t val = mem_read(c->b, param);
            val++;

            mem_write(c->b, param, val);
            set_cpu_flag(c, CPU_ZERO, val == 0);
            set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
        }
        break;
        //Decrements a given memory value by 1
        case CPU_OPCODE_DEC: {
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t val = mem_read(c->b, param);
            val--;

            mem_write(c->b, param, val);
            set_cpu_flag(c, CPU_ZERO, val == 0);
            set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
        }
        break;
        //Increments the value in the x register by 1
        case CPU_OPCODE_INX:
            c->x++;
            set_cpu_flag(c, CPU_ZERO, c->x == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->x & 0x80));
            break;
        //Decrements the value in the x register by 1
        case CPU_OPCODE_DEX:
            c->x--;
            set_cpu_flag(c, CPU_ZERO, c->x == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->x & 0x80));
            break;
        //Increments the value in the y register by 1
        case CPU_OPCODE_INY:
            c->y++;
            set_cpu_flag(c, CPU_ZERO, c->y == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->y & 0x80));
            break;
        //Decrements the value in the y register by 1
        case CPU_OPCODE_DEY:
            c->y--;
            set_cpu_flag(c, CPU_ZERO, c->y == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->y & 0x80));
            break;
        //Shifts a memory value one bit to the left,
        //Setting the carry flag to the bit shifted out
        case CPU_OPCODE_ASL:
            if(instr.addr_mode == CPU_ADDR_ACCUMULATOR) {
                set_cpu_flag(c, CPU_CARRY, (c->acc & 0x80));
                c->acc <<= 1;
                set_cpu_flag(c, CPU_ZERO, c->acc == 0);
                set_cpu_flag(c, CPU_NEGATIVE, c->acc & 0x80);
            }
            else {
                uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
                uint8_t val = mem_read(c->b, param);
                set_cpu_flag(c, CPU_CARRY, val & 0x80);
                val <<= 1;
                mem_write(c->b, param, val);
                set_cpu_flag(c, CPU_ZERO, val == 0);
                set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
            }
            break;
        //Shifts a memory value one bit to the right,
        //Setting the carry flag to the bit shifted out
        case CPU_OPCODE_LSR:
            if(instr.addr_mode == CPU_ADDR_ACCUMULATOR) {
                set_cpu_flag(c, CPU_CARRY, (c->acc & 0x1));
                c->acc >>= 1;
                set_cpu_flag(c, CPU_ZERO, c->acc == 0);
                set_cpu_flag(c, CPU_NEGATIVE, 0);
            }
            else {
                uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
                uint8_t val = mem_read(c->b, param);
                set_cpu_flag(c, CPU_CARRY, val & 0x1);
                val >>= 1;
                mem_write(c->b, param, val);
                set_cpu_flag(c, CPU_ZERO, val == 0);
                set_cpu_flag(c, CPU_NEGATIVE, 0);
            }
            break;
        //Rotates a memory value one bit to the left
        //by shifting one bit to the left and setting the MSB
        //to the bit shifted out
        //Sets the carry flag to the bit shifted out
        case CPU_OPCODE_ROL:
            if(instr.addr_mode == CPU_ADDR_ACCUMULATOR) {
                uint8_t old_carry = get_cpu_flag(c, CPU_CARRY);
                set_cpu_flag(c, CPU_CARRY, (c->acc & 0x80));
                c->acc <<= 1;
                c->acc |= old_carry;
                set_cpu_flag(c, CPU_ZERO, c->acc == 0);
                set_cpu_flag(c, CPU_NEGATIVE, c->acc & 0x80);
            }
            else {
                uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
                uint8_t old_carry = get_cpu_flag(c, CPU_CARRY);
                uint8_t val = mem_read(c->b, param);
                set_cpu_flag(c, CPU_CARRY, val & 0x80);
                val <<= 1;
                val |= old_carry;
                mem_write(c->b, param, val);
                set_cpu_flag(c, CPU_ZERO, val == 0);
                set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
            }
            break;
        //Rotates a memory value one bit to the right
        //by shifting one bit to the right and setting the LSB
        //to the bit shifted out
        //Sets the carry flag to the bit shifted out
        case CPU_OPCODE_ROR:
            if(instr.addr_mode == CPU_ADDR_ACCUMULATOR) {
                uint8_t old_carry = get_cpu_flag(c, CPU_CARRY);
                set_cpu_flag(c, CPU_CARRY, (c->acc & 0x1));
                c->acc >>= 1;
                c->acc |= (old_carry << 7);
                set_cpu_flag(c, CPU_ZERO, c->acc == 0);
                set_cpu_flag(c, CPU_NEGATIVE, c->acc & 0x80);
            }
            else {
                uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
                uint8_t old_carry = get_cpu_flag(c, CPU_CARRY);
                uint8_t val = mem_read(c->b, param);
                set_cpu_flag(c, CPU_CARRY, val & 0x1);
                val >>= 1;
                val |= (old_carry << 7);
                mem_write(c->b, param, val);
                set_cpu_flag(c, CPU_ZERO, val == 0);
                set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
            }
            break;
        //Ands the value in the accumulator and some memory value
        case CPU_OPCODE_AND: {
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            c->acc &= param;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
        }
        break;
        //Ors the value in the accumulator and some memory value
        case CPU_OPCODE_ORA: {
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            c->acc |= param;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
        }
        break;
        //Xors the value in the accumulator and some memory value
        case CPU_OPCODE_XOR: {
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            c->acc ^= param;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
        }
        break;
        //Ands the accumulator and a memory value and then sets CPU flags
        //if any bits are set, specifically NEGATIVE if bit 7 is set
        //OVERFLOW if bit 6 is set, or ZERO if the result is 0
        //Does not modfiy the value in the accumulator.
        case CPU_OPCODE_BIT: {
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            set_cpu_flag(c, CPU_NEGATIVE, (param & 0x80));
            set_cpu_flag(c, CPU_OVERFLOW, (param & 0x40));
            param &= c->acc;
            set_cpu_flag(c, CPU_ZERO, param == 0);
        }
        break;
        //Compares the accumulator to a memory value
        case CPU_OPCODE_CMP:
            compare(c, c->acc, get_argument(c, instr.addr_mode, 1, &cycles));
            break;
        //Compares the x register to a memory value
        case CPU_OPCODE_CPX:
            compare(c, c->x, get_argument(c, instr.addr_mode, 1, &cycles));
            break;
        //Compares the y register to a memory value
        case CPU_OPCODE_CPY:
            compare(c, c->y, get_argument(c, instr.addr_mode, 1, &cycles));
            break;
        //Branches if the carry flag is not set
        case CPU_OPCODE_BCC:
            branch(c, !get_cpu_flag(c, CPU_CARRY), &cycles);
            break;
        //Braches if the carry flag is set
        case CPU_OPCODE_BCS:
            branch(c, get_cpu_flag(c, CPU_CARRY), &cycles);
            break;
        //Branches if the zero flag is set
        case CPU_OPCODE_BEQ:
            branch(c, get_cpu_flag(c, CPU_ZERO), &cycles);
            break;
        //Branches if the zero flag is not set
        case CPU_OPCODE_BNE:
            branch(c, !get_cpu_flag(c, CPU_ZERO), &cycles);
            break;
        //Branches if the negative flag is not set
        case CPU_OPCODE_BPL:
            branch(c, !get_cpu_flag(c, CPU_NEGATIVE), &cycles);
            break;
        //Branches if the negative flag is set
        case CPU_OPCODE_BMI:
            branch(c, get_cpu_flag(c, CPU_NEGATIVE), &cycles);
            break;
        //Branches if the overflow flag is not set
        case CPU_OPCODE_BVC:
            branch(c, !get_cpu_flag(c, CPU_OVERFLOW), &cycles);
            break;
        //Branches if the overflow flag is set
        case CPU_OPCODE_BVS:
            branch(c, get_cpu_flag(c, CPU_OVERFLOW), &cycles);
            break;
        //Sets the pc to the memory address specified
        case CPU_OPCODE_JMP:
            c->pc = get_argument(c, instr.addr_mode, 0, &cycles);
            break;
        //Pushes the pc onto the stack and jumps to the subroutine
        //at the given memory address
        case CPU_OPCODE_JSR: {
            uint16_t addr = get_argument(c, instr.addr_mode, 0, &cycles);
            uint16_t ret = c->pc - 1;
            push(c, ret >> 8);
            push(c, ret & 0xFF);
            c->pc = addr;
        }
        break;
        //Returns from a subroutine by pulling the pc from the stack
        case CPU_OPCODE_RTS:
            pull_pc(c);
            c->pc++;
            break;
        //Pushes the high and low bytes of the pc to the stack (separately);
        //Pushes the processor status register with the Break flag set to true to the stack
        //Disables interrupts
        //Loads the pc low byte from 0xFFFE and the high byte from 0xFFFF
        case CPU_OPCODE_BRK:
            if(instr.addr_mode == CPU_ADDR_IMPLICIT) fetch_byte(c); //Need to fetch dummy opcode since BRK behaves as a 2 byte instruction even though it is only 1 byte
            push_pc(c);
            push(c, (c->proc_stat_reg | (1 << 4)));
            set_cpu_flag(c, CPU_INTERRUPT_DISABLE, 1);
            set_pc(c, 0xFFFE);
            break;
        //Returns from an interrupt by pulling the flags from
        //the stack and then pulling the old pc from the stack
        case CPU_OPCODE_RTI:
            c->proc_stat_reg = pull(c);
            pull_pc(c);
            break;
        //Pushes the accumulator to the stack
        case CPU_OPCODE_PHA:
            push(c, c->acc);
            break;
        //Pulls the accumulator from the stack
        case CPU_OPCODE_PLA:
            c->acc = pull(c);
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
            break;
        //Pushes the status register to the stack
        case CPU_OPCODE_PHP:
            push(c, c->proc_stat_reg | 0x30);
            break;
        //Pulls the status register from the stack
        case CPU_OPCODE_PLP:
            c->proc_stat_reg = pull(c);
            break;
        //Sets the stack pointer to the value of the x register
        case CPU_OPCODE_TXS:
            c->sp = c->x;
            break;
        //Sets the x register to the value of the stack pointer
        case CPU_OPCODE_TSX:
            c->x = c->sp;
            set_cpu_flag(c, CPU_ZERO, c->x == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->x & 0x80));
            break;
        //Clears the carry flag
        case CPU_OPCODE_CLC:
            set_cpu_flag(c, CPU_CARRY, 0);
            break;
        //Sets the carry flag
        case CPU_OPCODE_SEC:
            set_cpu_flag(c, CPU_CARRY, 1);
            break;
        //Clears the interrupt disable flag
        case CPU_OPCODE_CLI:
            set_cpu_flag(c, CPU_INTERRUPT_DISABLE, 0);
            break;
        //Sets the interrupt disable flag
        case CPU_OPCODE_SEI:
            set_cpu_flag(c, CPU_INTERRUPT_DISABLE, 1);
            break;
        //Clears the decimal flag
        case CPU_OPCODE_CLD:
            set_cpu_flag(c, CPU_DECIMAL, 0);
            break;
        //Sets the decimal flag
        case CPU_OPCODE_SED:
            set_cpu_flag(c, CPU_DECIMAL, 1);
            break;
        //Clears the overflow flag
        case CPU_OPCODE_CLV:
            set_cpu_flag(c, CPU_OVERFLOW, 0);
            break;
        //Do nothing. Its in the name
        case CPU_OPCODE_NOP:
            //Some of the invalid NOP-like operations still change the pc and consume bytes when addressing
            //So make an unused called to get_argument to simulate this
            get_argument(c, instr.addr_mode, 0, &cycles);
            break;
        //============================Illegal opcodes=============================
        //Stop the program early
        case CPU_OPCODE_STP:
            c->stop = 1;
            break;
        //ASL + ORA
        case CPU_OPCODE_SLO: {
            //ASL
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t val = mem_read(c->b, param);
            set_cpu_flag(c, CPU_CARRY, val & 0x80);
            val <<= 1;
            mem_write(c->b, param, val);

            //ORA
            c->acc |= val;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
        }
        break;
        //AND then set carry flag to bit 7
        case CPU_OPCODE_ANC: {
            //AND
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            c->acc &= param;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));

            //Setting carry flag to bit 7
            set_cpu_flag(c, CPU_CARRY, c->acc & 0x80);
        }
        break;
        //ROL + AND
        case CPU_OPCODE_RLA: {
            //ROL
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t old_carry = get_cpu_flag(c, CPU_CARRY);
            uint8_t val = mem_read(c->b, param);
            set_cpu_flag(c, CPU_CARRY, val & 0x80);
            val <<= 1;
            val |= old_carry;
            mem_write(c->b, param, val);

            //AND
            c->acc &= val;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
        }
        break;
        //LSR + XOR
        case CPU_OPCODE_SRE: {
            //LSR
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t val = mem_read(c->b, param);
            set_cpu_flag(c, CPU_CARRY, val & 0x1);
            val >>= 1;
            mem_write(c->b, param, val);
            set_cpu_flag(c, CPU_ZERO, val == 0);
            set_cpu_flag(c, CPU_NEGATIVE, 0);

            //XOR
            c->acc ^= val;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
        }
        break;
        //AND + LSR
        case CPU_OPCODE_ALR: {
            //AND
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            c->acc &= param;

            //LSR
            set_cpu_flag(c, CPU_CARRY, c->acc & 0x1);
            c->acc >>= 1;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, 0);
        }
        break;
        //AND then ROR
        case CPU_OPCODE_ARR:{
            //AND
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            c->acc &= param;

            //ROR:
            uint8_t old_carry = get_cpu_flag(c, CPU_CARRY);
            set_cpu_flag(c, CPU_CARRY, (c->acc & 0x1));
            c->acc >>= 1;
            c->acc |= (old_carry << 7);
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, c->acc & 0x80);
        }
        break;
        //ROR + ADC
        case CPU_OPCODE_RRA: {
            //ROR
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t old_carry = get_cpu_flag(c, CPU_CARRY);
            uint8_t val = mem_read(c->b, param);
            set_cpu_flag(c, CPU_CARRY, val & 0x1);
            val >>= 1;
            val |= (old_carry << 7);
            mem_write(c->b, param, val);

            //ADC
            uint16_t sum = c->acc + val + get_cpu_flag(c, CPU_CARRY);
            uint8_t trunc_sum = (uint8_t)sum;
            set_cpu_flag(c, CPU_CARRY, sum > 0xFF);
            set_cpu_flag(c, CPU_ZERO, trunc_sum == 0);
            set_cpu_flag(c, CPU_OVERFLOW, (~(c->acc ^ val) & (c->acc ^ trunc_sum) & 0x80));
            set_cpu_flag(c, CPU_NEGATIVE,  trunc_sum & 0x80);
            c->acc = trunc_sum;
        }
        break;
        //A AND X stored in some memory location
        case CPU_OPCODE_SAX: {
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t val = c->acc & c->x;
            mem_write(c->b, param, val);
        }
        break;
        //Apparently you shouldn't use this instruction: https://masswerk.at/nowgobang/2021/6502-illegal-opcodes
        //(A OR CONST) AND X AND oper -> A
        //Where const is one of 0x00, 0xFF, 0xEE, or others, depending on the chip series/temperature
        //I am just going to default it to 0xFF for now
        case CPU_OPCODE_XAA: {
            uint8_t cnst = 0xFF;
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            c->acc = (c->acc | cnst) & c->x & param;

            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, c->acc & 0x80);
        }
        break;
        //Stores ACC AND X AND (High byte of memory address + 1) at the given memory address
        case CPU_OPCODE_AHX: {
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t hi = (param & 0xFF00) >> 8;
            uint8_t val = c->acc & c->x & (hi + 1);
            mem_write(c->b, param, val);

            set_cpu_flag(c, CPU_ZERO, val == 0);
            set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
        }
        break;
        //A AND X -> SP, A AND X AND (high byte of memory address + 1) at address
        case CPU_OPCODE_TAS: {
            c->sp = c->acc & c->x;

            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t hi = (param & 0xFF00) >> 8;
            uint8_t val = c->acc & c->x & (hi + 1);
            mem_write(c->b, param, val);

            set_cpu_flag(c, CPU_ZERO, val == 0);
            set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
        }
        break;
        //LDA + LDX
        case CPU_OPCODE_LAX:
            c->acc = get_argument(c, instr.addr_mode, 1, &cycles);
            c->x = c->acc;
            set_cpu_flag(c, CPU_ZERO, c->acc == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (c->acc & 0x80));
            break;
        //Memory AND SP -> A, X, SP
        case CPU_OPCODE_LAS: {
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            param &= c->sp;
            c->acc = param;
            c->x = param;
            c->sp = param;

            set_cpu_flag(c, CPU_ZERO, param == 0);
            set_cpu_flag(c, CPU_NEGATIVE, (param & 0x80));
        }
        break;
        //DEC + CMP
        case CPU_OPCODE_DCP: {
            //DEC
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t val = mem_read(c->b, param);
            val--;
            mem_write(c->b, param, val);

            //CMP
            compare(c, c->acc, val);
        }
        break;
        //CMP and DEC simultaneously
        case CPU_OPCODE_AXS: {
            uint8_t param = get_argument(c, instr.addr_mode, 1, &cycles);
            uint8_t val = c->acc & c->x;
            compare(c, val, param);
            c->x = val - param;
        }
        break;
        //INC + SBC
        case CPU_OPCODE_ISC: {
            //INC
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t val = mem_read(c->b, param);
            val++;
            mem_write(c->b, param, val);

            //SBC
            uint16_t sum = c->acc - val - !get_cpu_flag(c, CPU_CARRY);
            uint8_t trunc_sum = (uint8_t)sum;
            set_cpu_flag(c, CPU_CARRY, !(sum > 0xFF));
            set_cpu_flag(c, CPU_ZERO, trunc_sum == 0);
            set_cpu_flag(c, CPU_OVERFLOW, (c->acc ^ trunc_sum) & (trunc_sum ^ ~val) & 0x80);
            set_cpu_flag(c, CPU_NEGATIVE,  trunc_sum & 0x80);
            c->acc = trunc_sum;
        }
        break;
        //Stores X and (high byte of memory address + 1) at given memory address
        case CPU_OPCODE_SHX: {
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t hi = (param & 0xFF00) >> 8;
            uint8_t val = c->x & (hi + 1);
            mem_write(c->b, param, val);

            set_cpu_flag(c, CPU_ZERO, val == 0);
            set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
        }
        break;
        //Stores Y and (high byte of memory address + 1) at given memory address
        case CPU_OPCODE_SHY: {
            uint16_t param = get_argument(c, instr.addr_mode, 0, &cycles);
            uint8_t hi = (param & 0xFF00) >> 8;
            uint8_t val = c->y & (hi + 1);
            mem_write(c->b, param, val);

            set_cpu_flag(c, CPU_ZERO, val == 0);
            set_cpu_flag(c, CPU_NEGATIVE, val & 0x80);
        }
        break;
        default:
            printf("Opcode not supported\n");
            break;
    }
    return cycles;
}

//Prints the state of the cpu, meaning the contents of its registers, the value of sp and pc, and the state of all flags
void print_cpu_state(cpu *c) {
    printf("Accumulator: %hhu \nX Register: %hhu \nY Register: %hhu \nStack Pointer: %hhu\nPC: %hu\n", c->acc, c->x, c->y, c->sp, c->pc);
    printf("Status Register:\n Carry: %hhu\n Zero: %hhu\n Interrupt Disable: %hhu\n Decimal: %hhu\n Break: %hhu\n Overflow: %hhu\n Negative: %hhu\n",
           get_cpu_flag(c, CPU_CARRY), get_cpu_flag(c, CPU_ZERO), get_cpu_flag(c, CPU_INTERRUPT_DISABLE), get_cpu_flag(c, CPU_DECIMAL),
           get_cpu_flag(c, CPU_BREAK), get_cpu_flag(c, CPU_OVERFLOW), get_cpu_flag(c, CPU_NEGATIVE));
}

//Prints the current contents of the stack in a numbered list with the most recently added value at the top
void print_stack(cpu *c) {
    printf("Addr    Val\n");
    for(int i = c->sp+1; i < 256; i++) {
        printf(" %03X     %02X\n", 0x100 + i, mem_read(c->b, 0x100 + i));
    }
}

//Prints the entire contents of a given page in memory in byte form in a nice square block
void print_page(cpu *c, uint8_t page_num) {
    uint16_t start_addr = page_num * 256;

    for(int i = 0; i < 256; i++) {
        if(i > 0 && i % 16 == 0) printf("\n");
        printf("%02x ", mem_read(c->b, start_addr+i));
    }
    printf("\n");
}

int append_disassembly_string(cpu *c, char *buf) {
    uint16_t disassembly_pc = c->pc;
    uint8_t opcode = mem_read(c->b, disassembly_pc++);

    instruction instr = instructions[opcode];

    char *opcode_str;

    switch(instr.instr) {
        case CPU_OPCODE_LDA: opcode_str = "LDA"; break;
        case CPU_OPCODE_LDX: opcode_str = "LDX"; break;
        case CPU_OPCODE_LDY: opcode_str = "LDY"; break;
        case CPU_OPCODE_STA: opcode_str = "STA"; break;
        case CPU_OPCODE_STX: opcode_str = "STX"; break;
        case CPU_OPCODE_STY: opcode_str = "STY"; break;
        case CPU_OPCODE_TAX: opcode_str = "TAX"; break;
        case CPU_OPCODE_TXA: opcode_str = "TXA"; break;
        case CPU_OPCODE_TAY: opcode_str = "TAY"; break;
        case CPU_OPCODE_TYA: opcode_str = "TYA"; break;
        case CPU_OPCODE_ADC: opcode_str = "ADC"; break;
        case CPU_OPCODE_SBC: opcode_str = "SBC"; break;
        case CPU_OPCODE_INC: opcode_str = "INC"; break;
        case CPU_OPCODE_DEC: opcode_str = "DEC"; break;
        case CPU_OPCODE_INX: opcode_str = "INX"; break;
        case CPU_OPCODE_DEX: opcode_str = "DEX"; break;
        case CPU_OPCODE_INY: opcode_str = "INY"; break;
        case CPU_OPCODE_DEY: opcode_str = "DEY"; break;
        case CPU_OPCODE_ASL: opcode_str = "ASL"; break;
        case CPU_OPCODE_LSR: opcode_str = "LSR"; break;
        case CPU_OPCODE_ROL: opcode_str = "ROL"; break;
        case CPU_OPCODE_ROR: opcode_str = "ROR"; break;
        case CPU_OPCODE_AND: opcode_str = "AND"; break;
        case CPU_OPCODE_ORA: opcode_str = "ORA"; break;
        case CPU_OPCODE_XOR: opcode_str = "XOR"; break;
        case CPU_OPCODE_BIT: opcode_str = "BIT"; break;
        case CPU_OPCODE_CMP: opcode_str = "CMP"; break;
        case CPU_OPCODE_CPX: opcode_str = "CPX"; break;
        case CPU_OPCODE_CPY: opcode_str = "CPY"; break;
        case CPU_OPCODE_BCC: opcode_str = "BCC"; break;
        case CPU_OPCODE_BCS: opcode_str = "BCS"; break;
        case CPU_OPCODE_BEQ: opcode_str = "BEQ"; break;
        case CPU_OPCODE_BNE: opcode_str = "BNE"; break;
        case CPU_OPCODE_BPL: opcode_str = "BPL"; break;
        case CPU_OPCODE_BMI: opcode_str = "BMI"; break;
        case CPU_OPCODE_BVC: opcode_str = "BVC"; break;
        case CPU_OPCODE_BVS: opcode_str = "BVS"; break;
        case CPU_OPCODE_JMP: opcode_str = "JMP"; break;
        case CPU_OPCODE_JSR: opcode_str = "JSR"; break;
        case CPU_OPCODE_RTS: opcode_str = "RTS"; break;
        case CPU_OPCODE_BRK: opcode_str = "BRK"; break;
        case CPU_OPCODE_RTI: opcode_str = "RTI"; break;
        case CPU_OPCODE_PHA: opcode_str = "PHA"; break;
        case CPU_OPCODE_PLA: opcode_str = "PLA"; break;
        case CPU_OPCODE_PHP: opcode_str = "PHP"; break;
        case CPU_OPCODE_PLP: opcode_str = "PLP"; break;
        case CPU_OPCODE_TXS: opcode_str = "TXS"; break;
        case CPU_OPCODE_TSX: opcode_str = "TSX"; break;
        case CPU_OPCODE_CLC: opcode_str = "CLC"; break;
        case CPU_OPCODE_SEC: opcode_str = "SEC"; break;
        case CPU_OPCODE_CLI: opcode_str = "CLI"; break;
        case CPU_OPCODE_SEI: opcode_str = "SEI"; break;
        case CPU_OPCODE_CLD: opcode_str = "CLD"; break;
        case CPU_OPCODE_SED: opcode_str = "SED"; break;
        case CPU_OPCODE_CLV: opcode_str = "CLV"; break;
        case CPU_OPCODE_NOP: opcode_str = "NOP"; break;
        case CPU_OPCODE_STP: opcode_str = "STP"; break;
        case CPU_OPCODE_SLO: opcode_str = "SLO"; break;
        case CPU_OPCODE_ANC: opcode_str = "ANC"; break;
        case CPU_OPCODE_RLA: opcode_str = "RLA"; break;
        case CPU_OPCODE_SRE: opcode_str = "SRE"; break;
        case CPU_OPCODE_ALR: opcode_str = "ALR"; break;
        case CPU_OPCODE_ARR: opcode_str = "ARR"; break;
        case CPU_OPCODE_RRA: opcode_str = "RRA"; break;
        case CPU_OPCODE_SAX: opcode_str = "SAX"; break;
        case CPU_OPCODE_XAA: opcode_str = "XAA"; break;
        case CPU_OPCODE_AHX: opcode_str = "AHX"; break;
        case CPU_OPCODE_TAS: opcode_str = "TAS"; break;
        case CPU_OPCODE_LAX: opcode_str = "LAX"; break;
        case CPU_OPCODE_LAS: opcode_str = "LAS"; break;
        case CPU_OPCODE_DCP: opcode_str = "DCP"; break;
        case CPU_OPCODE_AXS: opcode_str = "AXS"; break;
        case CPU_OPCODE_ISC: opcode_str = "ISC"; break;
        case CPU_OPCODE_SHX: opcode_str = "SHX"; break;
        case CPU_OPCODE_SHY: opcode_str = "SHY"; break;
    }

    int len;
    switch(instr.addr_mode) {
        case CPU_ADDR_ABSOLUTE: {
            uint8_t lo = mem_read(c->b, disassembly_pc++);
            uint16_t hi = mem_read(c->b, disassembly_pc++);
            uint16_t addr = (hi << 8) | lo;

            len = sprintf(buf, "%04X : %s $%04X\n", c->pc, opcode_str, addr);
        }
        break;
        case CPU_ADDR_ABSOLUTE_X: {
            uint8_t lo = mem_read(c->b, disassembly_pc++);
            uint16_t hi = mem_read(c->b, disassembly_pc++);
            uint16_t addr = (hi << 8) | lo;

            len = sprintf(buf, "%04X : %s $%04X, x\n", c->pc, opcode_str, addr);
        } break;
        case CPU_ADDR_ABSOLUTE_Y: {
            uint8_t lo = mem_read(c->b, disassembly_pc++);
            uint16_t hi = mem_read(c->b, disassembly_pc++);
            uint16_t addr = (hi << 8) | lo;

            len = sprintf(buf, "%04X : %s $%04X, y\n", c->pc, opcode_str, addr);
        }break;
        case CPU_ADDR_ACCUMULATOR: {
            len = sprintf(buf, "%04X : %s A\n", c->pc, opcode_str);
        } break;
        case CPU_ADDR_IMMEDIATE: {
            uint8_t val = mem_read(c->b, disassembly_pc++);

            len = sprintf(buf, "%04X : %s #$%02X\n", c->pc, opcode_str, val);
        } break;
        case CPU_ADDR_IMPLICIT:
            len = sprintf(buf, "%04X : %s\n", c->pc, opcode_str);
            break;
        case CPU_ADDR_INDIRECT: {
            uint8_t lo = mem_read(c->b, disassembly_pc++);
            uint16_t hi = mem_read(c->b, disassembly_pc++);
            uint16_t addr = (hi << 8) | lo;

            len = sprintf(buf, "%04X : %s ($%04X)\n", c->pc, opcode_str, addr);
        } break;
        case CPU_ADDR_INDIRECT_X: {
            uint8_t val = mem_read(c->b, disassembly_pc++);

            len = sprintf(buf, "%04X : %s ($%02X, x)\n", c->pc, opcode_str, val);
        } break;
        case CPU_ADDR_INDIRECT_Y: {
            uint8_t val = mem_read(c->b, disassembly_pc++);

            len = sprintf(buf, "%04X : %s ($%02X), y\n", c->pc, opcode_str, val);
        } break;
        case CPU_ADDR_RELATIVE: {
            uint8_t val = mem_read(c->b, disassembly_pc++);

            len = sprintf(buf, "%04X : %s $%02X\n", c->pc, opcode_str, val);
        } break;
        case CPU_ADDR_ZERO_PAGE: {
            uint8_t val = mem_read(c->b, disassembly_pc++);

            len = sprintf(buf, "%04X : %s $%02X\n", c->pc, opcode_str, val);
        } break;
        case CPU_ADDR_ZERO_PAGE_X: {
            uint8_t val = mem_read(c->b, disassembly_pc++);

            len = sprintf(buf, "%04X : %s $%02X, x\n", c->pc, opcode_str, val);
        } break;
        case CPU_ADDR_ZERO_PAGE_Y: {
            uint8_t val = mem_read(c->b, disassembly_pc++);

            len = sprintf(buf, "%04X : %s $%02X, y\n", c->pc, opcode_str, val);
        } break;
    }

    return len;
}
