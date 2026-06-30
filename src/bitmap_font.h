#ifndef __BITMAP_FONT_H__
#define __BITMAP_FONT_H__
#include "renderer.h"
#include <stdint.h>
#include <stddef.h>

//TODO: support the .fnt metadata from AngelCode
//      And various other bitmap formats, not just loading from a monospaced png

//Way of telling the bitmap renderer what layout your bitmap uses
typedef enum {
    BITMAP_STANDARD, //Standard ASCII bitmap with chars from 32 to 126
    BITMAP_OFFSET, //Some subset of the standard ASCII ordering that still adheres to the original, e.g. having only chars 46 - 97
    BITMAP_CUSTOM, //A completely custom ordering of chars
} Bitmap_Layout;

union layout_desc{
    //Stores the jump table for a custom layout
    //Stores the offsets for a subset layout
    struct {} standard_desc;
    struct {
        size_t start_offset;
        size_t end_offset;
    } offset_desc;
    struct {
        char *data;
        size_t len;
    } custom_desc;
};

typedef struct {
    uint32_t atlas; //Reference to the TextureAtlas used to store the bitmap

    uint32_t tex_pixel_width;
    uint32_t tex_pixel_height;
    uint32_t char_pixel_width;
    uint32_t char_pixel_height;

    uint32_t char_padding_x; //Horizontal padding between chars on the atlas
    uint32_t char_padding_y; //Vertical padding between chars on the atlas
    uint32_t tex_border_padding_x;
    uint32_t tex_border_padding_y;

    Bitmap_Layout layout;
    union layout_desc desc;
} Bitmap_Font_Desc;

uint8_t bitmap_font_init(Bitmap_Font_Desc *opts, uint32_t atls, uint32_t tpw, uint32_t tph, uint32_t cpw, uint32_t cph, uint32_t cpx, uint32_t cpy, uint32_t tbpx, uint32_t tbpy, Bitmap_Layout lyt, union layout_desc desc);
void bitmap_font_free(Bitmap_Font_Desc *bf);

uint8_t bitmap_draw_char(Renderer *r, Bitmap_Font_Desc *bf, char c, NES_Quad dimensions, NES_Vector4 colour);
uint8_t bitmap_draw_string(Renderer *r, Bitmap_Font_Desc *bf, const char *str, size_t str_len, NES_Vector2 gap, NES_Vector2 start, NES_Vector2 scale, NES_Vector4 colour);
NES_Vector2 bitmap_measure_text(const char *str, size_t str_len, NES_Vector2 gap, NES_Vector2 scale);

#endif //__BITMAP_FONT_H__
