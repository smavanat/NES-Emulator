//Implementation of helper functions and a layout in clay
#include "clay_layout.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ppu.h"
#include "cpu.h"

#define CLAY_IMPLEMENTATION
#include "../externals/clay.h"


//TODO: Fix the weird CPU state gliches
//      Make the pause/play/stop buttons do something
//      Add a button to step through one CPU instruction at a time
//      Add ability to switch between different regions of memory for CPU/ROM/PPU(switch between 1st and second nametable)
//      Add scrolling so things do not squish together when the screen is too small
//      Make it look nice - Add labels to everything so its clear what different parts represent
//      Once APU done, add Debug for it on the other side of the Game (opposite to Disassembly)

//Clay Colours to be reused
const Clay_Color COLOUR_WHITE = (Clay_Color){255, 255, 255, 255};
const Clay_Color COLOUR_BLACK = (Clay_Color){0, 0, 0, 255};
const Clay_Color COLOUR_LIGHT = (Clay_Color){224, 215, 210, 255};
const Clay_Color COLOUR_RED = (Clay_Color){168, 66, 28, 255};
const Clay_Color COLOUR_ORANGE = (Clay_Color){255, 138, 50, 255};

//TODO: unify all of the malloc calls in here into a global arena so that we don't keep allocating/freeing memory
BOB_Pixelbuffer_Handle nametable_buffer;
char *cpu_state_buf;
char *cpu_page_buf;
char *ppu_state_buf;
char *rom_state_buf;
char *rom_page_buf;
uint8_t *pixelbuf_data;

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
    CLAY(CLAY_ID("CPUSTATE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(8)}, .backgroundColor = COLOUR_BLACK}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT});
    }
}

//Generates a component that holds a text printout of a given CPU page
void cpu_page_component(cpu *c, uint8_t page_num) {
    uint16_t start_addr = page_num * 256;
    int len = 0;

    for(int i = 0; i < 256; i++) {
        if(i > 0 && i % 12 == 0) {
            cpu_page_buf[len] = '\n';
            len++;
        }
        len += sprintf(cpu_page_buf + len, "%02X ", mem_read(c->b, start_addr+i));
    }
    Clay_String str = (Clay_String){true, len, cpu_page_buf};
    CLAY(CLAY_ID("CPUPAGE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT, .fontSize = 17});
    }
}


// Source - https://stackoverflow.com/a/3208376
// Posted by William Whyte, modified by community. See post 'Timeline' for change history
// Retrieved 2026-07-02, License - CC BY-SA 4.0
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)   \
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
                 "\nOAMADDR: %02X\nOAMDATA: %02X\nPPUSCROLL: %04X\nPPUADDR: %04X\nPPUDATA: %02X\nOAMDMA: %02X",
                  BYTE_TO_BINARY(p->ctrl_reg), BYTE_TO_BINARY(p->mask_reg), BYTE_TO_BINARY(p->status_reg), p->oamaddr_reg, p->oamdma_reg,
                  get_scroll_register(p->scroll_reg), get_addr_register(p->addr_reg), p->data_reg, p->oamdma_reg);

    Clay_String str = (Clay_String){true, len, ppu_state_buf};
    CLAY(CLAY_ID("PPUSTATE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(8)}, .backgroundColor = COLOUR_BLACK}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT});
    }
}

typedef struct {
    void (*render_func)(Clay_String str, BOB_Renderer_Handle r, BOB_Pixelbuffer_Handle pb, void *data);
    void *data;
    Clay_String parent_name;
} Custom_Tex_Data;

void render_game(Clay_String str, BOB_Renderer_Handle r, BOB_Pixelbuffer_Handle pb, void *data) {
    uint8_t *pixels = (uint8_t *)data;
    Clay_ElementId gId = Clay_GetElementId(str);
    Clay_ElementData gData = Clay_GetElementData(gId);

    BOB_Quad dimensions = {gData.boundingBox.x, gData.boundingBox.y, gData.boundingBox.width, gData.boundingBox.height};

    void *mapped_mem_ptr;
    size_t mem_sz;
    BOB_bind_pixelbuffer_memory(pb, &mapped_mem_ptr, &mem_sz);
    memcpy(mapped_mem_ptr, data, mem_sz);
    BOB_unbind_pixelbuffer_memory(pb);
    BOB_pixelbuffer_upload(pb);
    BOB_draw_pixelbuffer(pb, dimensions, (BOB_Quad){0,0,FRAME_WIDTH,FRAME_HEIGHT}, (BOB_Vector4){1,1,1,1}, 10.0f, 0.0f);
}

void render_pattern_table(Clay_String str, BOB_Renderer_Handle r, BOB_Pixelbuffer_Handle pb, void *data) {
    cpu *c = (cpu *)data;
    Clay_ElementId gId = Clay_GetElementId(str);
    Clay_ElementData gData = Clay_GetElementData(gId);

    BOB_Quad dimensions = {gData.boundingBox.x, gData.boundingBox.y, gData.boundingBox.width, gData.boundingBox.height};
    void *mapped_mem_ptr;
    size_t mem_sz;
    BOB_bind_pixelbuffer_memory(nametable_buffer, &mapped_mem_ptr, &mem_sz);
    debug_draw_pattern_table(c->b->p, pixelbuf_data, 0);
    memcpy(mapped_mem_ptr, pixelbuf_data, mem_sz);
    BOB_unbind_pixelbuffer_memory(nametable_buffer);
    BOB_pixelbuffer_upload(nametable_buffer);
    BOB_draw_pixelbuffer(nametable_buffer, dimensions, (BOB_Quad){0,0,16*8,16*8}, (BOB_Vector4){1,1,1,1}, 10.0f, 0.0f);
}

char *get_mirroring_as_string(mirroring m) {
    switch (m) {
        case(MIR_VERTICAL): return "VERTICAL";
        case(MIR_HORIZONTAL): return "HORIZONTAL";
        case(MIR_FOURSCREEN): return "FOURSCREEN";
        case(MIR_ONESCREEN_LO): return "ONESCREEN_LO";
        case(MIR_ONESCREEN_HI): return "ONESCREEN_HI";
    }
}

void rom_state_component(rom *r) {
    int len = sprintf(rom_state_buf, "PGR ROM Size: %zu\nCHR ROM Size: %zu\nPGR RAM Size: %zu\nCHR RAM Size: %zu\nPGR EEPROM Size: %zu\nMapper: %hu\nSubmapper: %hhu\nMirroring: %s\n",
                      r->prg_rom_sz, r->chr_rom_sz, r->prg_ram_sz, r->chr_ram_sz, r->prg_eeprom_sz, r->mapper, r->submapper, get_mirroring_as_string(r->mirroring));

    if(r->num_mapper_registers > 0) {
        len += sprintf(rom_state_buf + len, "Mapper Registers:");

        for(int i = 0; i < r->num_mapper_registers; i++) {
            len += sprintf(rom_state_buf + len, "\n[%i]:"BYTE_TO_BINARY_PATTERN, i, BYTE_TO_BINARY(r->mapper_registers[i]));
        }
    }

    Clay_String str = (Clay_String){true, len, rom_state_buf};
    CLAY(CLAY_ID("ROMSTATE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(8)}, .backgroundColor = COLOUR_BLACK}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT});
    }
}

void rom_page_component(rom *r, uint8_t memory_component, uint16_t page) {
    uint8_t *mem_region;
    uint16_t start_addr = page * 256;
    int len = 0;

    switch(memory_component) {
        case 0: 
            mem_region = r->chr_ram;
            if(start_addr + 256 >= r->chr_ram_sz) {
                printf("Requested memory is out of bounds\n");
                return;
            }
            break;
        case 1:
            mem_region = r->chr_rom;
            if(start_addr + 256 >= r->chr_rom_sz) {
                printf("Requested memory is out of bounds\n");
                return;
            }
            break;
        case 2:
            mem_region = r->prg_ram;
            if(start_addr + 256 >= r->prg_ram_sz) {
                printf("Requested memory is out of bounds\n");
                return;
            }
            break;
        case 3:
            mem_region = r->prg_rom;
            if(start_addr + 256 >= r->prg_rom_sz) {
                printf("Requested memory is out of bounds\n");
                return;
            }
            break;
        case 4:
            mem_region = r->prg_eeprom;
            if(start_addr + 256 >= r->prg_eeprom_sz) {
                printf("Requested memory is out of bounds\n");
                return;
            }
            break;
        default:
            printf("Invalid Memory region\n");
            return;
    }

    for(int i = 0; i < 256; i++) {
        if(i > 0 && i % 16 == 0) {
            rom_page_buf[len] = '\n';
            len++;
        }
        len += sprintf(rom_page_buf + len, "%02X ", mem_region[start_addr+i]);
    }
    Clay_String str = (Clay_String){true, len, rom_page_buf};
    CLAY(CLAY_ID("ROMPAGE"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT, .fontSize = 17});
    }
}

void disassembly_component(char *buf, size_t buf_sz) {
    Clay_String str = (Clay_String){true, buf_sz, buf};
    CLAY(CLAY_ID("DISASSEMBLY_DATA"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, .clip = {.vertical = true, .childOffset = Clay_GetScrollOffset()}}) {
        CLAY_TEXT(str, {.textColor = COLOUR_WHITE, .textAlignment = CLAY_TEXT_ALIGN_LEFT, .fontSize = 17});
    }
}

void HandleButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    if(pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
        printf("Button Clicked\n");
}

Clay_Dimensions clay_measure_text_cb(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    uint16_t fontSize = (config->fontSize > 0) ? config->fontSize : 20;
    BOB_Vector2 res;
    BOB_measure_char_string(text.chars, text.length, bitmap, &res);

    return (Clay_Dimensions){res.x, res.y};
}

void clay_init(BOB_Renderer_Handle r, size_t width, size_t height) {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions){width, height}, (Clay_ErrorHandler){HandleClayErrors});
    Clay_SetMeasureTextFunction(clay_measure_text_cb, NULL);

    cpu_state_buf = malloc(256);
    ppu_state_buf = malloc(256);
    cpu_page_buf = malloc(1024);
    rom_state_buf = malloc(512);
    rom_page_buf = malloc(1024);
    BOB_pixelbuffer_init(r, 16 * 8, 16 * 8, BOB_RGB, &nametable_buffer);
    pixelbuf_data = malloc(sizeof(uint8_t) * 16 * 8 * 16 * 8 * 3);
}

void clay_update_dimensions(BOB_Renderer_Handle r, mouse_state *mstate, float dt, size_t width, size_t height) {
    //Update Clay internal layout dimensions to support resizing
    Clay_SetLayoutDimensions((Clay_Dimensions){width, height});
    Clay_SetPointerState((Clay_Vector2){mstate->mouse_pos.x, mstate->mouse_pos.y}, mstate->mouse_down);
    Clay_UpdateScrollContainers(true, (Clay_Vector2){mstate->scroll_pos.x, mstate->scroll_pos.y}, dt);
}

Clay_RenderCommandArray clay_set_layout(cpu *c, uint8_t *frame_data, float dt, char *disassembly_buf, size_t disassembly_buf_sz) {
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
                .padding = CLAY_PADDING_ALL(8), .childGap = 16}, .backgroundColor = COLOUR_LIGHT
            }) {
                cpu_state_component(c);
                CLAY(CLAY_ID("CPU_PAGE_OUTER"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(8), .childGap = 8}, .backgroundColor = COLOUR_BLACK}) {
                    ButtonComponent(5, COLOUR_RED, COLOUR_ORANGE, 32, 30, 145, 30, CLAY_STRING("Change Page"), CLAY_STRING("Change Page"), HandleButtonInteraction, NULL, 6, CLAY_TEXT_ALIGN_CENTER);
                    cpu_page_component(c, 0);
                }
             }
            CLAY(CLAY_ID("MainContent"), { .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(300), .height = CLAY_SIZING_GROW(0) },
                .childGap = 8}}) {
                CLAY(CLAY_ID("Top"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .childGap = 8, .childAlignment = {.x = CLAY_ALIGN_X_CENTER}}}) {
                    CLAY(CLAY_ID("Disassembly"), {.layout = {.sizing = {CLAY_SIZING_GROW(0, 200), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {
                        disassembly_component(disassembly_buf, disassembly_buf_sz);
                    }
                    CLAY(CLAY_ID("Game"), {.layout = {.sizing = {CLAY_SIZING_GROW(256, 512), CLAY_SIZING_GROW(240, 480)}}, .backgroundColor = COLOUR_LIGHT}) {
                            Custom_Tex_Data *data = malloc(sizeof(Custom_Tex_Data));
                            *data = (Custom_Tex_Data){.parent_name = CLAY_STRING("Game"), .render_func = render_game, .data = (void *)frame_data};
                            CLAY(CLAY_ID("Frame"), {.custom = {.customData = data}}) {}
                        }
                    CLAY(CLAY_ID("APU"), {.layout = {.sizing = {CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {}
                }
                CLAY(CLAY_ID("ROM_OUTER"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(8)}, .backgroundColor = COLOUR_LIGHT}) {
                    rom_state_component(c->b->rom);
                    CLAY(CLAY_ID("ROM_MEMORY"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .childGap = 8}, .backgroundColor = COLOUR_BLACK}) {
                        CLAY(CLAY_ID("ROM_BUTTONS"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)}, .padding = CLAY_PADDING_ALL(8), .childGap = 8}}) {
                            ButtonComponent(6, COLOUR_RED, COLOUR_ORANGE, 32, 30, 170, 30, CLAY_STRING("Change Memory"), CLAY_STRING("Change Memory"), HandleButtonInteraction, NULL, 6, CLAY_TEXT_ALIGN_CENTER);
                            ButtonComponent(7, COLOUR_RED, COLOUR_ORANGE, 32, 30, 145, 30, CLAY_STRING("Change Page"), CLAY_STRING("Change Page"), HandleButtonInteraction, NULL, 6, CLAY_TEXT_ALIGN_CENTER);
                        }
                        rom_page_component(c->b->rom, 3, 0);
                    }
                }
            }
            CLAY(CLAY_ID("PPU_Sidebar"), {
                .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_GROW(200, 400), .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(8), .childGap = 16}, .backgroundColor = COLOUR_LIGHT
            }) {
                ppu_state_component(c->b->p);
                CLAY(CLAY_ID("PPUNAMETABLE_OUTER"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(8)}, .backgroundColor = COLOUR_BLACK}) {
                    CLAY(CLAY_ID("PPUNAMETABLE_INNER"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                        Custom_Tex_Data *data = malloc(sizeof(Custom_Tex_Data));
                        *data = (Custom_Tex_Data){.parent_name = CLAY_STRING("PPUNAMETABLE_INNER"), .render_func = render_pattern_table, .data = (void *)c};
                        CLAY(CLAY_ID("Nametable"), {.custom = {.customData = data}}) {}
                    }
                }
            }
        }
    }
    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    return Clay_EndLayout(dt); // deltaTime is the time since the last frame, and is used for transitions
}

void clay_render(BOB_Renderer_Handle r, BOB_Font_Handle bitmap, Clay_RenderCommandArray renderCommands, BOB_Pixelbuffer_Handle pb) {
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
                BOB_draw_quad(r, (BOB_Quad){
                    renderCommand->boundingBox.x,
                    renderCommand->boundingBox.y,
                    renderCommand->boundingBox.width,
                    renderCommand->boundingBox.height,
                }, (BOB_Vector4){
                    renderCommand->renderData.rectangle.backgroundColor.r/255.0f,
                    renderCommand->renderData.rectangle.backgroundColor.g/255.0f,
                    renderCommand->renderData.rectangle.backgroundColor.b/255.0f,
                    renderCommand->renderData.rectangle.backgroundColor.a/255.0f,
                }, 0.0f, 0.0f);
            break;
            // The renderer should draw a colored border inset into the bounding box.
            case CLAY_RENDER_COMMAND_TYPE_BORDER:
                printf("Border Rendering not currently implemented\n");
            break;
            // The renderer should draw text.
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData data = renderCommand->renderData.text;
                uint16_t fontSize = (data.fontSize > 0) ? data.fontSize : 20;
                BOB_Vector2 start_pos = {renderCommand->boundingBox.x, renderCommand->boundingBox.y};
                BOB_draw_char_string(bitmap, data.stringContents.chars, data.stringContents.length, &start_pos,
                                   (BOB_Vector4) { data.textColor.r/255.0f, data.textColor.g/255.0f, data.textColor.b/255.0f, data.textColor.a/255.0f, }, 10.0f);
            }
            break;
            // The renderer should draw an image.
            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                printf("Image Rendering not currently implemented\n");
            break;
            // The renderer should begin clipping all future draw commands, only rendering content that falls within the provided boundingBox.
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                BOB_Clip_Dir dir;
                if(renderCommand->renderData.clip.vertical && renderCommand->renderData.clip.vertical) dir = BOB_CLIP_BOTH;
                else if(renderCommand->renderData.clip.vertical) dir = BOB_CLIP_VERT;
                else dir = BOB_CLIP_HORZ;
                BOB_start_clip(r, (BOB_Quad){renderCommand->boundingBox.x, renderCommand->boundingBox.y, renderCommand->boundingBox.width, renderCommand->boundingBox.height}, dir);
            }
            break;
            // The renderer should finish any previously active clipping, and begin rendering elements in full again.
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                BOB_end_clip(r);
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
    free(rom_page_buf);
    free(rom_state_buf);
    free(pixelbuf_data);
}
