// Records docs/demo.gif without a window: scripted mouse strokes run through the
// same simulation and drawing code as main.c, and every other frame is saved as a BMP.
//   gcc -O2 docs/demo.c -o /tmp/demo $(pkg-config --cflags --libs sdl3)
//   mkdir -p /tmp/frames && SDL_VIDEODRIVER=offscreen /tmp/demo /tmp/frames
//   ffmpeg -framerate 30 -i /tmp/frames/f%04d.bmp -vf "scale=450:-1:flags=area,split[a][b];[a]palettegen[p];[b][p]paletteuse=dither=none" docs/demo.gif
#define NO_MAIN
#include "../main.c"

// What a left-button drag over these cells does with the given key selected (-1 = wall).
static void paint(Cell env[], int type, int x0, int x1, int y0, int y1){
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++){
            int idx = y * COLS + x;
            if (type == -1)
                env[idx] = (Cell){-1, 0.0, x, y, 0, wall};
            else if (env[idx].type != -1)
                env[idx] = (Cell){type, 1.0, x, y, liquids[type].weight, liquids[type].color};
        }
}

int main(int argc, char **argv){
    if (argc != 2){
        SDL_Log("usage: %s <frames dir>", argv[0]);
        return 1;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)){
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Fluid Simu", WIDTH_SCREEN, HEIGHT_SCREEN, 0);
    SDL_Renderer *render = window ? SDL_CreateRenderer(window, "software") : NULL;
    if (!render){
        SDL_Log("Renderer ERROR: %s", SDL_GetError());
        return 1;
    }

    static Cell env[COLS * ROWS];
    prep_env(env);
    paint(env, -1,  8, 36, 40, 40);  // cup
    paint(env, -1,  8,  8, 18, 40);
    paint(env, -1, 36, 36, 18, 40);
    paint(env, -1, 13, 21, 12, 12);  // shelf under the water stream

    bool pression = false;
    for (int frame = 0; frame < 400; frame++){
        if (frame < 80)                  paint(env,  0, 17, 18, 2, 2);  // W: water
        if (frame >= 50 && frame < 130)  paint(env,  5, 29, 30, 2, 2);  // 5: oil
        if (frame >= 150 && frame < 190) paint(env, 19, 23, 24, 2, 2);  // M: mercury
        if (frame == 320) pression = true;                              // P: pressure view

        for (int it = 0; it < 5; it++){
            simulation_step(env);
            mix_step(env);
            density_step(env);
        }

        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderClear(render);
        draw_env(env, render, pression);
        draw_grid(render);
        if (frame % 2 == 0){
            char path[1024];
            SDL_snprintf(path, sizeof path, "%s/f%04d.bmp", argv[1], frame / 2);
            SDL_Surface *shot = SDL_RenderReadPixels(render, NULL);
            if (!shot || !SDL_SaveBMP(shot, path)){
                SDL_Log("Capture ERROR: %s", SDL_GetError());
                return 1;
            }
            SDL_DestroySurface(shot);
        }
        SDL_RenderPresent(render);
    }

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
