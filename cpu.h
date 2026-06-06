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
    LDA,
    LDX,
    LDY,
    STA,
    STX,
    STY,
    TAX,
    TXA,
    TAY,
    TYA,
    INX,
    DEX,
    INY,
    DEY,
    AND,
    ORA,
    XOR,
    BIT,
    BNE,
    BRK,
    JMP,
} opcode;

