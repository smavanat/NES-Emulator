#ifndef __PPU_H__
#define __PPU_H__
#include <stdint.h>

typedef struct {
    uint8_t h_byte;
    uint8_t l_byte;
    uint8_t h_ptr;
} addr_register;

//Address register functions
void set_addr_register(addr_register *reg, uint16_t val);
uint16_t get_addr_register(addr_register *reg);
void update_addr_register(addr_register *reg, uint8_t val);
void increment_addr_register(addr_register *reg, uint8_t val);

typedef struct {
    uint8_t xscroll;
    uint8_t yscroll;
    uint8_t s_ptr;
} scroll_register;

typedef enum {
    PPU_CR_NAMETABLE_1,
    PPU_CR_NAMETABLE_2,
    PPU_CR_VRAM_ADD_INCREMENT,
    PPU_CR_SPRITE_PATTERN_ADDR,
    PPU_CR_BACKGROUND_PATTERN_ADDR,
    PPU_CR_SPRITE_SIZE,
    PPU_CR_MASTER_SLAVE_SELECT,
    PPU_CR_GENERATE_NMI,
} ppu_cr_flag;

typedef enum {
    MIR_VERTICAL,
    MIR_HORIZONTAL,
    MIR_FOURSCREEN,
} mirroring;

typedef struct {
    uint8_t vram[2048];
    uint8_t oam_data[256];
    uint8_t palette_table[32];
    uint8_t *chr_rom;
    addr_register *addr_reg;
    scroll_register *scroll_reg;
    uint8_t ctrl_reg;
    uint8_t mask_reg;
    uint8_t status_reg;
    uint8_t oamaddr_reg;
    uint8_t oamdata_reg;
    uint8_t data_reg;
    uint8_t oamdma_reg;
    uint8_t internal_data_buf;
    mirroring mirroring;
} ppu;

//Control Register functions
void set_ppu_cr_flag(ppu* p, ppu_cr_flag flag, uint8_t cond);
uint8_t get_ppu_cr_flag(ppu* p, ppu_cr_flag flag);
uint8_t vram_addr_increment(ppu *p);

//PPU functions
void ppu_increment_vram_addr(ppu *p);
uint16_t ppu_mirror_vram_addr(ppu *p, uint16_t addr);
uint8_t ppu_read_data(ppu *p);

#endif
