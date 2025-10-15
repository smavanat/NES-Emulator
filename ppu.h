#ifndef __PPU_H__
#define __PPU_H__
#include "memory.h"
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_video.h>
#include <stdint.h>
#include "include/SDL3/SDL.h"
#include "shader.h"
//TODO: Figure out how colour palettes are stored. But do rendering first since that is more important

//For rendering
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240
#define SCALE 5
#define FRAME_RATE 1000/60.0f
#define PIXEL_WIDTH 8
#define PIXEL_HEIGHT 8
#define CYCLES_PER_FRAME 89342
#define NMI_INTERRUPT_CYCLES 29780

//PPU memory layout
#define PATTERN_TABLE_0_START 0x0
#define PATTERN_TABLE_0_END 0xfff
#define PATTERN_TABLE_1_START 0x1000
#define PATTERN_TABLE_1_END 0x1FFF
#define NAMETABLE_0_START 0x2000
#define NAMETABLE_0_END 0x23BF
#define ATTRIBUTE_TABLE_0_START 0x23C0
#define ATTRIBUTE_TABLE_0_END 0x23FF
#define NAMETABLE_1_START 0x2400
#define NAMETABLE_1_END 0x27BF
#define ATTRIBUTE_TABLE_1_START 0x27C0
#define ATTRIBUTE_TABLE_1_END 0x27FF
#define NAMETABLE_2_START 0x2800
#define NAMETABLE_2_END 0x2BBF
#define ATTRIBUTE_TABLE_2_START 0x2BC0
#define ATTRIBUTE_TABLE_2_END 0x2BFF
#define NAMETABLE_3_START 0x2C00
#define NAMETABLE_3_END 0x2FBF
#define ATTRIBUTE_TABLE_3_START 0x2FC0
#define ATTRIBUTE_TABLE_3_END 0x2FFF
#define PALETTE_RAM_START 0x3F00
#define PALETTE_RAM_END 0x3F1F

typedef enum {
    NAMETABLE_1,
    NAMETABLE_2,
    VRAM_AND_INCREMENT,
    SPRITE_PATTERN_ADDR,
    BACKGROUND_PATTERN_ADDR,
    SPRITE_SIZE,
    MASTER_SLAVE_SELECT,
    GENERATE_NMI
} PPUCTRL_flag;

typedef enum {
    GREYSCALE,
    SHOW_BACKGROUND,
    SHOW_PIXELS,
    ENABLE_BACKGROUND_RENDERING,
    ENABLE_SPRITE_RENDERING,
    EMPHASIZE_RED,
    EMPHASIZE_GREEN,
    EMPHASIZE_BLUE
} PPUMASK_flag;

typedef enum {
    SPRITE_OVERFLOW = 5,
    SPRITE_ZERO_HIT = 6,
    V_BLANK = 7
} PPUSTATUS_flag;

typedef struct {
    //Memory:
    uint8_t vram[2048];
    uint8_t object_attribute_mem_data[256];
    uint8_t memory[0x3FFF];
} ppu;

extern SDL_Color palette[64];
extern unsigned int texture, pbo, vao;
extern shader *s;
extern SDL_Window *gWindow;
extern SDL_Renderer *gRenderer;
extern SDL_GLContext glContext;

void set_ppu_flag(memory *m, int ppu_register, int flag, int val);
int get_ppu_flag(memory *m, int ppu_register, int flag);

int ppu_init(ppu *p);
int ppu_load(ppu* p);

#endif
