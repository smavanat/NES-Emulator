#ifndef __CPU_H__
#define __CPU_H__
#include <stddef.h>
#include <stdint.h>
#include "bus.h"

#define MEMORY_SIZE 65536
#define CPU_CYCLES_PER_FRAME 29780

typedef struct {
    uint8_t acc;
    uint8_t x, y;
    uint8_t proc_stat_reg;
    uint8_t sp;
    uint8_t stop;
    uint16_t pc;
    bus *b;
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

//Uses the address given as a pointer
//to set the pc to a two byte address
//starting at the pointer
void set_pc(cpu *c, uint16_t addr);
//Returns the bit stored at the given flag
uint8_t get_cpu_flag(cpu *c, cpu_flag fl);
//Sets a cpu flag to a 1 if the given condition is true, other wise 0
void set_cpu_flag(cpu *c, cpu_flag fl, uint8_t cond);

//Debug function to print the given page of a CPU
void print_page(cpu *c, uint8_t page_num);
//Debug function to print the stack
void print_stack(cpu *c);
//Debug function to print the status of the registers in the CPU
void print_cpu_state(cpu *c);
//Main execution function of the CPU
size_t execute_instr(cpu *c);
//Starts a NMI interrupt
void interrupt_nmi(cpu *c);
//Starts an IRQ interrupt
void interrupt_irq(cpu *c);

//Array holding all of the instructions for the 6502
extern instruction instructions[256];
#endif //__CPU_H__
