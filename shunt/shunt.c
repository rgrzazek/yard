#include <emscripten/emscripten.h>
#include <stdint.h>
#include <stdlib.h>

// Shunt — core. The browser owns the clock and the canvas size;
// C owns the rules. Boundary:
//   init(seed)  start a run at the page-chosen framebuffer size
//   tick()            advance the simulation by exactly one step
//   click(x, y)       feed one input (framebuffer pixel coords)
//   framebuffer()     RGBA bytes the browser blits; fb_width()/fb_height() report size
//
// There is no time in here, ticks provided by browser.

#define WIDTH 1000
#define HEIGHT 1000
#define PIXEL(X, Y) (((Y) * (WIDTH) + (X)) * 4)

#define GAME_W 20
#define GAME_H 20
#define GRID_W (WIDTH / GAME_W)
#define GRID_H (HEIGHT / GAME_H)

static uint8_t fb[WIDTH * HEIGHT * 4]; // RGBA

void draw_background();
void draw_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void draw_tile(int x, int y);

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

void load_level() {
    int r = rand();
}

EMSCRIPTEN_KEEPALIVE void init(uint32_t seed) {
    srand(seed);
    load_level();
}

static void update(void) {
    S.tick++;
}

// Paint S into the framebuffer. Pure: depends only on S, never mutates it.
static void render(void) {
    draw_background();
}

EMSCRIPTEN_KEEPALIVE void tick(void) {
    update();
    render();
}

EMSCRIPTEN_KEEPALIVE void click(int x, int y) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return; // ignore out-of-bounds
    S.cursor_x = x;
    S.cursor_y = y;
    S.has_cursor = 1;
}

/* ****************************** DRAWING ****************************** */

void draw_tile(int x, int y) {
    int dir = (x + y) % 4;
    int UP = 0;
    int RIGHT = 1;
    int DOWN = 2;
    int LEFT = 3;

    x = x * GRID_W;
    y = y * GRID_H;

    draw_line(x, y, x + GRID_W - 1, y, 0, 0, 0);
    draw_line(x, y, x, y + GRID_H - 1, 0, 0, 0);
    draw_line(x + GRID_W - 1, y, x + GRID_W - 1, y + GRID_H - 1, 0, 0, 0);
    draw_line(x, y + GRID_H - 1, x + GRID_W - 1, y + GRID_H - 1, 0, 0, 0);

    // for arrows
    int x1 = x + (GRID_W / 2);
    int x2 = x + (GRID_W * 3 / 4);
    int x3 = x + (GRID_W / 2);
    int x4 = x + (GRID_W / 4);
    int y1 = y + (GRID_H / 4);
    int y2 = y + (GRID_H / 2);
    int y3 = y + (GRID_H * 3 / 4);
    int y4 = y + (GRID_H / 2);

    if (dir == LEFT || dir == RIGHT) {
        draw_line(x2, y2, x4, y4, 255, 0, 0);
    } else {
        draw_line(x1, y1, x3, y3, 255, 0, 0);
    }
    if (dir == UP || dir == LEFT) {
        draw_line(x4, y4, x1, y1, 255, 0, 0);
    }
    if (dir == UP || dir == RIGHT) {
        draw_line(x2, y2, x1, y1, 255, 0, 0);
    }
    if (dir == DOWN || dir == RIGHT) {
        draw_line(x2, y2, x3, y3, 255, 0, 0);
    }
    if (dir == DOWN || dir == RIGHT) {
        draw_line(x2, y2, x3, y3, 255, 0, 0);
    }
    if (dir == DOWN || dir == LEFT) {
        draw_line(x4, y4, x3, y3, 255, 0, 0);
    }
}

void draw_background() {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            int temp = (x + y + S.tick) % 512;
            int i = PIXEL(x, y);

            fb[i] = temp < 256 ? temp : 511 - temp;
            temp = (temp + 170) % 512;
            fb[i + 1] = temp < 256 ? temp : 511 - temp;
            temp = (temp + 170) % 512;
            fb[i + 2] = temp < 256 ? temp : 511 - temp;
            fb[i + 3] = 55;
        }
    }

    for (int x = 0; x < 20; x++) {
        for (int y = 0; y < 20; y++) {
            draw_tile(x, y);
        }
    }
}

// put_pixel clips, so line and fill are safe with off-screen coordinates.
void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return;
    int i = PIXEL(x, y);
    fb[i + 0] = r;
    fb[i + 1] = g;
    fb[i + 2] = b;
    fb[i + 3] = 255;
}

// Bresenham's line algorithm — integer-only, handles every octant.
void draw_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put_pixel(x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            put_pixel(i, j, r, g, b);
}
