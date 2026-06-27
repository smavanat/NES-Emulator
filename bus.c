#include "bus.h"
#include "joypad.h"
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
                fprintf(stderr, "ERROR: Attempting to read from write only PPU register at address: %04X\n", addr);
        }
    }
    else if(addr <= PPU_REGISTERS_MIRRORS_END) {
        uint16_t mirror_down_addr = 0x2000 + (addr % 8);
        return mem_read(b, mirror_down_addr);
    }
    else if(addr == 0x4016) {
        return joypad_read(b->player_1);
    }
    else if(addr == 0x4017) {
        return joypad_read(b->player_2);
    }
    else if(addr >= 0x8000 && addr <= 0xFFFF) {
        return b->rom->cpu_read(b->rom, addr);
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
            case 0x4014: {
                uint16_t page_start = (uint16_t)val << 8;
                for(int i = 0; i < 256; i++) {
                    b->p->oam_data[b->p->oamaddr_reg] = mem_read(b, page_start + i);
                    b->p->oamaddr_reg++;
                }
                b->dma_stall = (b->total_cycles % 2 == 0) ? 514 : 513;
            }
            break;
            default:
                fprintf(stderr, "Attempting to write to read-only PPU register at address %04X\n", addr);
        }
    }
    else if(addr <= PPU_REGISTERS_MIRRORS_END) {
        uint16_t mirror_down_addr = 0x2000 + (addr % 8);
        mem_write(b, mirror_down_addr, val);
    }
    else if(addr == 0x4016) {
        joypad_write(b->player_1, val);
    }
    else if(addr == 0x4017) {
        joypad_write(b->player_2, val);
    }
    else if(addr >= 0x8000 && addr <= 0xFFFF) {
        b->rom->cpu_write(b->rom, addr, val);
    }
    else {
        printf("Ignoring memory write-access at %04X\n", addr);
    }
}

//Functions for Mapper 0 (NROM)
uint8_t mapper_0_cpu_read(rom *r, uint16_t addr) {
    addr -= 0x8000;
    if(r->prg_rom_sz == 0x4000 && addr >= 0x4000) addr = addr % 0x4000;
    return r->prg_rom[addr];
}
void mapper_0_cpu_write(rom *r, uint16_t addr, uint8_t val) {
    fprintf(stderr, "ERROR: Attempting to write to cartridge ROM\n");
}
uint8_t mapper_0_ppu_read(rom *r, uint16_t addr) {
    return r->chr_rom[addr];
}
void mapper_0_ppu_write(rom *r, uint16_t addr, uint8_t val) {
    // If chr_rom_sz == 0, treat at RAM
    if(r->chr_rom_sz == 0) r->chr_rom[addr % 8192] = val;
    else fprintf(stderr, "ERROR: Attempting to write to CHR ROM\n");
}

//Functions for Mapper 1 (MMC1)
uint8_t mapper_1_cpu_read(rom *r, uint16_t addr) {
    //Control(0) register bits 2 and 3 set PRG ROM bank mode
    uint8_t prg_mode = (r->mapper_registers[0]) & 0xC;

    uint32_t offset;
    if(prg_mode == 0 || prg_mode == 1) {
        //32 KB mode: switch two banks at once, ignore low bit of bank number
        uint8_t bank = r->prg_bank & 0xFE;
        if(addr <= 0xBFFF) offset = bank * 0x4000 + (addr - 0x8000);
        else offset = (bank + 1) * 0x4000 + (addr - 0xC000);
    }
    else if(prg_mode == 2) {
        //Fix first bank at 0x8000 and switch 16KB bank at 0xC000
        if(addr <= 0xBFFF) offset = addr - 0x8000; //First bank (fixed)
        else offset = r->prg_bank * 0x4000 + (addr - 0xC000);
    }
    else {
        //Fix last bank at 0xC000 and switch 16KB bank at 0x8000
        if(addr <= 0xBFFF) offset = r->prg_bank * 0x4000 + (addr - 0x8000);
        else offset = (r->prg_rom_sz - 0x4000) + (addr - 0xC000); //Last bank (fixed)
    }

    //Clamp to avoid out of bounds
    offset %= r->prg_rom_sz;
    return r->prg_rom[offset];
}

//NOTE: Apparently MMC1 ignores writes on consecutive cycles
//      To prevent instructions such as INC from writing twice
//      accidentally. Currently not implemented
void mapper_1_cpu_write(rom *r, uint16_t addr, uint8_t val) {
    //If bit 7 off the given value is 1, clear shift and count registers
    if(val & 0x80) {
        r->mapper_registers[4] = 0; //Clear the shift register
        r->mapper_registers[5] = 0; //Clear the count
        return;
    }

    //Shift bit 0 of the given value into the shift register
    //Since we start from the LSB, need to shift it in reverse
    if(r->mapper_registers[5] < 5) {
        r->mapper_registers[4] |= (val & 0x1) << r->mapper_registers[5];
    }
    r->mapper_registers[5]++; //Increment how many writes there have been

    //If we are on the fifth write, determine what register we want to copy the
    //shift register value into
    if(r->mapper_registers[5] == 5) {
        //Get the index of the register to be copied into
        uint8_t register_index;
        if(addr <= 0x9FFF) register_index = 0;
        else if(addr <= 0xBFFF) register_index = 1;
        else if(addr <= 0xDFFF) register_index = 2;
        else if(addr <= 0xFFFF) register_index = 3;

        r->mapper_registers[register_index] = r->mapper_registers[4]; //Copying the value from the shift register into the desired register

        //PRG Bank(3) register bits 0 - 3 control PRG bank
        if(register_index == 3) r->prg_bank = r->mapper_registers[3] & 0x0F;
        //Control(0) register bits control mirroring
        if(register_index == 0) {
            switch(r->mapper_registers[0] & 0x3) {
                case 0: r->mirroring = MIR_ONESCREEN_LO; break;
                case 1: r->mirroring = MIR_ONESCREEN_HI; break;
                case 2: r->mirroring = MIR_VERTICAL;     break;
                case 3: r->mirroring = MIR_HORIZONTAL;   break;
            }
        }

        //Clear count and shift registers
        r->mapper_registers[5] = 0;
        r->mapper_registers[4] = 0;
    }

}
uint8_t mapper_1_ppu_read(rom *r, uint16_t addr) {
    //Control(0) register bit 4 sets the CHR mode
    uint8_t chr_mode = r->mapper_registers[0] & 0x10;

    uint32_t offset;
    if(chr_mode == 0) {
        //8KB mode: register 1 selects entire 8KB bank, ignore low bit
        uint8_t bank = r->mapper_registers[1] & 0xFE;
        offset = bank * 0x1000 + addr;
    }
    else {
        //4KB mode: register 1 selects bank at 0x0000, register 2 selects bank at 0x1000
        if(addr < 0x1000) {
            uint8_t bank = r->mapper_registers[1];
            offset = bank * 0x1000 + addr;
        }
        else {
            uint8_t bank = r->mapper_registers[2];
            offset = bank * 0x1000 + (addr - 0x1000);
        }
    }

    // If chr_rom_sz == 0, treat at RAM
    if(r->chr_rom_sz == 0) return r->chr_rom[offset % 8192];
    else return r->chr_rom[offset % r->chr_rom_sz];
}
void mapper_1_ppu_write(rom *r, uint16_t addr, uint8_t val) {
    // If chr_rom_sz == 0, treat at RAM
    if(r->chr_rom_sz == 0) r->chr_rom[addr % 8192] = val;
    else fprintf(stderr, "ERROR: Attempting to write to CHR ROM\n");
}

//Functions for Mapper 2 (UxROM)
uint8_t mapper_2_cpu_read(rom *r, uint16_t addr) {
    if(addr >= 0xC000) {
        //Last bank is fixed
        uint32_t offset = (r->prg_rom_sz - 0x4000) + (addr - 0xC000);
        return r->prg_rom[offset];
    }
    //Switchable bank at 0x8000
    uint32_t offset = (r->prg_bank * 0x4000) + (addr - 0x8000);
    return r->prg_rom[offset];
}
void mapper_2_cpu_write(rom *r, uint16_t addr, uint8_t val) {
    //Any write to 0x8000 - 0xFFFF selects a prg bank
    r->prg_bank = val & 0x0F;
}
uint8_t mapper_2_ppu_read(rom *r, uint16_t addr) {
    return r->chr_rom[addr];
}
void mapper_2_ppu_write(rom *r, uint16_t addr, uint8_t val) {
    // If chr_rom_sz == 0, treat at RAM
    if(r->chr_rom_sz == 0) r->chr_rom[addr % 8192] = val;
    else fprintf(stderr, "ERROR: Attempting to write to CHR ROM\n");
}

//Functions for mapper 3 (CNROM)
uint8_t mapper_3_cpu_read(rom *r, uint16_t addr) {
    uint16_t offset = addr - 0x8000;
    if(r->prg_rom_sz == 0x4000) offset %= 0x4000;
    return r->prg_rom[offset];
}
void mapper_3_cpu_write(rom *r, uint16_t addr, uint8_t val) {
    uint16_t offset = addr - 0x8000;
    if(r->prg_rom_sz == 0x4000) offset %= 0x4000;
    r->mapper_registers[0] = val & r->prg_rom[offset];
}
uint8_t mapper_3_ppu_read(rom *r, uint16_t addr) {
    return r->chr_rom[((r->mapper_registers[0] & 0x3) * 0x2000) + addr];
}
void mapper_3_ppu_write(rom *r, uint16_t addr, uint8_t val) {
    fprintf(stderr, "ERROR: Attempting to write to CHR ROM\n");
}

//Functions for mapper 4 (MMC3)
//Register layout:
//[0] = bank select ($8000 even)
//[1] = mirroring ($A000 even)
//[2] = IRQ latch ($C000 even)
//[3] = IRQ counter
//[4] = IRQ enabled
//[5-12] = bank registers R0-R7
//TODO: Implement PRG RAM?
uint8_t mapper_4_cpu_read(rom *r, uint16_t addr) {
    uint8_t prg_mode = (r->mapper_registers[0] >> 6) & 0x01;

    uint32_t offset;
    if(addr <= 0x9FFF) {
        if(prg_mode == 0) offset = (r->mapper_registers[11] & 0x3F) * 0x2000 + (addr - 0x8000); //R6 switchable
        else offset = (r->prg_rom_sz - 0x4000) + (addr - 0x8000); //Fixed to second to last bank
    }
    else if(addr <= 0xBFFF) {
        offset = (r->mapper_registers[12] & 0x3F) * 0x2000 + (addr - 0xA000); //R7 always switchable
    }
    else if(addr <= 0xDFFF) {
        if(prg_mode == 0) offset = (r->prg_rom_sz - 0x4000) + (addr - 0xC000); //Fixed to second to last bank
        else offset = (r->mapper_registers[11] & 0x3F) * 0x2000 + (addr - 0xC000); //R6 switchable
    }
    else {
        offset = (r->prg_rom_sz - 0x2000) + (addr - 0xE000); //Always switched to last bank
    }

    return r->prg_rom[offset % r->prg_rom_sz];
}
//Each 2KB range (0x8000 - 0x9FFF, 0xA000 - 0xBFFF, 0xC000 - 0xDFFF, 0xE000 - 0xFFFF)
//Maps to a pair of registers. Which of the pair is being mapped to is determined by the parity
//(odd/even) of the address
//More info: https://www.nesdev.org/wiki/MMC3#Registers
void mapper_4_cpu_write(rom *r, uint16_t addr, uint8_t val) {
    if(addr <= 0x9FFF) {
        if(addr % 2 == 0) r->mapper_registers[0] = val; //Bank select
        //Bank data: write to register selected by bits 0-2 of register 0
        else r->mapper_registers[5 + (r->mapper_registers[0] & 0x07)] = val;
    }
    else if(addr <= 0xBFFF) {
        //Nametable arrangement. Does not affect cartridges with hardwired fourscreen mirroring
        if(addr % 2 == 0 && r->mirroring != MIR_FOURSCREEN) {
            r->mapper_registers[1] = val;
            r->mirroring = (val & 0x1) ? MIR_VERTICAL : MIR_HORIZONTAL;
        }
        //Odd is PRG RAM protection. Not currently implemented
    }
    else if(addr <= 0xDFFF) {
        //Even sets IRQ latch value
        if(addr % 2 == 0) r->mapper_registers[2] = val;
        //Clear IRQ counter so it reloads from latch on next scanline
        else r->mapper_registers[3] = 0;
    }
    else if(addr <= 0xFFFF) {
        //IRQ disable
        if(addr % 2 == 0) {
            r->mapper_registers[4] = 0;
            r->irq_pending = 0;
        }
        else {
            r->mapper_registers[4] = 1;
        }
    }
}
uint8_t mapper_4_ppu_read(rom *r, uint16_t addr) {
    uint8_t chr_mode = r->mapper_registers[0] & 0x80;
    uint32_t offset;

    if(!chr_mode) {
        //R0: 2KB CHR bank from 0x0000 - 0x07FF
        if(addr < 0x800) offset = r->mapper_registers[5] * 0x800 + addr;
        //R1: 2KB CHR bank from 0x0800 - 0x0FFF
        if(addr < 0x1000) offset = r->mapper_registers[6] * 0x800 + (addr - 0x0800);
        //R2: 1KB CHR bank from 0x1000 - 0x13FF
        if(addr < 0x1400) offset = r->mapper_registers[7] * 0x400 + (addr - 0x1000);
        //R3: 1KB CHR bank from 0x1400 - 0x17FF
        if(addr < 0x1800) offset = r->mapper_registers[8] * 0x400 + (addr - 0x1400);
        //R4: 1KB CHR bank from 0x1800 - 0x1BFF
        if(addr < 0x1C00) offset = r->mapper_registers[9] * 0x400 + (addr - 0x1800);
        //R5: 1KB CHR bank from 0x1C00 - 0x1FFF
        if(addr < 0x2000) offset = r->mapper_registers[10] * 0x400 + (addr - 0x1C00);
    }
    else {
        //R0: 1KB CHR bank from 0x0000 - 0x03FF
        if(addr < 0x400) offset = r->mapper_registers[5] * 0x400 + addr;
        //R1: 1KB CHR bank from 0x0400 - 0x07FF
        if(addr < 0x0800) offset = r->mapper_registers[6] * 0x400 + (addr - 0x0400);
        //R2: 1KB CHR bank from 0x0800 - 0x0BFF
        if(addr < 0x0C00) offset = r->mapper_registers[7] * 0x400 + (addr - 0x0800);
        //R3: 1KB CHR bank from 0x0C00 - 0x0FFF
        if(addr < 0x1000) offset = r->mapper_registers[8] * 0x400 + (addr - 0x0C00);
        //R4: 2KB CHR bank from 0x1000 - 0x17FF
        if(addr < 0x1800) offset = r->mapper_registers[9] * 0x400 + (addr - 0x1000);
        //R5: 2KB CHR bank from 0x1800 - 0x1FFF
        if(addr < 0x2000) offset = r->mapper_registers[10] * 0x400 + (addr - 0x1800);
    }

    if(r->chr_rom_sz == 0) return r->chr_rom[offset % 8192]; //CHR RAM
    else return r->chr_rom[offset % r->chr_rom_sz]; //CHR ROM
}
void mapper_4_ppu_write(rom *r, uint16_t addr, uint8_t val) {
    // If chr_rom_sz == 0, treat at RAM
    if(r->chr_rom_sz == 0) r->chr_rom[addr % 8192] = val;
    else fprintf(stderr, "ERROR: Attempting to write to CHR ROM\n");
}

//Functions for mapper 7 (AxROM)
//Map rom.mapper_registers[0] to be the bank select register
uint8_t mapper_7_cpu_read(rom *r, uint16_t addr) {
    return r->prg_rom[(((r->mapper_registers[0] & 0x7) * 0x8000) + (addr - 0x8000)) % r->prg_rom_sz];
}
void mapper_7_cpu_write(rom *r, uint16_t addr, uint8_t val) {
    r->mapper_registers[0] = val;
    r->mirroring = (r->mapper_registers[0] & 0x10) ? MIR_ONESCREEN_HI : MIR_ONESCREEN_LO;
}
uint8_t mapper_7_ppu_read(rom *r, uint16_t addr) {
    return r->chr_rom[addr % 8192];
}
void mapper_7_ppu_write(rom *r, uint16_t addr, uint8_t val) {
    r->chr_rom[addr % 8192] = val;
}

int assign_rom_functions(rom *r) {
    switch (r->mapper) {
        case 0:
            r->cpu_read = mapper_0_cpu_read;
            r->cpu_write = mapper_0_cpu_write;
            r->ppu_read = mapper_0_ppu_read;
            r->ppu_write = mapper_0_ppu_write;
        break;
        case 1:
            r->mapper_registers[0] = 0x0C;
            r->mapper_registers[4] = 0;
            r->mapper_registers[5] = 0;
            r->cpu_read = mapper_1_cpu_read;
            r->cpu_write = mapper_1_cpu_write;
            r->ppu_read = mapper_1_ppu_read;
            r->ppu_write = mapper_1_ppu_write;
        break;
        case 2:
            r->cpu_read = mapper_2_cpu_read;
            r->cpu_write = mapper_2_cpu_write;
            r->ppu_read = mapper_2_ppu_read;
            r->ppu_write = mapper_2_ppu_write;
        break;
        case 3:
            r->cpu_read = mapper_3_cpu_read;
            r->cpu_write = mapper_3_cpu_write;
            r->ppu_read = mapper_3_ppu_read;
            r->ppu_write = mapper_3_ppu_write;
        break;
        case 4:
            r->cpu_read = mapper_4_cpu_read;
            r->cpu_write = mapper_4_cpu_write;
            r->ppu_read = mapper_4_ppu_read;
            r->ppu_write = mapper_4_ppu_write;
        break;
        case 7:
            r->cpu_read = mapper_7_cpu_read;
            r->cpu_write = mapper_7_cpu_write;
            r->ppu_read = mapper_7_ppu_read;
            r->ppu_write = mapper_7_ppu_write;
        default:
            return 0;
    }

    return 1;
}

//TODO: Add support for NES2.0
//      Add support for different mapper types
//      Mapper 0 (NROM) -> DONE
//      Mapper 1 (MMC1) -> DONE
//      Mapper 2 (UxROM) -> DONE
//      Mapper 3 (CNROM) -> DONE
//      Mapper 4 (MMC3) -> DONE
//      Mapper 5 (MMC5) -> TODO
//      Mapper 7 (AxROM) -> DONE
//      Mapper 9 (MMC2) -> TODO
//      Mapper 11 (Color Dreams) -> TODO
//      Mapper 19 (Namco 163) -> TODO
//      Mapper 69 (Sunsoft FME-7) -> TODO
//      Mapper 66 (GxROM) -> TODO
int rom_load(rom *r, uint8_t *buf, int buf_len) {
    if(buf_len < 16) {
        fprintf(stderr, "ROM is too small to contain an iNES header\n");
        return 1;
    }

    if(buf[0] != 'N' || buf[1] != 'E' || buf[2] != 'S' || buf[3] != 0x1A) {
        fprintf(stderr, "ROM is not in the iNES format\n");
        return 1;
    }

    if(buf[6] & 0b1000) r->mirroring = MIR_FOURSCREEN;
    else if(buf[6] & 0b1) r->mirroring = MIR_VERTICAL;
    else r->mirroring = MIR_HORIZONTAL;

    r->mapper = (buf[7] & 0xF0) | (buf[6] >> 4);
    if(!assign_rom_functions(r)) {
        fprintf(stderr, "Mapper %hhu not supported\n", r->mapper);
        return 1;
    }

    uint8_t ines_ver = (buf[7] >> 2) & 0x3;
    if(ines_ver != 0) {
        fprintf(stderr, "NES2.0 format is not supported\n");
        return 1;
    }

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

    //Allocate CHR RAM if no CHR ROM
    if(r->chr_rom_sz == 0) r->chr_rom = calloc(8192, 1);
    else {
        r->chr_rom = malloc(sizeof(uint8_t) * r->chr_rom_sz);
        memcpy(r->chr_rom, &buf[chr_rom_start], r->chr_rom_sz * sizeof(uint8_t));
    }

    return 0;
}
