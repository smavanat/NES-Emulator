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
    BRK,

    NOP,
} opcode;

