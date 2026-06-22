#include <stdint.h>

#define MEMORY_SIZE 65536

typedef struct {
    uint8_t acc;
    uint8_t x, y;
    uint8_t proc_stat_reg;
    uint8_t sp;
    uint16_t pc;
    uint8_t memory[MEMORY_SIZE];
} cpu;

typedef enum {
    CARRY,
    ZERO,
    INTERRUPT_DISABLE,
    DECIMAL,
    BREAK,
    UNUSED,
    OVERFLOW,
    NEGATIVE
} cpu_flag;

typedef enum {
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    ACCUMULATOR,
    IMMEDIATE,
    IMPLICIT,
    INDIRECT,
    INDIRECT_X,
    INDIRECT_Y,
    RELATIVE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
} address_mode;

typedef enum {
    // Load and store
    LDA,
    LDX,
    LDY,
    STA,
    STX,
    STY,

    //Transfer
    TAX,
    TXA,
    TAY,
    TYA,

    //Arithmetic
    ADC,
    SBC,
    INC,
    DEC,
    INX,
    DEX,
    INY,
    DEY,

    //Shift
    ASL,
    LSR,
    ROL,
    ROR,

    //Bitwise
    AND,
    ORA,
    XOR,
    BIT,

    //Compare
    CMP,
    CPX,
    CPY,

    //Branch
    BCC,
    BCS,
    BEQ,
    BNE,
    BPL,
    BMI,
    BVC,
    BVS,

    //Jump
    JMP,
    JSR,
    RTS,
    BRK,
    RTI,

    //Stack
    PHA,
    PLA,
    PHP,
    PLP,
    TXS,
    TSX,

    //Flags
    CLC,
    SEC,
    CLI,
    SEI,
    CLD,
    SED,
    CLV,

    NOP,
    STP,
    SLO,
    ANC,
    RLA,
    SRE,
    ALR,
    ARR,
    RRA,
    SAX,
    XAA,
    AHX,
    TAS,
    LAX,
    LAS,
    DCP,
    AXS,
    ISC,
    SHX,
    SHY
} opcode;

typedef struct {
    opcode instr;
    address_mode addr_mode;
    uint8_t num_cycles;
} instruction;

//Array holding all of the instructions for the 6502
extern instruction instructions[256];
