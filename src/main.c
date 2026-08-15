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
#include "joypad.h"
#include "ppu.h"
#include "clay_layout.h"
#define BOB_INCLUDE_GLAD
#define BOB_IMPLEMENTATION
#include "../externals/bob.h"

#include "../externals/GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../externals/stb_image.h"

#define FRAME_RATE 1000 / 60.0f

//TODO: Debugger
//      JIT
//      APU
uint8_t stop = 0;
GLFWwindow *window;
cpu c = {0};
BOB_Renderer_Handle r;
BOB_Font_Handle bitmap;
mouse_state mstate = {0};
int screen_width = 800;
int screen_height = 600;

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
    #ifdef BOB_INCLUDE_GLAD
    glViewport(0, 0, width, height);
    #endif
    glfwGetWindowSize(window, &screen_width, &screen_height);
    BOB_renderer_update_dimensions(r, screen_width, screen_height, width, height);
    // r.screen_width = width;
    // r.screen_height = height;
    //
    // //Update projection matrix for pixel renderer
    // ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f, r.projection);
    // glUseProgram(r.shader);
    // int proj_loc = glGetUniformLocation(r.shader, "uProjection");
    // glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *)r.projection);
    //
    // //Update the uv coordinates of the texture the pixel renderer is rendering to
    // float quadVertices[] = {
    //     0.0f, 0.0f,          0.0f, 0.0f,
    //     0.0f, height,        0.0f, 1.0f,
    //     width, height,       1.0f, 1.0f,
    //     width, 0.0f,         1.0f, 0.0f
    // };
    //
    // glBindBuffer(GL_ARRAY_BUFFER, r.vbo);
    // glBufferSubData(GL_ARRAY_BUFFER, 0,
    //                 sizeof(quadVertices), quadVertices);
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

uint8_t cursor_in_bounds(mouse_state *state) {
    if(state->mouse_pos.x < 0 || state->mouse_pos.x >= screen_width) return 0;
    if(state->mouse_pos.y < 0 || state->mouse_pos.y >= screen_height) return 0;
    return 1;
}

void cursor_callback(GLFWwindow *window, double xpos, double ypos) {
    if(glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        mstate.mouse_pos.x = xpos;
        mstate.mouse_pos.y = ypos;
    }
}

void mouse_callback(GLFWwindow *window, int key, int action, int mods) {
    if(glfwGetWindowAttrib(window, GLFW_FOCUSED) && cursor_in_bounds(&mstate) && key < 3) {
        if(key == GLFW_MOUSE_BUTTON_LEFT) mstate.mouse_down = (action == GLFW_PRESS);
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if(glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        mstate.scroll_pos.x = xoffset;
        mstate.scroll_pos.y = yoffset;
    }
}

//Function that initialises GLFW and glad
int init(GLFWwindow **window) {
    //Initialising GLFW:
    glfwInit();
    #ifdef BOB_INCLUDE_GLAD
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24); //Necessary for enabling the depth buffer

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    *window = glfwCreateWindow(screen_width, screen_height, "JES", NULL, NULL);

    if(*window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 0;
    }

    //Setting callback functions
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);

    if(!BOB_init((GLADloadproc)glfwGetProcAddress, 1)) {
        glfwTerminate();
        return 0;
    }
    if(!BOB_create_opengl_renderer(BOB_MAX_ATLAS_CAPACITY, BOB_MAX_PIXELBUFFER_CAPACITY, BOB_MAX_TEX_CAPACITY, BOB_MAX_MATERIAL_CAPACITY, BOB_MAX_FONT_CAPACITY,
                                   800, 600, BOB_MAX_VERTEX_CAPACITY, BOB_MAX_INDEX_CAPACITY, BOB_MAX_DRAW_CALL_CAPACITY, &r)) {
        glfwTerminate();
        return 0;
    }
    #endif

    #ifdef BOB_INCLUDE_VULKAN
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Set to NO_API so that it uses Vulkan instead of OpenGL
    *window = glfwCreateWindow(screen_width, screen_height, "JES", NULL, NULL);

    if(*window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 0;
    }

    //Setting callback functions
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);

    //Get the required extensions from GLFW
    uint32_t glfw_extension_count = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    if(glfwExtensions == NULL) {
        printf("Failed to get required GLFW extensions\n");
        return 0;
    }

    if(!BOB_init(glfwExtensions, glfw_extension_count, 1)) {
        glfwTerminate();
        return 0;
    }

    int width, height;
    glfwGetFramebufferSize(*window, &width, &height);
    if(!BOB_create_vulkan_renderer(BOB_MAX_ATLAS_CAPACITY, BOB_MAX_PIXELBUFFER_CAPACITY, BOB_MAX_TEX_CAPACITY, BOB_MAX_MATERIAL_CAPACITY, BOB_MAX_FONT_CAPACITY,
                                  800, 600, width, height, BOB_MAX_VERTEX_CAPACITY, BOB_MAX_INDEX_CAPACITY, BOB_MAX_DRAW_CALL_CAPACITY, &create_window_surface, &r)) {
        glfwTerminate();
        return 0;
    }
    #endif

    return 1;
}

uint32_t load_atlas(char *path, BOB_Font_Handle font) {
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        switch (nrChannels) {
            case 1: format = BOB_RED; break;
            case 2: format = BOB_RG; break;
            case 3: format = BOB_RGB; break;
            case 4: format = BOB_RGBA; break;
        }

        BOB_add_font_page(font, width, height, data, format);
    }
    else {
        fprintf(stderr, "Failed to load texture at path %s\n", path);
        stbi_image_free(data);
        return 0;
    }
    stbi_image_free(data);

    return 1;
}

int main(void) {
    //Initialising pc
    c.sp = 0xFF; //Setting stack pointer to top of stack
    c.proc_stat_reg = 0x34; //Setting BREAK and UNUSED flags

    //Reading the data from a ROM
    //TODO: Make the rom path user inputable
    uint8_t *buf;
    int sz = read_to_end("../roms/zelda.nes", &buf, 0);
    if(sz < 0) {
        fprintf(stderr, "Error when opening a file\n");
        return 0;
    }

    if(init(&window)) {
        printf("Initialised\n");

        BOB_load_bmf_font(r, "../nes.fnt", BOB_BMF_TEXT, &bitmap);
        load_atlas("../nes.png", bitmap); //This bitmap png is from: https://frostyfreeze.itch.io/pixel-bitmap-fonts-png-xml

        // BOB_bitmap_font_init(&bitmap, tex, 6, 10, 0, 0, 0, 0, BOB_BITMAP_CUSTOM, (BOB_Bitmap_Layout_Desc){.custom_desc = {.data = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-=()[]{}<>/*:#%!?.,'\"@&$", .len = 87}});

        c.b = calloc(1, sizeof(bus));
        c.b->rom = calloc(1, sizeof(rom));
        c.b->p = calloc(1, sizeof(ppu));
        c.b->p->addr_reg = calloc(1, sizeof(addr_register));
        c.b->p->addr_reg->h_ptr = 1;
        c.b->p->scroll_reg = calloc(1, sizeof(scroll_register));
        c.b->p->scroll_reg->s_ptr = 1;
        c.b->player_1 = calloc(1, sizeof(joypad));
        c.b->player_2 = calloc(1, sizeof(joypad));

        clay_init(r, screen_width, screen_height);

        struct timeval stop, start; //Store the start and end times of a frame
        float dt = 0.0f; //Holds the time passed between frames

        if(!rom_load(c.b->rom, buf, sz)) { //Loading the rom into memory
            printf("Loaded\n");
            free(buf);
            buf = NULL;

            //Some debug functions
            //TODO: Write a proper debugger and get rid of this
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
            BOB_Pixelbuffer_Handle pb;
            BOB_pixelbuffer_init(r, FRAME_WIDTH, FRAME_HEIGHT, BOB_RGB, &pb);

            char disassembly_buf[1024];
            size_t disassembly_buf_used = 0;

            while(!glfwWindowShouldClose(window) && !c.stop) {
                gettimeofday(&start, NULL); //Getting time at start of frame

                clay_update_dimensions(r, &mstate, dt, screen_width, screen_height);
                Clay_RenderCommandArray renderCommands = clay_set_layout(&c, (uint8_t *)&tile_frame[curr_frame].data, dt, disassembly_buf, disassembly_buf_used);

                //Clear the screen
                BOB_renderer_begin(r, (float[4]){0.0f, 0.0f, 0.0f, 0.0f});
                    clay_render(r, bitmap, renderCommands, pb);
                BOB_renderer_end(r);

                //Clear the backbuffer
                curr_frame = !curr_frame;
                memset(tile_frame[curr_frame].data, 0, FRAME_WIDTH * FRAME_HEIGHT *3);
                disassembly_buf_used = 0;

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
                    if(c.b->p->rom->irq_pending && !get_cpu_flag(&c, CPU_INTERRUPT_DISABLE)) {
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

                    if(count >= CPU_CYCLES_PER_FRAME - 32) {
                        disassembly_buf_used += (size_t)append_disassembly_string(&c, &disassembly_buf[disassembly_buf_used]);
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
            clay_free();
            glfwTerminate();
        }
        //Freeing memory
        if(buf) free(buf);
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

        BOB_terminate();
    }

    return 0;
}
