#include "ppu.h"
#include <stdint.h>
#include <stdio.h>

void set_addr_register(addr_register *reg, uint16_t val) {
    reg->h_byte = (uint8_t)(val >> 8);
    reg->l_byte = (uint8_t)val;
}
uint16_t get_addr_register(addr_register *reg) {
    return ((uint16_t)reg->h_byte) << 8 | (uint16_t)reg->l_byte;
}
void update_addr_register(addr_register *reg, uint8_t val) {
    if(reg->h_ptr) reg->h_byte = val; else reg->l_byte = val;

    //Mirror down any values above 0x3FFF
    if(get_addr_register(reg) > 0x3FFF) set_addr_register(reg, get_addr_register(reg) & 0x3FFF);
    reg->h_ptr = !reg->h_ptr;
}
void increment_addr_register(addr_register *reg, uint8_t val) {
    uint8_t lo = reg->l_byte;

    reg->l_byte += val;
    if(lo > reg->l_byte) reg->h_byte += 1;

    //Mirror down any values above 0x3FFF
    if(get_addr_register(reg) > 0x3FFF) set_addr_register(reg, get_addr_register(reg) & 0x3FFF);
}

void set_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag, uint8_t cond) {
    if(cond) p->ctrl_reg |= (uint8_t)(1 << flag);
    else p->ctrl_reg &= (uint8_t)~(1 << flag);
}

uint8_t get_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag) {
    if(p->ctrl_reg & (1 << flag)) return 1;
    else return 0;
}

uint8_t vram_addr_increment(ppu *p) {
    if(get_ppu_cr_flag(p, PPU_CR_VRAM_ADD_INCREMENT)) return 32; else return 1;
}

void ppu_increment_vram_addr(ppu *p) {
    increment_addr_register(p->areg, vram_addr_increment(p));
}

uint16_t ppu_mirror_vram_addr(ppu *p, uint16_t addr) {
    uint16_t mirrored_vram = addr & 0x2fff; //Mirror down 0x3000-0x3eff to 0x2000-0x2eff
    uint16_t vram_index = mirrored_vram - 0x2000; //To vram vector
    uint16_t name_table_index = vram_index / 0x400; //To name table index
    if(p->mirroring == MIR_VERTICAL && (name_table_index == 2 || name_table_index == 3)) vram_index -= 0x800;
    else if(p->mirroring == MIR_HORIZONTAL && (name_table_index == 2 || name_table_index == 1)) vram_index -= 0x400;
    else if(p->mirroring == MIR_HORIZONTAL && name_table_index == 3) vram_index -= 0x800;

    return vram_index;
}

uint8_t ppu_read_data(ppu *p) {
    uint16_t addr = get_addr_register(p->areg);
    ppu_increment_vram_addr(p);

    //Reading from chr_rom
    if(addr <= 0x1fff) {
        uint8_t res = p->internal_data_buf;
        p->internal_data_buf = p->chr_rom[addr];
        return res;
    }
    //Reading from RAM
    else if(addr <= 0x2fff) {
        uint8_t res = p->internal_data_buf;
        p->internal_data_buf = p->vram[ppu_mirror_vram_addr(p, addr)];
        return res;
    }
    else if(addr <= 0x3eff) {
        fprintf(stderr, "Address Space 0x3000 ... 0x3eff is not expected to be used, requested address: %hu", addr);
    }
    else if(addr <= 0x3fff) {
        return p->palette_table[addr - 0x3f00];
    }
    else {
        fprintf(stderr, "Unexpected access to mirrored space, requested address: %hu", addr);
    }

    return 1;
}
