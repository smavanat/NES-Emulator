#ifndef __BUS_H__
#define __BUS_H__
#include <stddef.h>
#include <stdint.h>
#include "ppu.h"
#include "joypad.h"

#define RAM 0x0000
#define RAM_MIRRORS_END 0x1FFF
#define PPU_REGISTERS 0x2000
#define PPU_REGISTERS_MIRRORS_END 0x3FFF
#define PRG_PAGE_SIZE 16384
#define CHR_PAGE_SIZE 8192

typedef struct {
    //Memory reachable from the bus
    uint8_t cpu_vram[2048];
    rom *rom;
    ppu *p;

    //User input handled through the bus
    joypad *player_1;
    joypad *player_2;

    uint64_t total_cycles; //Total cycles elapsed since program start
    uint16_t dma_stall; //Whether the cpu needs to be stalled to transport data via DMA
} bus;

//Function for reading from memory to cpu through the bus
uint8_t mem_read(bus *b, uint16_t addr);
//Function for writing to memory from cpu through the bus
void mem_write(bus *b, uint16_t addr, uint8_t val);

#endif //__BUS_H__
