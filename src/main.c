#include <SDL2/SDL.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GRAY(n) \
    (0xff000000 | ((n)&0xff) | (((n)&0xff) << 8) | (((n)&0xff) << 16))

#define RGB(r, g, b) \
    (0xff000000 | (((r)&0xff) << 16) | (((g)&0xff) << 16) | (((b)&0xff)))

#define WIDTH 1000
#define HEIGHT 1000

Uint32 minU32(Uint32 a, Uint32 b) { return (a > b) ? b : a; }
Uint32 maxU32(Uint32 a, Uint32 b) { return (a > b) ? a : b; }
Sint32 minS32(Sint32 a, Sint32 b) { return (a > b) ? b : a; }
Sint32 maxS32(Sint32 a, Sint32 b) { return (a > b) ? a : b; }

float slen(float a, float b) { return a * a + b * b; }

float lerp(float a, float b, float t) { return a + (b - a) * t; }

typedef enum { K_UP = 1, K_DOWN, K_LEFT, K_RIGHT, K_BOMB } Controls;
bool key_pressed[32];

typedef struct {
    int w, h, s;
    int* pts;
} Grid;

typedef struct {
    float x, y;
    float vx, vy;
    int r;
    SDL_Texture* tex;
} Player;

typedef struct {
    float x, y, r;
    float vx, vy;
    bool done;
} Fish;

typedef struct {
    float x, y;
    int r;
    float life;
} Bomb;

void update_bomb(Bomb* bomb, float dt) {
    const float gravity = 100;
    bomb->life -= dt;
    bomb->y += gravity * dt;
}

void draw_bomb(SDL_Renderer* renderer, Bomb* bomb, SDL_Texture* bomb_tex) {
    SDL_Rect bomb_rect = {.x = bomb->x - bomb->r,
                          .y = bomb->y - bomb->r,
                          .w = bomb->r * 2,
                          .h = bomb->r * 2};
    SDL_RenderCopy(renderer, bomb_tex, NULL, &bomb_rect);
}

int is_point_colliding(int x, int y, Grid* grid) {
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
    if (c < 140) {
        return c;
    } else
        return 0;
}

SDL_Surface* generate_surface(int w, int h, Grid* grid) {
    Uint32 pixels[w * h];

    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            int col = is_point_colliding(x, y, grid);
            pixels[x + y * w] = col ? GRAY(col / 20 * 30 + 20) : 0;
        }
    }
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        pixels, w, h, 32, 1, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    return surface;
}

bool is_fish_colliding_with_player(Fish* fish, Player* player) {
    if (fabs(fish->x - player->x) <= fish->r + player->r &&
        fabs(fish->y - player->y) <= fish->r + player->r) {
        return true;
    }
    return false;
}

void draw_fish(SDL_Renderer* renderer, Fish* fish, SDL_Texture* fish_tex) {
    SDL_Rect fish_rect = {.x = fish->x - fish->r * 2 * (64 + 32) / 128.f,
                          .y = fish->y - fish->r,
                          .w = fish->r * 2 * (128 + 32) / 128.f,
                          .h = fish->r * 2};
    int flip = 0;
    if (fish->vx < 0) {
        flip = SDL_FLIP_HORIZONTAL;
        fish_rect.x += fish->r * 2 * (32) / 128.f;
    }
    SDL_RenderCopyEx(renderer, fish_tex, NULL, &fish_rect, 0, NULL, flip);
}

void update_fish(Fish* fish, Grid* grid, float dt) {
    const float fish_speed = 20;
    fish->x += fish->vx * dt;
    fish->y += fish->vy * dt;

    if (is_point_colliding(fish->x, fish->y, grid)) {
        fish->x -= fish->vx * dt;
        fish->y -= fish->vy * dt;
        float angle = (rand() % (31415926 * 2)) / 10000000.f;
        fish->vx = cos(angle) * fish_speed;
        fish->vy = sin(angle) * fish_speed;
    }
}

bool is_player_colliding(Player* player, Grid* grid) {
    bool collision = 0;
    for (int _x = -2; _x <= 2; _x++) {
        for (int _y = -2; _y <= 2; _y++) {
            if (abs(_x) == 2 || abs(_y) == 2 || (_x == 0 && _y == 0))
                if (is_point_colliding(player->x + player->r * _x / 2.f,
                                       player->y + player->r * _y / 2.f,
                                       grid)) {
                    collision = 1;
                    break;
                }
        }
        if (collision) break;
    }
    return collision;
}

void draw_player(SDL_Renderer* renderer, Player* player) {
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

void update_player(Player* player, Grid* grid, float dt) {
    const float max_v = 200;
    const float gravity = 30;
    const float bounciness = 0.6f;
    int dx = 0, dy = 0;
    if (key_pressed[K_UP]) dy--;
    if (key_pressed[K_DOWN]) dy++;
    if (key_pressed[K_LEFT]) dx--;
    if (key_pressed[K_RIGHT]) dx++;

    player->vx = lerp(player->vx, max_v * dx, dt * 3);
    player->vy = lerp(player->vy, max_v * dy, dt * 3);

    player->x += player->vx * dt;
    if (is_player_colliding(player, grid)) {
        player->x -= player->vx * dt;
        player->vx *= -bounciness;
    }
    player->y += (player->vy + (!dy ? gravity : 0)) * dt;
    if (is_player_colliding(player, grid)) {
        player->y -= (player->vy + (!dy ? gravity : 0)) * dt;
        player->vy *= -bounciness;
    }
}

void generate_grid(int gw, int gh, int gs, Grid* grid) {
    int* _grid = malloc(sizeof(int) * gw * gh);
    int _grid2[gw * gh];
    for (int x = 0; x < gw; x++) {
        for (int y = 0; y < gh; y++) {
            if (x != 0 && x != gw - 1 && y != 0 && y != gh - 1) {
                int r = rand() % 300;
                _grid[x + y * gw] = r;
            } else {
                _grid[x + y * gw] = 0;
            }
        }
    }

    int sw[9] = {
        1, 3, 1,  //
        3, 1, 3,  //
        1, 3, 1   //
    };
    int weight_sum = 0;
    for (int i = 0; i < 9; i++) weight_sum += sw[i];

    // NOTE: Smoothing seems to cause blocks appearing on the screen
    for (int i = 0; i < 15; i++) {
        for (int x = 1; x < gw - 1; x++) {
            for (int y = 1; y < gh - 1; y++) {
                _grid2[x + y * gw] = 0;
                for (int _x = -1; _x < 2; _x++) {
                    for (int _y = -1; _y < 2; _y++) {
                        _grid2[x + y * gw] += sw[(_x + 1) + (_y + 1) * 3] *
                                              _grid[x + _x + (y + _y) * gw];
                    }
                }
                _grid2[x + y * gw] /= weight_sum;
                /* assert(_grid2[x + y * gw] < 256); */
            }
        }
        for (int x = 1; x < gw - 1; x++) {
            for (int y = 1; y < gh - 1; y++) {
                _grid[x + y * gw] = _grid2[x + y * gw];
            }
        }
    }
    grid->w = gw;
    grid->h = gh;
    grid->s = gs;
    if (grid->pts) free(grid->pts);
    grid->pts = _grid;
}

void explode_bomb(int bx, int by, int rad, int val, Grid* grid) {
    Sint32 bgx = bx / grid->s, bgy = by / grid->s;
    Sint32 gr = ceil(rad / (float)grid->s);

    for (Sint32 gx = maxS32(1, bgx - gr - 2);
         gx <= minS32(grid->w - 2, bgx + gr + 2); gx++) {
        for (Sint32 gy = maxS32(1, bgy - gr - 2);
             gy <= minS32(grid->h - 2, bgy + gr + 2); gy++) {
            int x = gx * grid->s;
            int y = gy * grid->s;
            float dist = slen(bx - x, by - y);
            dist = sqrt(dist);
            int val_to_add = maxS32(0, lerp(val, 0, dist / rad));
            grid->pts[gx + gy * grid->w] += val_to_add;
        }
    }
}

void generate_fishes(Fish* fishes, int fish_count, Grid* grid) {
    for (int i = 0; i < fish_count; i++) {
        fishes[i].x = rand() % WIDTH;
        fishes[i].y = rand() % HEIGHT;
        fishes[i].r = 8;
        int tries = 0;
        while (is_point_colliding(fishes[i].x, fishes[i].y, grid) &&
               tries++ < 100) {
            fishes[i].x = rand() % WIDTH;
            fishes[i].y = rand() % HEIGHT;
        }
        float angle = (rand() % (31415926 * 2)) / 10000000.f;
        fishes[i].vx = cos(angle) * 20;
        fishes[i].vy = sin(angle) * 20;
        fishes[i].done = 0;
    }
}

void place_player_at_random(Player* player, Grid* grid) {
    int tries = 0;
    while (is_player_colliding(player, grid) && tries++ < 200) {
        player->x = rand() % WIDTH;
        player->y = rand() % HEIGHT;
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
    SDL_Surface* surf;

    // Load textures
    surf = SDL_LoadBMP("cubsub.bmp");
    SDL_Texture* player_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    surf = SDL_LoadBMP("cubfish.bmp");
    SDL_Texture* fish_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    surf = SDL_LoadBMP("cubbomb.bmp");
    SDL_Texture* bomb_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    // generating texture
    printf("[INFO] Starting to generate texture\n");

    int gw = 101, gh = 101, gs = HEIGHT / 99;

    Grid grid = {};
    generate_grid(gw, gh, gs, &grid);

    surf = generate_surface(WIDTH, HEIGHT, &grid);

    SDL_Texture* bg_tex = SDL_CreateTextureFromSurface(renderer, surf);

    SDL_FreeSurface(surf);

    printf("[INFO] Done generating texture\n");

    // Create player
    Player player = {.x = 200, .y = 200, .r = 16, .tex = player_tex};
    place_player_at_random(&player, &grid);

    // List for bombs
#define MAX_BOMBS 16
    Bomb bombs[MAX_BOMBS] = {};
    int bomb_count = 0;

    // Create fishes
#define MAX_FISHES 16
    Fish fishes[MAX_FISHES];
    int fish_count = 15;
    generate_fishes(fishes, fish_count, &grid);

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
                        case SDLK_SPACE:
                            key_pressed[K_BOMB] = pressed;
                            break;
                        case SDLK_r: {
                            // regen noise
                            generate_grid(gw, gh, gs, &grid);
                            SDL_Surface* surf =
                                generate_surface(WIDTH, HEIGHT, &grid);
                            bg_tex =
                                SDL_CreateTextureFromSurface(renderer, surf);
                            SDL_FreeSurface(surf);

                            generate_fishes(fishes, fish_count, &grid);
                            place_player_at_random(&player, &grid);
                        } break;
                    }
                } break;
            }
        }

        // clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, bg_tex, NULL, NULL);

        update_player(&player, &grid, dt);

        for (int i = 0; i < fish_count; i++) {
            if (!fishes[i].done) {
                update_fish(&fishes[i], &grid, dt);
                draw_fish(renderer, &fishes[i], fish_tex);
            }
            if (!fishes[i].done &&
                is_fish_colliding_with_player(&fishes[i], &player)) {
                fishes[i].done = 1;
            }
        }

        draw_player(renderer, &player);

        if (key_pressed[K_BOMB] && bomb_count < MAX_BOMBS) {
            bombs[bomb_count].x = player.x;
            bombs[bomb_count].y = player.y;
            bombs[bomb_count].life = 5;
            bombs[bomb_count].r = 8;
            bomb_count++;
            key_pressed[K_BOMB] = 0;
        }

        for (int i = 0; i < bomb_count; i++) {
            update_bomb(&bombs[i], dt);
            draw_bomb(renderer, &bombs[i], bomb_tex);
            if (bombs[i].life <= 0 ||
                is_point_colliding(bombs[i].x, bombs[i].y, &grid)) {
                explode_bomb(bombs[i].x, bombs[i].y, 70, 100, &grid);
                SDL_Surface* surf = generate_surface(WIDTH, HEIGHT, &grid);
                bg_tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FreeSurface(surf);
                bomb_count--;
                if (bomb_count > 0) {
                    memcpy(&bombs[i], &bombs[bomb_count], sizeof(Bomb));
                }
                i--;
            }
        }

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
