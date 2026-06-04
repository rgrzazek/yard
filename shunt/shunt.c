#include <stdint.h>
#include <stdlib.h>
#include <emscripten/emscripten.h>

// Shunt — core. The browser owns the clock and the canvas size;
// C owns the rules. Boundary:
//   init(seed)  start a run at the page-chosen framebuffer size
//   tick()            advance the simulation by exactly one step
//   click(x, y)       feed one input (framebuffer pixel coords)
//   framebuffer()     RGBA bytes the browser blits; fb_width()/fb_height() report size
//
// There is no time in here, ticks provided by browser.

#define WIDTH 1280
#define HEIGHT 720
#define PIXEL(X, Y) (((Y) * (WIDTH) + (X)) * 4)

#define GAME_W 20
#define GAME_H 20

static uint8_t fb[WIDTH * HEIGHT * 4]; // RGBA, row-major, top-left origin (matches ImageData)

// ── State: everything the simulation *is*. Reset by init(), mutated only by
//    tick() and click().
static struct
{
    int w, h; // active framebuffer size, chosen by the page at init
    uint32_t tick;
    int cursor_x, cursor_y; // placeholder: last click, so you can see input land
    int has_cursor;
} S;

EMSCRIPTEN_KEEPALIVE int fb_width(void) { return WIDTH; }
EMSCRIPTEN_KEEPALIVE int fb_height(void) { return HEIGHT; }
EMSCRIPTEN_KEEPALIVE uint8_t *framebuffer(void) { return fb; } // address in wasm memory

typedef struct
{
    uint8_t type;
    uint8_t direction;
    uint8_t flags;
} Tile;

typedef struct
{
    float x;
    float y;
    uint8_t color;
    uint8_t active;
} Box;

static Tile map[GAME_W * GAME_H];
static Box box[10];

void load_level()
{
    int r = rand();
}

EMSCRIPTEN_KEEPALIVE void init(uint32_t seed)
{
    srand(seed);
    load_level();
}

void draw_tile(int i)
{
    int x = i % GAME_W;
    int y = i / GAME_W;
    fb[40 * x + 40 * WIDTH * y] = 0;
    fb[40 * x + 40 * WIDTH * y + 1] = 0;
    fb[40 * x + 40 * WIDTH * y + 2] = 0;
}

void draw_background()
{
    for (int x = 0; x < WIDTH; x++)
    {
        for (int y = 0; y < HEIGHT; y++)
        {
            int temp = (x + y + S.tick) % 256;
            int i = PIXEL(x, y);
            fb[i] = temp;
            fb[i + 1] = (temp + 85) % 256;
            fb[i + 2] = (temp + 170) % 256;
            fb[i + 3] = 255;
        }
    }

    for (int i = 0; i < GAME_W * GAME_H; i++)
    {
        draw_tile(i);
    }
}

static void update(void)
{
    S.tick++;
}

// Paint S into the framebuffer. Pure: depends only on S, never mutates it.
static void render(void)
{
    draw_background();

    if (S.has_cursor)
    { // placeholder: show the last click
        int i = PIXEL(S.cursor_x, S.cursor_y);
        fb[i + 0] = 255;
        fb[i + 1] = 255;
        fb[i + 2] = 255;
        fb[i + 3] = 255;
    }

    for (int i = 0; i < GAME_W * GAME_H; i++)
    {
    }
}

EMSCRIPTEN_KEEPALIVE void tick(void)
{
    update();
    render();
}

EMSCRIPTEN_KEEPALIVE void click(int x, int y)
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return; // ignore out-of-bounds
    S.cursor_x = x;
    S.cursor_y = y;
    S.has_cursor = 1;
}
