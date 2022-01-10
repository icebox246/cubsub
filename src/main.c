#include <SDL2/SDL.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define GRAY(n) \
    (0xff000000 | (n & 0xff) | ((n & 0xff) << 8) | ((n & 0xff) << 16))

#define RGB(r, g, b) \
    (0xff000000 | ((r & 0xff) << 16) | ((g & 0xff) << 16) | ((b & 0xff)))

#define WIDTH 800
#define HEIGHT 800

Uint32 minU32(Uint32 a, Uint32 b) { return (a > b) ? b : a; }

float slen(float a, float b) { return a * a + b * b; }

float lerp(float a, float b, float t) { return a + (b - a) * t; }

typedef enum { K_UP = 1, K_DOWN, K_LEFT, K_RIGHT } Controls;
bool key_pressed[32];

typedef struct {
    int w, h, s;
    int* pts;
} Grid;

bool is_point_colliding(int x, int y, Grid* grid) {
    int gx = minU32(x / grid->s, grid->w - 1);
    int gy = minU32(y / grid->s, grid->h - 1);

    const int v[4] = {
        grid->pts[gx + gy * grid->w],
        grid->pts[gx + 1 + gy * grid->w],
        grid->pts[gx + (gy + 1) * grid->w],
        grid->pts[gx + 1 + (gy + 1) * grid->w],
    };

    float dx = (x % grid->s) / (float)grid->s;
    float dy = (y % grid->s) / (float)grid->s;

    float c1 = lerp(v[0], v[1], dx);
    float c2 = lerp(v[2], v[3], dx);

    int c = lerp(c1, c2, dy);
    return c < 150;
}

SDL_Surface* generate_surface(int w, int h, Grid* grid) {
    Uint32 pixels[w * h];

    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            pixels[x + y * w] = is_point_colliding(x, y, grid) ? GRAY(200) : 0;
        }
    }
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        pixels, w, h, 32, 1, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    return surface;
}

typedef struct {
    float x, y;
    float vx, vy;
    int r;
    SDL_Texture* tex;
} PlayerSub;

void draw_player(SDL_Renderer* renderer, PlayerSub* player) {
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    const SDL_Rect body_rect = {.x = 0, .y = 0, .w = 256, .h = 256};
    const SDL_Rect visor_rect = {.x = 0, .y = 256, .w = 256, .h = 256};
    SDL_Rect player_rect = {.x = player->x - player->r,
                            .y = player->y - player->r,
                            .w = player->r * 2,
                            .h = player->r * 2};
    SDL_RenderCopy(renderer, player->tex, &body_rect, &player_rect);
    player_rect.x += player->r * lerp(0, 64.f / 256, player->vx / 200);
    player_rect.y += player->r * lerp(0, 48.f / 256, player->vy / 200);
    SDL_RenderCopy(renderer, player->tex, &visor_rect, &player_rect);
}

void update_player(PlayerSub* player, Grid* grid, float dt) {
    const float max_v = 200;
    int dx = 0, dy = 0;
    if (key_pressed[K_UP]) dy--;
    if (key_pressed[K_DOWN]) dy++;
    if (key_pressed[K_LEFT]) dx--;
    if (key_pressed[K_RIGHT]) dx++;

    player->vx = lerp(player->vx, max_v * dx, dt * 3);
    player->vy = lerp(player->vy, max_v * dy, dt * 3);

    player->x += player->vx * dt;
    bool collision = 0;
    for (int _x = -1; _x <= 1; _x++) {
        for (int _y = -1; _y <= 1; _y++) {
            if (is_point_colliding(player->x + player->r * _x,
                                   player->y + player->r * _y, grid)) {
                collision = 1;
                break;
            }
        }
        if (collision) break;
    }
    if (collision) {
        player->x -= player->vx * dt;
        player->vx *= -1;
    }
    player->y += player->vy * dt;
    collision = 0;
    for (int _x = -1; _x <= 1; _x++) {
        for (int _y = -1; _y <= 1; _y++) {
            if (is_point_colliding(player->x + player->r * _x,
                                   player->y + player->r * _y, grid)) {
                collision = 1;
                break;
            }
        }
        if (collision) break;
    }
    if (collision) {
        player->y -= player->vy * dt;
        player->vy *= -1;
    }
}

int main() {
    printf("[INFO] Hello, SubCub!\n");
    srand(time(0));

    // init sdl
    if (SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "[ERROR] Failed to initialize SDL\n");
    }
    SDL_Window* window = SDL_CreateWindow("SubCub", 10, 10, WIDTH, HEIGHT,
                                          SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    printf("[INFO] Created window and renderer\n");

    // generating texture
    printf("[INFO] Starting to generate texture\n");
    int gw = 31, gh = 31, gs = 800 / 29;
    int _grid[gw * gh];
    int _grid2[gw * gh];
    for (int x = 0; x < gw; x++) {
        for (int y = 0; y < gh; y++) {
            if (x != 0 && x != gw - 1 && y != 0 && y != gh - 1) {
                int r = rand() % 120 + 100;
                _grid[x + y * gw] = r;
            } else {
                _grid[x + y * gw] = 0;
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        for (int x = 1; x < gw - 1; x++) {
            for (int y = 1; y < gh - 1; y++) {
                _grid2[x + y * gw] = _grid[x + y * gw] * 8;
                _grid2[x + y * gw] += _grid[x + 1 + y * gw];
                _grid2[x + y * gw] += _grid[x - 1 + y * gw];
                _grid2[x + y * gw] += _grid[x + (y + 1) * gw];
                _grid2[x + y * gw] += _grid[x + (y - 1) * gw];

                _grid2[x + y * gw] /= 12;
                assert(_grid2[x + y * gw] < 256);
            }
        }
        for (int x = 1; x < gw - 1; x++) {
            for (int y = 1; y < gh - 1; y++) {
                _grid[x + y * gw] = _grid2[x + y * gw];
            }
        }
    }

    Grid grid = {.w = gw, .h = gh, .s = gs, .pts = _grid};

    SDL_Surface* surf = generate_surface(WIDTH, HEIGHT, &grid);
    SDL_Texture* bg_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    printf("[INFO] Done generating texture\n");

    surf = SDL_LoadBMP("cubsub.bmp");
    SDL_Texture* player_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    PlayerSub player = {.x = 200, .y = 200, .r = 16, .tex = player_tex};

    while (is_point_colliding(player.x, player.y, &grid)) {
        player.x = rand() % WIDTH;
        player.y = rand() % HEIGHT;
    }

    bool quit = 0;
    SDL_Event event;
    Uint32 lastTicks = SDL_GetTicks();
    while (!quit) {
        Uint32 nowTicks = SDL_GetTicks();
        float dt = (nowTicks - lastTicks) / 1000.f;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = 1;
                } break;
                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    bool pressed = event.type == SDL_KEYDOWN;
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                        case SDLK_UP:
                            key_pressed[K_UP] = pressed;
                            break;
                        case SDLK_s:
                        case SDLK_DOWN:
                            key_pressed[K_DOWN] = pressed;
                            break;
                        case SDLK_a:
                        case SDLK_LEFT:
                            key_pressed[K_LEFT] = pressed;
                            break;
                        case SDLK_d:
                        case SDLK_RIGHT:
                            key_pressed[K_RIGHT] = pressed;
                            break;
                    }
                } break;
            }
        }

        // clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, bg_tex, NULL, NULL);

        update_player(&player, &grid, dt);
        draw_player(renderer, &player);

        // swap buffers
        SDL_RenderPresent(renderer);
        lastTicks = nowTicks;
    }

    // quitting
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("[INFO] Destroyed renderer, window, shutdown SDL and quitting\n");
    return 0;
}