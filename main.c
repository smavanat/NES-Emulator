#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

//Logical operators
//Mathematical operators

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
uint8_t pop(cpu *c) {
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

//Sets the pc to the little endian 2 byte value stored at the given address and its successor
void set_pc(cpu *c, uint16_t addr) {
    c->pc = (c->memory[addr+1] << 8) | c->memory[addr];
}

void compare(cpu *c, uint8_t v1, uint8_t v2) {
    set_cpu_flag(c, CARRY, v1 >= v2);
    set_cpu_flag(c, ZERO, v1 == v2);
    set_cpu_flag(c, NEGATIVE, (v1 - v2) & 0x80);
}

void execute_instr(cpu *c, opcode op) {
    uint16_t param[4]; //Sticking with immediate mode addressing for all instructions for now
    switch(op) {
        //Loads value from memory address into accumulator
        case LDA:
            param[0] = fetch_byte(c);
            c->acc = param[0];
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Loads value from memory address into x register
        case LDX:
            param[0] = fetch_byte(c);
            c->x = param[0];
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Loads value from memory address into y register
        case LDY:
            param[0] = fetch_byte(c);
            c->y = param[0];
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Stores the value in the accumulator at the given memory address
        case STA:
            param[0] = fetch_byte(c);
            c->memory[param[0]] = c->acc;
            break;
        //Stores the value in the x register at the given memory address
        case STX:
            param[0] = fetch_byte(c);
            c->memory[param[0]] = c->x;
            break;
        //Stores the value in the y register at the given memory address
        case STY:
            param[0] = fetch_byte(c);
            c->memory[param[0]] = c->y;
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
            param[0] = fetch_byte(c);
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
            param[0] = fetch_byte(c);
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
            param[0] = fetch_byte(c);
            c->memory[param[0]]++;
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Decrements a given memory value by 1
        case DEC:
            param[0] = fetch_byte(c);
            c->memory[param[0]]--;
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
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
            param[0] = fetch_byte(c);
            set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x80));
            c->memory[param[0]] <<= 1;
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, c->memory[param[0]] & 0x80);
            break;
        //Shifts a memory value one bit to the right,
        //Setting the carry flag to the bit shifted out
        case LSR:
            param[0] = fetch_byte(c);
            set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x1));
            c->memory[param[0]] >>= 1;
            c->memory[param[0]] |= (get_cpu_flag(c, CARRY) << 7);
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, 0);
            break;
        //Rotates a memory value one bit to the left
        //by shifting one bit to the left and setting the MSB
        //to the bit shifted out
        //Sets the carry flag to the bit shifted out
        case ROL:
            param[0] = fetch_byte(c);
            set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x80));
            c->memory[param[0]] <<= 1;
            c->memory[param[0]] |= get_cpu_flag(c, CARRY);
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, c->memory[param[0]] & 0x80);
            break;
        //Rotates a memory value one bit to the right
        //by shifting one bit to the right and setting the LSB
        //to the bit shifted out
        //Sets the carry flag to the bit shifted out
        case ROR:
            param[0] = fetch_byte(c);
            set_cpu_flag(c, CARRY, (c->memory[param[0]] & 0x1));
            c->memory[param[0]] >>= 1;
            set_cpu_flag(c, ZERO, c->memory[param[0]] == 0);
            set_cpu_flag(c, NEGATIVE, c->memory[param[0]] & 0x80);
            break;
        //Ands the value in the accumulator and some memory value
        case AND:
            param[0] = fetch_byte(c);
            c->acc &= param[0];
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Ors the value in the accumulator and some memory value
        case ORA:
            param[0] = fetch_byte(c);
            c->acc |= param[0];
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Xors the value in the accumulator and some memory value
        case XOR:
            param[0] = fetch_byte(c);
            c->acc ^= param[0];
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Ands the accumulator and a memory value and then sets CPU flags
        //if any bits are set, specifically NEGATIVE if bit 7 is set
        //OVERFLOW if bit 6 is set, or ZERO if the result is 0
        //Does not modfiy the value in the accumulator.
        case BIT:
            param[0] = fetch_byte(c);
            param[0] &= c->acc;
            set_cpu_flag(c, ZERO, param[0] == 0);
            set_cpu_flag(c, NEGATIVE, (param[0] & 0x80));
            set_cpu_flag(c, OVERFLOW, (param[0] & 0x20));
            break;
        //Compares the accumulator to a memory value
        case CMP:
            compare(c, c->acc, fetch_byte(c));
            break;
        //Compares the x register to a memory value
        case CPX:
            compare(c, c->x, fetch_byte(c));
            break;
        //Compares the y register to a memory value
        case CPY:
            compare(c, c->y, fetch_byte(c));
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
        //Pushes the high and low bytes of the pc to the stack (separately);
        //Pushes the processor status register with the Break flag set to true to the stack
        //Disables interrupts
        //Loads the pc low byte from 0xFFFE and the high byte from 0xFFFF
        case BRK:
            fetch_byte(c); //Need to fetch dummy opcode since BRK behaves as a 2 byte instruction even though it is only 1 byte
            push_pc(c);
            push(c, (c->proc_stat_reg | (1 << 4)));
            set_cpu_flag(c, INTERRUPT_DISABLE, 1);
            set_pc(c, 0xFFFE);
            break;
        //Sets the pc to the memory address specified
        case JMP:
            param[0] = fetch_byte(c);
            c->pc = c->memory[param[0]];
            break;
        //Do nothing. Its in the name
        case NOP:
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

    char *buf;
    int sz = read_to_end("output.txt", &buf, 0);
    if(sz < 0) {
        fprintf(stderr, "Error when opening a file\n");
        return 0;
    }

    uint16_t endpt = c.pc + sz;
    memcpy(c.memory, buf, sz);

    for(int i = 0; i < sz; i++) {
        printf("%i:   %02X\n", i, c.memory[i]);
    }

    while(c.pc < endpt) {
        uint8_t instr = fetch_byte(&c);
        execute_instr(&c, instr);
    }
    print_cpu_state(&c);
    print_stack(&c);
    print_page(&c, 0);

    free(buf);

    return 0;
}
