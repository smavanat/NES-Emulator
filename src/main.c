#define _XOPEN_SOURCE 500 //Needed to get deprecated functions
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L  // must come first
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "cpu.h"
#include "../externals/glad.h"
#include "../externals/GLFW/glfw3.h"
#include "joypad.h"
#include "ppu.h"

#define CLAY_IMPLEMENTATION
#include "../externals/clay.h"

#define FRAME_RATE 1000 / 60.0f

//TODO: Debugger
//      JIT
//      APU
uint8_t stop = 0;
GLFWwindow *window;
cpu c = {0};
Renderer r = {0};

//Thank you Bernardo: https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
#ifdef WIN32
#include <windows.h>
#else
#include <time.h>   // for nanosleep
#include <unistd.h> // for usleep
#endif

/**
 * Cross-platform sleep function
 * @param milliseconds the time the thread should sleep for in milliseconds
 */
void sleep_ms(int milliseconds){
#ifdef WIN32
    Sleep(milliseconds);
#else
#if defined(_POSIX_VERSION) && (POSIX_VERSION >= 199309L)
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    if (milliseconds >= 1000)
      sleep(milliseconds / 1000);
    usleep((milliseconds % 1000) * 1000);
#endif
#endif
}

//Reads the entirety of a file specified at 'path' into the buffer whose
//pointer is stored at *buf. Mallocs the size of *buf itself (since it has to figure out the file size)
//add_null specifies if a null terminator needs to be added to the buffer
//Returns the size of the buffer on succes, or -1 on failure
int read_to_end(char const *path, uint8_t **buf, uint8_t add_null) {
    FILE *fp;
    size_t fsz;
    long offEnd;
    int rc;

    //Open the file
    fp = fopen(path, "rb");
    if(NULL == fp) {
        fclose(fp);
        return -1;
    }

    //Seek to the end of the file
    rc = fseek(fp, 0L, SEEK_END);
    if(0 != rc) {
        fclose(fp);
        return -1;
    }

    //Byte offset to the end of the file size
    if(0 > (offEnd = ftell(fp))) {
        fclose(fp);
        return -1;
    }
    fsz = (size_t)offEnd;

    //Allocate a buffer to hold the whole file
    *buf = malloc(fsz + (int)add_null);
    if(NULL == *buf) {
        fclose(fp);
        return -1;
    }

    //Rewind file pointer to the start of the file:
    rewind(fp);

    //Place the file into a buffer
    if(fsz != fread(*buf, 1, fsz, fp)) {
        free(*buf);
        fclose(fp);
        return -1;
    }

    //Close the file
    if(EOF == fclose(fp)) {
        free(*buf);
        return -1;
    }

    //Add null terminator
    if(add_null) {
        (*buf)[fsz] = '\0';
    }

    return fsz;
}

//GLFW callback for when the window is resized
//Updates the width and height in the renderer so that the projection matrix
//(and the objects being rendered) can be adjusted to fit the new screen size
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    //Update viewport dimensions
    glViewport(0, 0, width, height);
    r.screen_width = width;
    r.screen_height = height;

    //Update projection matrix for pixel renderer
    ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f, r.projection);
    glUseProgram(r.shader);
    int proj_loc = glGetUniformLocation(r.shader, "uProjection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *)r.projection);

    //Update the uv coordinates of the texture the pixel renderer is rendering to
    float quadVertices[] = {
        0.0f, 0.0f,          0.0f, 0.0f,
        0.0f, height,        0.0f, 1.0f,
        width, height,       1.0f, 1.0f,
        width, 0.0f,         1.0f, 0.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, r.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    sizeof(quadVertices), quadVertices);
}

//GLFW callback for user input
//Maps specific keys on the keyboard to joypad buttons
//Current layout:
//Joypad Button | Player 1 | Player 2
//     UP       |    W     |    I
//    LEFT      |    A     |    J
//    DOWN      |    S     |    K
//    RIGHT     |    D     |    L
//      A       |    Q     |    U
//      B       |    E     |    O
//   SELECT     |    Z     |    M
//    START     |    C     |    .
//TODO: Make this configurable by the user
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if(glfwGetWindowAttrib(window, GLFW_FOCUSED)) { //Only update user input when this window is the selected one
        switch(key) {
            case GLFW_KEY_W:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_UP, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_A:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_LEFT, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_S:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_DOWN, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_D:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_RIGHT, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_Q:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_A, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_E:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_B, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_Z:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_SELECT, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_C:
                joypad_set_button_pressed(c.b->player_1, JOYPAD_BUTTON_START, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_I:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_UP, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_J:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_LEFT, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_K:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_DOWN, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_L:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_RIGHT, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_U:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_A, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_O:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_B, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_M:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_SELECT, !(action == GLFW_RELEASE));
            break;
            case GLFW_KEY_PERIOD:
                joypad_set_button_pressed(c.b->player_2, JOYPAD_BUTTON_START, !(action == GLFW_RELEASE));
            break;
        }
    }
}

//Function that initialises GLFW and glad
int init(GLFWwindow **window) {
    //Initialising GLFW:
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    *window = glfwCreateWindow(INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT, "JES", NULL, NULL);

    if(window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 0;
    }

    //Setting callback functions
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
    glfwSetKeyCallback(*window, key_callback);

    //Loading GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialise GLAD");
        glfwTerminate();
        return 0;
    }

    return 1;
}

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
void SidebarItemComponent() {
    CLAY(CLAY_ID("Sidebar"), sidebarItemConfig) {
        //Children go here
    }
}

int main(void) {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    init(&window);
    r = render_init(); //Initialising the renderer
    Clay_Initialize(arena, (Clay_Dimensions){r.screen_width, r.screen_height}, (Clay_ErrorHandler){HandleClayErrors});

    struct timeval stop, start; //Store the start and end times of a frame
    float dt = 0.0f; //Holds the time passed between frames

    while(!glfwWindowShouldClose(window)) {
        gettimeofday(&start, NULL); //Getting time at start of frame

        //Clear the screen
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        //Update Clay internal layout dimensions to support resizing
        Clay_SetLayoutDimensions((Clay_Dimensions){r.screen_width, r.screen_height});
        //TODO:Update internal pointer position for handling mouseover/click/touch events
        //Clay_SetPointerState((Clay_Vector2){mouseX, mouseY}, isMouseDown);
        //Clay_UpdateScrollContainers(true, (Clay_Vector2){mouseWheelX, mouseWheelY}, dt);

        //All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
        Clay_BeginLayout();

        //Clay example UI with a fixed width sidebar and flexible width main content
        CLAY(CLAY_ID("OuterContainer"), { .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16},
             .backgroundColor = {250, 250, 255, 255}}) {
            CLAY(CLAY_ID("SideBar"), {
                .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0)}},
                .backgroundColor = COLOUR_LIGHT
            }) {
                CLAY(CLAY_ID("ProfilePictureOuter"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}}, .backgroundColor = COLOUR_RED}) {
                    CLAY_TEXT(CLAY_STRING("Clay - UI Library"), {.fontSize = 24, .textColor = {255, 255, 255, 255}});
                }

                // Standard C code like loops etc work inside components
                for (int i = 0; i < 5; i++) {
                    SidebarItemComponent();
                }

                CLAY(CLAY_ID("MainContent"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, .backgroundColor = COLOUR_LIGHT }) {}
             }
        }

        // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
        Clay_RenderCommandArray renderCommands = Clay_EndLayout(dt); // deltaTime is the time since the last frame, and is used for transitions

        //TODO: Implement renderer that handles all of these rendering commands. Use stb for text renderering
        for(int i = 0; i < renderCommands.length; i++) {
            Clay_RenderCommand *renderCommand = &renderCommands.internalArray[i];

            switch(renderCommand->commandType) {
                // This command type should be skipped.
                case CLAY_RENDER_COMMAND_TYPE_NONE:
                break;
                // The renderer should draw a solid color rectangle.
                case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                    printf("Rectangle Rendering not currently implemented\n");
                break;
                // The renderer should draw a colored border inset into the bounding box.
                case CLAY_RENDER_COMMAND_TYPE_BORDER:
                    printf("Border Rendering not currently implemented\n");
                break;
                // The renderer should draw text.
                case CLAY_RENDER_COMMAND_TYPE_TEXT:
                    printf("Text Rendering not currently implemented\n");
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
}

int main_old(void) {
    c.sp = 0xFF; //Setting stack pointer to top of stack
    c.proc_stat_reg = 0x34; //Setting BREAK and UNUSED flags

    //Reading the data from a ROM
    //TODO: Make the rom path user inputable
    uint8_t *buf;
    int sz = read_to_end("../super_mario_bros.nes", &buf, 0);
    if(sz < 0) {
        fprintf(stderr, "Error when opening a file\n");
        return 0;
    }

    if(init(&window)) {
        printf("Initialised\n");

        //Allocating the bus and all of its peripherals
        c.b = calloc(1, sizeof(bus));
        c.b->rom = calloc(1, sizeof(rom));
        c.b->p = calloc(1, sizeof(ppu));
        c.b->p->addr_reg = calloc(1, sizeof(addr_register));
        c.b->p->addr_reg->h_ptr = 1;
        c.b->p->scroll_reg = calloc(1, sizeof(scroll_register));
        c.b->p->scroll_reg->s_ptr = 1;
        c.b->player_1 = calloc(1, sizeof(joypad));
        c.b->player_2 = calloc(1, sizeof(joypad));
        r = render_init(); //Initialising the renderer

        struct timeval stop, start; //Store the start and end times of a frame
        float dt = 0.0f; //Holds the time passed between frames

        if(!rom_load(c.b->rom, buf, sz)) { //Loading the rom into memory
            printf("Loaded\n");
            free(buf);

            //Some debug functions
            //TODO: Write a proper debugger and get rid of this
            printf("PRG ROM size: %zu\n", c.b->rom->prg_rom_sz);
            printf("CHR ROM size: %zu\n", c.b->rom->chr_rom_sz);
            printf("CHR RAM size: %zu\n", c.b->rom->chr_ram_sz);
            printf("Mapper: %u\n", c.b->rom->mapper);
            printf("Initial control reg: %02X\n", c.b->rom->mapper_registers[0]);
            // Manually read the reset vector bytes through the mapper
            uint8_t lo = c.b->rom->cpu_read(c.b->rom, 0xFFFC);
            uint8_t hi = c.b->rom->cpu_read(c.b->rom, 0xFFFD);
            printf("Raw reset vector bytes: %02X %02X\n", lo, hi);
            printf("Expected reset vector: %04X\n", (hi << 8) | lo);

            set_pc(&c, 0xFFFC); //Resetting the pc
            printf("Reset vector: %04X\n", c.pc);

            //Setting the ppu's reference to the ROM to the bus's ROM
            c.b->p->rom = c.b->rom;

            //Doubling buffering the screen
            frame tile_frame[2] = {{0}, {0}};
            uint8_t curr_frame = 0;

            while(!glfwWindowShouldClose(window) && !c.stop) {
                gettimeofday(&start, NULL); //Getting time at start of frame

                //Clear the screen
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                //Render the current frame
                render_begin(&r);
                    draw_frame(&r, &tile_frame[curr_frame]);
                render_end(&r);

                //Clear the backbuffer
                curr_frame = !curr_frame;
                memset(tile_frame[curr_frame].data, 0, FRAME_WIDTH * FRAME_HEIGHT *3);

                //Run the CPU for this frame
                //Assume ratio of 1 CPU clock to 3 PPU clocks
                //Could add a master clock to make this more accurate
                size_t count = 0; //Number of cycles this frame
                while(count < CPU_CYCLES_PER_FRAME) {
                    if(c.b->dma_stall > 0) { //If we need to stall the cpu because of a mass DMA transfer
                        c.b->dma_stall--;
                        ppu_tick(c.b->p, &tile_frame[curr_frame]);
                        ppu_tick(c.b->p, &tile_frame[curr_frame]);
                        ppu_tick(c.b->p, &tile_frame[curr_frame]);
                        c.b->total_cycles++;
                        count++;
                        continue;
                    }

                    size_t cycles; //Number of cycles for this cpu instruction
                    //If an IRQ occurs
                    if(c.b->p->rom->irq_pending && !get_cpu_flag(&c, INTERRUPT_DISABLE)) {
                        cycles = 7;
                        interrupt_irq(&c);
                    }
                    //If an NMI occurs
                    else if(c.b->p->nmi_triggered) {
                        cycles = 7;
                        interrupt_nmi(&c);
                    }
                    //Run as normal
                    else {
                        cycles = execute_instr(&c);
                        // printf("0x02: %02X, 0x03: %02X\n", mem_read(c.b, (0x02)), mem_read(c.b, (0x03)));
                    }
                    //Tick up the ppu so they are synchronised
                    for(int i = 0; i < cycles; i++) {
                        ppu_tick(c.b->p, &tile_frame[curr_frame]);
                        ppu_tick(c.b->p, &tile_frame[curr_frame]);
                        ppu_tick(c.b->p, &tile_frame[curr_frame]);
                        count++;
                        c.b->total_cycles++;
                    }
                }

                glfwSwapBuffers(window);
                glfwPollEvents();

                gettimeofday(&stop, NULL); //Get the time at the end of the frame
                dt = (stop.tv_sec - start.tv_sec) * 1000 + (stop.tv_usec - start.tv_usec) / 1000.0f; //Get the frame length
                // If the frame took less time than the set frame rate, wait until the time is the frame rate
                if(dt < FRAME_RATE) {
                    sleep_ms((int)FRAME_RATE - dt);
                    dt = FRAME_RATE;
                }
            }
            glfwTerminate();

            print_cpu_state(&c);
            print_stack(&c);
            print_page(&c, 0);

        }

        //Freeing memory
        if(c.b->rom->chr_rom) free(c.b->rom->chr_rom);
        if(c.b->rom->prg_rom) free(c.b->rom->prg_rom);
        if(c.b->rom->chr_ram) free(c.b->rom->chr_ram);
        if(c.b->rom->prg_ram) free(c.b->rom->prg_ram);
        if(c.b->rom->prg_eeprom) free(c.b->rom->prg_eeprom);
        free(c.b->rom);
        free(c.b->p->addr_reg);
        free(c.b->p->scroll_reg);
        free(c.b->p);
        free(c.b->player_1);
        free(c.b->player_2);
        free(c.b);
    }
    return 0;
}
