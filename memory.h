#ifndef __MEMORY_H__
#define __MEMORY_H__
#include <stdint.h>
//This file defines memory (just a large byte array) and some constants for dealing with memory
#define MEMORY_SIZE 0xFFFF
#define RAM_START 0x0
#define RAM_END 0x7FF

//PPU register indexes
#define PPUCTRL 0x2000
#define PPUMASK 0x2001
#define PPUSTATUS 0x2002
#define OAMADDR 0x2003
#define OAMDATA 0x2004
#define PPUSCROLL 0x2005
#define PPUADDR 0x2006
#define PPUDATA 0x2007
#define OAMDMA 0x4014

typedef struct {
    uint8_t memory[MEMORY_SIZE];
} memory;

#endif
