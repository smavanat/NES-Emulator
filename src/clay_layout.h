#ifndef __CLAY_LAYOUT_H__
#define __CLAY_LAYOUT_H__
#include "renderer.h"
#include "bitmap_font.h"
#include "../externals/clay.h"

typedef struct {
    NES_Vector2 mouse_pos;
    NES_Vector2 scroll_pos;
    uint8_t mouse_down;
} mouse_state;

void clay_init(Renderer *r);
void clay_update_dimensions(Renderer *r, mouse_state *mstate, float dt);
Clay_RenderCommandArray clay_set_layout(float dt);
void clay_render(Renderer *r, Bitmap_Font_Desc *bitmap, Clay_RenderCommandArray renderCommands);

#endif //__CLAY_LAYOUT_H__
