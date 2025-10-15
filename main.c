#include <SDL3/SDL_events.h>
#include <stdio.h>
#include <time.h>
#include "cpu.h"
#include "ppu.h"
#include "shader.h"

int start_cpu(cpu *c) {
    if(!cpu_init(c)) {
        printf("Failed to initialise cpu\n");
        return 0;
    }
    else {
        if(!cpu_load(c)) {
            printf("Failed to load rom\n");
            return 0;
        }
    }
    return 1;
}

int start_ppu(ppu *p) {
    if(!ppu_init(p)) {
        printf("Failed to load ppu\n");
        return 0;
    }
    else {
        if(!ppu_load(p)) {
            printf("Failed to load ppu\n");
            return 0;
        }
    }
    return 1;
}

void loop(cpu *c, ppu *p) {
    int quit = 0;
    SDL_Event e;
    float dt = 0.0f;

    while(!quit) {
        time_t startTime = time(0);

        while(SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_EVENT_QUIT:
                    quit = 1;
                    break;
            }
        }
        int cpu_cycle_count = 0;
        for(int i = 0; i < CYCLES_PER_FRAME; i++) {
            // int cycle_count = 0;
            // while(c.pc != 0x900) {
            if(i % 3 == 0) {
                if(cpu_cycle_count == 0) {
                    execute(c, c->memory.memory[c->pc], &cpu_cycle_count);
                }
                cpu_cycle_count--;
            }
                // cycle_count++;
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        use(s);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        SDL_GL_SwapWindow(gWindow);

        dt = time(0) - startTime;
        if(dt < FRAME_RATE) SDL_Delay(FRAME_RATE - dt); //Getting 60 fps
        // }
    }
}

int main(int argc, char **argv) {
    cpu c;
    ppu p;

    if(!start_cpu(&c)) {
        return 1;
    }
    else {
        if(!start_ppu(&p)) {
            return 1;
        }
        else {
            loop(&c, &p);
        }
    }
    return 0;
}
