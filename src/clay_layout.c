//Implementation of helper functions and a layout in clay
#include "clay_layout.h"
#include <stdint.h>
#include <stdio.h>
#include "renderer.h"
#include "bitmap_font.h"

#define CLAY_IMPLEMENTATION
#include "../externals/clay.h"

//Clay Colours to be reused
const Clay_Color COLOUR_WHITE = (Clay_Color){255, 255, 255, 255};
const Clay_Color COLOUR_LIGHT = (Clay_Color){224, 215, 210, 255};
const Clay_Color COLOUR_RED = (Clay_Color){168, 66, 28, 255};
const Clay_Color COLOUR_ORANGE = (Clay_Color){255, 138, 50, 255};

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

typedef void (*render_func)(Clay_String str, Renderer *r, PixelBuffer *pb, uint8_t *data);
void render_game(Clay_String str, Renderer *r, PixelBuffer *pb, uint8_t *data) {
    Clay_ElementId gId = Clay_GetElementId(str);
    Clay_ElementData gData = Clay_GetElementData(gId);

    NES_Quad dimensions = {gData.boundingBox.x, gData.boundingBox.y, gData.boundingBox.width, gData.boundingBox.height};

    pixelbuffer_updload_data(pb, data);
    render_draw_pixel_buffer(r, pb, dimensions, (NES_Quad){0,0,1,1}, (NES_Vector4){1,1,1,1}, 10);
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
}

void clay_update_dimensions(Renderer *r, mouse_state *mstate, float dt) {
    //Update Clay internal layout dimensions to support resizing
    Clay_SetLayoutDimensions((Clay_Dimensions){r->screen_width, r->screen_height});
    Clay_SetPointerState((Clay_Vector2){mstate->mouse_pos.x, mstate->mouse_pos.y}, mstate->mouse_down);
    Clay_UpdateScrollContainers(true, (Clay_Vector2){mstate->scroll_pos.x, mstate->scroll_pos.y}, dt);
}

Clay_RenderCommandArray clay_set_layout(float dt) {
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
            CLAY(CLAY_ID("CPU_Sidebar"), {
                .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_GROW(200, 400), .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = COLOUR_LIGHT
            }) {
                CLAY(CLAY_ID("ProfilePictureOuter"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}}, .backgroundColor = COLOUR_RED}) {
                    CLAY_TEXT(CLAY_STRING("Clay - UI Library"), {.fontSize = 24, .textColor = COLOUR_WHITE});
                }

                // Standard C code like loops etc work inside components
                for (int i = 0; i < 5; i++) {
                    SidebarItemComponent(i);
                }
                ButtonComponent(1, COLOUR_RED, COLOUR_ORANGE, 100, 50, 400, 50, CLAY_STRING("Hovered!"), CLAY_STRING("Hover me"), HandleButtonInteraction, NULL, 16, CLAY_TEXT_ALIGN_LEFT);
             }
            CLAY(CLAY_ID("MainContent"), { .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(300), .height = CLAY_SIZING_GROW(0) },
                .childGap = 8}}) {
                CLAY(CLAY_ID("Top"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .childGap = 8, .childAlignment = {.x = CLAY_ALIGN_X_CENTER}}}) {
                    CLAY(CLAY_ID("Disassembly"), {.layout = {.sizing = {CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {}
                    CLAY(CLAY_ID("Game"), {.layout = {.sizing = {CLAY_SIZING_GROW(256, 512), CLAY_SIZING_GROW(240, 480)}}, .backgroundColor = COLOUR_LIGHT}) {
                            CLAY(CLAY_ID("Frame"), {.custom = {.customData = render_game}}) {}
                        }
                    CLAY(CLAY_ID("Undefined"), {.layout = {.sizing = {CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {}
                }
                CLAY(CLAY_ID("ROM"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = COLOUR_LIGHT}) {}
            }
            CLAY(CLAY_ID("PPU_Sidebar"), {
                .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_GROW(200, 400), .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = COLOUR_LIGHT
            }) {}
        }
    }
    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    return Clay_EndLayout(dt); // deltaTime is the time since the last frame, and is used for transitions
}

void clay_render(Renderer *r, Bitmap_Font_Desc *bitmap, Clay_RenderCommandArray renderCommands, uint8_t *data, PixelBuffer *pb) {
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
                render_func rf = (render_func)renderCommand->renderData.custom.customData;
                rf(CLAY_STRING("Game"), r, pb, data);
            }
                // printf("Custom Rendering not currently implemented\n");
            break;
        }
    }
}
