#ifndef __BUS_H__
#define __BUS_H__
#include <stddef.h>
#include <stdint.h>
#include "ppu.h"

#define RAM 0x0000
#define RAM_MIRRORS_END 0x1FFF
#define PPU_REGISTERS 0x2000
#define PPU_REGISTERS_MIRRORS_END 0x3FFF
#define PRG_PAGE_SIZE 16384
#define CHR_PAGE_SIZE 8192

typedef struct {
    size_t prg_rom_sz;
    size_t chr_rom_sz;
    uint8_t *prg_rom;
    uint8_t *chr_rom;
    uint8_t mapper;
    mirroring mirroring;
} rom;

int rom_load(rom *r, uint8_t *buf, int buf_len);

typedef struct {
    uint8_t cpu_vram[2048];
    rom *rom;
    ppu *p;
} bus;

uint8_t mem_read(bus *b, uint16_t addr);
void mem_write(bus *b, uint16_t addr, uint8_t val);

#endif //__BUS_H__
