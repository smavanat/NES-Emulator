//Implementation of helper functions and a layout in clay
#include "clay_layout.h"
#include <stdio.h>
#include "renderer.h"
#include "bitmap_font.h"

#define CLAY_IMPLEMENTATION
#include "../externals/clay.h"

//Clay functionality
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

Clay_Dimensions clay_measure_text_cb(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    //TODO: Figure out how to convert the config.fontSize variable to measure the scale of the lettering
    NES_Vector2 res = bitmap_measure_text(text.chars, text.length, (NES_Vector2){config->letterSpacing, config->lineHeight}, (NES_Vector2){20, 20});

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

    //Clay example UI with a fixed width sidebar and flexible width main content
    CLAY(CLAY_ID("OuterContainer"), { .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16},
         .backgroundColor = {250, 250, 255, 255}}) {
        CLAY(CLAY_ID("SideBar"), {
            .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0)},
            .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = COLOUR_LIGHT
        }) {
            CLAY(CLAY_ID("ProfilePictureOuter"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}}, .backgroundColor = COLOUR_RED}) {
                CLAY_TEXT(CLAY_STRING("Clay - UI Library"), {.fontSize = 24, .textColor = {255, 255, 255, 255}});
            }

            // Standard C code like loops etc work inside components
            for (int i = 0; i < 5; i++) {
                SidebarItemComponent(i);
            }
         }
        CLAY(CLAY_ID("MainContent"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, .backgroundColor = COLOUR_LIGHT }) {}
    }

    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    return Clay_EndLayout(dt); // deltaTime is the time since the last frame, and is used for transitions
}

void clay_render(Renderer *r, Bitmap_Font_Desc *bitmap, Clay_RenderCommandArray renderCommands) {
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
                }, 62);
            break;
            // The renderer should draw a colored border inset into the bounding box.
            case CLAY_RENDER_COMMAND_TYPE_BORDER:
                printf("Border Rendering not currently implemented\n");
            break;
            // The renderer should draw text.
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData data = renderCommand->renderData.text;
                bitmap_draw_string(r, bitmap, data.stringContents.chars, data.stringContents.length, (NES_Vector2) {data.letterSpacing, data.lineHeight},
                                   (NES_Vector2){renderCommand->boundingBox.x, renderCommand->boundingBox.y}, (NES_Vector2){20,20},
                                   (NES_Vector4) {
                                        data.textColor.r/255.0f,
                                        data.textColor.g/255.0f,
                                        data.textColor.b/255.0f,
                                        data.textColor.a/255.0f,
                                   }, 63);
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
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
                printf("Custom Rendering not currently implemented\n");
            break;
        }
    }
}
