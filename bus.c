#include "bus.h"
#include "ppu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t mem_read(bus *b, uint16_t addr) {
    if(addr <= RAM_MIRRORS_END) {
        uint16_t mirror_down_addr = addr & 0x7FF;
        return b->cpu_vram[mirror_down_addr];
    }
    else if((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014) {
        switch (addr) {
            case 0x2002: {
                uint8_t status = b->p->status_reg;
                set_ppu_stat_reg_flag(b->p, PPU_STAT_VBLANK, 0);

                b->p->scroll_reg->s_ptr = 1;
                b->p->addr_reg->h_ptr = 1;

                return status;
            }
            case 0x2004:
                return b->p->oam_data[b->p->oamaddr_reg];
            case 0x2007:
                return ppu_read_data(b->p);
            default:
                fprintf(stderr, "ERROR: Attempting to read from write only PPU register at address: %02X\n", addr);
        }
    }
    else if(addr <= PPU_REGISTERS_MIRRORS_END) {
        uint16_t mirror_down_addr = 0x2000 + (addr % 8);
        return mem_read(b, mirror_down_addr);
    }
    else if(addr >= 0x8000 && addr <= 0xFFFF) {
        addr -= 0x8000;
        if(b->rom->prg_rom_sz == 0x4000 && addr >= 0x4000) addr = addr % 0x4000;
        return b->rom->prg_rom[addr];
    }
    else {
        printf("Ignoring memory access at %02X\n", addr);
    }

    return 0;
}

void mem_write(bus *b, uint16_t addr, uint8_t val) {
    if(addr <= RAM_MIRRORS_END) {
        uint16_t mirror_down_addr = addr & 0x7FF;
        b->cpu_vram[mirror_down_addr] = val;
    }
    else if(addr < 0x2008 || addr == 0x4014) {
        switch(addr) {
            case 0x2000: {
                uint8_t old = get_ppu_ctrl_reg_flag(b->p, PPU_CR_GENERATE_NMI);
                b->p->ctrl_reg = val;
                uint8_t cur = get_ppu_ctrl_reg_flag(b->p, PPU_CR_GENERATE_NMI);

                if(!old && cur && get_ppu_stat_reg_flag(b->p, PPU_STAT_VBLANK))
                    b->p->nmi_triggered = 1;
            }
            break;
            case 0x2001:
                b->p->mask_reg = val;
            break;
            case 0x2003:
                b->p->oamaddr_reg = val;
            break;
            case 0x2004:
                b->p->oam_data[b->p->oamaddr_reg] = val;
            break;
            case 0x2005:
                update_scroll_register(b->p->scroll_reg, val);
            break;
            case 0x2006:
                update_addr_register(b->p->addr_reg, val);
            break;
            case 0x2007:
                ppu_write_data(b->p, val);
            break;
            case 0x4014:
                b->p->oamdma_reg = val;
            break;
            default:
                fprintf(stderr, "Attempting to write to read-only PPU register at address %02X\n", addr);
        }
    }
    else if(addr <= PPU_REGISTERS_MIRRORS_END) {
        uint16_t mirror_down_addr = 0x2000 + (addr % 8);
        mem_write(b, mirror_down_addr, val);
    }
    else if(addr >= 0x8000 && addr <= 0xFFFF) {
        fprintf(stderr, "ERROR: Attempting to write to cartridge ROM\n");
    }
    else {
        printf("Ignoring memory write-access at %hu\n", addr);
    }
}

//TODO: Add support for NES2.0
int rom_load(rom *r, uint8_t *buf, int buf_len) {
    if(buf_len < 16) {
        fprintf(stderr, "ROM is too small to contain an iNES header\n");
        return 1;
    }

    if(buf[0] != 'N' || buf[1] != 'E' || buf[2] != 'S' || buf[3] != 0x1A) {
        fprintf(stderr, "ROM is not in the iNES format\n");
        return 1;
    }

    r->mapper = (buf[7] & 0xF0) | (buf[6] >> 4);
    uint8_t ines_ver = (buf[7] >> 2) & 0x3;
    if(ines_ver != 0) {
        fprintf(stderr, "NES2.0 format is not supported\n");
        return 2;
    }

    if(buf[6] & 0b1000) r->mirroring = MIR_FOURSCREEN;
    else if(buf[6] & 0b1) r->mirroring = MIR_VERTICAL;
    else r->mirroring = MIR_HORIZONTAL;

    r->prg_rom_sz = buf[4] * PRG_PAGE_SIZE;
    r->chr_rom_sz = buf[5] * CHR_PAGE_SIZE;

    size_t trainer_size = (buf[6] & 0x04) ? 512 : 0;
    size_t required = 16 + trainer_size + r->prg_rom_sz + r->chr_rom_sz;

    if ((size_t)buf_len < required) {
        fprintf(stderr, "ROM file truncated\n");
        return 1;
    }

    size_t prg_rom_start = 16 + ((buf[6] & 0b100) ? 512 : 0);
    size_t chr_rom_start = prg_rom_start + r->prg_rom_sz;

    r->prg_rom = malloc(sizeof(uint8_t) * r->prg_rom_sz);
    memcpy(r->prg_rom, &buf[prg_rom_start], r->prg_rom_sz * sizeof(uint8_t));
    r->chr_rom = malloc(sizeof(uint8_t) * r->chr_rom_sz);
    memcpy(r->chr_rom, &buf[chr_rom_start], r->chr_rom_sz * sizeof(uint8_t));

    return 0;
}
