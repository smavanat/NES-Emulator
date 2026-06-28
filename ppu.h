#ifndef __PPU_H__
#define __PPU_H__
#include <stddef.h>
#include <stdint.h>

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

//Pixel renderer that renders a single frame
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

//Initialises the pixel renderer
Renderer render_init(void);
//Frees a pixel renderer
void render_free(Renderer *r);
//Sets up the variables for renderering to the pbo from the Renderer
void render_begin(Renderer *r);
//Ends rendering to the current pixel frame
void render_end(Renderer *r);
//Draws the entire frame directly on the screen by copying its entire contents into the renderer's pixel buffer
void draw_frame(Renderer *r, frame *fr);

//Determines the projection matrix
void ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest);
#endif
