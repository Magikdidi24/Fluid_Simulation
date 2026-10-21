// https://www.jgallant.com/2d-liquid-simulator-with-cellular-automaton-in-unity/

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH_SCREEN  900
#define HEIGHT_SCREEN 900
#define CELL_SIZE     20
#define COLS (WIDTH_SCREEN / CELL_SIZE)
#define ROWS (HEIGHT_SCREEN / CELL_SIZE)

#define MAX_LIQUID       1.0
#define MAX_COMPRESSION  0.25
#define MIN_FLOW         0.005
#define MIX_TYPE         100
#define MIX_THRESHOLD    10

typedef struct {
    SDL_Color color;
    double weight;
} Liquid;

SDL_Color wall = {0, 255, 0, 255};

const Liquid liquids[] = {
    {{0, 164, 223,   255}, 1.00},  // water
    {{255, 140,   0, 255}, 0.72},  // gasoline
    {{200, 200, 255, 255}, 0.79},  // ethanol
    {{180, 180, 100, 255}, 0.81},  // kerosene
    {{100, 180,   0, 255}, 0.91},  // olive oil
    {{255, 255,   0, 255}, 0.92},  // oil
    {{240, 240, 180, 255}, 0.93},  // coconut oil
    {{220, 220, 180, 255}, 1.01},  // vinegar
    {{101,  67,  33, 255}, 1.01},  // coffee
    {{245, 245, 220, 255}, 1.03},  // milk
    {{0, 100, 200,  255}, 1.03},  // sea water
    {{255, 165,   0, 255}, 1.04},  // orange juice
    {{139,  69,  19, 255}, 1.04},  // soda
    {{255,   0,   0, 255}, 1.05},  // blood
    {{180, 180, 255, 255}, 1.26},  // glycerin
    {{160,  82,  45, 255}, 1.33},  // syrup
    {{255, 191,   0, 255}, 1.42},  // honey
    {{128, 255, 128, 255}, 1.84},  // sulfuric acid
    {{139,  34,  34, 255}, 3.12},  // bromine
    {{192, 192, 192, 255}, 13.56}  // mercury
};

typedef struct {
    int    type;
    double fill_level;
    int    x;
    int    y;
    double weight;
    SDL_Color color;
} Cell;

void color_cell(SDL_Renderer *render, Cell cell, SDL_Color color){
    SDL_SetRenderDrawColor(render, color.r, color.g, color.b, color.a);
    SDL_FRect r = {cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_RenderFillRect(render, &r);
}

double pression_calcul(Cell env[], int x, int y){
    double pression = 0;
    for (int i = y - 1; i >= 0; i--){
        int idx = i * COLS + x;
        if (env[idx].type == -1 || env[idx].fill_level <= MIN_FLOW)
            break;
        pression += env[idx].fill_level;
    }
    return pression / 20;
}

SDL_Color gradient_blue(SDL_Color a, SDL_Color b, double t){
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    SDL_Color c;
    c.r = (Uint8)(a.r + (b.r - a.r) * t);
    c.g = (Uint8)(a.g + (b.g - a.g) * t);
    c.b = (Uint8)(a.b + (b.b - a.b) * t);
    c.a = 255;
    return c;
}

void draw_env(Cell env[], SDL_Renderer *render, bool pression){
    for (int y = 0; y < ROWS; y++){
        for (int x = 0; x < COLS; x++){
            Cell c = env[y * COLS + x];
            if (c.type == -1){
                color_cell(render, c, wall);
            } else if (c.fill_level > 0.001){
                double f = c.fill_level > 1.0 ? 1.0 : c.fill_level;
                int h = (int)(f * CELL_SIZE + 0.5);
                if (pression){
                    double p = pression_calcul(env, x, y);
                    SDL_Color clair = {140, 210, 250, 255};
                    SDL_Color fonce = {  5,  30,  90, 255};
                    SDL_Color col = gradient_blue(clair, fonce, p);
                    SDL_SetRenderDrawColor(render, col.r, col.g, col.b, liquids[0].color.a);
                } else {
                    SDL_Color col;
                    if (c.type == MIX_TYPE) col = c.color;
                    else if (c.type >= 0 && c.type < 20)  col = liquids[c.type].color;
                    else col = c.color;
                    SDL_SetRenderDrawColor(render, col.r, col.g, col.b, col.a);
                }
                SDL_FRect rect = { x * CELL_SIZE, y * CELL_SIZE + (CELL_SIZE - h), CELL_SIZE, h };
                SDL_RenderFillRect(render, &rect);
            }
        }
    }
}

void draw_grid(SDL_Renderer *render){
    SDL_SetRenderDrawColor(render, 125, 125, 125, 255);
    for (int x = 0; x <= WIDTH_SCREEN; x += CELL_SIZE)
        SDL_RenderLine(render, x, 0, x, HEIGHT_SCREEN);
    for (int y = 0; y <= HEIGHT_SCREEN; y += CELL_SIZE)
        SDL_RenderLine(render, 0, y, WIDTH_SCREEN, y);
}


void prep_env(Cell env[]){
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            env[y * COLS + x] = (Cell){-2, 0.0, x, y, 0, {0,0,0,0}};
}

double vertical_flow_value(double remaining, double dest){
    double sum = remaining + dest;
    if (sum <= MAX_LIQUID)
        return MAX_LIQUID;
    else if (sum < 2 * MAX_LIQUID + MAX_COMPRESSION)
        return (MAX_LIQUID * MAX_LIQUID + sum * MAX_COMPRESSION) / (MAX_LIQUID + MAX_COMPRESSION);
    else
        return (sum + MAX_COMPRESSION) / 2.0;
}

void simulation_step(Cell env[]){
    double    diff[COLS * ROWS] = {0};
    int       src[COLS * ROWS];
    SDL_Color src_col[COLS * ROWS];
    double    src_w[COLS * ROWS];
    for (int i = 0; i < COLS * ROWS; i++) src[i] = -1;

    for (int y = 0; y < ROWS; y++){
        for (int x = 0; x < COLS; x++){
            int idx = y * COLS + x;
            Cell c = env[idx];
            if (c.type == -1 || c.fill_level <= 0) continue;

            double remaining = c.fill_level;
            double flow;

            // --- règle #1 : vers le bas ---
            if (y < ROWS - 1){
                int below = (y + 1) * COLS + x;
                if (env[below].type == -2 || env[below].type == c.type){
                    flow = vertical_flow_value(remaining, env[below].fill_level) - env[below].fill_level;
                    if (flow > MIN_FLOW) flow *= 0.5;
                    if (flow < 0) flow = 0;
                    if (flow > remaining) flow = remaining; 
                    if (flow > 0){
                        diff[idx] -= flow; diff[below] += flow; remaining -= flow;
                        if (src[below] < 0){ src[below]=c.type; src_col[below]=c.color; src_w[below]=c.weight; }
                    }
                }
            }
            if (remaining < MIN_FLOW) continue;

            // --- règle #2 : gauche ---
            if (x > 0){
                int left = y * COLS + (x - 1);
                if (env[left].type == -2 || env[left].type == c.type){
                    flow = (remaining - env[left].fill_level) / 4.0;
                    if (flow < 0) flow = 0;
                    if (flow > remaining) flow = remaining; 
                    if (flow > 0){
                        diff[idx] -= flow; diff[left] += flow; remaining -= flow;
                        if (src[left] < 0){ src[left]=c.type; src_col[left]=c.color; src_w[left]=c.weight; }
                    }
                }
            }
            if (remaining < MIN_FLOW) continue;

            // --- règle #2 : droite ---
            if (x < COLS - 1){
                int right = y * COLS + (x + 1);
                if (env[right].type == -2 || env[right].type == c.type){
                    flow = (remaining - env[right].fill_level) / 4.0;
                    if (flow < 0) flow = 0;
                    if (flow > remaining) flow = remaining; 
                    if (flow > 0){
                        diff[idx] -= flow; diff[right] += flow; remaining -= flow;
                        if (src[right] < 0){ src[right]=c.type; src_col[right]=c.color; src_w[right]=c.weight; }
                    }
                }
            }
            if (remaining < MIN_FLOW) continue;

            // --- règle #3 : pression vers le haut ---
            if (y > 0){
                int above = (y - 1) * COLS + x;
                if (env[above].type == -2 || env[above].type == c.type){
                    flow = remaining - vertical_flow_value(remaining, env[above].fill_level);
                    if (flow > MIN_FLOW) flow *= 0.5;
                    if (flow < 0) flow = 0;
                    if (flow > remaining) flow = remaining;
                    if (flow > 0){
                        diff[idx] -= flow; diff[above] += flow; remaining -= flow;
                        if (src[above] < 0){ src[above]=c.type; src_col[above]=c.color; src_w[above]=c.weight; }
                    }
                }
            }
        }
    }

    for (int i = 0; i < COLS * ROWS; i++){
        if (env[i].type == -1) continue;
        env[i].fill_level += diff[i];
        if (env[i].fill_level < MIN_FLOW){
            env[i].fill_level = 0;
            env[i].type = -2;
        } else if (env[i].type == -2){
            env[i].type   = src[i] >= 0 ? src[i] : 0;
            env[i].color  = src_col[i];
            env[i].weight = src_w[i];
        }
    }
}

void density_step(Cell env[]){
    for (int y = ROWS - 2; y >= 0; y--){
        for (int x = 0; x < COLS; x++){
            int idx   = y * COLS + x;
            int below = (y + 1) * COLS + x;
            Cell top = env[idx];
            Cell bot = env[below];

            if (top.type < 0 || bot.type < 0) continue;
            if (top.type == bot.type) continue;

            if (top.weight > bot.weight){
                env[idx]   = bot;
                env[below] = top;
                env[idx].x   = x; env[idx].y   = y;
                env[below].x = x; env[below].y = y + 1;
            }
        }
    }
}

void mix_step(Cell env[]){
    for (int y = 0; y < ROWS; y++){
        for (int x = 0; x < COLS; x++){
            int idx = y * COLS + x;
            Cell a = env[idx];
            if (a.type < 0 || a.fill_level <= MIN_FLOW) continue;

            int neigh[2] = { (x < COLS-1) ? idx + 1    : -1,
                             (y < ROWS-1) ? idx + COLS : -1 };
            for (int d = 0; d < 2; d++){
                int j = neigh[d];
                if (j < 0) continue;
                Cell b = env[j];
                if (b.type < 0 || b.fill_level <= MIN_FLOW) continue;
                if (a.type == b.type) continue;
                if (fabs(a.weight - b.weight) > MIX_THRESHOLD) continue;

                double ta = a.fill_level, tb = b.fill_level, tt = ta + tb;
                SDL_Color m = {
                    (Uint8)((a.color.r*ta + b.color.r*tb)/tt),
                    (Uint8)((a.color.g*ta + b.color.g*tb)/tt),
                    (Uint8)((a.color.b*ta + b.color.b*tb)/tt),
                    255
                };
                double mw = (a.weight*ta + b.weight*tb) / tt;

                env[idx].type=MIX_TYPE; env[idx].color=m; env[idx].weight=mw;
                env[j].type  =MIX_TYPE; env[j].color  =m; env[j].weight  =mw;
                a = env[idx];
            }
        }
    }
}

#ifndef NO_MAIN // tests/test_sim.c includes this file for the simulation functions
int main(void){
    if (!SDL_Init(SDL_INIT_VIDEO)){
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("Fluid Simu", WIDTH_SCREEN, HEIGHT_SCREEN, 0);
    if (!window){
        SDL_Log("Window ERROR: %s", SDL_GetError());
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    SDL_Renderer *render = SDL_CreateRenderer(window, NULL);
    if (!render){
        SDL_Log("Renderer ERROR: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    Cell env[COLS * ROWS];
    prep_env(env);

    SDL_Event event;
    int running = 1;
    int color = 0;
    int pression = false;

    while (running){
        while (SDL_PollEvent(&event)){
            if (event.type == SDL_EVENT_QUIT) running = 0;

            if (event.type == SDL_EVENT_MOUSE_MOTION){
                int cell_x = (int)SDL_floorf(event.motion.x / CELL_SIZE);
                int cell_y = (int)SDL_floorf(event.motion.y / CELL_SIZE);
                if (cell_x >= 0 && cell_x < COLS && cell_y >= 0 && cell_y < ROWS){
                    int idx = cell_y * COLS + cell_x;
                    if (event.motion.state & SDL_BUTTON_LMASK){
                        if (color == -1)
                            env[idx] = (Cell){-1, 0.0, cell_x, cell_y,0, wall};
                        else{
                            if (env[idx].type != -1)
                                env[idx] = (Cell){color, 1.0, cell_x, cell_y,liquids[color].weight, liquids[color].color};
                        }
                            
                    }
                    if (event.motion.state & SDL_BUTTON_RMASK)
                        env[idx] = (Cell){-2, 0.0, cell_x, cell_y, 0,{0,0,0,0}};
                }
            }

            if (event.type == SDL_EVENT_KEY_DOWN){
                switch (event.key.key){
                    case SDLK_SPACE: color = -1; break; // wall
                    case SDLK_W: color =  0; break; // water
                    case SDLK_1: color =  1; break; // gasoline
                    case SDLK_2: color =  2; break; // ethanol
                    case SDLK_3: color =  3; break; // kerosene
                    case SDLK_4: color =  4; break; // olive oil
                    case SDLK_5: color =  5; break; // oil
                    case SDLK_6: color =  6; break; // coconut oil
                    case SDLK_7: color =  7; break; // vinegar
                    case SDLK_8: color =  8; break; // coffee
                    case SDLK_9: color =  9; break; // milk

                    case SDLK_Q: color = 10; break; // sea water
                    case SDLK_S: color = 11; break; // orange juice
                    case SDLK_D: color = 12; break; // soda
                    case SDLK_F: color = 13; break; // blood
                    case SDLK_G: color = 14; break; // glycerin
                    case SDLK_H: color = 15; break; // syrup
                    case SDLK_J: color = 16; break; // honey
                    case SDLK_K: color = 17; break; // sulfuric acid
                    case SDLK_L: color = 18; break; // bromine

                    case SDLK_M: color = 19; break; // mercury

                    case SDLK_P: pression = !pression; break; // pressure view
                }
            }
        }

        for (int it = 0; it < 5; it++){
            simulation_step(env);
            mix_step(env);
            density_step(env);
        }

        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderClear(render);
        draw_env(env, render,pression);
        draw_grid(render);
        SDL_RenderPresent(render);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif
