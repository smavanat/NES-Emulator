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

uint8_t fetch_byte(cpu *c) {
    return c->memory[c->pc++];
}

uint8_t get_cpu_flag(cpu *c, cpu_flag fl) {
    if(c->proc_stat_reg & (1 << fl)) return 1;
    else return 0;
}

void set_cpu_flag(cpu *c, cpu_flag fl, uint8_t cond) {
    if(cond) c->proc_stat_reg |= (uint8_t)(1 << fl);
    else c->proc_stat_reg &= (uint8_t)~(1 << fl);
}

void branch(cpu *c, uint8_t expr) {
    int8_t offset = (int8_t)fetch_byte(c);
    if(expr) {
        c->pc += offset;
    }
}

void push(cpu* c, uint8_t val) {
    c->memory[0x100 + c->sp] = val;
    c->sp--;
}

uint8_t pop(cpu *c) {
    c->sp++;
    return c->memory[0x100 + c->sp];
}

void push_pc(cpu *c) {
    uint8_t high = (uint8_t)(c->pc >> 8);
    uint8_t low = (uint8_t)(c->pc);

    push(c, high);
    push(c, low);
}

void set_pc(cpu *c, uint16_t addr) {
    c->pc = (c->memory[addr+1] << 8) | c->memory[addr];
}

void execute_instr(cpu *c, opcode op) {
    uint16_t param = fetch_byte(c); //Sticking with immediate mode addressing for all instructions for now
    switch(op) {
        //Loads value from memory address into accumulator
        case LDA:
            c->acc = param;
            set_cpu_flag(c, ZERO, c->acc == 0);
            set_cpu_flag(c, NEGATIVE, (c->acc & 0x80));
            break;
        //Loads value from memory address into x register
        case LDX:
            c->x = param;
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        case LDY:
            c->y = param;
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Stores the value in the accumulator at the given memory address
        case STA:
            c->memory[param] = c->acc;
            break;
        //Stores the value in the x register at the given memory address
        case STX:
            c->memory[param] = c->x;
            break;
        //Increments the value in the x register by 1
        case INX:
            c->x++;
            set_cpu_flag(c, ZERO, c->x == 0);
            set_cpu_flag(c, NEGATIVE, (c->x & 0x80));
            break;
        //Branches if the zero flag is not set
        case BNE:
            branch(c, !get_cpu_flag(c, ZERO));
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
            c->pc = c->memory[param];
            break;
        default:
            printf("Opcode not supported\n");
            break;
    }
}

void print_cpu_state(cpu *c) {
    printf("Accumulator: %hhu \nX Register: %hhu \nY Register: %hhu \nStack Pointer: %hhu\nPC: %hu\n", c->acc, c->x, c->y, c->sp, c->pc);
    printf("Status Register:\n Carry: %hhu\n Zero: %hhu\n Interrupt Disable: %hhu\n Decimal: %hhu\n Break: %hhu\n Overflow: %hhu\n Negative: %hhu\n",
           get_cpu_flag(c, CARRY), get_cpu_flag(c, ZERO), get_cpu_flag(c, INTERRUPT_DISABLE), get_cpu_flag(c, DECIMAL),
           get_cpu_flag(c, BREAK), get_cpu_flag(c, OVERFLOW), get_cpu_flag(c, NEGATIVE));
}

void print_stack(cpu *c) {
    printf("Addr    Val\n");
    for(int i = c->sp+1; i < 256; i++) {
        printf(" %03X     %02X\n", 0x100 + i, c->memory[0x100 + i]);
    }
}

void print_page(cpu *c, uint8_t page_num) {
    uint16_t start_addr = page_num * 256;

    for(int i = 0; i < 256; i++) {
        if(i > 0 && i % 16 == 0) printf("\n");
        printf("%02X ", c->memory[start_addr+i]);
    }
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
    print_page(&c, 2);

    free(buf);

    return 0;
}
