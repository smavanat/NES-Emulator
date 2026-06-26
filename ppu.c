#include "ppu.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "externals/glad.h"

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

void write_to_ctrl_reg(ppu *p, uint8_t val) {
    uint8_t vblank_status = get_ppu_ctrl_reg_flag(p, PPU_CR_GENERATE_NMI);
    p->ctrl_reg = val;
    if(!vblank_status && get_ppu_ctrl_reg_flag(p, PPU_CR_GENERATE_NMI) && get_ppu_stat_reg_flag(p, PPU_STAT_VBLANK)) {
        p->nmi_triggered = 1;
    }
}

uint8_t vram_addr_increment(ppu *p) {
    if(get_ppu_ctrl_reg_flag(p, PPU_CR_VRAM_ADD_INCREMENT)) return 32; else return 1;
}

void set_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag, uint8_t cond) {
    if(cond) p->status_reg |= (uint8_t)(1 << flag);
    else p->status_reg &= (uint8_t)~(1 << flag);
}

uint8_t get_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag) {
    if(p->status_reg & (1 << flag)) return 1;
    else return 0;
}

void ppu_increment_vram_addr(ppu *p) {
    increment_addr_register(p->addr_reg, vram_addr_increment(p));
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
    uint16_t addr = get_addr_register(p->addr_reg);
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
        fprintf(stderr, "Address Space 0x3000 ... 0x3eff is not expected to be used, requested address: %02X\n", addr);
    }
    else if(addr <= 0x3fff) {
        return p->palette_table[addr - 0x3f00];
    }
    else {
        fprintf(stderr, "Unexpected access to mirrored space, requested address: %02X\n", addr);
    }

    return 1;
}

uint8_t get_tile_pixel(const uint8_t *chr_rom, size_t bank, size_t tile, int x, int y) {
    bank *= 0x1000;

    const uint8_t *tile_data = &chr_rom[bank + tile * 16];

    uint8_t upper = tile_data[y];
    uint8_t lower = tile_data[y + 8];

    int bit = 7 - x;

    return (((upper >> bit) & 1) << 1) | ((lower >> bit) & 1);
}

// uint8_t ppu_tick(ppu *p, size_t cycles) {
uint8_t ppu_tick(ppu *p, frame *fr) {

    if(p->scanline < 240 && p->cycles < 256) {
        uint8_t bank = get_ppu_ctrl_reg_flag(p, PPU_CR_BACKGROUND_PATTERN_ADDR);
        int tile_x = p->cycles / 8;
        int tile_y = p->scanline / 8;

        int pixel_x = p->cycles % 8;
        int pixel_y = p->scanline % 8;

        uint16_t tile = p->vram[tile_y * 32 + tile_x];

        uint8_t colour = get_tile_pixel(p->chr_rom, bank, tile, pixel_x, pixel_y);

        switch (colour) {
            case 0:
                set_frame_pixel(fr, tile_x * 8 + pixel_x, tile_y * 8 + pixel_y, system_palette[0x01]);
            break;
            case 1:
                set_frame_pixel(fr, tile_x * 8 + pixel_x, tile_y * 8 + pixel_y, system_palette[0x23]);
            break;
            case 2:
                set_frame_pixel(fr, tile_x * 8 + pixel_x, tile_y * 8 + pixel_y, system_palette[0x27]);
            break;
            case 3:
                set_frame_pixel(fr, tile_x * 8 + pixel_x, tile_y * 8 + pixel_y, system_palette[0x30]);
            break;
            default:
                fprintf(stderr, "ERROR: Wrong colour index\n");
            break;
        }
    }

    //Advance one dot
    p->cycles++;

    //End of scanline
    if(p->cycles >= 341) {
        p->cycles = 0;
        p->scanline++;

        //Enter VBlank
        if(p->scanline == 241) {
            if(get_ppu_ctrl_reg_flag(p, PPU_CR_GENERATE_NMI)) {
                set_ppu_stat_reg_flag(p, PPU_STAT_VBLANK, 1);
                p->nmi_triggered = 1;
            }
        }

        //End of Frame
        if(p->scanline >= 262) {
            p->scanline = 0;
            set_ppu_stat_reg_flag(p, PPU_STAT_VBLANK, 0);
            return 1;
        }
    }

    return 0;
}

void set_frame_pixel(frame *fr, int x, int y, uint8_t rgb[3]) {
    int base = (y * 3 * FRAME_WIDTH) + (x * 3);

    if(base + 2 < FRAME_WIDTH * FRAME_HEIGHT * 3) {
        fr->data[base] = rgb[0];
        fr->data[base+1] = rgb[1];
        fr->data[base+2] = rgb[2];
    }
}

frame show_tile(uint8_t *chr_rom, size_t chr_sz, size_t bank, size_t tile_n) {
    assert(bank <= 1);

    frame fr = {0};
    bank *= 0x1000;
    uint8_t tile[16];

    memcpy(tile, &chr_rom[bank + tile_n * 16], 16 * sizeof(uint8_t));

    for(int y = 0; y < 8; y++) {
        uint8_t upper = tile[y];
        uint8_t lower = tile[y + 8];

        for(int x = 7; x > -1; x--) {
            uint8_t value = (1 & upper ) << 1 | (1 & lower);
            upper >>= 1;
            lower >>= 1;

            switch (value) {
                case 0:
                    set_frame_pixel(&fr, x, y, system_palette[0x01]);
                break;
                case 1:
                    set_frame_pixel(&fr, x, y, system_palette[0x23]);
                break;
                case 2:
                    set_frame_pixel(&fr, x, y, system_palette[0x27]);
                break;
                case 3:
                    set_frame_pixel(&fr, x, y, system_palette[0x30]);
                break;
                default:
                    fprintf(stderr, "ERROR: Wrong colour index\n");
                break;
            }
        }
    }

    return fr;
}

void set_tile(ppu *p, frame *fr) {
    uint8_t bank = get_ppu_ctrl_reg_flag(p, PPU_CR_BACKGROUND_PATTERN_ADDR);
    bank *= 0x1000;

    for(int i = 0; i < 0x03c0; i++) {
        uint16_t tile_n = p->vram[i];
        uint16_t tile_x = i % 32;
        uint16_t tile_y = i / 32;

        uint8_t tile[16];
        memcpy(tile, &p->chr_rom[bank + tile_n * 16], 16 * sizeof(uint8_t));

        for(int y = 0; y < 8; y++) {
            uint8_t upper = tile[y];
            uint8_t lower = tile[y + 8];

            for(int x = 7; x > -1; x--) {
                uint8_t value = (1 & upper ) << 1 | (1 & lower);
                upper >>= 1;
                lower >>= 1;

                switch (value) {
                    case 0:
                        set_frame_pixel(fr, tile_x * 8 + x, tile_y * 8 + y, system_palette[0x01]);
                    break;
                    case 1:
                        set_frame_pixel(fr, tile_x * 8 + x, tile_y * 8 + y, system_palette[0x23]);
                    break;
                    case 2:
                        set_frame_pixel(fr, tile_x * 8 + x, tile_y * 8 + y, system_palette[0x27]);
                    break;
                    case 3:
                        set_frame_pixel(fr, tile_x * 8 + x, tile_y * 8 + y, system_palette[0x30]);
                    break;
                    default:
                        fprintf(stderr, "ERROR: Wrong colour index\n");
                    break;
                }
            }
        }
    }
}

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
/**
 * Initialises the pixel renderer
 * @param r the pixel renderer to initialise
 * @param vert_path the filepath to load the vertex shader from
 * @param frag_path the filepath to load the fragment shader from
 */
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

/**
 * Frees a pixel renderer
 * @param r the pixel renderer to free
 */
void render_free(Renderer *r) {
    glDeleteBuffers(1, &r->vbo);
    glDeleteBuffers(1, &r->pbo);
    glDeleteVertexArrays(1, &r->vao);

    glDeleteProgram(r->shader);
    glDeleteTextures(1, &r->pixel_tex);
}

/**
 * Sets up the variables for renderering to the pbo from the pixel_renderer
 * @param r the pixel render to begin the pixel frame for
 */
void render_begin(Renderer *r) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, r->pbo); //Binding the pbo
    glBufferData(GL_PIXEL_UNPACK_BUFFER, FRAME_HEIGHT * FRAME_WIDTH * 3, NULL, GL_STREAM_DRAW); //Forcing the gpu to discard any data it is currently using
    r->pixels = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(r->pixels) {
        memset(r->pixels, 0x00, FRAME_HEIGHT * FRAME_WIDTH * 3); //Set the pixels to be blank to start with to avoid noise
    }
}

/**
 * Ends rendering to the current pixel frame
 * @param r the pixel_renderer whose frame should end
 */
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

/**
 * Draws the entire grid directly on the screen by copying its entire contents into the renderer's pixel buffer
 * @param r the pixel renderer to render to
 * @oaram g the pixel grid whose pixels to use
 */
void draw_frame(Renderer *r, frame *fr) {
    for(int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT*3; i++) {
        r->pixels[i] = fr->data[i];
    }
    // memcpy(r->pixels, fr->data, FRAME_HEIGHT * FRAME_WIDTH * 3);
}

/**
 * Draws a pixel to the pixel buffer
 * @param r the pixel renderer to render to
 * @param position where the pixel should be rendererd in the texture
 * @param colour the colour of the pixel
 */
void draw_pixel(Renderer *r, uint32_t position, uint8_t colour[3]) {
    if(!r->pixels) {
        printf("Pixel Buffer is NULL!!\n");
        return;
    }
    if (position >= 0 && position < FRAME_HEIGHT * FRAME_WIDTH) { //Boundary check
        for(int i = 0; i < 3; i++) {
            r->pixels[(position*3)+i] = colour[i];
        }
    }
}

