#ifndef __PPU_H__
#define __PPU_H__
#include <stddef.h>
#include <stdint.h>
#include "../externals/glad.h"

//Struct to represet the 16 byte value stored in the ppu's address register
typedef struct {
    uint8_t h_byte;
    uint8_t l_byte;
    uint8_t h_ptr; //Determines which byte (hi/lo) should be read in the current operation
} addr_register;

//Address register functions

//Sets the address register to an explicit 16 bit address
void set_addr_register(addr_register *reg, uint16_t val);
//Gets the address stored in the address register
uint16_t get_addr_register(addr_register *reg);
//Updates the byte in the address register pointed to by h_ptr
void update_addr_register(addr_register *reg, uint8_t val);
//Increments the value in the address register by 1
void increment_addr_register(addr_register *reg, uint8_t val);

//Struct to represet the 16 byte value stored in the ppu's scroll register
typedef struct {
    uint8_t xscroll;
    uint8_t yscroll;
    uint8_t s_ptr; //Determines which byte (x/y) should be read in the current operation
} scroll_register;

//Scroll register functions
//Sets the scroll register to an explicit 16 bit value
void set_scroll_register(scroll_register *reg, uint16_t val);
//Gets the value stored in the scroll register
uint16_t get_scroll_register(scroll_register *reg);
//Updates the byte in the address register pointed to by s_ptr
void update_scroll_register(scroll_register *reg, uint8_t val);

//Control register flags
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

//Status register flags (bits 0-4 are not used)
typedef enum {
    PPU_STAT_VBLANK = 7,
    PPU_STAT_SPRITE_ZERO_HIT = 6,
    PPU_STAT_SPRITE_OVERFLOW = 5,
} ppu_stat_flag;

//Types of nametable mirroring
typedef enum {
    MIR_VERTICAL,
    MIR_HORIZONTAL,
    MIR_FOURSCREEN,
    MIR_ONESCREEN_LO,
    MIR_ONESCREEN_HI,
} mirroring;

//Represents a sprite entry for the current scanline
//Used by the ppu to predetermine the sprites
//that could be rendered on the current scanline
//to speed up rendering
typedef struct {
    uint8_t y;
    uint8_t tile;
    uint8_t attributes;
    uint8_t x;
    uint8_t oam_index;
} sprite_entry;

typedef struct rom rom; //Need to typedef it early for use in the function pointers

//Represents the ROM cartridge inserted into a real NES
struct rom {
    //Memory regions
    size_t prg_rom_sz;
    size_t prg_ram_sz;
    size_t prg_eeprom_sz;
    size_t chr_rom_sz;
    size_t chr_ram_sz;
    uint8_t *prg_rom;
    uint8_t *prg_ram;
    uint8_t *prg_eeprom;
    uint8_t *chr_rom;
    uint8_t *chr_ram;

    //Virtual function pointers for mapper specific behaviours
    uint8_t (*cpu_read)(rom *r, uint16_t addr);
    void (*cpu_write)(rom *r, uint16_t addr, uint8_t val);
    uint8_t (*ppu_read)(rom *r, uint16_t addr);
    void (*ppu_write)(rom *r, uint16_t addr, uint8_t val);

    //Mapper state
    uint16_t mapper;
    uint8_t submapper;
    mirroring mirroring;
    uint8_t prg_bank;
    uint8_t chr_bank;
    uint8_t irq_pending;
    uint8_t battery_memory;
    uint8_t mapper_registers[16];
};

//Loads a rom from a buffer
int rom_load(rom *r, uint8_t *buf, int buf_len);

typedef struct {
    //Memory regions
    uint8_t vram[2048];
    uint8_t oam_data[256];
    uint8_t palette_table[32];
    sprite_entry secondary_oam[8]; //Buffer used to store sprite data for the current scanline
    rom *rom;

    //Registers
    addr_register *addr_reg;
    scroll_register *scroll_reg;
    uint8_t ctrl_reg;
    uint8_t mask_reg;
    uint8_t status_reg;
    uint8_t oamaddr_reg;
    uint8_t data_reg;
    uint8_t oamdma_reg;

    //Flags/internal value trackers
    uint8_t internal_data_buf; //Buffer to store data from previous read
    uint8_t nmi_triggered;
    size_t cycles;
    uint16_t scanline;
    uint8_t secondary_oam_count;
    uint8_t odd_frame;
} ppu;

#define FRAME_WIDTH 256
#define FRAME_HEIGHT 240
#define INITIAL_SCREEN_WIDTH 800
#define INITIAL_SCREEN_HEIGHT 600

typedef struct {
    uint8_t data[FRAME_WIDTH * FRAME_HEIGHT * 3];
} frame;

//Control Register functions

//Sets a specific control register flag based on a condition.
//1 if condition is true, 0 otherwise
void set_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag, uint8_t cond);
//Gets the bit value stored in a specific control register flag
uint8_t get_ppu_ctrl_reg_flag(ppu* p, ppu_cr_flag flag);
//Writes to control register. Checks if VBlank flag is set to true
//(while being not set before), and if GENERATE_NMI is also set,
//triggers an NMI
void write_to_ctrl_reg(ppu *p, uint8_t val);
//What value the VRAM address should be incremented by.
//1 means that we are going across, 32 means down
uint8_t vram_addr_increment(ppu *p);

//Status register functions

//Sets a specific status register flag based on a condition.
//1 if condition is true, 0 otherwise
void set_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag, uint8_t cond);
//Gets the bit value stored in a specific status register flag
uint8_t get_ppu_stat_reg_flag(ppu* p, ppu_stat_flag flag);

//PPU functions

//Increments the address register depending on the value of the PPU_CR_VRAM_ADD_INCREMENT
//flag in the control register
void ppu_increment_vram_addr(ppu *p);
//Converts a PPU nametable address into an index into VRAM
//Since all values in 0x3000 - 0x3eFF are mirrors of 0x2000 - 0x2EFF
//mirrors them down
//Supports Vertical/Horizontal, Fourscreen, and Onescreen mirroring
uint16_t ppu_mirror_vram_addr(ppu *p, uint16_t addr);
//Reads data from ppu memory using the address stored in the address register
uint8_t ppu_read_data(ppu *p);
//Writes data to ppu memory using the address stored in the address register
void ppu_write_data(ppu *p, uint8_t val);
//Represents the ppu operations every clock cycle
uint8_t ppu_tick(ppu *p, frame *fr);

//Array to store the colours used by the NES palette
//Represents the ppu operations every clock cycle
extern uint8_t system_palette[64][3];

typedef float mat4[4][4];

typedef struct {
    uint8_t *pixel_buf; //Array of pixel data
    size_t buf_sz;
    size_t width, height;
    uint32_t pbo; //pbo this renderer uses
    uint32_t pixel_tex; //The texture the pbo is rendered to
} PixelBuffer;

//Draws a frame straight to a texture by uploading it to a pixel buffer
void pixelbuffer_updload_frame(PixelBuffer *pb, frame *fr);
//Creates a pixel buffer to hold the pixels representing
//a texture of size width * height
//Pixel size should be either 3 or 4 (rgb/rgba)
PixelBuffer pixelbuffer_init(size_t width, size_t height, uint8_t pixel_size);
//Frees the data used by a pixel buffer
void pixelbuffer_free(PixelBuffer *pb);

//Arbitrary constants for now
#define MAX_TRIANGLES 2048
#define MAX_QUADS 4096

#define VERTICES_PER_QUAD 4
#define VERTICES_PER_TRIANGLE 3
#define INDECIES_PER_QUAD 6
#define INDECIES_PER_TRIANGLE 3
#define MAX_VERTICIES MAX_QUADS * VERTICES_PER_QUAD + MAX_TRIANGLES * VERTICES_PER_TRIANGLE
#define MAX_INDECIES MAX_QUADS * INDECIES_PER_QUAD + MAX_TRIANGLES * INDECIES_PER_TRIANGLE
#define INVALID_TEX_INDEX 1248
#define CIRCLE_LINE_SEGMENTS 64 //Number of line segments that make up the circumference of a circle

typedef struct {
    float x;
    float y;
    float w;
    float h;
} NES_Quad;

typedef struct {
    float x, y;
} NES_Vector2;

typedef struct {
    float x, y, z, w;
} NES_Vector4;

//Data structure to hold data about a single render vertex
typedef struct {
    NES_Vector2 pos; //The on-screen position of the render vertex
    NES_Vector4 colour; //The colour of the vertex
    NES_Vector2 uv; //The (u,v) coordinates of the vertex
} Render_Vertex;

typedef struct {
    uint32_t texture; //GL index of the atlas texture
    uint32_t width, height; //Dimensions of the atlas
    uint32_t cursor_x, cursor_y; //Current packing position
    uint32_t row_height; //Height of the tallest texture on the current row
    uint8_t pixel_size; //Number of bytes a pixel in the atlas takes up. Must be either 3 or 4
} TextureAtlas;

//Initialises a texture atlas
//Optionally packs a single white pixel at the start of the texture atlas to render a solid quad
TextureAtlas atlas_init(uint32_t width, uint32_t height, uint8_t pixel_size, uint8_t solid);
//Returns the UV rect where the texture was placed
//pixel_size must be either 3 or 4. If it does not match the pixel size of the atlas,
//an empty quad will returned as the pixel formats are different
NES_Quad atlas_pack(TextureAtlas *a, uint8_t* pixels, size_t w, size_t h, uint8_t pixel_size);
void atlas_free(TextureAtlas *a);

//TODO: Add a way of ordering textures in the renderer (some sort of depth filter)

//Represents a single batch sent off in a draw call from a texture atlas
typedef struct {
    Render_Vertex *vertex_data;
    uint32_t *index_data; //The index count (for ebo) for this renderer
    TextureAtlas *a;
    size_t vertex_size;
    size_t index_size;
    size_t vertex_count;
    size_t index_count;
} AtlasRenderBatch;

typedef struct {
    Render_Vertex *vertex_data;
    uint32_t *index_data; //The index count (for ebo) for this renderer
    size_t vertex_size;
    size_t index_size;
    size_t vertex_count;
    size_t index_count;
} DebugRenderBatch;

//Pixel renderer that renders a single frame
typedef struct {
    mat4 projection; //projection matrix for this renderer
    AtlasRenderBatch *rb;
    DebugRenderBatch *db;
    uint32_t vao; //vao this renderer uses
    uint32_t vbo; //vbo this renderer uses
    uint32_t ebo; //ebo this renderer uses
    uint32_t shader; //shader this renderer uses

    uint32_t screen_height;
    uint32_t screen_width;
} Renderer;

#define GET_ATLAS_BATCH(r) (r)->rb[(r)->atlas_batch_ptr]
#define GET_DEBUG_BATCH(r) (r)->db[(r)->debug_batch_ptr]

//Initialises the pixel renderer
Renderer render_init(TextureAtlas *a);
//Frees a pixel renderer
void render_free(Renderer *r);
//Sets up the variables for renderering to the pbo from the Renderer
void render_begin(Renderer *r);
//Ends rendering to the current pixel frame
void render_end(Renderer *r);
//Draws a quad
void render_draw_atlas_quad(Renderer *r, NES_Quad dimensions, NES_Quad uv_dimensions, NES_Vector4 colour);
//Draws a dynamically allocated texture
void render_draw_pixel_texture(Renderer *r, uint32_t texture, NES_Quad dimensions, NES_Quad uv_dimensions, NES_Vector4 colour);
//Draws a pixel buffer
void render_draw_pixel_buffer(Renderer *r, PixelBuffer *pb);
//Draws a filled circle
void render_draw_circle(Renderer *r, NES_Vector2 centre, float radius, NES_Vector4 colour);
//Draws a filled quad
void render_draw_quad(Renderer *r, NES_Quad quad, NES_Vector4 colour);
//TODO:Draws an unfilled circle
void render_draw_unfilled_circle(Renderer *r, NES_Vector2 centre, float radius, NES_Vector4 colour);
//Draws an unfilled quad
void render_draw_unfilled_quad(Renderer *r, NES_Quad quad, float thickness, NES_Vector4 colour);
//Draws a line between two points
void render_draw_line(Renderer *r, NES_Vector2 start_pos, NES_Vector2 end_pos, float thickness, NES_Vector4 colour);

//Determines the projection matrix
void ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest);
#endif
