#ifndef __CPU_H__
#define __CPU_H__
#include <stdint.h>
#define MEMORY_SIZE 65536
#define ROM_START 0x200

typedef enum {
    CARRY,
    ZERO,
    INTERRPUT_DISABLE,
    DECIMAL,
    OVERFLOW,
    NEGATIVE,
    B
} status_flag;

typedef enum {
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    ACCUMULATOR,
    RELATIVE,
    INDIRECT,
    INDIRECT_X,
    INDIRECT_Y,
    IMMEDIATE,
    IMPLICIT,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y
} address_mode;

typedef enum {
    //Load/Store operations:
    LDA,
    LDX,
    LDY,
    STA,
    STX,
    STY,

    //Register transfers
    TAX,
    TAY,
    TXA,
    TYA,

    //Stack operations:
    TSX,
    TXS,
    PHA,
    PHP,
    PLA,
    PLP,

    //Logical:
    AND,
    XOR,
    ORA,
    BIT,

    //Arithmetic:
    ADC,
    SBC,
    CMP,
    CPX,
    CPY,

    //Increments and decrements:
    INC,
    INX,
    INY,
    DEC,
    DEX,
    DEY,

    //Shifts:
    ASL,
    LSR,
    ROL,
    ROR,

    //Jumps and Calls:
    JMP,
    JSR,
    RTS,

    //Branches:
    BCC,
    BCS,
    BEQ,
    BMI,
    BNE,
    BPL,
    BVC,
    BVS,

    //Status Flag Changes:
    CLC,
    CLD,
    CLI,
    CLV,
    SEC,
    SED,
    SEI,

    //System functions:
    BRK,
    NOP,
    RTI,

    //Unofficial opcodes:
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
    STP,
    SHX,
    SHY
} opcode;

typedef struct cpu cpu;
typedef struct {
    uint16_t value;
    uint16_t addr;
    uint8_t byte;
} cycle_data;

typedef struct {
    opcode instr;
    address_mode addr;
    uint8_t num_cycles;
} instruction;

typedef struct {
    opcode instr;
    void (*cyclefunc[8])(cpu *c, cycle_data *d);
    uint8_t length;
} instruction_new;

extern instruction instructions[256];

//Functions as a sort of linked-list store of functions that need one cycle to complete for each opcode
typedef struct {
    uint16_t data; //Current data from the previous function in the store
    uint8_t header; //Points to the first function
    uint8_t tailer; //Points to the last function
    void (*cyclefunc[16])(cpu *c, cycle_data *d);
} cycle_instructions;

struct cpu {
    uint8_t accumulator;
    uint8_t x;
    uint8_t y;
    uint8_t processor_status_register;
    uint8_t stack_pointer;
    // uint8_t instruction_completed;
    uint16_t pc;
    // cycle_instructions cycle_instructions;
    uint8_t memory[MEMORY_SIZE];
};

//Functions to add to the cycle linked list:
// uint8_t read_memory(cpu *c, uint16_t data);
// uint8_t write_memory(cpu *c, uint16_t data);
// uint8_t dummy(cpu *c, uint16_t data);
// uint8_t push_stack_data(cpu *c, uint16_t data);
// uint8_t increment_stack_pointer(cpu *c, uint16_t data);
// uint8_t pop_stack_data(cpu *c, uint16_t data);

int init(cpu *c);
int load(cpu *c);
void execute_cycle(cpu *c);
void execute_old(cpu *c, uint8_t instr);
void execute(cpu *c, uint8_t instr, int *num_cycles);

#endif
