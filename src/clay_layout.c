//Implementation of helper functions and a layout in clay
#include "clay_layout.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ppu.h"
#include "renderer.h"
#include "bitmap_font.h"
#include "cpu.h"

#define CLAY_IMPLEMENTATION
#include "../externals/clay.h"

//Clay Colours to be reused
const Clay_Color COLOUR_WHITE = (Clay_Color){255, 255, 255, 255};
const Clay_Color COLOUR_BLACK = (Clay_Color){0, 0, 0, 255};
const Clay_Color COLOUR_LIGHT = (Clay_Color){224, 215, 210, 255};
const Clay_Color COLOUR_RED = (Clay_Color){168, 66, 28, 255};
const Clay_Color COLOUR_ORANGE = (Clay_Color){255, 138, 50, 255};

//TODO: unify all of the malloc calls in here into a global arena so that we don't keep allocating/freeing memory
char *cpu_state_buf;
char *ppu_state_buf;
char *cpu_page_buf;
PixelBuffer *nametable_buffer;

void HandleClayErrors(Clay_ErrorData errorData) {
    //NOTE: See Clay_ErrorData struct for more info
    printf("%s\n", errorData.errorText.chars);
    switch (errorData.errorType) {
        //TODO:
    }
}

//Example measure text function
static inline Clay_Dimensions MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, uintptr_t userData) {
    //Clay_TextElementConfig contains members such as fontId, fontSize, letterSpacing
    //Note: Clay_String->chars is not guaranteed to be null terminated
    return (Clay_Dimensions){
        .width = text.length * config->fontSize, //Only works for monospace fonts
        .height = config->fontSize
    };
}

//Layout config is just a struct that can be declared statically or inline
Clay_ElementDeclaration sidebarItemConfig = (Clay_ElementDeclaration){
    .layout = {
        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50)}
    },
    .backgroundColor = COLOUR_ORANGE
};

//Reusable components are just normal functions
void SidebarItemComponent(int index) {
    CLAY(CLAY_IDI("Sidebar", index), sidebarItemConfig) {
        //Children go here
    }
}

//Generates a reusable button component
void ButtonComponent(int id, Clay_Color basic_color, Clay_Color hover_color, uint16_t minX, uint16_t minY, uint16_t maxX, uint16_t maxY,
                     Clay_String hover_string, Clay_String regular_string, void (*button_hander)(Clay_ElementId id, Clay_PointerData info, void *userData),
                     void *userData, uint8_t padding, Clay_TextAlignment align) {
    CLAY(CLAY_IDI("Button", id), {.layout = {.sizing = {CLAY_SIZING_GROW(minX, maxX), CLAY_SIZING_GROW(minY, maxY)},
        .padding = CLAY_PADDING_ALL(padding)}, .backgroundColor = Clay_Hovered() ? hover_color : basic_color}) {
        Clay_OnHover(button_hander, userData);
        bool buttonHovered = Clay_Hovered();

        CLAY_TEXT(buttonHovered ? hover_string : regular_string, {.textColor = COLOUR_WHITE, .textAlignment = align});
    }
}

//Generates a component that holds a text printout of the CPU state:
void cpu_state_component(cpu *c) {
    int len = sprintf(cpu_state_buf, "Accumulator: %hhu \nX Register: %hhu \nY Register: %hhu \nStack Pointer: %02X \nPC: %04X\n"
                 "Status Register:\n Carry: %hhu\n Zero: %hhu\n Interrupt Disable: %hhu\n Decimal: %hhu\n Break: %hhu\n Overflow: %hhu\n Negative: %hhu",
             c->acc, c->x, c->y, c->sp, c->pc, get_cpu_flag(c, CPU_CARRY), get_cpu_flag(c, CPU_ZERO), get_cpu_flag(c, CPU_INTERRUPT_DISABLE), get_cpu_flag(c, CPU_DECIMAL),
           get_cpu_flag(c, CPU_BREAK), get_cpu_flag(c, CPU_OVERFLOW), get_cpu_flag(c, CPU_NEGATIVE));

    Clay_String str = (Clay_String){true, len, cpu_state_buf};
    CLAY(CLAY_ID("CPUSTATE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0)}}}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT});
    }
}

//Generates a component that holds a text printout of a given CPU page
void cpu_page_component(cpu *c, uint8_t page_num) {
    uint16_t start_addr = page_num * 256;
    int len = 0;

    for(int i = 0; i < 256; i++) {
        if(i > 0 && i % 8 == 0) {
            cpu_page_buf[len] = '\n';
            len++;
        }
        len += sprintf(cpu_page_buf + len, "%02X ", mem_read(c->b, start_addr+i));
    }
    // printf("%s\n", cpu_page_buf);
    Clay_String str = (Clay_String){true, len, cpu_page_buf};
    CLAY(CLAY_ID("CPUPAGE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0)}}}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT, .fontSize = 17});
    }
}


// Source - https://stackoverflow.com/a/3208376
// Posted by William Whyte, modified by community. See post 'Timeline' for change history
// Retrieved 2026-07-02, License - CC BY-SA 4.0
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

void ppu_state_component(ppu *p) {
    int len = sprintf(ppu_state_buf, "PPUCTRL:"BYTE_TO_BINARY_PATTERN"\nPPUMASK:"BYTE_TO_BINARY_PATTERN"\nPPUSTATUS:"BYTE_TO_BINARY_PATTERN
                 "\nOAMADDR: %02X\nOAMDATA: %02X\nPPUSCROLL: %04X\nPPUADDR: %04X\nPPUDATA: %02X\nOAMDMA: %02X\nNMI Triggered: %hhu\n",
                  BYTE_TO_BINARY(p->ctrl_reg), BYTE_TO_BINARY(p->mask_reg), BYTE_TO_BINARY(p->status_reg), p->oamaddr_reg, p->oamdma_reg,
                  get_scroll_register(p->scroll_reg), get_addr_register(p->addr_reg), p->data_reg, p->oamdma_reg, p->nmi_triggered);

    Clay_String str = (Clay_String){true, len, ppu_state_buf};
    CLAY(CLAY_ID("PPUSTATE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0)}}}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT});
    }
}

typedef struct {
    void (*render_func)(Clay_String str, Renderer *r, PixelBuffer *pb, void *data);
    void *data;
    Clay_String parent_name;
} Custom_Tex_Data;

void render_game(Clay_String str, Renderer *r, PixelBuffer *pb, void *data) {
    uint8_t * pixels = (uint8_t *)data;
    Clay_ElementId gId = Clay_GetElementId(str);
    Clay_ElementData gData = Clay_GetElementData(gId);

    NES_Quad dimensions = {gData.boundingBox.x, gData.boundingBox.y, gData.boundingBox.width, gData.boundingBox.height};

    pixelbuffer_updload_data(pb, data);
    render_draw_pixel_buffer(r, pb, dimensions, (NES_Quad){0,0,1,1}, (NES_Vector4){1,1,1,1}, 10);
}

void render_pattern_table(Clay_String str, Renderer *r, PixelBuffer *pb, void *data) {
    cpu *c = (cpu *)data;
    Clay_ElementId gId = Clay_GetElementId(str);
    Clay_ElementData gData = Clay_GetElementData(gId);

    NES_Quad dimensions = {gData.boundingBox.x, gData.boundingBox.y, gData.boundingBox.width, gData.boundingBox.height};
    debug_draw_pattern_table(c->b->p, nametable_buffer->pixel_buf, 0);
    render_draw_pixel_buffer(r, nametable_buffer, dimensions, (NES_Quad){0,0,1,1}, (NES_Vector4){1,1,1,1}, 10);
}

void HandleButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    if(pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
        printf("Button Clicked\n");
}

Clay_Dimensions clay_measure_text_cb(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    uint16_t fontSize = (config->fontSize > 0) ? config->fontSize : 20;
    NES_Vector2 res = bitmap_measure_text(text.chars, text.length, (NES_Vector2){config->letterSpacing, config->lineHeight}, (NES_Vector2){0.6 * fontSize, 1 * fontSize});

    return (Clay_Dimensions){res.x, res.y};
}

void clay_init(Renderer *r) {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions){r->screen_width, r->screen_height}, (Clay_ErrorHandler){HandleClayErrors});
    Clay_SetMeasureTextFunction(clay_measure_text_cb, NULL);

    cpu_state_buf = malloc(256);
    ppu_state_buf = malloc(256);
    cpu_page_buf = malloc(1024);
    nametable_buffer = malloc(sizeof(PixelBuffer));
    *nametable_buffer = pixelbuffer_init(16 * 8, 16 * 8, 3);
}

void clay_update_dimensions(Renderer *r, mouse_state *mstate, float dt) {
    //Update Clay internal layout dimensions to support resizing
    Clay_SetLayoutDimensions((Clay_Dimensions){r->screen_width, r->screen_height});
    Clay_SetPointerState((Clay_Vector2){mstate->mouse_pos.x, mstate->mouse_pos.y}, mstate->mouse_down);
    Clay_UpdateScrollContainers(true, (Clay_Vector2){mstate->scroll_pos.x, mstate->scroll_pos.y}, dt);
}

Clay_RenderCommandArray clay_set_layout(cpu *c, uint8_t *frame_data, float dt) {
    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    Clay_BeginLayout();

    CLAY(CLAY_ID("OuterContainer"), { .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 8}, .backgroundColor = {250, 250, 255, 255}}) {
        CLAY(CLAY_ID("Buttons"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32)}, .childGap = 16, .childAlignment = {.x = CLAY_ALIGN_X_CENTER}}}) {
            CLAY(CLAY_ID("ButtonContainer"), { .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .childGap = 4, .childAlignment = {.x = CLAY_ALIGN_X_CENTER}}}) {
                ButtonComponent(2, COLOUR_RED, COLOUR_ORANGE, 32, 40, 100, 40, CLAY_STRING("Pause"), CLAY_STRING("Pause"), HandleButtonInteraction, NULL, 10, CLAY_TEXT_ALIGN_CENTER);
                ButtonComponent(3, COLOUR_RED, COLOUR_ORANGE, 32, 40, 100, 40, CLAY_STRING("Play"), CLAY_STRING("Play"), HandleButtonInteraction, NULL, 10, CLAY_TEXT_ALIGN_CENTER);
                ButtonComponent(4, COLOUR_RED, COLOUR_ORANGE, 32, 40, 100, 40, CLAY_STRING("Stop"), CLAY_STRING("Stop"), HandleButtonInteraction, NULL, 10, CLAY_TEXT_ALIGN_CENTER);
            }
        }
        CLAY(CLAY_ID("Main_Content"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 8}}) {
            //TODO: Make this scrollable
            CLAY(CLAY_ID("CPU_Sidebar"), {
                .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_GROW(200, 400), .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = COLOUR_RED
            }) {
                cpu_state_component(c);
                cpu_page_component(c, 0);
             }
            CLAY(CLAY_ID("MainContent"), { .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(300), .height = CLAY_SIZING_GROW(0) },
                .childGap = 8}}) {
                CLAY(CLAY_ID("Top"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .childGap = 8, .childAlignment = {.x = CLAY_ALIGN_X_CENTER}}}) {
                    CLAY(CLAY_ID("Disassembly"), {.layout = {.sizing = {CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {}
                    CLAY(CLAY_ID("Game"), {.layout = {.sizing = {CLAY_SIZING_GROW(256, 512), CLAY_SIZING_GROW(240, 480)}}, .backgroundColor = COLOUR_LIGHT}) {
                            Custom_Tex_Data *data = malloc(sizeof(Custom_Tex_Data));
                            *data = (Custom_Tex_Data){.parent_name = CLAY_STRING("Game"), .render_func = render_game, .data = (void *)frame_data};
                            CLAY(CLAY_ID("Frame"), {.custom = {.customData = data}}) {}
                        }
                    CLAY(CLAY_ID("Undefined"), {.layout = {.sizing = {CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {}
                }
                CLAY(CLAY_ID("ROM"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {}
            }
            CLAY(CLAY_ID("PPU_Sidebar"), {
                .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_GROW(200, 400), .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = COLOUR_RED
            }) {
                CLAY(CLAY_ID("PPUINTERN"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)}}}) {
                    ppu_state_component(c->b->p);
                }
                CLAY(CLAY_ID("PPUDATA"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                    Custom_Tex_Data *data = malloc(sizeof(Custom_Tex_Data));
                    *data = (Custom_Tex_Data){.parent_name = CLAY_STRING("PPUDATA"), .render_func = render_pattern_table, .data = (void *)c};
                    CLAY(CLAY_ID("Nametable"), {.custom = {.customData = data}}) {}
                }
            }
        }
    }
    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    return Clay_EndLayout(dt); // deltaTime is the time since the last frame, and is used for transitions
}

void clay_render(Renderer *r, Bitmap_Font_Desc *bitmap, Clay_RenderCommandArray renderCommands, PixelBuffer *pb) {
    //TODO: Implement renderer that handles all of these rendering commands.
    for(int i = 0; i < renderCommands.length; i++) {
        Clay_RenderCommand *renderCommand = &renderCommands.internalArray[i];

        switch(renderCommand->commandType) {
            // This command type should be skipped.
            case CLAY_RENDER_COMMAND_TYPE_NONE:
            break;
            // The renderer should draw a solid color rectangle.
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                //TODO: Implement rounded corners
                render_draw_quad(r, (NES_Quad){
                    renderCommand->boundingBox.x,
                    renderCommand->boundingBox.y,
                    renderCommand->boundingBox.width,
                    renderCommand->boundingBox.height,
                }, (NES_Vector4){
                    renderCommand->renderData.rectangle.backgroundColor.r/255.0f,
                    renderCommand->renderData.rectangle.backgroundColor.g/255.0f,
                    renderCommand->renderData.rectangle.backgroundColor.b/255.0f,
                    renderCommand->renderData.rectangle.backgroundColor.a/255.0f,
                }, 9);
            break;
            // The renderer should draw a colored border inset into the bounding box.
            case CLAY_RENDER_COMMAND_TYPE_BORDER:
                printf("Border Rendering not currently implemented\n");
            break;
            // The renderer should draw text.
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData data = renderCommand->renderData.text;
                uint16_t fontSize = (data.fontSize > 0) ? data.fontSize : 20;
                bitmap_draw_string(r, bitmap, data.stringContents.chars, data.stringContents.length, (NES_Vector2) {data.letterSpacing, data.lineHeight},
                                   (NES_Vector2){renderCommand->boundingBox.x, renderCommand->boundingBox.y}, (NES_Vector2){0.6 * fontSize, 1 * fontSize},
                                   (NES_Vector4) {
                                        data.textColor.r/255.0f,
                                        data.textColor.g/255.0f,
                                        data.textColor.b/255.0f,
                                        data.textColor.a/255.0f,
                                   }, 10);
            }
            break;
            // The renderer should draw an image.
            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                printf("Image Rendering not currently implemented\n");
            break;
            // The renderer should begin clipping all future draw commands, only rendering content that falls within the provided boundingBox.
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
                printf("Clipping not currently implemented\n");
            break;
            // The renderer should finish any previously active clipping, and begin rendering elements in full again.
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                printf("Clipping not currently implemented\n");
            break;
            // The renderer should begin performing a "color overlay" on all subsequent render commands until disabled again.
            case CLAY_RENDER_COMMAND_TYPE_OVERLAY_COLOR_START:
                printf("Colour Overlay not currently implemented\n");
            break;
            // The renderer should disable any previously active "color overlay" and render elements with their standard colors again.
            case CLAY_RENDER_COMMAND_TYPE_OVERLAY_COLOR_END:
                printf("Colour Overlay not currently implemented\n");
            break;
            // The renderer should provide a custom implementation for handling this render command based on its .customData
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                Custom_Tex_Data *ctd = (Custom_Tex_Data *)renderCommand->renderData.custom.customData;
                ctd->render_func(ctd->parent_name, r, pb, ctd->data);
                free(ctd);
            }
                // printf("Custom Rendering not currently implemented\n");
            break;
        }
    }
}

void clay_free() {
    free(cpu_state_buf);
    free(ppu_state_buf);
    free(cpu_page_buf);
}
