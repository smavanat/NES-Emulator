#include "ppu.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../externals/glad.h"

//Stores the entire system palette for use by the ppu
uint8_t system_palette[64][3] = {
    {0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0}, {0x44, 0x00, 0x96}, {0xA1, 0x00, 0x5E},
    {0xC7, 0x00, 0x28}, {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00}, {0x5C, 0x2F, 0x00}, {0x10, 0x45, 0x00},
    {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E}, {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05},
    {0x05, 0x05, 0x05}, {0xC7, 0xC7, 0xC7}, {0x00, 0x77, 0xFF}, {0x21, 0x55, 0xFF}, {0x82, 0x37, 0xFA},
    {0xEB, 0x2F, 0xB5}, {0xFF, 0x29, 0x50}, {0xFF, 0x22, 0x00}, {0xD6, 0x32, 0x00}, {0xC4, 0x62, 0x00},
    {0x35, 0x80, 0x00}, {0x05, 0x8F, 0x00}, {0x00, 0x8A, 0x55}, {0x00, 0x99, 0xCC}, {0x21, 0x21, 0x21},
    {0x09, 0x09, 0x09}, {0x09, 0x09, 0x09}, {0xFF, 0xFF, 0xFF}, {0x0F, 0xD7, 0xFF}, {0x69, 0xA2, 0xFF},
    {0xD4, 0x80, 0xFF}, {0xFF, 0x45, 0xF3}, {0xFF, 0x61, 0x8B}, {0xFF, 0x88, 0x33}, {0xFF, 0x9C, 0x12},
    {0xFA, 0xBC, 0x20}, {0x9F, 0xE3, 0x0E}, {0x2B, 0xF0, 0x35}, {0x0C, 0xF0, 0xA4}, {0x05, 0xFB, 0xFF},
    {0x5E, 0x5E, 0x5E}, {0x0D, 0x0D, 0x0D}, {0x0D, 0x0D, 0x0D}, {0xFF, 0xFF, 0xFF}, {0xA6, 0xFC, 0xFF},
    {0xB3, 0xEC, 0xFF}, {0xDA, 0xAB, 0xEB}, {0xFF, 0xA8, 0xF9}, {0xFF, 0xAB, 0xB3}, {0xFF, 0xD2, 0xB0},
    {0xFF, 0xEF, 0xA6}, {0xFF, 0xF7, 0x9C}, {0xD7, 0xE8, 0x95}, {0xA6, 0xED, 0xAF}, {0xA2, 0xF2, 0xDA},
    {0x99, 0xFF, 0xFC}, {0xDD, 0xDD, 0xDD}, {0x11, 0x11, 0x11}, {0x11, 0x11, 0x11}
};

//Sets the address register to an explicit 16 bit address
void set_addr_register(addr_register *reg, uint16_t val) {
    reg->h_byte = (uint8_t)(val >> 8);
    reg->l_byte = (uint8_t)val;
}
//Gets the address stored in the address register
uint16_t get_addr_register(addr_register *reg) {
    return ((uint16_t)reg->h_byte) << 8 | (uint16_t)reg->l_byte;
}
//Updates the byte in the address register pointed to by h_ptr
void update_addr_register(addr_register *reg, uint8_t val) {
    if(reg->h_ptr) reg->h_byte = val; else reg->l_byte = val;

    //Mirror down any values above 0x3FFF
    if(get_addr_register(reg) > 0x3FFF) set_addr_register(reg, get_addr_register(reg) & 0x3FFF);
    reg->h_ptr = !reg->h_ptr;
}
//Increments the value in the address register by 1
void increment_addr_register(addr_register *reg, uint8_t val) {
    uint8_t lo = reg->l_byte;

    reg->l_byte += val;
    if(lo > reg->l_byte) reg->h_byte += 1;

    //Mirror down any values above 0x3FFF
    if(get_addr_register(reg) > 0x3FFF) set_addr_register(reg, get_addr_register(reg) & 0x3FFF);
}

//Sets the scroll register to an explicit 16 bit value
void set_scroll_register(scroll_register *reg, uint16_t val) {
    reg->xscroll = (uint8_t)(val >> 8);
    reg->yscroll = (uint8_t)val;
}
//Gets the value stored in the scroll register
uint16_t get_scroll_register(scroll_register *reg) {
    return ((uint16_t)reg->xscroll) << 8 | (uint16_t)reg->yscroll;
}
//Updates the byte in the address register pointed to by s_ptr
void update_scroll_register(scroll_register *reg, uint8_t val) {
    if(reg->s_ptr) reg->xscroll = val; else reg->yscroll = val;

    reg->s_ptr = !reg->s_ptr;
}

//Sets a specific control register flag based on a condition.
//1 if condition is true, 0 otherwise
void set_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag, uint8_t cond) {
    if(cond) p->ctrl_reg |= (uint8_t)(1 << flag);
    else p->ctrl_reg &= (uint8_t)~(1 << flag);
}
//Gets the bit value stored in a specific control register flag
uint8_t get_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag) {
    if(p->ctrl_reg & (1 << flag)) return 1;
    else return 0;
}
//Writes to control register. Checks if VBlank flag is set to true
//(while being not set before), and if GENERATE_NMI is also set,
//triggers an NMI
void write_to_ctrl_reg(ppu *p, uint8_t val) {
    uint8_t vblank_status = get_ppu_ctrl_reg_flag(p, PPU_CR_GENERATE_NMI);
    p->ctrl_reg = val;

    //If we just set VBlank status and also NMI is true, trigger an NMI
    if(!vblank_status && get_ppu_ctrl_reg_flag(p, PPU_CR_GENERATE_NMI) && get_ppu_stat_reg_flag(p, PPU_STAT_VBLANK)) {
        p->nmi_triggered = 1;
    }
}
//What value the VRAM address should be incremented by.
//1 means that we are going across, 32 means down
uint8_t vram_addr_increment(ppu *p) {
    if(get_ppu_ctrl_reg_flag(p, PPU_CR_VRAM_ADD_INCREMENT)) return 32; else return 1;
}

//Sets a specific status register flag based on a condition.
//1 if condition is true, 0 otherwise
void set_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag, uint8_t cond) {
    if(cond) p->status_reg |= (uint8_t)(1 << flag);
    else p->status_reg &= (uint8_t)~(1 << flag);
}
//Gets the bit value stored in a specific status register flag
uint8_t get_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag) {
    if(p->status_reg & (1 << flag)) return 1;
    else return 0;
}

//Increments the address register depending on the value of the PPU_CR_VRAM_ADD_INCREMENT
//flag in the control register
void ppu_increment_vram_addr(ppu *p) {
    increment_addr_register(p->addr_reg, vram_addr_increment(p));
}

//Converts a PPU nametable address into an index into VRAM
//Since all values in 0x3000 - 0x3eFF are mirrors of 0x2000 - 0x2EFF
//mirrors them down
//Supports Vertical/Horizontal, Fourscreen, and Onescreen mirroring
uint16_t ppu_mirror_vram_addr(ppu *p, uint16_t addr) {
    uint16_t mirrored_vram = addr & 0x2FFF; //Mirror down 0x3000-0x3eff to 0x2000-0x2eff
    uint16_t vram_index = mirrored_vram - 0x2000; //To vram vector
    uint16_t name_table_index = vram_index >> 10; //To nametable index
    //Fourscreen mirroring maps nametables 2 and 3 to 0 and 1 respectively
    if(p->rom->mirroring == MIR_VERTICAL && (name_table_index == 2 || name_table_index == 3)) vram_index -= 0x800;
    else if(p->rom->mirroring == MIR_HORIZONTAL && (name_table_index == 2 || name_table_index == 1)) vram_index -= 0x400;
    else if(p->rom->mirroring == MIR_HORIZONTAL && name_table_index == 3) vram_index -= 0x800;
    else if(p->rom->mirroring == MIR_ONESCREEN_LO) vram_index &= 0x3FF; //Always map to first 1KB
    else if(p->rom->mirroring == MIR_ONESCREEN_HI) vram_index = 0x400 + (vram_index & 0x3FF); //Always map to second 1KB

    return vram_index;
}
//Internal ppu function to read data from ppu memory using an
//arbitrary address
uint8_t ppu_bus_read(ppu *p, uint16_t addr) {
    addr &= 0x3FFF; //Make sure it is in the 16kb range

    if(addr <= 0x1FFF) return p->rom->ppu_read(p->rom, addr); //CHR ROM/RAM read using mapper defined function
    if(addr <= 0x2FFF) return p->vram[ppu_mirror_vram_addr(p, addr)]; //Nametables
    if(addr <= 0x3EFF) return ppu_bus_read(p, addr - 0x1000); //Mirror down to 0x2000 - 0x2EFFF range
    if(addr <= 0x3FFF) { //Palette RAM indecies
        //Palette region is mirrored every 32 bytes
        addr &= 0x1F;

        //Handling special mirrors
        if(addr == 0x10) addr = 0x00;
        if(addr == 0x14) addr = 0x04;
        if(addr == 0x18) addr = 0x08;
        if(addr == 0x1C) addr = 0x0C;
        return p->palette_table[addr];
    }

    return 0;
}
//Reads data from ppu memory using the address stored in the address register
uint8_t ppu_read_data(ppu *p) {
    uint16_t addr = get_addr_register(p->addr_reg);
    uint8_t res;

    if(addr >= 0x3F00) {
        //Palette reads are immediate
        res = ppu_bus_read(p, addr);

        //Internal buffer is still updated from mirrored nametable
        p->internal_data_buf = ppu_bus_read(p, addr - 0x1000);
    }
    else { //All other values are buffered
        res = p->internal_data_buf;
        p->internal_data_buf = ppu_bus_read(p, addr);
    }

    ppu_increment_vram_addr(p);

    return res;
}
//Internal ppu function to write data to an arbitrary address in ppu memory
void ppu_bus_write(ppu *p, uint16_t addr, uint8_t val) {
    addr &= 0x3FFF; //Make sure the address is in the 16kb range

    //Write to CHR ROM/RAM based on mapper defined function in ROM struct
    if(addr <= 0x1FFF) {
        p->rom->ppu_write(p->rom, addr, val);
    }
    //Nametables
    else if(addr <= 0x2FFF) {
        uint16_t mirrored = ppu_mirror_vram_addr(p, addr);
        p->vram[mirrored] = val;
        return;
    }
    //Nametable mirrors
    else if(addr <= 0x3EFF) {
        ppu_bus_write(p, addr - 0x1000, val);
    }
    //Palette tables
    else if(addr <= 0x3FFF) {
        //Palette region is mirrored every 32 bytes
        addr &= 0x1F;

        //Handling special mirrors
        if(addr == 0x10) addr = 0x00;
        if(addr == 0x14) addr = 0x04;
        if(addr == 0x18) addr = 0x08;
        if(addr == 0x1C) addr = 0x0C;
        p->palette_table[addr] = val;
    }
}
//Writes data to ppu memory using the address stored in the address register
void ppu_write_data(ppu *p, uint8_t val) {
    uint16_t addr = get_addr_register(p->addr_reg);
    ppu_bus_write(p, addr, val);
    ppu_increment_vram_addr(p);
}

//Gets the palette index of a specific tile pixel using the pattern table index (bank),
//specific tile index in that pattern table, and x and y coordinates of the
//pixel in the tile
uint8_t get_tile_pixel(ppu *p, size_t bank, size_t tile, int x, int y) {
    bank *= 0x1000;

    uint8_t upper = ppu_bus_read(p, y + bank + tile * 16);
    uint8_t lower = ppu_bus_read(p, 8 + y + bank + tile * 16);

    int bit = 7 - x;

    return (((lower >> bit) & 1) << 1) | ((upper >> bit) & 1);
}
//Sets the r/g/b value of a specific pixel in the frame
void set_frame_pixel(frame *fr, int x, int y, uint8_t rgb[3]) {
    int base = (y * 3 * FRAME_WIDTH) + (x * 3);

    if(base + 2 < FRAME_WIDTH * FRAME_HEIGHT * 3) {
        fr->data[base] = rgb[0];
        fr->data[base+1] = rgb[1];
        fr->data[base+2] = rgb[2];
    }
}

//Gets the palette of a background tile
void bg_palette(ppu *p, uint16_t nametable_base, size_t tile_x, size_t tile_y, uint8_t *colour) {
    size_t attr_table_idx = tile_y / 4 * 8 + tile_x / 4;
    uint8_t attr_byte = ppu_bus_read(p, nametable_base + 0x3C0 + attr_table_idx);
    uint8_t palette_idx;

    uint8_t x_quad = (tile_x % 4) >= 2 ? 1 : 0;
    uint8_t y_quad = (tile_y % 4) >= 2 ? 1 : 0;
    uint8_t shift = (y_quad * 2 + x_quad) * 2;  // 0, 2, 4, or 6
    palette_idx = (attr_byte >> shift) & 0b11;

    uint8_t palette_start = 1 + palette_idx * 4;
    colour[0] = p->palette_table[palette_start];
    colour[1] = p->palette_table[palette_start+1];
    colour[2] = p->palette_table[palette_start+2];
}

//Gets the palette of a sprite tile
void sprite_palette(ppu *p, uint8_t palette_idx, uint8_t *colour) {
    uint8_t palette_start = 0x11 + palette_idx * 4;
    colour[0] = p->palette_table[palette_start];
    colour[1] = p->palette_table[palette_start+1];
    colour[2] = p->palette_table[palette_start+2];
}
//Gets the data of every sprite that is rendererd on the current scanline
void evaluate_sprites(ppu *p) {
    p->secondary_oam_count = 0;
    uint8_t sprite_height = get_ppu_ctrl_reg_flag(p, PPU_CR_SPRITE_SIZE) ? 16 : 8;

    for(int i = 0; i < 64; i++) {
        uint8_t sprite_y = p->oam_data[i * 4];

        //If the sprite is not on the current line, skip it
        int diff_y = (int)p->scanline - (int)sprite_y;
        if(diff_y < 0 || diff_y >= sprite_height) continue;

        if(p->secondary_oam_count >= 8) {
            // Overflow if more than 8 sprites on a scanline
            set_ppu_stat_reg_flag(p, PPU_STAT_SPRITE_OVERFLOW, 1);
            break;
        }

        p->secondary_oam[p->secondary_oam_count].y = p->oam_data[i*4];
        p->secondary_oam[p->secondary_oam_count].tile = p->oam_data[i*4+1];
        p->secondary_oam[p->secondary_oam_count].attributes = p->oam_data[i*4+2];
        p->secondary_oam[p->secondary_oam_count].x = p->oam_data[i*4+3];
        // Track original OAM index for sprite zero hit detection
        p->secondary_oam[p->secondary_oam_count].oam_index   = i;
        p->secondary_oam_count++;
    }
}

//Mapper 4 has its own IRQ which needs to be handled on its own at the start of every scanline
void mapper_4_scanline(rom *r) {
    if(r->mapper_registers[3] == 0) r->mapper_registers[3] = r->mapper_registers[2]; //Reload from latch
    else r->mapper_registers[3]--;

    if(r->mapper_registers[3] == 0 && r->mapper_registers[4]) r->irq_pending = 1; //Signal IRQ to CPU
}

//Represents the ppu operations every clock cycle
uint8_t ppu_tick(ppu *p, frame *fr) {
    //Check if background/sprite rendering is enabled
    uint8_t bg_enabled     = (p->mask_reg >> 3) & 1;
    uint8_t sprite_enabled = (p->mask_reg >> 4) & 1;
    uint8_t colour = 0;

    //Draw a pixel
    if(p->scanline < 240 && p->cycles < 256) {
        if(bg_enabled) {
            //Drawing background
            uint8_t bank = get_ppu_ctrl_reg_flag(p, PPU_CR_BACKGROUND_PATTERN_ADDR); //Get the bank to get the tile from

            //Get the scroll position:
            uint8_t scroll_x = p->scroll_reg->xscroll;
            uint8_t scroll_y = p->scroll_reg->yscroll;

            //Get the absolute screen position
            size_t abs_x = p->cycles + scroll_x;
            size_t abs_y = p->scanline + scroll_y;

            //Get the tile position
            size_t tile_x = (abs_x % FRAME_WIDTH) / 8;
            size_t tile_y = (abs_y % FRAME_HEIGHT) / 8;

            //Get the pixel position
            size_t pixel_x = abs_x % 8;
            size_t pixel_y = abs_y % 8;

            uint16_t nametable_base = 0x2000 + ((p->ctrl_reg & 0x03) * 0x400);
            if(abs_x >= FRAME_WIDTH) nametable_base ^= 0x0400; //Horizontal flip if scroll crosses horizontal boundary
            if(abs_y >= FRAME_HEIGHT) nametable_base ^= 0x0800; //Vertical flip if scroll crosses vertical boundary

            //Get the tile
            uint16_t tile = ppu_bus_read(p, nametable_base + tile_y * 32 + tile_x);

            //Get the colour used for the tile
            colour = get_tile_pixel(p, bank, tile, pixel_x, pixel_y);

            //Get the palette
            uint8_t pal[3] = {0};
            bg_palette(p, nametable_base, tile_x, tile_y, pal);

            //Draw the pixel to the frame
            switch (colour) {
                case 0:
                    set_frame_pixel(fr, p->cycles, p->scanline, system_palette[p->palette_table[0]]);
                break;
                case 1:
                    set_frame_pixel(fr, p->cycles, p->scanline, system_palette[pal[0]]);
                break;
                case 2:
                    set_frame_pixel(fr, p->cycles, p->scanline, system_palette[pal[1]]);
                break;
                case 3:
                    set_frame_pixel(fr, p->cycles, p->scanline, system_palette[pal[2]]);
                break;
                default:
                    fprintf(stderr, "ERROR: Wrong colour index\n");
                break;
            }
        }
        //Drawing sprites
        if(sprite_enabled) {
            //Get sprite height
            uint8_t sprite_height = get_ppu_ctrl_reg_flag(p, PPU_CR_SPRITE_SIZE) ? 16 : 8;

            for(int i = 0; i < p->secondary_oam_count; i++) {
                //Check if this sprite falls on the current pixel
                int diff_x = (int)p->cycles - (int)p->secondary_oam[i].x;
                int diff_y = (int)p->scanline - (int)p->secondary_oam[i].y;

                if(diff_x < 0 || diff_x > 7) continue;
                if(diff_y < 0 || diff_y >= sprite_height) continue;

                //Extract attribute fields:
                uint8_t flip_h = (p->secondary_oam[i].attributes >> 6) & 1;
                uint8_t flip_v = (p->secondary_oam[i].attributes >> 7) & 1;
                uint8_t priority = (p->secondary_oam[i].attributes >> 5) & 1;
                uint8_t pal_idx = p->secondary_oam[i].attributes & 0x03;

                //Apply flipping
                int px = flip_h ? (7 - diff_x) : diff_x;
                int py = flip_v ? (7 - diff_y) : diff_y;

                //Get sprite pattern table and tile index depending on whether its a normal or large sprite
                uint8_t pat_table;
                uint8_t tile_idx;
                if(sprite_height == 16) {
                    pat_table = p->secondary_oam[i].tile & 0x1; //Get the bank from bit 0;
                    tile_idx = p->secondary_oam[i].tile & 0xFE; //Tile index is stored in rest of bits
                }
                else {
                    pat_table = get_ppu_ctrl_reg_flag(p, PPU_CR_SPRITE_PATTERN_ADDR);
                    tile_idx = p->secondary_oam[i].tile;
                }

                //For 8x16 sprites, need to select which tile we are using based on py
                if(sprite_height == 16) {
                    if(flip_v) { //When flipped, bottom half uses base tile, top half uses tile+1
                        if(py >= 8) py -= 8; //Set py back so we use the correct pixel in top tile on bottom half
                        else tile_idx += 1; //Otherwise increment the tile index so we use the bottom tile to draw the top half
                    }
                    else if(py >= 8) { //If the pixel is in the bottom half, need to move to the next tile and adjust py to match
                        tile_idx += 1;
                        py -= 8;
                    }
                }

                //Get the pixel colour index
                uint8_t sprite_colour = get_tile_pixel(p, pat_table, tile_idx, px, py);

                //0 means transparent for sprites
                if(sprite_colour == 0) continue;

                //Priority: If bit set, sprite is behind background (only draw over colour 0)
                if(priority && colour != 0) continue;

                //Sprite zero hit detection
                if(p->secondary_oam[i].oam_index == 0) set_ppu_stat_reg_flag(p, PPU_STAT_SPRITE_ZERO_HIT, 1);

                //Get palette and draw the sprite
                uint8_t pal[3] = {0};
                sprite_palette(p, pal_idx, pal);

                switch(sprite_colour) {
                    case 1: set_frame_pixel(fr, p->cycles, p->scanline, system_palette[pal[0]]); break;
                    case 2: set_frame_pixel(fr, p->cycles, p->scanline, system_palette[pal[1]]); break;
                    case 3: set_frame_pixel(fr, p->cycles, p->scanline, system_palette[pal[2]]); break;
                }
            }
        }
    }

    //Advance one dot
    p->cycles++;

    //Odd frames end the last scanline one cycle early to keep up with the cpu
    if(p->odd_frame && p->scanline == 261 && p->cycles == 339) {
        p->cycles = 0;
        p->scanline = 0;
        p->odd_frame = !p->odd_frame;
        return 1;
    }

    //Pre-render scanline
    if(p->scanline == 261 && p->cycles == 1) {
        set_ppu_stat_reg_flag(p, PPU_STAT_VBLANK, 0);
        set_ppu_stat_reg_flag(p, PPU_STAT_SPRITE_ZERO_HIT, 0);
        set_ppu_stat_reg_flag(p, PPU_STAT_SPRITE_OVERFLOW, 0);
    }

    //End of scanline
    if(p->cycles >= 341) {
        p->cycles = 0;
        p->scanline++;

        if(p->scanline < 241) {
            evaluate_sprites(p);
            if(p->rom->mapper == 4) mapper_4_scanline(p->rom);
        }

        //Enter VBlank
        if(p->scanline == 241) {
            set_ppu_stat_reg_flag(p, PPU_STAT_VBLANK, 1);
            set_ppu_stat_reg_flag(p, PPU_STAT_SPRITE_ZERO_HIT, 0);
            set_ppu_stat_reg_flag(p, PPU_STAT_SPRITE_OVERFLOW, 0);
            if(get_ppu_ctrl_reg_flag(p, PPU_CR_GENERATE_NMI)) {
                p->nmi_triggered = 1;
            }
        }

        //End of frame
        if(p->scanline >= 262) {
            p->scanline = 0;
            p->odd_frame = !p->odd_frame;
            return 1;
        }
    }

    return 0;
}

//Calculates the projection matrix
void ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            dest[i][j] = 0;
        }
    }

    float rl = 1.0 / (right  - left);
    float tb = 1.0 / (top    - bottom);
    float mfn =-1.0 / (farZ - nearZ);

    dest[0][0] = 2.0 * rl;
    dest[1][1] = 2.0 * tb;
    dest[2][2] = 2.0 * mfn;
    dest[3][0] =-(right  + left) * rl;
    dest[3][1] =-(top    + bottom) * tb;
    dest[3][2] = (farZ + nearZ) * mfn;
    dest[3][3] = 1.0;
}

//Compiles a shader from a source file given the desired shader type
unsigned int create_shader(const char **src, int shader_type) {
    unsigned int shader;
    shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, src, NULL);
    glCompileShader(shader);

    int result;
    char infolog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(!result) {
        glGetShaderInfoLog(shader, 512, NULL, infolog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n");
        for(int i = 0; i < 512; i++){
            if(infolog[i] == '\0') break;
            printf("%c", infolog[i]);
        }
        printf("\n");
    }

    return shader;
}

//Shaders for this program are simple enough that we can just encode them as strings
//to avoid annoying file loading/reading every startup
const char *vertex_shader = "#version 330 core\n"
                            "layout (location = 0) in vec2 aPos;\n"
                            "layout (location = 1) in vec2 aTexCoord;\n"
                            "uniform mat4 uProjection;\n"
                            "out vec2 TexCoord;\n"
                            "void main() {\n"
                            "    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);\n"
                            "    TexCoord = aTexCoord;\n"
                            "}\n";
const char *fragment_shader = "#version 330 core\n"
                              "in vec2 TexCoord;\n"
                              "out vec4 FragColor;\n"
                              "uniform sampler2D screenTexture;\n"
                              "void main() {\n"
                              "    FragColor = texture(screenTexture, TexCoord);\n"
                              "}\n";

//Initialises the pixel renderer
Renderer render_init(void) {
    Renderer r = {0};
    r.screen_height = INITIAL_SCREEN_HEIGHT;
    r.screen_width = INITIAL_SCREEN_WIDTH;

    //Setting up the texture for the pixel simulations:
    glGenTextures(1, &r.pixel_tex); //Only use one texture for the pixels that we just write to. Could switch to two and swap them out (like framebuffers)
    glBindTexture(GL_TEXTURE_2D, r.pixel_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); //Setting it to use rgba colours
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Setting up the pbo for the pixel simulations
    glGenBuffers(1, &r.pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, r.pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, FRAME_WIDTH* FRAME_HEIGHT * 3, NULL, GL_STREAM_DRAW);
    r.pixels = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(r.pixels) {
        memset(r.pixels, 0x00, FRAME_WIDTH * FRAME_HEIGHT * 3); //Setting all of the pixels to be colourless initially
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    //Getting the shader for this renderer
    r.shader = glCreateProgram();
    unsigned int vert = create_shader(&vertex_shader, GL_VERTEX_SHADER);
    unsigned int frag = create_shader(&fragment_shader, GL_FRAGMENT_SHADER);
    glAttachShader(r.shader, vert);
    glAttachShader(r.shader, frag);
    glLinkProgram(r.shader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    float quadVertices[] = {
        // positions         // texCoords
        0.0f, 0.0f,          0.0f, 0.0f,                   // top-left
        0.0f, r.screen_height, 0.0f, 1.0f,                   // bottom-left
        r.screen_width,        r.screen_height,  1.0F, 1.0F,   // BOTTOM-RIGHT
        r.screen_width, 0.0F,  1.0F, 0.0F                    // TOP-RIGHT
    };
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &r.vao);
    glGenBuffers(1, &r.vbo);
    glGenBuffers(1, &r.ebo);

    glBindVertexArray(r.vao);

    glBindBuffer(GL_ARRAY_BUFFER, r.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Setting the projection matrix
    ortho(0.0f, r.screen_width, r.screen_height, 0.0f, -1.0f, 1.0f, r.projection);

    return r;
}

//Frees a pixel renderer
void render_free(Renderer *r) {
    glDeleteBuffers(1, &r->vbo);
    glDeleteBuffers(1, &r->pbo);
    glDeleteVertexArrays(1, &r->vao);

    glDeleteProgram(r->shader);
    glDeleteTextures(1, &r->pixel_tex);
}

//Sets up the variables for renderering to the pbo from the Renderer
void render_begin(Renderer *r) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, r->pbo); //Binding the pbo
    glBufferData(GL_PIXEL_UNPACK_BUFFER, FRAME_HEIGHT * FRAME_WIDTH * 3, NULL, GL_STREAM_DRAW); //Forcing the gpu to discard any data it is currently using
    r->pixels = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(r->pixels) {
        memset(r->pixels, 0x00, FRAME_HEIGHT * FRAME_WIDTH * 3); //Set the pixels to be blank to start with to avoid noise
    }
}

//Ends rendering to the current pixel frame
void render_end(Renderer *r) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, r->pbo); //Get the pbo
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glBindTexture(GL_TEXTURE_2D, r->pixel_tex); //Get the texture used for rendering the pixels
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, FRAME_WIDTH, FRAME_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glUseProgram(r->shader);
    int proj_loc = glGetUniformLocation(r->shader, "uProjection"); //Use the matrix projection
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *)r->projection);

    // Bind texture to correct unit and uniform
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, r->pixel_tex);
    glUniform1i(glGetUniformLocation(r->shader, "screenTexture"), 0);
    glBindVertexArray(r->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

//Draws the entire frame directly on the screen by copying its entire contents into the renderer's pixel buffer
void draw_frame(Renderer *r, frame *fr) {
    for(int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT*3; i++) {
        r->pixels[i] = fr->data[i];
    }
}
