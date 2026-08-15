#ifndef CLAY_LAYOUT_H
#define CLAY_LAYOUT_H
#include "cpu.h"

#include "../externals/bob.h"
#include "../externals/clay.h"

typedef enum {
    JES_START,
    JES_GAME,
    JES_DEBUGGER,
} JES_Screen;

typedef struct {
    BOB_Vector2 mouse_pos;
    BOB_Vector2 scroll_pos;
    uint8_t mouse_down;
} mouse_state;

extern BOB_Font_Handle bitmap;
extern JES_Screen curr_screen;

void clay_init(BOB_Renderer_Handle r, size_t width, size_t height);
void clay_update_dimensions(BOB_Renderer_Handle r, mouse_state *mstate, float dt, size_t width, size_t height);
Clay_RenderCommandArray clay_set_debugger_layout(cpu *c, uint8_t *frame_data, float dt, char *disassembly_buf, size_t disassembly_buf_sz);
Clay_RenderCommandArray clay_set_start_layout(float dt);
Clay_RenderCommandArray clay_set_game_layout(cpu *c, uint8_t *frame_data, float dt);
void clay_render(BOB_Renderer_Handle r, BOB_Font_Handle bitmap, Clay_RenderCommandArray renderCommands, BOB_Pixelbuffer_Handle pb);
void clay_free();

#endif //CLAY_LAYOUT_H
