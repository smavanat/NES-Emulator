#include "bitmap_font.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../externals/glad.h"
#include "renderer.h"

uint8_t bitmap_font_init(Bitmap_Font_Desc *opts, uint32_t atls, uint32_t tpw, uint32_t tph, uint32_t cpw, uint32_t cph, uint32_t cpx, uint32_t cpy, uint32_t tbpx, uint32_t tbpy, Bitmap_Layout lyt, union layout_desc desc) {
    if(!opts) return 0;

    opts->atlas = atls;
    opts->tex_pixel_width = tpw;
    opts->tex_pixel_height = tph;
    opts->char_pixel_width = cpw;
    opts->char_pixel_height = cph;

    opts->char_padding_x = cpx; //Horizontal padding between chars on the atlas
    opts->char_padding_y = cpy; //Vertical padding between chars on the atlas
    opts->tex_border_padding_x = tbpx;
    opts->tex_border_padding_y = tbpy;

    opts->layout = lyt;
    opts->desc = desc;

    return 1;
}
void bitmap_font_free(Bitmap_Font_Desc *bf) {
    // glDeleteTextures(1, &bf->tex); //Replace with some render function to remove an atlas
    // if(bf->layout == BITMAP_CUSTOM) {
    //     free(bf->desc.custom_desc.data);
    //     bf->desc.custom_desc.data = NULL;
    // }
}

uint8_t bitmap_draw_char(Renderer *r, Bitmap_Font_Desc *bf, char c, NES_Quad dimensions, NES_Vector4 colour, uint8_t layer) {
    if(c < 32 || c > 126) {
        fprintf(stderr, "Trying to draw a non-ASCII character\n");
        return 0;
    }
    int index = -1;

    if(bf->layout == BITMAP_OFFSET) {
        index = c - 32;
        if(index < bf->desc.offset_desc.start_offset || index > (96 - bf->desc.offset_desc.end_offset)) {
            fprintf(stderr, "Char %c is not in the range specified by the offsets (%zu, %zu)\n", c, bf->desc.offset_desc.start_offset, bf->desc.offset_desc.end_offset);
            return 0;
        }
        index -= bf->desc.offset_desc.start_offset;
    }
    else if(bf->layout == BITMAP_CUSTOM) {
        //This is fine. There's only going to be a maximum of 96
        //elements in this array. There won't be a noticeable performance drop
        for(int i = 0; i < bf->desc.custom_desc.len; i++) {
            if(bf->desc.custom_desc.data[i] == c){
                index = i;
                break;
            }
        }

        if(index == -1) {
            fprintf(stderr, "The character \'%c\' was not specified in the bitmap's custom charset\n", c);
            return 0;
        }
    }
    else if(bf->layout == BITMAP_STANDARD) {
        index = c-32;
        if(index < 0 || index > 94) {
            fprintf(stderr, "Trying to draw a non-ASCII character\n");
            return 0;
        }
    }
    else {
        fprintf(stderr, "Unsupported layout\n");
        return 0;
    }

    size_t img_width_chars = (bf->tex_pixel_width + bf->char_padding_x - (bf->tex_border_padding_x * 2)) / (bf->char_pixel_width + bf->char_padding_x);

    uint32_t x_tiles = index % img_width_chars;
    uint32_t y_tiles = index / img_width_chars;

    uint32_t x_pixels = x_tiles * (bf->char_pixel_width + bf->char_padding_x) + bf->tex_border_padding_x;
    uint32_t y_pixels = y_tiles * (bf->char_pixel_height + bf->char_padding_y) + bf->tex_border_padding_y;

    render_draw_atlas_quad(r, dimensions, (NES_Quad){x_pixels, y_pixels, bf->char_pixel_width, bf->char_pixel_height}, colour, bf->atlas, layer);

    return 1;
}

uint8_t bitmap_draw_string(Renderer *r, Bitmap_Font_Desc *bf, const char *str, size_t str_len, NES_Vector2 gap, NES_Vector2 start, NES_Vector2 scale, NES_Vector4 colour, uint8_t layer) {
    float x = start.x;
    float y = start.y;

    for(size_t i = 0; i < str_len; i++) {
        switch (str[i]) {
            case '\n':
                x = start.x;
                y += scale.y + gap.y;
            continue;
            case '\t':
                x += (scale.x + gap.x) * 4;
            continue;
            case ' ':
                x += scale.x + gap.x;
            continue;
            default:
            break;
        }

        if(!bitmap_draw_char(r, bf, str[i], (NES_Quad){x, y, scale.x, scale.y}, colour, layer))
            return 0;

        x += scale.x + gap.x;
    }

    return 1;
}

NES_Vector2 bitmap_measure_text(const char *str, size_t str_len, NES_Vector2 gap, NES_Vector2 scale) {
    float max_w = 0;
    float h = scale.y + gap.y;
    float cur_w = 0;

    for(int i = 0; i < str_len; i++) {
        switch(str[i]) {
            case '\n':
                if(cur_w > max_w) max_w = cur_w;
                cur_w = 0;
                h += scale.y + gap.y;
            continue;
            case '\t':
                cur_w += (scale.x + gap.x) * 4;
            continue;
            default:
            break;
        }

        if(str[i] < 32 || str[i] > 126) {
            fprintf(stderr, "Measuring text with non-printable ASCII characters\n");
            return (NES_Vector2){-1, -1};
        }
        cur_w += scale.x + gap.x;
    }

    if(cur_w > max_w) max_w = cur_w;

    return (NES_Vector2){max_w, h};
}
