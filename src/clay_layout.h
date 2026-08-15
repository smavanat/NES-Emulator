#ifndef __CLAY_LAYOUT_H__
#define __CLAY_LAYOUT_H__
#include "cpu.h"

#include "../externals/bob.h"
#include "../externals/clay.h"

extern BOB_Font_Handle bitmap;

typedef struct {
    BOB_Vector2 mouse_pos;
    BOB_Vector2 scroll_pos;
    uint8_t mouse_down;
} mouse_state;

void clay_init(BOB_Renderer_Handle r, size_t width, size_t height);
void clay_update_dimensions(BOB_Renderer_Handle r, mouse_state *mstate, float dt, size_t width, size_t height);
Clay_RenderCommandArray clay_set_layout(cpu *c, uint8_t *frame_data, float dt, char *disassembly_buf, size_t disassembly_buf_sz);
void clay_render(BOB_Renderer_Handle r, BOB_Font_Handle bitmap, Clay_RenderCommandArray renderCommands, BOB_Pixelbuffer_Handle pb);
void clay_free();

#endif //__CLAY_LAYOUT_H__
