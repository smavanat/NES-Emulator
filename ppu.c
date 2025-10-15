#include "ppu.h"
#include "memory.h"
#include <SDL3/SDL_pixels.h>
#include <stdint.h>
#include <stdio.h>

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
SDL_GLContext glContext;

SDL_Color palette[64] = {{0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0}, {0x44, 0x00, 0x96}, {0xA1, 0x00, 0x5E},
                       {0xC7, 0x00, 0x28}, {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00}, {0x5C, 0x2F, 0x00}, {0x10, 0x45, 0x00},
                       {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E}, {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05},
                       {0x05, 0x05, 0x05}, {0xC7, 0xC7, 0xC7}, {0x00, 0x77, 0xFF}, {0x21, 0x55, 0xFF}, {0x82, 0x37, 0xFA},
                       {0xEB, 0x2F, 0xB5}, {0xFF, 0x29, 0x50}, {0xFF, 0x22, 0x00}, {0xD6, 0x32, 0x00}, {0xC4, 0x62, 0x00},
                       {0x35, 0x80, 0x00}, {0x05, 0x8F, 0x00}, {0x00, 0x8A, 0x55}, {0x00, 0x99, 0xCC}, {0x21, 0x21, 0x21},
                       {0x09, 0x09, 0x09}, {0x09, 0x09, 0x09}, {0xFF, 0xFF, 0xFF}, {0x0F, 0xD7, 0xFF}, {0x69, 0xA2, 0xFF},
                       {0xD4, 0x80, 0xFF}, {0xFF, 0x45, 0xF3}, {0xFF, 0x61, 0x8B}, {0xFF, 0x88, 0x33}, {0xFF, 0x9C, 0x12},
                       {0xFA, 0xBC, 0x20}, {0x9F, 0xE3, 0x0E}, {0x2B, 0xF0, 0x35}, {0x0C, 0xF0, 0xA4}, {0x05, 0xFB, 0xFF},
                       {0x5E, 0x5E, 0x5E}, {0x0D, 0x0D, 0x0D}, {0x0D, 0x0D, 0x0D}, {0xFF, 0xFF, 0xFF}, {0xA6, 0xFC, 0xFF},
                       {0xB3, 0xEC, 0xFF}, {0xDA, 0xAB, 0xEB}, {0xFF, 0xA8, 0xF9}, {0xFF, 0xAB, 0xB3}, {0xFF, 0xD2, 0xB0},
                       {0xFF, 0xEF, 0xA6}, {0xFF, 0xF7, 0x9C}, {0xD7, 0xE8, 0x95}, {0xA6, 0xED, 0xAF}, {0xA2, 0xF2, 0xDA},
                       {0x99, 0xFF, 0xFC}, {0xDD, 0xDD, 0xDD}, {0x11, 0x11, 0x11}, {0x11, 0x11, 0x11}};

unsigned int texture, pbo, vao;
shader *s;

void set_ppu_flag(memory *m, int ppu_register, int flag, int val) {
    if(val)
        m->memory[ppu_register] |= (uint8_t)(1 << flag);
    else
        m->memory[ppu_register] &= ~(uint8_t)(0 << flag);
}

int get_ppu_flag(memory *m, int ppu_register, int flag) {
    if (m->memory[ppu_register] & (uint8_t)(1 << flag))
        return 1;
    return 0;
}

int ppu_init(ppu *p) {
    int success = 1;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        success = 0;
    }
    else
    {
        //Create window
        gWindow = SDL_CreateWindow("CHIP-8", SCREEN_WIDTH*SCALE, SCREEN_HEIGHT*SCALE, SDL_EVENT_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (gWindow == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            success = 0;
        }
        else {
            //Create the OpenGL context:
            glContext = SDL_GL_CreateContext(gWindow);
            //Create renderer for window
            gRenderer = SDL_CreateRenderer(gWindow, "opengl");
            SDL_SetRenderVSync(gRenderer, SDL_RENDERER_VSYNC_ADAPTIVE); 
            SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
            if (gRenderer == NULL) {
                printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                success = 0;
            }
            else {
                //Initialize renderer color
                SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
            }
            if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
                printf("Failed to initialise GLAD");
                success = 0;
            }
        }
    }
    return success;
}

int ppu_load(ppu *p) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, SCREEN_HEIGHT*SCREEN_WIDTH*3, NULL, GL_STREAM_DRAW);
    s = load_shader("./data/shaders/shader.vert", "./data/shaders/shader.frag");

    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    unsigned int VBO, EBO;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Bind pbo
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    uint8_t* pixels = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(pixels) {
        memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 3);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    return 1;
}

void render_tile(ppu *p, int pattern_start, int tile_num, uint8_t *pixels){
    uint8_t tile[16];

    for(int i = 0; i < 16; i++) {
        tile[i] = p->memory[pattern_start + tile_num * 16 + i];
    }

    for(int i = 0; i < 8; i++) {
        uint8_t upper = tile[i];
        uint8_t lower = tile[i+8];

        for(int j = 7; j > -1; j--) {
            uint8_t val = (1 & upper) << 1 | (1 & lower);
            upper >>= 1;
            lower >>= 1;

            SDL_Color colour;
            switch(val) {
                case 0:
                    colour = palette[0x01];
                    break;
                case 1:
                    colour = palette[0x23];
                    break;
                case 2:
                    colour = palette[0x27];
                    break;
                case 3:
                    colour = palette[0x30];
                    break;
            }
            int base = (i*3*SCREEN_WIDTH) + (j*3);
            pixels[base] = colour.r;
            pixels[base] = colour.g;
            pixels[base] = colour.b;
        }
    }
}

void render(ppu *p, memory *m) {
    int current_nametable = (2*get_ppu_flag(m, PPUCTRL, NAMETABLE_2))+get_ppu_flag(m, PPUCTRL, NAMETABLE_1);
    int n_start, n_end;
    switch(current_nametable){
        case 0:
            n_start = NAMETABLE_0_START;
            n_end = NAMETABLE_0_END;
        break;
        case 1:
            n_start = NAMETABLE_1_START;
            n_end = NAMETABLE_1_END;
        break;
        case 2:
            n_start = NAMETABLE_2_START;
            n_end = NAMETABLE_2_END;
        break;
        case 3:
            n_start = NAMETABLE_3_START;
            n_end = NAMETABLE_3_END;
        break;
    }

    int pattern_start = (get_ppu_flag(m, PPUCTRL, SPRITE_PATTERN_ADDR)) ? PATTERN_TABLE_1_START : PATTERN_TABLE_0_START;
    int pattern_end = (get_ppu_flag(m, PPUCTRL, SPRITE_PATTERN_ADDR)) ? PATTERN_TABLE_1_END: PATTERN_TABLE_0_END;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    uint8_t *pixels = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(!pixels) {
        printf("Could not get pbo\n");
        return;
    }

    render_tile(p, pattern_start, 0, pixels);
}
