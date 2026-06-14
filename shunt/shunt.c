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

#define WIDTH 1280
#define HEIGHT 720
#define PIXEL(X, Y) (((Y) * (WIDTH) + (X)) * 4)

#define GAME_W 20
#define GAME_H 20
#define NUM_TILES GAME_W *GAME_H

// Isometric projection: 2:1
// UP -> top-left (toward 0,0), DOWN -> bottom-right.
#define HX 32           // half tile width  (2 ...)
#define HY 16           // half tile height (... 1)
#define TILE_W (HX * 2) // sprite art size: 64
#define TILE_H (HY * 2) // 32
#define OX 0            // board offset within the framebuffer
#define OY 360

#define SUBTILE 100                // virtual units per tile edge
#define WORLD_W (GAME_W * SUBTILE) // 2000
#define WORLD_H (GAME_H * SUBTILE)
#define SPEED 2 // world units per tick (100u = 50px, so 2u ≈ the old 1px/tick)

#define ORIGIN_COL 10
#define ORIGIN_ROW 2

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

// ── Sprites. JS decodes each PNG and writes its RGBA into sprites[id].rgba at
//    load, then reports its size (the write-direction mirror of framebuffer()).
//    Ids are ordered so a tile id is SPR_TILE_UP + Dirs, a box id is
//    SPR_BOX_RED + Colour — selection is just a base plus the enum value.
typedef enum {
    SPR_TILE_UP,
    SPR_TILE_RIGHT,
    SPR_TILE_DOWN,
    SPR_TILE_LEFT, // align with Dirs
    SPR_BOX_RED,
    SPR_BOX_BLUE,
    SPR_BOX_GREEN,
    SPR_BOX_YELLOW,
    SPR_BOX_WHITE, // align with Colour
    SPR_SWITCH_UP,
    SPR_SWITCH_RIGHT,
    SPR_SWITCH_DOWN,
    SPR_SWITCH_LEFT, // align with Dirs
    SPR_COUNT
} SpriteId;

#define SPRITE_MAX (64 * 64 * 4) // generous per-sprite buffer

typedef struct {
    uint8_t rgba[SPRITE_MAX];
    int w, h;
} Sprite;

static Sprite sprites[SPR_COUNT];

EMSCRIPTEN_KEEPALIVE uint8_t *sprite_ptr(int id) { return sprites[id].rgba; }
EMSCRIPTEN_KEEPALIVE void set_sprite_size(int id, int w, int h) {
    sprites[id].w = w;
    sprites[id].h = h;
}

void draw_background();
void draw_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void draw_tile(Tile tile, int col, int row);
void draw_boxes();
int get_grid(Box box);

static void load_level_1(void);
static void blit_sprite(int id, int dx, int dy);

static Tile tiles[GAME_W * GAME_H];
static Box box[MAX_BOXES];

static int tile_centre(int t) { return t * SUBTILE + SUBTILE / 2; } // grid  -> world centre
static int world_to_tile(int w) { return w / SUBTILE; }             // world -> grid

typedef struct {
    int x, y;
} Pt;

static Pt project(int wx, int wy) {
    return (Pt){(wx + wy) * HX / SUBTILE + OX, (wy - wx) * HY / SUBTILE + OY};
}

// screen pixel -> tile index, or -1 if the click lands outside the board.
static int click_to_tile(int sx, int sy) {
    int u = (sx - OX) * SUBTILE / HX; // wx + wy
    int v = (sy - OY) * SUBTILE / HY; // wy - wx
    int wx = (u - v) / 2;
    int wy = (u + v) / 2;
    if (wx < 0 || wx >= WORLD_W || wy < 0 || wy >= WORLD_H)
        return -1;
    return world_to_tile(wx) + world_to_tile(wy) * GAME_W;
}

// step v toward target by up to SPEED, never overshooting — keeps == exact
static int approach(int v, int target) {
    int d = target - v;
    if (d > SPEED)
        return v + SPEED;
    if (d < -SPEED)
        return v - SPEED;
    return target;
}

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
        tiles[i].centre_x = tile_centre(i % GAME_W);
        tiles[i].centre_y = tile_centre(i / GAME_W);
    }
}

static void update(void) {
    S.tick++;
    if (S.delay < 1) {
        for (int i = 0; i < MAX_BOXES; i++) {
            if (box[i].active) {
                continue;
            }
            box[i].x = tile_centre(ORIGIN_COL);
            box[i].y = tile_centre(ORIGIN_ROW);
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

        // Ensure orthogonal alignment with belt before moving the "correct" direction
        if ((dir == UP || dir == DOWN) && box[i].x != tile_x) {
            box[i].x = approach(box[i].x, tile_x);
            continue;
        }
        if ((dir == LEFT || dir == RIGHT) && box[i].y != tile_y) {
            box[i].y = approach(box[i].y, tile_y);
            continue;
        }
        if (dir == UP)
            box[i].y = (box[i].y - SPEED + WORLD_H) % WORLD_H; // mod doesn't like negative numbers
        if (dir == RIGHT)
            box[i].x = (box[i].x + SPEED) % WORLD_W;
        if (dir == DOWN)
            box[i].y = (box[i].y + SPEED) % WORLD_H;
        if (dir == LEFT)
            box[i].x = (box[i].x - SPEED + WORLD_W) % WORLD_W;
    }
}

// Paint into the framebuffer
static void render(void) {
    draw_background();
    draw_boxes();
}

EMSCRIPTEN_KEEPALIVE void tick(void) {
    update();
    render();
}

EMSCRIPTEN_KEEPALIVE void click(int x, int y) {
    int idx = click_to_tile(x, y);
    if (idx < 0)
        return;
    Tile *t = &tiles[idx];
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

// filled diamond (TILE_W x TILE_H) centred at (cx,cy)... placeholder for sprites
static void fill_diamond(int cx, int cy, uint8_t r, uint8_t g, uint8_t b) {
    for (int dy = -HY; dy <= HY; dy++) {
        int half = HX - HX * abs(dy) / HY; // width tapers linearly to the tips
        for (int dx = -half; dx <= half; dx++)
            put_pixel(cx + dx, cy + dy, r, g, b);
    }
}

void draw_tile(Tile tile, int col, int row) {
    if (tile.type != TILE_CONVEYOR && tile.type != TILE_SWITCH && tile.type != TILE_EXIT) {
        return; // Remove guard when all the tiles are finished
    }
    Pt p = project(tile_centre(col), tile_centre(row));

    if (tile.type == TILE_EXIT) {
        // no exit art yet
        RGB c = COLOURS[tile.colour];
        fill_diamond(p.x, p.y, c.r, c.g, c.b);
        return;
    }

    // CONVEYOR / SWITCH: a directional sprite, centred on the diamond.
    int base = (tile.type == TILE_SWITCH) ? SPR_SWITCH_UP : SPR_TILE_UP;
    int id = base + tile.dir;
    blit_sprite(id, p.x - sprites[id].w / 2, p.y - sprites[id].h / 2);
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

    // Non-switch tiles first.
    for (int i = 0; i < NUM_TILES; i++) {
        if (tiles[i].type == TILE_SWITCH)
            continue;
        draw_tile(tiles[i], i % GAME_W, i / GAME_W);
    }
    // Switches last: the circular sprite overhangs neighbouring tiles, so it
    // must paint after every adjacent tile is already down.
    for (int i = 0; i < NUM_TILES; i++) {
        if (tiles[i].type != TILE_SWITCH)
            continue;
        draw_tile(tiles[i], i % GAME_W, i / GAME_W);
    }
}

// blit an RGBA sprite at (dx,dy); skip fully transparent pixels (1-bit alpha)
static void blit(const uint8_t *src, int width, int height, int dx, int dy) {
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            const uint8_t *px = &src[(y * width + x) * 4];
            if (px[3] == 0)
                continue;
            put_pixel(dx + x, dy + y, px[0], px[1], px[2]);
        }
}

static void blit_sprite(int id, int dx, int dy) {
    Sprite *s = &sprites[id];
    blit(s->rgba, s->w, s->h, dx, dy);
}

void draw_boxes() {
    for (int i = 0; i < MAX_BOXES; i++) {
        if (!box[i].active) {
            continue;
        }
        Pt p = project(box[i].x, box[i].y);
        int id = SPR_BOX_RED + box[i].colour;
        // always anchor to bottom because boxes stick up over top of their tile
        blit_sprite(id, p.x - sprites[id].w / 2, p.y + HY - sprites[id].h);
    }
}

int get_grid(Box box) {
    return world_to_tile(box.x) + world_to_tile(box.y) * GAME_W;
}

// put_pixel, safe for out of range coordinates
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
// Probably won't be required now I'm using sprites
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
