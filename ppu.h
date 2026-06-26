#ifndef __PPU_H__
#define __PPU_H__
#include <stddef.h>
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
    PPU_STAT_VBLANK = 7,
    PPU_STAT_SPRITE_ZERO_HIT = 6,
    PPU_STAT_SPRITE_OVERFLOW = 5,
} ppu_stat_flag;

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
    size_t cycles;
    size_t chr_rom_sz;
    uint16_t scanline;
    uint8_t ctrl_reg;
    uint8_t mask_reg;
    uint8_t status_reg;
    uint8_t oamaddr_reg;
    uint8_t oamdata_reg;
    uint8_t data_reg;
    uint8_t oamdma_reg;
    uint8_t internal_data_buf;
    uint8_t nmi_triggered;
    mirroring mirroring;
} ppu;

#define FRAME_WIDTH 256
#define FRAME_HEIGHT 240
#define INITIAL_SCREEN_WIDTH 800
#define INITIAL_SCREEN_HEIGHT 600

typedef struct {
    uint8_t data[FRAME_WIDTH * FRAME_HEIGHT * 3];
} frame;

void set_frame_pixel(frame *fr, int x, int y, uint8_t rgb[3]);
frame show_tile(uint8_t *chr_rom, size_t chr_sz, size_t bank, size_t tile_n);
void set_tile(ppu *p, frame *fr);

//Control Register functions
void set_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag, uint8_t cond);
uint8_t get_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag);
void write_to_ctrl_reg(ppu *p, uint8_t val);
uint8_t vram_addr_increment(ppu *p);

//Status register functions
void set_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag, uint8_t cond);
uint8_t get_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag);

//PPU functions
void ppu_increment_vram_addr(ppu *p);
uint16_t ppu_mirror_vram_addr(ppu *p, uint16_t addr);
uint8_t ppu_read_data(ppu *p);
uint8_t ppu_tick(ppu *p, frame *fr);

extern uint8_t system_palette[64][3];

typedef float mat4[4][4];

typedef struct {
    uint32_t vao; //vao this renderer uses
    uint32_t vbo; //vbo this renderer uses
    uint32_t ebo; //ebo this renderer uses
    uint32_t shader; //shader this renderer uses
    mat4 projection; //projection matrix for this renderer

    uint32_t pbo; //pbo this renderer uses
    uint8_t *pixels; //Array of pixel data
    uint32_t pixel_tex; //The texture the pbo is rendered to

    uint32_t screen_height;
    uint32_t screen_width;
} Renderer;

/**
 * Initialises the pixel renderer
 * @param r the pixel renderer to initialise
 * @param vert_path the filepath to load the vertex shader from
 * @param frag_path the filepath to load the fragment shader from
 */
Renderer render_init(void);
/**
 * Frees a pixel renderer
 * @param r the pixel renderer to free
 */
void render_free(Renderer *r);
/**
 * Sets up the variables for renderering to the pbo from the Renderer
 * @param r the pixel render to begin the pixel frame for
 */
void render_begin(Renderer *r);
/**
 * Ends rendering to the current pixel frame
 * @param r the Renderer whose frame should end
 */
void render_end(Renderer *r);
/**
 * Draws a pixel to the pixel buffer
 * @param r the pixel renderer to render to
 * @param position where the pixel should be rendererd in the texture
 * @param colour the colour of the pixel
 */
void draw_pixel(Renderer *r, uint32_t position, uint8_t colour[4]);
/**
 * Draws the entire grid directly on the screen by copying its entire contents into the renderer's pixel buffer
 * @param r the pixel renderer to render to
 * @oaram g the pixel grid whose pixels to use
 */
void draw_frame(Renderer *r, frame *fr);

void ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest);
#endif
