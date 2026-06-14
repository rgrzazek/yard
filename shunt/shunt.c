#include <emscripten/emscripten.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Shunt — core. The browser owns the clock; C owns the rules.
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
#define NUM_TILES GAME_W *GAME_H
#define GRID_W (WIDTH / GAME_W)
#define GRID_H (HEIGHT / GAME_H)

#define ORIGIN_X (10 * GRID_W)
#define ORIGIN_Y (2 * GRID_H)

#define FPS 60
#define MAX_BOXES 10
#define DELAY 3 * FPS

static uint8_t fb[WIDTH * HEIGHT * 4]; // RGBA

// ── State: everything the simulation *is*. Mutated only by tick() and click().
static struct
{
    uint32_t tick;
    uint16_t delay; // gate to stop the next box appearing
} S;

EMSCRIPTEN_KEEPALIVE int fb_width(void) { return WIDTH; }
EMSCRIPTEN_KEEPALIVE int fb_height(void) { return HEIGHT; }
EMSCRIPTEN_KEEPALIVE uint8_t *framebuffer(void) { return fb; } // address in wasm memory

typedef struct
{
    uint8_t type;
    uint8_t dir;
    uint16_t centre_x;
    uint16_t centre_y;
    uint8_t colour; // For exits
} Tile;

typedef struct
{
    int x;
    int y;
    uint8_t colour;
    bool active;
} Box;

typedef enum {
    COLOUR_RED,
    COLOUR_BLUE,
    COLOUR_GREEN,
    COLOUR_YELLOW,
    COLOUR_WHITE,
    COLOUR_COUNT // final index is the number of actual colours
} Colour;

typedef enum {
    UP,
    RIGHT,
    DOWN,
    LEFT
} Dirs;

typedef enum {
    TILE_CONVEYOR,
    TILE_ENTRY,
    TILE_EXIT,
    TILE_SWITCH,
    TILE_INACTIVE
} Tile_Type;

void draw_background();
void draw_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void draw_tile(Tile tile);
void draw_boxes();
int get_grid(Box box);

static void load_level_1(void);

static int tile_index(int px, int py);
static Tile *tile_at(int px, int py);

static Tile tiles[GAME_W * GAME_H];
static Box box[MAX_BOXES];

void load_level() {
    int r = rand();
    for (int i = 0; i < MAX_BOXES; i++) {
        box[i].x = rand() % (WIDTH);
        box[i].y = rand() % (HEIGHT);
        box[i].active = 1;
        box[i].colour = rand() % (COLOUR_COUNT);
    }
}

EMSCRIPTEN_KEEPALIVE void init(uint32_t seed) {
    srand(seed);
    load_level_1();
    // load_level();
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].centre_x = (i % GAME_W * GRID_W) + (GRID_W / 2);
        tiles[i].centre_y = (i / GAME_W * GRID_H) + (GRID_H / 2);
    }
}

static void update(void) {
    S.tick++;
    if (S.delay < 1) {
        for (int i = 0; i < MAX_BOXES; i++) {
            if (box[i].active) {
                continue;
            }
            box[i].x = ORIGIN_X;
            box[i].y = ORIGIN_Y;
            box[i].active = 1;
            box[i].colour = rand() % (COLOUR_COUNT);
            break;
        }
        S.delay = DELAY;
    }
    S.delay -= 1;

    for (int i = 0; i < MAX_BOXES; i++) {
        Tile this_tile = tiles[get_grid(box[i])];

        if (this_tile.type == TILE_EXIT) {
            box[i].active = false;
            continue;
        }

        int dir = this_tile.dir;
        int tile_x = this_tile.centre_x;
        int tile_y = this_tile.centre_y;

        if ((dir == UP || dir == DOWN) && box[i].x != tile_x) {
            box[i].x += box[i].x < tile_x ? 1 : -1;
            continue;
        }
        if ((dir == LEFT || dir == RIGHT) && box[i].y != tile_y) {
            box[i].y += box[i].y < tile_y ? 1 : -1;
            continue;
        }
        if (dir == UP)
            box[i].y = (box[i].y - 1 + HEIGHT) % HEIGHT;
        if (dir == RIGHT)
            box[i].x = (box[i].x + 1) % WIDTH;
        if (dir == DOWN)
            box[i].y = (box[i].y + 1) % HEIGHT;
        if (dir == LEFT)
            box[i].x = (box[i].x - 1 + WIDTH) % WIDTH;
    }
}

// Paint S into the framebuffer. Pure: depends only on S, never mutates it.
static void render(void) {
    draw_background();
    draw_boxes();
}

EMSCRIPTEN_KEEPALIVE void tick(void) {
    update();
    render();
}

EMSCRIPTEN_KEEPALIVE void click(int x, int y) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return;
    Tile *t = tile_at(x, y);
    if (t->type != TILE_SWITCH)
        return;
    t->dir = (t->dir + 1) % 4;
}

/* ****************************** LEVELS  ****************************** */
static void set_tile(int x, int y, uint8_t type, uint8_t dir) {
    int i = x + y * GAME_W;
    tiles[i].type = type;
    tiles[i].dir = dir;
}

static void load_level_1(void) {
    // Standard reset: everything inactive.
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].type = TILE_INACTIVE;
    }

    for (int col = 2; col <= 16; col++) {
        set_tile(10, col, TILE_CONVEYOR, DOWN);
    }
    set_tile(10, 17, TILE_EXIT, 0);

    set_tile(2, 10, TILE_EXIT, 0);
    for (int row = 3; row <= 9; row++) {
        set_tile(row, 10, TILE_CONVEYOR, LEFT);
    }
    set_tile(10, 10, TILE_SWITCH, DOWN); // intersection

    for (int row = 11; row <= 16; row++) {
        set_tile(row, 10, TILE_CONVEYOR, RIGHT);
    }
    set_tile(17, 10, TILE_EXIT, 0);
}
/* ****************************** DRAWING ****************************** */

typedef struct {
    uint8_t r, g, b;
} RGB;

static const RGB COLOURS[COLOUR_COUNT] = {
    [COLOUR_RED] = {200, 50, 50},
    [COLOUR_BLUE] = {50, 50, 200},
    [COLOUR_GREEN] = {50, 200, 50},
    [COLOUR_YELLOW] = {200, 220, 0},
    [COLOUR_WHITE] = {150, 150, 150},
};

static int tile_index(int x, int y) {
    return (x / GRID_W) + (y / GRID_H) * GAME_W;
}

static Tile *tile_at(int x, int y) {
    return &tiles[tile_index(x, y)];
}

void draw_tile(Tile tile) {
    if (tile.type != TILE_CONVEYOR && tile.type != TILE_SWITCH && tile.type != TILE_EXIT) {
        return;
    }
    int dir = tile.dir;

    int x = tile.centre_x - (GRID_W / 2);
    int y = tile.centre_y - (GRID_H / 2);

    if (tile.type == TILE_EXIT) {
        RGB c = COLOURS[tile.colour];
        fill_rect(x, y, GRID_W, GRID_H, c.r, c.g, c.b);
        return;
    }

    if (tile.type == TILE_SWITCH) {
        fill_rect(x, y, GRID_W, GRID_H, 50, 200, 50);
    }

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

    for (int i = 0; i < GAME_W * GAME_H; i++) {
        draw_tile(tiles[i]);
    }
}

void draw_boxes() {
    for (int i = 0; i < MAX_BOXES; i++) {
        // Draw if box is active
        if (!box[i].active) {
            continue;
        }
        RGB c = COLOURS[box[i].colour];
        fill_rect(box[i].x - (GRID_W / 2), box[i].y - (GRID_H / 2), GRID_W, GRID_H, c.r, c.g, c.b);
    }
}

int get_grid(Box box) {
    return tile_index(box.x, box.y);
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
