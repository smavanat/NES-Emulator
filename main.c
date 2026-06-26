#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "externals/glad.h"
#include "externals/GLFW/glfw3.h"
#include "ppu.h"

uint8_t stop = 0;
GLFWwindow *window;
Renderer r = {0};

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    r.screen_width = width;
    r.screen_height = height;

    //Projection matrix for pixel renderer
    ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f, r.projection);
    glUseProgram(r.shader);
    int proj_loc = glGetUniformLocation(r.shader, "uProjection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *)r.projection);

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

    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);

    //Loading GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialise GLAD");
        glfwTerminate();
        return 0;
    }

    return 1;
}

int main(void) {
    cpu c = {0};
    c.sp = 0xFF;

    uint8_t *buf;
    int sz = read_to_end("../pacman.nes", &buf, 0);
    if(sz < 0) {
        fprintf(stderr, "Error when opening a file\n");
        return 0;
    }

    if(init(&window)) {
        printf("Initialised\n");
        c.b = malloc(sizeof(bus));
        c.b->rom = malloc(sizeof(rom));
        c.b->p = malloc(sizeof(ppu));
        c.b->p->addr_reg = malloc(sizeof(addr_register));
        c.b->p->addr_reg->h_ptr = 1;
        c.b->p->scroll_reg = malloc(sizeof(scroll_register));
        c.b->p->scroll_reg->s_ptr = 1;
        r = render_init();

        if(!rom_load(c.b->rom, buf, sz)) {
            printf("Loaded\n");
            free(buf);

            set_pc(&c, 0xFFFC); //Resetting the pc
            c.b->p->chr_rom = c.b->rom->chr_rom;
            c.b->p->chr_rom_sz = c.b->rom->chr_rom_sz;
            frame tile_frame = {0};

            while(!glfwWindowShouldClose(window) && !c.stop) {
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                render_begin(&r);
                    draw_frame(&r, &tile_frame);
                render_end(&r);

                size_t count = 0;
                while(count < CPU_CYCLES_PER_FRAME) {
                    size_t cycles;
                    if(c.b->p->nmi_triggered) {
                        cycles = 2;
                        interrupt_nmi(&c);
                    }
                    else {
                        cycles = execute_instr(&c);
                        printf("0x02: %02X, 0x03: %02X\n", mem_read(c.b, (0x02)), mem_read(c.b, (0x03)));
                    }
                    for(int i = 0; i < cycles; i++) {
                        ppu_tick(c.b->p, &tile_frame);
                        ppu_tick(c.b->p, &tile_frame);
                        ppu_tick(c.b->p, &tile_frame);
                        count++;
                    }
                }

                glfwSwapBuffers(window);
                glfwPollEvents();
            }
            glfwTerminate();

            print_cpu_state(&c);
            print_stack(&c);
            print_page(&c, 0);

        }
        free(c.b->rom->chr_rom);
        free(c.b->rom->prg_rom);
        free(c.b->rom);
        free(c.b->p->addr_reg);
        free(c.b->p->scroll_reg);
        free(c.b->p);
        free(c.b);
    }
    return 0;
}
