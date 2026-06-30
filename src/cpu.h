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
    CPU_CARRY,
    CPU_ZERO,
    CPU_INTERRUPT_DISABLE,
    CPU_DECIMAL,
    CPU_BREAK,
    CPU_UNUSED,
    CPU_OVERFLOW,
    CPU_NEGATIVE
} cpu_flag;

typedef enum {
    CPU_ADDR_ABSOLUTE,
    CPU_ADDR_ABSOLUTE_X,
    CPU_ADDR_ABSOLUTE_Y,
    CPU_ADDR_ACCUMULATOR,
    CPU_ADDR_IMMEDIATE,
    CPU_ADDR_IMPLICIT,
    CPU_ADDR_INDIRECT,
    CPU_ADDR_INDIRECT_X,
    CPU_ADDR_INDIRECT_Y,
    CPU_ADDR_RELATIVE,
    CPU_ADDR_ZERO_PAGE,
    CPU_ADDR_ZERO_PAGE_X,
    CPU_ADDR_ZERO_PAGE_Y,
} address_mode;

typedef enum {
    // Load and store
    CPU_OPCODE_LDA,
    CPU_OPCODE_LDX,
    CPU_OPCODE_LDY,
    CPU_OPCODE_STA,
    CPU_OPCODE_STX,
    CPU_OPCODE_STY,

    //Transfer
    CPU_OPCODE_TAX,
    CPU_OPCODE_TXA,
    CPU_OPCODE_TAY,
    CPU_OPCODE_TYA,

    //Arithmetic
    CPU_OPCODE_ADC,
    CPU_OPCODE_SBC,
    CPU_OPCODE_INC,
    CPU_OPCODE_DEC,
    CPU_OPCODE_INX,
    CPU_OPCODE_DEX,
    CPU_OPCODE_INY,
    CPU_OPCODE_DEY,

    //Shift
    CPU_OPCODE_ASL,
    CPU_OPCODE_LSR,
    CPU_OPCODE_ROL,
    CPU_OPCODE_ROR,

    //Bitwise
    CPU_OPCODE_AND,
    CPU_OPCODE_ORA,
    CPU_OPCODE_XOR,
    CPU_OPCODE_BIT,

    //Compare
    CPU_OPCODE_CMP,
    CPU_OPCODE_CPX,
    CPU_OPCODE_CPY,

    //Branch
    CPU_OPCODE_BCC,
    CPU_OPCODE_BCS,
    CPU_OPCODE_BEQ,
    CPU_OPCODE_BNE,
    CPU_OPCODE_BPL,
    CPU_OPCODE_BMI,
    CPU_OPCODE_BVC,
    CPU_OPCODE_BVS,

    //Jump
    CPU_OPCODE_JMP,
    CPU_OPCODE_JSR,
    CPU_OPCODE_RTS,
    CPU_OPCODE_BRK,
    CPU_OPCODE_RTI,

    //Stack
    CPU_OPCODE_PHA,
    CPU_OPCODE_PLA,
    CPU_OPCODE_PHP,
    CPU_OPCODE_PLP,
    CPU_OPCODE_TXS,
    CPU_OPCODE_TSX,

    //Flags
    CPU_OPCODE_CLC,
    CPU_OPCODE_SEC,
    CPU_OPCODE_CLI,
    CPU_OPCODE_SEI,
    CPU_OPCODE_CLD,
    CPU_OPCODE_SED,
    CPU_OPCODE_CLV,

    CPU_OPCODE_NOP,

    //Unofficial opcodes
    CPU_OPCODE_STP,
    CPU_OPCODE_SLO,
    CPU_OPCODE_ANC,
    CPU_OPCODE_RLA,
    CPU_OPCODE_SRE,
    CPU_OPCODE_ALR,
    CPU_OPCODE_ARR,
    CPU_OPCODE_RRA,
    CPU_OPCODE_SAX,
    CPU_OPCODE_XAA,
    CPU_OPCODE_AHX,
    CPU_OPCODE_TAS,
    CPU_OPCODE_LAX,
    CPU_OPCODE_LAS,
    CPU_OPCODE_DCP,
    CPU_OPCODE_AXS,
    CPU_OPCODE_ISC,
    CPU_OPCODE_SHX,
    CPU_OPCODE_SHY
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
